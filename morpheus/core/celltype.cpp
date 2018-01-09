#include "celltype.h"
#include "expression_evaluator.h"
#include "function.h"
#include "diffusion.h"
#include "symbol_accessor.h"
#include "focusrange.h"



CPM::INDEX CellIndexStorage::emptyIndex()
{
	CPM::INDEX  t;
	t.celltype = 0;
	t.status = CPM::NO_CELL;
	t.sub_cell_id = 0;
	t.super_cell_id = 0;
	return t;
}

// Cell& CellIndexStorage::cell( cell_id) { 
// 	assert( cell_id < cell_by_id.size() );
// 	assert( cell_by_id[cell_id]);
// 	
// // 	if ( ! cell_by_id[cell_id] ) {
// // 		if (cell_index_by_id[cell_id].status == SIM::VIRTUAL_CELL)
// // 			cerr << "A virtual cell is associated with cell id " << cell_id << endl;
// // 		else //  if (cell_index_by_id[cell_id].status = SIM::NO_CELL)
// // 			cerr << "No cell is associated with cell id " << cell_id << endl;
// // 		assert(0); exit(-1);
// // 	}
// 	return *cell_by_id[cell_id];
// };


shared_ptr<Cell> CellIndexStorage::addCell(shared_ptr<Cell> c, CPM::INDEX idx)
{
	if (cell_by_id.size() <= c->getID()) {
		cell_by_id.resize(c->getID()+5);
		cell_index_by_id.resize(c->getID()+5, emptyIndex());
	}
	cell_by_id[c->getID()] = c;
	cell_index_by_id[c->getID()] = idx;
	if (c->getID()>=free_cell_name) 
		free_cell_name = c->getID()+1;
	
	used_cell_names.insert(c->getID());
// 	cout << " Storage: added Cell " << c->getID()  << endl;
	return cell_by_id[c->getID()];
};

shared_ptr<Cell> CellIndexStorage::replaceCell(shared_ptr<Cell> c, CPM::INDEX idx)
{
	if ( isFree(c->getID()) ) {
		throw string("Cannot replace cell. Cell does not exist");
	}
	auto old_cell = cell_by_id[c->getID()];
	cell_by_id[c->getID()] = c;
	cell_index_by_id[c->getID()] = idx;
	
	return old_cell;
};

shared_ptr<Cell> CellIndexStorage::removeCell(CPM::CELL_ID id)
{
	used_cell_names.erase(id);
	shared_ptr<Cell> c = cell_by_id[id];
	cell_by_id[id].reset();
	cell_index_by_id[id] = emptyIndex();
// 	cout << " Storage: removed Cell " << c->getID()  << endl;
	return c;
}


using namespace SIM;
// string CellType::XMLClassName() const { return string("abstract prototype"); };
registerCellType(CellType);

CellIndexStorage CellType::storage;

CellType::CellType(uint ct_id) :  default_properties(_default_properties), default_membranes(_default_membranePDEs)
{
	id= ct_id;
	name ="";
};

CellType* CellType::createInstance(uint ct_id) {
	return new CellType(ct_id);
};

XMLNode CellType::saveToXML() const {
	XMLNode xNode = XMLNode::createXMLTopNode("CellType");
	xNode.addAttribute("class",XMLClassName().c_str());
	xNode.addAttribute("name",name.c_str());

	for(uint i=0;i<plugins.size();i++) {
		xNode.addChild(plugins[i]->saveToXML());
	}
	return xNode;
}

void CellType::loadFromXML(const XMLNode xCTNode) {
	stored_node = xCTNode;
	getXMLAttribute(xCTNode,"name",name);
	string classname;
	getXMLAttribute(xCTNode,"class",classname,false);
	if (classname != XMLClassName()) {
		throw string("wrong celltype classname ")+classname+", expected "+ XMLClassName();
	}
	
	local_scope = SIM::createSubScope(string("CellType[")+name + "]",this);
}


void CellType::loadPlugins()
{
	assert(plugins.empty());
	SIM::enterScope(local_scope);
	int nPlugins = stored_node.nChildNode();
	for(int i=0; i<nPlugins; i++) {
		XMLNode xNode = stored_node.getChildNode(i);
		try {
			string xml_tag_name(xNode.getName());
// 			if (xml_tag_name=="Segment") continue; // for compatibility with the segmented celltype
			shared_ptr<Plugin> p = PluginFactory::CreateInstance(xml_tag_name);
			if (! p.get()) 
				throw(string("Unknown plugin " + xml_tag_name));
			
			p->loadFromXML(xNode);
			uint n_interfaces=0;
			
			if ( dynamic_pointer_cast< AbstractProperty >(p) ) {
				// note that the AbstractProperty is still maintained by the Plugin
				shared_ptr<AbstractProperty> property( dynamic_pointer_cast< AbstractProperty >(p) ); 
				
				if (!property->isConstant()) {
					if (property_by_symbol.find( property->getName() ) != property_by_symbol.end()) {
						throw MorpheusException("Redefinition of Property \""+property->getSymbol()+"\". ", stored_node);
						//cerr << "Redefinition of Property " << property->getSymbol() << endl;
						//exit(-1);
					}
					_default_properties.push_back(property);
					property_by_name[property->getName()]=_default_properties.size()-1;
					property_by_symbol[property->getSymbol()] = _default_properties.size()-1;
				}
				defineSymbol(property);
				n_interfaces++; 
			}
			 
			if ( dynamic_pointer_cast< MembraneProperty >(p) ) {
				shared_ptr< MembraneProperty > membrane(dynamic_pointer_cast< MembraneProperty >(p));
// 				if (membrane_by_name.find( membrane->getName() ) != membrane_by_name.end()) {
// 					cerr << "Redefinition of Membrane " << membrane->getName() << endl;
// 					exit(-1);
// 				}
				_default_membranePDEs.push_back( membrane->getPDE() );
// 				membrane_by_name[membrane->getName()] = _default_membranePDEs.size()-1;
				if (membrane->getPDE()->getDiffusionRate() > 0.0)
					plugins.push_back(shared_ptr<Plugin>(new Diffusion(CellMembraneAccessor(this,_default_membranePDEs.size()-1))));
				
				// registering global symbol
				SymbolData symbol;
				symbol.name = membrane->getSymbolName();
				symbol.fullname = membrane->getName();
				symbol.link = SymbolData::CellMembraneLink;
				symbol.granularity = Granularity::MembraneNode;
				symbol.type_name = TypeInfo<double>::name();
				symbol.writable = true;
				SIM::defineSymbol(symbol);
				membrane_by_symbol[symbol.name] = _default_membranePDEs.size()-1;
				n_interfaces++;
				cout << "Registered Membrane " << _default_membranePDEs.size() << " " << membrane->getName() << " with diffusion rate " << membrane->getPDE()->getDiffusionRate() << endl;
			}
			if ( dynamic_pointer_cast< Function >(p) ) { defineSymbol(dynamic_pointer_cast< Function >(p)); n_interfaces++; }
			if ( dynamic_pointer_cast< VectorFunction >(p) ) { defineSymbol(dynamic_pointer_cast< VectorFunction >(p)); n_interfaces++; }
			if ( dynamic_pointer_cast< CPM_Energy >(p) ) { energies.push_back(dynamic_pointer_cast< CPM_Energy >(p) ); n_interfaces++; }
			if ( dynamic_pointer_cast< CPM_Check_Update >(p) ) { check_update_listener.push_back(dynamic_pointer_cast< CPM_Check_Update >(p) ); n_interfaces++; }
			if ( dynamic_pointer_cast< CPM_Update_Listener >(p) ) { update_listener.push_back(dynamic_pointer_cast< CPM_Update_Listener >(p) ); n_interfaces++; }
			if ( dynamic_pointer_cast< TimeStepListener >(p) ) { /*timestep_listener.push_back(dynamic_pointer_cast< TimeStepListener >(p) );*/ n_interfaces++; }
			if ( ! n_interfaces ) 
				throw(xml_tag_name + " is not a valid celltype plugin");
			plugins.push_back( p );
		}
		catch(string er) {
			throw MorpheusException(er, stored_node);
			//cerr << er << " - leaving you alone ..." << endl; exit(-1);
		}
	}
	SIM::leaveScope();
}


void CellType::init() {
	if (!local_scope)
		local_scope = SIM::createSubScope(string("CellType[")+name + "]",this);
	SIM::enterScope(local_scope);
	
	// Property initializer may fail gracefully when expressions require an explicite cell
	for (auto plug : plugins) {
		if ( dynamic_pointer_cast<AbstractProperty>( plug) ) {
			try { 
				plug->init(local_scope); } catch (...) {/* There might be errors due to the fact that there is no real cell present !!!*/ }
		}
	}

	for (auto mem: default_membranes) {
		try { mem->init(local_scope); } catch (...) {/* There might be errors due to the fact that there is no real cell present !!!*/ }
	}

	for (uint i=0;i<plugins.size();i++) {
		if ( dynamic_pointer_cast<AbstractProperty>( plugins[i]) )
			continue;
		try { 
			plugins[i]->init(local_scope);
		}
		catch (string er) {
			throw MorpheusException(er, plugins[i]->saveToXML());
		}
		catch (SymbolError er) {
			throw MorpheusException(er.what(), plugins[i]->saveToXML());
		}
	}
	
	// Create all cell populations through Cell definitions / initializers / at random positions
	map<CPM::CELL_ID, XMLNode> predefined_cells;
	for (auto& cp : cell_populations) {
		// Cell definitions : parse "Cell" nodes and create a cell
		if (cp.xPopNode.nChildNode("Cell")>0) {
			cout << "CellType \'" << name << "\': loading " << cp.xPopNode.nChildNode("Cell") << " cells from XML" << endl;
		}
		for (int i=0; i < cp.xPopNode.nChildNode("Cell"); i++) {
			XMLNode xCellNode = cp.xPopNode.getChildNode("Cell",i);
			uint cell_id;
			if ( getXMLAttribute(xCellNode, "name", cell_id) )
				cell_id =  createCell(cell_id);
			else
				cell_id =  createCell();
			
			storage.cell(cell_id).loadNodesFromXML(xCellNode);
			if( storage.cell(cell_id).getNodes().size() == 0){
				cout << "Created empty cell, removing it again" << endl;
				removeCell( cell_id );
			}
			else {
				predefined_cells[cell_id] = xCellNode;
				cp.cells.push_back(cell_id);
			}
		}

		// Cell initializers
		for (auto ini : cp.pop_initializers) {
			ini->init(local_scope);
			auto new_cells = ini->run(this);
			cp.cells.insert(cp.cells.end(),new_cells.begin(),new_cells.end());

		}
		
		// Create all yet undefined cells at random positions
		if (cell_ids.size() < cp.pop_size) {
			cout << "CellType \'" << name << "\': " << cp.pop_size - cell_ids.size() << " uninitialized cells." << endl ;
// 			for (int i=cell_ids.size(); i < cp.pop_size; i++) {
// 				//cout << " - Creating cell "<< i <<" at random position" << endl ;
// 				auto new_cell = createRandomCell();
// 				cp.cells.push_back(new_cell);
// 			}
		}
	}
	
	
	// Initialization of Cell Properties
	
	// i run individual property initialisation through Cell::init()
	for (auto cell_id : cell_ids) {
		storage.cell(cell_id).init();
	}
	
	
	// ii Run property initialization plugins from Population initializers
	for (auto& cp : cell_populations) {
		for ( const auto& ip : cp.property_initializers) {
			
			SymbolRWAccessor<double> symbol;
			symbol = local_scope->findRWSymbol<double>(ip.symbol);

			ExpressionEvaluator<double> init_expression(ip.expression);
			init_expression.init(local_scope);
			
			// Apply InitProperty expressions for all cells
			for (auto cell : cp.cells) {
				FocusRange range(symbol.getGranularity(),cell);
				for ( const auto& focus : range) {
					symbol.set(focus, init_expression.get(focus));
				}
			}
		}
	}
	
	// iii Override with per cell property definitions from xml
	for (const auto& cell : predefined_cells) {
		XMLNode xCellNode = cell.second;
		CPM::CELL_ID cell_id = cell.first;
		storage.cell(cell_id).loadFromXML( xCellNode );
	}
	
	SIM::leaveScope();
}

multimap<Plugin*, SymbolDependency > CellType::cpmDependSymbols() const
{
	multimap<Plugin*, SymbolDependency > s;
	for (uint i=0; i<energies.size();i++) {
		set<SymbolDependency> s2 = energies[i]->getDependSymbols();
		for (auto& dep : s2) {
			s.insert(make_pair(energies[i].get(),dep));
		}
	}
	for (uint i=0; i<check_update_listener.size();i++) {
		set<SymbolDependency> s2 = check_update_listener[i]->getDependSymbols();
		for (auto& dep : s2) {
			s.insert(make_pair(check_update_listener[i].get(),dep));
		}
	}
	for (uint i=0; i<update_listener.size();i++) {
		set<SymbolDependency> s2 = update_listener[i]->getDependSymbols();
		for (auto& dep : s2) {
			s.insert(make_pair(update_listener[i].get(),dep));
		}
	}
	
	return s;
}


XMLNode  CellType::savePopulationToXML() const {
	XMLNode xCPNode = XMLNode::createXMLTopNode("Population");

	xCPNode.addAttribute("type",name.c_str());
	xCPNode.addAttribute("size", to_cstr(cell_ids.size()) );
	for (uint i=0; i<cell_ids.size();i++) {
		xCPNode.addChild( storage.cell(cell_ids[i]).saveToXML() );
	}
	return xCPNode;
}

void CellType::loadPopulationFromXML(const XMLNode xNode) {

	SIM::enterScope(local_scope);
	
	CellPopDesc cp;
	cp.xPopNode = xNode;
	// ensure the type is my name
	string type; getXMLAttribute(xNode,"type",type,false);
	if ( type != name) throw string("wrong type name in cell population");
// 	if ( ! cell_ids.empty() && !dynamic_cast<MediumCellType*>(this) ) throw string("CellType ") + this->name + " has a second CellPopulations defined.\nCurrently, only one Population per CellType is supported";

	cp.pop_size=1; 
	getXMLAttribute(xNode,"size",cp.pop_size);

	// parse all Initializers
	for (int i=0; i < xNode.nChildNode(); i++) {
		XMLNode xcpNode = xNode.getChildNode(i);
		// Defer loading cells
		if (string(xcpNode.getName()) == "Cell") continue;
		if (string(xcpNode.getName()) =="InitProperty") {
			IntitPropertyDesc ip;
			if ( ! getXMLAttribute(xcpNode, "symbol-ref", ip.symbol)) {
				throw string ("Missing symbol in Population[") + this->name + "]/InitProperty";
			}
			if ( ! getXMLAttribute(xcpNode,"Expression/text",ip.expression)) {
				throw string ("Missing expression in Population[") + this->name + "]/InitProperty["+ip.symbol+"]";
			}
			cp.property_initializers.push_back(ip);
		}
		else {
			// assume its an initilizer
			shared_ptr<Plugin> p = PluginFactory::CreateInstance(string(xcpNode.getName()));
			if ( dynamic_pointer_cast<Population_Initializer>( p ) ) {
				cp.pop_initializers.push_back(dynamic_pointer_cast<Population_Initializer>( p ));
				cp.pop_initializers.back()->loadFromXML(xcpNode);
			}
		}
	}
	
	cell_populations.push_back(cp);

	SIM::leaveScope();
}



CPM::CELL_ID  CellType::createCell(CPM::CELL_ID cell_id) {
	// maintaining unique cell_ids
	if ( ! storage.isFree(cell_id) ) {
		cout << "!! Warning !! Given cell id " << cell_id << " is already used" << endl;
		cell_id = storage.getFreeID();
		cout << "!! Warning !! Overriding id with " << cell_id << "." << endl;
	}
// 	cout << "creating real cell "<< cell_id << endl;
	shared_ptr<Cell> c(new Cell(cell_id, this ) );
	if ( ! c ) 
		throw string("unable to create cell");

	//	maintain local associations
	cell_ids.push_back(cell_id);
	
	CPM::INDEX t = storage.emptyIndex();
	t.celltype = id;
	t.status = CPM::REGULAR_CELL;
	storage.addCell(c,t);
	
// 	c->init();
	
	return cell_id;
}


pair<CPM::CELL_ID, CPM::CELL_ID> CellType::divideCell2(CPM::CELL_ID cell_id, division mode, VDOUBLE orientation) {
	VDOUBLE division_plane_normal = VDOUBLE(0,0,0);

	const EllipsoidShape& shape = storage.cell(cell_id).currentShape().ellipsoidApprox();
	switch ( mode ){
		case CellType::MAJOR:{
			division_plane_normal = shape.axes[1];
			break;
		}
		case CellType::MINOR:{
			division_plane_normal = shape.axes[0];
			break;
		}
		case CellType::RANDOM:{
			if ( SIM::getLattice()->getDimensions()==2) {
				division_plane_normal = VDOUBLE::from_radial(VDOUBLE(getRandom01()*2*M_PI,0,1));
			}
			else if ( SIM::getLattice()->getDimensions()==3){
				division_plane_normal = VDOUBLE::from_radial(VDOUBLE(getRandom01()*2*M_PI,(getRandom01()-0.5)*M_PI,1));
			}
			break;
		}
		case CellType::ORIENTED:{
			division_plane_normal = orientation;
			break;
		}
		default:{
			throw string("CellDivision: Unknown division plane specification.");
		}
	}
	return divideCell2(cell_id, division_plane_normal, shape.center);
}

pair<CPM::CELL_ID, CPM::CELL_ID> CellType::divideCell2(CPM::CELL_ID mother_id, VDOUBLE split_plane_normal,  VDOUBLE split_plane_center ) {
	
	CPM::CELL_ID daughter1_id = createCell();
	Cell& daughter1 = storage.cell(daughter1_id);
	CPM::CELL_ID daughter2_id = createCell();
	Cell& daughter2 = storage.cell(daughter2_id);

	Cell& mother = storage.cell(mother_id);
	shared_ptr <const Lattice > lattice = SIM::getLattice();

	// choose a random orientation, split orientation is given 
	if (split_plane_normal.abs()==0) {
		double angle=getRandom01()*2*M_PI;
		split_plane_normal = VDOUBLE(sin(angle),cos(angle),0);
	}
	
	// redistribute the Nodes following the split plane rules.
	Cell::Nodes mother_nodes =  mother.getNodes();
	Cell::Nodes deferred_nodes;
	for (Cell::Nodes::const_iterator node = mother_nodes.begin(); node != mother_nodes.end();node++) {
		double distance = distance_plane_point( split_plane_normal, split_plane_center, lattice->to_orth(*node) );
// 		cout << "Distance d" << distance << "\tn" << split_plane_normal << "\tc" << split_plane_center << "\tnode" << VDOUBLE(*node) << endl;
		if ( distance > 0 ) {
			if (! CPM::setNode(*(node), daughter1_id))
				cerr << "unable to set Cell " << daughter1_id << " at position " << *node << endl;
		}
		else if (distance == 0) {
			deferred_nodes.insert(*(node));
		}
		else
			if (! CPM::setNode(*(node), daughter2_id))
				cerr << "unable to set Cell " << daughter2_id << " at position " << *node << endl;
	}
	
	// Distribute Nodes lying right on the split plane
	int current_cell = (daughter1.nNodes()>daughter2.nNodes());
	for (auto const & n : deferred_nodes) {
		if (current_cell == 0) {
			if (! CPM::setNode(n, daughter1_id))
				cerr << "unable to set Cell " << daughter1_id << " at position " << n << endl;
			current_cell=1;
		}
		else {
		if (! CPM::setNode(n, daughter2_id))
				cerr << "unable to set Cell " << daughter2_id << " at position " << n << endl;
			current_cell=0;
		}
	}

	//cout << "Cell division: mother: " << mother.nNodes() << ", daughter1: " << daughter1.nNodes() << ", daughter2: " << daughter2.nNodes() << endl;	
	
	if( mother.nNodes() == 0 ){
		removeCell( mother_id );
	}
	else{
		cerr << "divideCell2: Mother cell ("<<  mother_id << ") is not empty after cell division (nodes: " <<  mother.nNodes() << " ) and cannot be removed!" << endl;
		exit(-1);
	}
	
	daughter1.init();
	daughter1.assignMatchingProperties(mother.properties);
	daughter1.assignMatchingMembranes(mother.membranes);
	daughter2.init();
	daughter2.assignMatchingProperties(mother.properties);
	daughter2.assignMatchingMembranes(mother.membranes);
	
	return pair<CPM::CELL_ID, CPM::CELL_ID>(daughter1_id, daughter2_id);
}


CPM::CELL_ID CellType::addCell(CPM::CELL_ID cell_id) 
{
	CPM::INDEX old_index = storage.index(cell_id);
	if( this->id == old_index.celltype) return cell_id;
	
	// create a cell with the same properties and id, that also owns the same nodes.
	shared_ptr<Cell> new_cell = shared_ptr<Cell>( new Cell(storage.cell(cell_id), this ) );
	
	CPM::INDEX t = storage.emptyIndex();
	t.celltype = id;
	t.status = CPM::REGULAR_CELL;
	
	// change storage associations
	cell_ids.push_back(new_cell->getID());
	shared_ptr<Cell> old_cell_ptr = storage.replaceCell(new_cell,t);
	
	new_cell->init();
	new_cell->assignMatchingProperties(old_cell_ptr->properties);
	new_cell->assignMatchingMembranes(old_cell_ptr->membranes);
	
	old_cell_ptr->celltype->removeCell(cell_id);
	
	assert( old_cell_ptr.unique() );
	return new_cell->getID();
};

void CellType::removeCell(CPM::CELL_ID cell_id) {
	cell_ids.erase(remove(cell_ids.begin(), cell_ids.end(), cell_id), cell_ids.end());
}

CPM::CELL_ID CellType::createRandomCell() {
// add n cells and initializes them with a single random node.

	CPM::CELL_ID cell_id = createCell();
	CPM::setNode(CPM::findEmptyNode(), cell_id);
	return cell_id;
}

double CellType::hamiltonian() const {
	vector<CPM_Energy*>::iterator e;
	vector<Cell*>::iterator c;
	double hamil=0;
	for (uint ic=0; ic<cell_ids.size(); ic++) {
		for ( uint ie = 0; ie < energies.size(); ie++) {
// 			energies[ie]->attachTo(cell_ids[ic]);
			hamil += energies[ie]->hamiltonian( cell_ids[ic] );
		}
	}
	return hamil;
}

bool CellType::check_update(const CPM::Update& update) const
{
	if ((update.opRemove()) && update.focus().cell().nNodes() == 1)
		return false;;

	if (!check_update_listener.empty()) {
		if (update.opAdd()) {
			auto update_add = update.selectOp(CPM::Update::ADD);
			for ( uint c=0; c < check_update_listener.size(); c++ ) {
				if (! check_update_listener[c] -> update_check( update.focusStateAfter().cell_id , update_add))
					return false;
			}
		}
	
		if (update.opRemove()) {
			auto update_remove = update.selectOp(CPM::Update::REMOVE);
			for ( uint c=0; c < check_update_listener.size(); c++ ) {
				if (! check_update_listener[c] -> update_check( update.focusStateBefore().cell_id , update_remove)) 
					return false;
			}
		}
	}
	
	return true;
}

double CellType::delta(const CPM::Update& update) const
{
	
	double delta=0;
	if (update.opAdd()) {
		auto update_add = update.selectOp(CPM::Update::ADD);
		for (uint e = 0; e<energies.size(); ++e) {
			delta += energies[e]->delta(update.focusUpdated(), update_add);
		}
	}
	if (update.opRemove()) {
		auto update_remove = update.selectOp(CPM::Update::REMOVE);
		for (uint e = 0; e<energies.size(); ++e) {
			delta += energies[e]->delta(update.focus(), update_remove);
		}
	}
	return delta;
}

void CellType::set_update(const CPM::Update& update) {
	
// 	if (update.opNeighborhood()) {
// 		auto update_neigh = update.selectOp(CPM::Update::NEIGHBORHOOD_UPDATE);
// 		const auto& states = update.boundaryStencil().getStatistics();
// 		for (const StatisticalLatticeStencil::STATS& state : states) {
// 			if ( state.cell != update.focusStateAfter().cell_id && state.cell != update.focusStateBefore().cell_id )
// 				storage.cell(state.cell) . setUpdate(update_neigh);
// 		}
// 	}
	if (update.opAdd()) {
		auto cell_id = update.focusStateAfter().cell_id;
		auto update_add = update.selectOp(CPM::Update::ADD);
		storage.cell(cell_id) . setUpdate(update_add);
		for (uint i=0; i<update_listener.size(); i++) {
			update_listener[i]->set_update_notify(cell_id, update_add);
		}
	}
	if (update.opRemove()) {
		auto cell_id = update.focusStateBefore().cell_id;
		auto update_remove = update.selectOp(CPM::Update::REMOVE);
		storage.cell(cell_id) . setUpdate(update_remove);
		for (uint i=0; i<update_listener.size(); i++) {
			update_listener[i]->set_update_notify(cell_id, update_remove);
		}
	}
}

void CellType::apply_update(const CPM::Update& update) {
	if (update.opAdd()) {
		auto cell_id = update.focusStateAfter().cell_id;
		auto update_add = update.selectOp(CPM::Update::ADD);
		storage.cell(cell_id) . applyUpdate(update_add);
		for (uint i=0; i<update_listener.size(); i++) {
			update_listener[i]->update_notify(cell_id, update_add);
		}
	}
	if (update.opRemove()) {
		auto cell_id = update.focusStateBefore().cell_id;
		auto update_remove = update.selectOp(CPM::Update::REMOVE);
		storage.cell(cell_id) . applyUpdate(update_remove);
		for (uint i=0; i<update_listener.size(); i++) {
			update_listener[i]->update_notify(cell_id, update_remove);
		}
	}
}


CellType* MediumCellType::createInstance(uint ct_id) {
	return new MediumCellType(ct_id);
}

registerCellType(MediumCellType);

MediumCellType::MediumCellType(uint ct_id) :  CellType(ct_id) {}

CPM::CELL_ID MediumCellType::createCell(CPM::CELL_ID name) {
	if ( ! cell_ids.empty() ) 
		return *(cell_ids.begin());
	
	CPM::CELL_ID cell_id = CellType::createCell(name);
	storage.cell(cell_id).disableNodeTracking();
	return cell_id;
};

CPM::CELL_ID MediumCellType::addCell(CPM::CELL_ID cell_id) {
	// don't copy the cell, just soak off the nodes and then unregister the cell id.
	
	Cell& other_cell = storage.cell(cell_id);
	while ( ! other_cell.getNodes().empty()) {
		CPM::setNode( *other_cell.getNodes().begin(), cell_ids[0] );
	}
	// now that we don't overtake the cell_id we should clear its global references
	shared_ptr<Cell> other_cell_ptr = storage.removeCell(cell_id);
	other_cell_ptr->getCellType()->removeCell(cell_id);
	assert( other_cell_ptr.unique());
	return cell_ids[0];
}

void MediumCellType::removeCell(CPM::CELL_ID cell_id) {
	// We use just one cell and never remove it ...
}

CellMembraneAccessor CellType::findMembrane(string symbol, bool required) const
{
	map<string,uint>::const_iterator membrane_idx = membrane_by_symbol.find(symbol);
	
	if ( membrane_idx != membrane_by_symbol.end() ) {
		return CellMembraneAccessor(this,membrane_idx->second );
	}
	if (required){
		cerr << (string("CellType[\"") + name + string("\"].findMembrane: requested membrane [")+symbol+string("] not found")) << endl;
		exit(-1);
		//throw (string("CellType[\"") + name + string("\"].findMembrane: requested membrane [")+symbol+string("] not found"));
	}
	return CellMembraneAccessor();
}

// CellMembraneAccessor CellType::findMembraneByName(string name) const
// {
// 	map<string,uint>::const_iterator membrane_idx = membrane_by_name.find(name);
// 	
// 	if ( membrane_idx != membrane_by_name.end() ) {
// 		return CellMembraneAccessor(this,membrane_idx->second );
// 	}
// 	return CellMembraneAccessor();
// }
