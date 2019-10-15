#define CPM_CPP

#include "cpm_p.h"
// #include "simulation_p.h"


namespace CPM {
	
	bool enabled = false;
	
	STATE InitialState, EmptyState; // get overridden during load process;
	uint EmptyCellType;
	
// 	Time_Scale time_per_mcs("MCSDuration",1);
	UpdateData global_update_data;
	
	shared_ptr<LAYER> layer;
	shared_ptr<CPMSampler> cpm_sampler;
	Neighborhood boundary_neighborhood;
	Neighborhood update_neighborhood;
	Neighborhood surface_neighborhood;
	
	bool surface_everywhere=false;
	shared_ptr<EdgeTrackerBase> edgeTracker;
	
	vector< shared_ptr<CellType> > celltypes;
	map< std::string, uint > celltype_names;
	Scope* scope;
	XMLNode xCellPop,xCellTypes,xCPM;
	
	class BoundaryReader : public CPM::LAYER::ValueReader {
	public:
		void set(string input) override {
				state.pos = VINT(0,0,0);
				auto ct = celltype_names.find(input);
				if ( ct== celltype_names.end()) {
					throw MorpheusException(string("Unknown celltype '") + input + "' at the boundary", xCellPop);
				}
				if ( ! dynamic_pointer_cast<MediumCellType>( celltypes[ct->second] ) ) {
					throw MorpheusException(string("Unable to set celltype '")+input+"' at the boundary. " +
							+ "Medium-like celltype required! ", xCellPop);
				}
				state.cell_id = celltypes[ct->second]->createCell();
			}
			bool isSpaceConst() const override { return false; }
			bool isTimeConst() const override { return true; }
			CPM::STATE get(const VINT& pos) const override { CPM::STATE s(state); s.pos=pos; return s; }
			shared_ptr<CPM::LAYER::ValueReader> clone() const override { return make_shared<BoundaryReader>(*this); }
		private:
			CPM::STATE state;
	};
	
	
	
bool isEnabled() { return enabled; }

const Cell& getCell(CELL_ID cell_id) {
	return CellType::storage.cell(cell_id);
}

bool cellExists(CELL_ID cell_id) {
	return (CellType::storage.isFree(cell_id) ? false : true);
}

const CPM::INDEX& getCellIndex(const CELL_ID cell_id) {
	return CellType::storage.index(cell_id);
}


const CPM::STATE& getEmptyState() {
	return EmptyState;
};

uint getEmptyCelltypeID() { return EmptyCellType; }

weak_ptr<const CellType> getEmptyCelltype() {
	return celltypes[ EmptyCellType ];
}

weak_ptr<const CellType> findCellType(string name) {
	if (celltype_names.find(name) != celltype_names.end())
		return celltypes[celltype_names[name]];
	else
		return weak_ptr<const CellType>();
}

vector< weak_ptr<const CellType> > getCellTypes() {
	return vector< weak_ptr<const CellType> > (celltypes.begin(),celltypes.end());
}

map<string, weak_ptr<const CellType> > getCellTypesMap() {
	
	map<string, weak_ptr<const CellType> > m;
	for (auto ct : celltypes ) {
		m[ct->getName()] = weak_ptr<const CellType>(ct);
	}
	return m;
}

double getMCSDuration() {
	if (cpm_sampler)
		return cpm_sampler->MCSDuration();
	else 
		return 0;
};

ostream& operator <<(ostream& os, const CPM::STATE& n) {
	os << n.cell_id << " ["<< n.pos << "]";
	// celltypes[getCellIndex(n.cell_id).celltype] -> getName()
	return os;
}

void enableEgdeTracking()
{
	// Don't enable the edge tracker when just creating a dependency graph
	if (SIM::dependencyGraphMode()) return ;
	
	if (!dynamic_pointer_cast<EdgeListTracker>(edgeTracker)) {
		if ( ! update_neighborhood.empty() ) {
			edgeTracker = shared_ptr<EdgeTrackerBase>(new EdgeListTracker(layer, update_neighborhood.neighbors(), surface_neighborhood.neighbors()));
		}
		else {
			edgeTracker = shared_ptr<EdgeTrackerBase>(new EdgeListTracker(layer, surface_neighborhood.neighbors(), surface_neighborhood.neighbors()));
		}
	}
}

shared_ptr<const EdgeTrackerBase> cellEdgeTracker() {                                                      
	return edgeTracker;
}

const Neighborhood& getBoundaryNeighborhood()
{
	return boundary_neighborhood;
}

const Neighborhood& getSurfaceNeighborhood()
{
	return surface_neighborhood;
}

void setInteractionSurface(bool enabled) {
	surface_everywhere = enabled;
}

bool isSurface(const VINT& pos) {
	return surface_everywhere ? true : edgeTracker->has_surface(pos);
}

uint nSurfaces(const VINT& pos) {
	return edgeTracker->n_surfaces(pos);
};


void loadFromXML(XMLNode xMorph, Scope* local_scope) {
	scope = local_scope;
	if ( ! xMorph.getChildNode("CellTypes").isEmpty() ) 
		loadCellTypes(xMorph.getChildNode("CellTypes"));
	
	boundary_neighborhood = SIM::lattice().getDefaultNeighborhood();
	CPMShape::boundaryNeighborhood = boundary_neighborhood;
	
// 	if (SIM::lattice().getStructure() == Lattice::square)
// 		surface_neighborhood = SIM::lattice().getNeighborhoodByOrder(2);
// 	else if (SIM::lattice().getStructure() == Lattice::hexagonal)
// 		surface_neighborhood = SIM::lattice().getNeighborhoodByOrder(1);
// 	else if (SIM::lattice().getStructure() == Lattice::cubic)
// 		surface_neighborhood = SIM::lattice().getNeighborhoodByOrder(3);
// 	else if (SIM::lattice().getStructure() == Lattice::linear)
// 		surface_neighborhood = SIM::lattice().getNeighborhoodByOrder(1);
	
	surface_neighborhood = SIM::lattice().getDefaultNeighborhood();
	
	if ( surface_neighborhood.distance() > 3 || (SIM::lattice().getStructure()==Lattice::hexagonal && surface_neighborhood.order()>5)) {
		throw string("Default neighborhood is too large for estimation of cell surface nodes");
	}
	
	// look for a medium type in predefined celltypes, or create one ...
	if ( ! celltypes.empty()) {
		for (uint i=0;; i++) {
			if (i == celltypes.size()) {
				// create a default medium celltype, in case no medium was defined
				shared_ptr<CellType> ct =  shared_ptr<CellType>( new MediumCellType( i ) );
				XMLNode medium_node = XMLNode::createXMLTopNode("CellType");
				medium_node.addAttribute("name","Medium");
				medium_node.addAttribute("class","medium");
				ct->loadFromXML(medium_node, scope);
				celltype_names[ct->getName()] = i;
				EmptyCellType = i;
				celltypes.push_back(ct);
				break;
			}
			if (celltypes[i]->isMedium()) {
				EmptyCellType = i;
				break;
			}
		}

		EmptyState.cell_id = celltypes[EmptyCellType]->createCell(); // make sure the the medium contains the cell representing the medium
	}
	
	if ( ! xMorph.getChildNode("CPM").isEmpty() ) {
		xCPM = xMorph.getChildNode("CPM");
		try {
			// CPM Cell representation requires the definition of the CPM ShapeSurface for shape length estimations
			boundary_neighborhood = SIM::lattice().getNeighborhood(xCPM.getChildNode("ShapeSurface").getChildNode("Neighborhood"));
			if ( boundary_neighborhood.distance() > 3 || (SIM::lattice().getStructure()==Lattice::hexagonal && boundary_neighborhood.order()>5)) {
				throw string("Shape neighborhood is too large");
			}
			CPMShape::boundaryNeighborhood = boundary_neighborhood;
		} 
		catch (string e) { 
			throw MorpheusException(e, xCPM.getChildNode("ShapeSurface").getChildNode("Neighborhood"));
		}
		string boundary_scaling;
		if (getXMLAttribute(xCPM,"ShapeSurface/scaling",boundary_scaling)) {
			if (boundary_scaling == "none") {
				CPMShape::scalingMode = CPMShape::BoundaryScalingMode::None;
			}
			else if (boundary_scaling == "norm") {
				CPMShape::scalingMode = CPMShape::BoundaryScalingMode::Magno;
			}
			else if (boundary_scaling == "size") {
				CPMShape::scalingMode = CPMShape::BoundaryScalingMode::NeigborNumber;
			}
			else if (boundary_scaling == "magno") {
				CPMShape::scalingMode = CPMShape::BoundaryScalingMode::Magno;
			}
		}
		
		// CPM time evolution is defined by a MonteCarlo simulation based on the a Hamiltionian and the metropolis kintics
		if ( ! xCPM.getChildNode("MonteCarloSampler").isEmpty() ) {
			cpm_sampler =  shared_ptr<CPMSampler>(new CPMSampler());
			cpm_sampler->loadFromXML(xCPM, scope);
// 			time_per_mcs.set( cpm_sampler->timeStep() );
			update_neighborhood = cpm_sampler->getUpdateNeighborhood();
		}
		
		enabled = true;
		
	}
	if ( ! xMorph.getChildNode("CellPopulations").isEmpty()) {
		xCellPop = xMorph.getChildNode("CellPopulations");
	}
	
	if ( ! celltypes.empty()) {
		cout << "Creating cell layer ";
   
		InitialState = EmptyState;
		cout << "with initial state set to CellType \'" << celltypes[EmptyCellType]->getName() << "\'" << endl;

		layer = shared_ptr<LAYER>(new LAYER(SIM::getLattice(), 3, InitialState, "cpm") );
		assert(layer);
		
		// Setting up lattice boundaries
		if (! xCellPop.isEmpty()) {
			layer->loadFromXML( xCellPop, make_shared<BoundaryReader>());
		}
		
		// Setting the initial state
		VINT size = SIM::lattice().size();
		for (InitialState.pos.z=0; InitialState.pos.z<size.z; InitialState.pos.z++)
			for (InitialState.pos.y=0; InitialState.pos.y<size.y; InitialState.pos.y++)
				for (InitialState.pos.x=0; InitialState.pos.x<size.x; InitialState.pos.x++)
// 					if (cpm_layer->writable(InitialState.pos))
						layer->set(InitialState.pos, InitialState);

		// Creating a default global update template
		global_update_data.boundary = unique_ptr<StatisticalLatticeStencil>(new StatisticalLatticeStencil(layer, boundary_neighborhood.neighbors()));
		global_update_data.surface = unique_ptr<LatticeStencil>(new LatticeStencil(layer, surface_neighborhood.neighbors()));
		if ( ! update_neighborhood.empty() ) {
			if (update_neighborhood.neighbors() == surface_neighborhood.neighbors()) {
				global_update_data.update = global_update_data.surface;
			}
			else {
				global_update_data.update = make_unique<LatticeStencil>(layer, update_neighborhood.neighbors());
			}
			// Setting up the EdgeTracker
			edgeTracker = shared_ptr<EdgeTrackerBase>(new NoEdgeTracker(layer, update_neighborhood.neighbors(), surface_neighborhood.neighbors()));
		}
		else {
			edgeTracker = shared_ptr<EdgeTrackerBase>(new NoEdgeTracker(layer, surface_neighborhood.neighbors(), surface_neighborhood.neighbors()));
		}
		
	}
	else {
		global_update_data.boundary = 0;
		global_update_data.update = 0;
		global_update_data.surface = 0;
	}
	
}


void loadCellTypes(XMLNode xCellTypesNode) {
	xCellTypes = xCellTypesNode;
	for (int i=0; i<xCellTypesNode.nChildNode("CellType");i++) {
		string classname;
		XMLNode xCTNode = xCellTypesNode.getChildNode("CellType",i);
		try {
			if ( ! getXMLAttribute(xCTNode,"class", classname))
				throw string("No classname provided for celltype ")+to_str(i);
			shared_ptr<CellType> ct = CellTypeFactory::CreateInstance( classname, celltypes.size() );
			if (ct.get() == NULL)
				throw string("No celltype class ")+classname+" available";
			ct->loadFromXML( xCTNode, scope );
			string name=ct->getName();
			if (name.empty())
				throw string("No name for provided for celltype ")+to_str(i);
			if (name.find_first_of(" \t\n\r") != string::npos)
				throw string("Celltype names may not contain whitespace characters. Invalid name \"")+name+" \"";
			if (celltype_names.find(name) != celltype_names.end()) 
				throw string("Redefinition of celltype ")+name;
			
			celltype_names[name] = celltypes.size();
			celltypes.push_back(ct);
			
			auto celltype_name = SymbolAccessorBase<double>::createConstant(string("celltype.") + name+ ".id", "CellType ID", double(celltype_names[name]));
			scope->registerSymbol(celltype_name);
 			auto celltype_size = make_shared<CellPopulationSizeSymbol>(string("celltype.") + name + ".size", celltypes.back().get());
			scope->registerSymbol(celltype_size);
		}
		catch (string e) {
			throw MorpheusException(string("Unable to create CellType\n") + e, xCTNode);
		}
	}
	
	for (uint i=0; i<celltypes.size(); i++) {
		celltypes[i]->loadPlugins();
#ifdef HAVE_SUPERCELLS
		if (dynamic_pointer_cast<SuperCT>(celltypes[i]) )
			dynamic_pointer_cast<SuperCT>(celltypes[i])->bindSubCelltype();
#endif
	}
	
	if (!celltype_names.empty()) {
		cout << "CellTypes defined: ";
		for (map<string,uint>::iterator ct1=celltype_names.begin(); ct1 != celltype_names.end();ct1++)
			cout << "\'" << ct1->first << "\' ";
		cout << endl;
	}
}

void init() {

	loadCellPopulations();

	// Init the CellTypes and their (CPM) Plugins
	for (uint i=0; i<celltypes.size(); i++) {
		cout << "Initializing celltype \'" << celltypes[i]->getName() << "\'" <<endl;
		celltypes[i]->init();
	}
	// Init the sampler
	if ( cpm_sampler) {
		cpm_sampler->init(SIM::getGlobalScope());
	}
}

void loadCellPopulations()
{
	// Don't load the CellPopulations when we just want to create the symbol graph.
	if (SIM::dependencyGraphMode())
		return;
	
	if ( ! layer &&  ! xCellPop.isEmpty()) {
		// We need at least a cpm Layer and have to specify the neighborhood ...
		throw MorpheusException(string("Unable to create cell populations with no CPM layer available."), xCellPop);
	}
// 	if ( celltypes.size() > 1 && populations.isEmpty()) {
// 		cerr << "No CellPopulations specified." << endl;
// 	}
	
	
	vector<XMLNode> defered_poulations;
	for (int i=0; i<xCellPop.nChildNode("Population"); i++) {
		XMLNode population = xCellPop.getChildNode("Population",i);
		string type;
		getXMLAttribute( population,"type",type);
		auto ct= celltype_names.find(type);
		if (ct == celltype_names.end()) {
			throw MorpheusException(string("Unable to create cell populations for celltype \"")+type+"\"", xCellPop);
		}

#ifdef HAVE_SUPERCELLS
		if (dynamic_pointer_cast< SuperCT >(celltypes[ct->second]) ) {
			defered_poulations.push_back(population);
			continue;
		}
#endif

		celltypes[ct->second] -> loadPopulationFromXML(population);
	}

	for (uint i=0; i<defered_poulations.size(); i++) {
		// we already know that the celltype exists !!
		string type;
		getXMLAttribute( defered_poulations[i],"type",type);
		celltypes[celltype_names[type]] -> loadPopulationFromXML(defered_poulations[i]);
	}
}

XMLNode saveCPM() { return xCPM; };

XMLNode saveCellTypes() { return xCellTypes; }

XMLNode saveCellPopulations() {
	if ( ! CPM::celltypes.empty() ) {
		XMLNode xCP = XMLNode::createXMLTopNode("CellPopulations");
		for (uint ct=0; ct<celltypes.size(); ++ct ) {
			xCP.addChild(celltypes[ct] -> savePopulationToXML());
		}
		xCP.addChild(layer->saveToXML().getChildNode("BoundaryValues") );
		return xCP; 
	}
	else 
		return XMLNode();
}

shared_ptr<const CPM::LAYER> getLayer() {
	if (!layer) {
		throw string("Cell layer is undefined. Probably no CellTypes have been defined.");
	}
	return layer;
};

const CPM::STATE& getNode(const VINT& pos) {
	assert(layer);
	return layer->get(pos);
};

bool executeCPMUpdate(const CPM::Update& update) {
//      cout << cpm_layer->get(update.focus) << endl;
//      cout << update.focus.pos() << " - "<< update.remove_state.cell_id << " - " << update.add_state.cell_id << endl;
	assert(SIM::lattice().equal_pos(update.focus().pos(), update.focusStateAfter().pos));
	assert(SIM::lattice().equal_pos(update.focusStateBefore().pos, update.focusStateAfter().pos));

	try {
		if (update.opAdd() && update.opRemove()) {
			
			if ( update.focus().celltype() == update.focusUpdated().celltype()) {
				celltypes[update.focus().celltype()] -> apply_update(update.selectOp(Update::ADD_AND_REMOVE));
			}
			else {
				celltypes[update.focus().celltype()] -> apply_update(update.selectOp(Update::REMOVE));
				celltypes[update.focusUpdated().celltype()] -> apply_update(update.selectOp(Update::ADD));
			}
			
			// Notify all the cells that are adajcent to the focal node wrt. the boundary neighborhood
			if (update.opNeighborhood()) {
				const vector<StatisticalLatticeStencil::STATS>& neighbor_stats = update.boundaryStencil()->getStatistics();
				for (uint i=0; i<neighbor_stats.size(); i++) {
					if ( (neighbor_stats[i].cell != update.focusStateAfter().cell_id) && (neighbor_stats[i].cell != update.focusStateBefore().cell_id)) {
						CELL_INDEX_STATE state = getCellIndex(neighbor_stats[i].cell).status;
						if ( state != NO_CELL && state != VIRTUAL_CELL)
							CellType::storage.cell(neighbor_stats[i].cell) . applyUpdate(update.selectOp(Update::NEIGHBORHOOD_UPDATE));
					}
				}
			}
			VINT position = update.focus().pos();
// 			assert( layer -> writable_resolve(position) );
			layer->set(position,update.focusStateAfter());
			assert(edgeTracker);
			if (update.updateStencil())
				edgeTracker->update_notifier(position, *update.updateStencil());
			else 
				edgeTracker->update_notifier(position, *update.surfaceStencil());
		}
		else if (update.opMove()) {
			
		}
	} catch ( string e ) {
		stringstream s;
		s << "error while applying executeCPMUpdate()" << endl;
		s << "[" << update.focusStateBefore() << "] -> [" << update.focusStateAfter() << "]" << endl;
		s << e << endl;
		throw s.str();
	}
	return true;
}

CPM::Update& getGlobalUpdate() { static Update global_update(&global_update_data, layer); return global_update;}

const CPM::Update& createUpdate(VINT source, VINT direction, CPM::Update::Operation opx) {
	
	VINT latt_pos = source + direction;
	Update& global_update = getGlobalUpdate();
	global_update.unset();
	if ( ! layer->writable_resolve(latt_pos) ) {
		cout << "Cannot write to constant node " << latt_pos << ". Rejecting update." << endl;
	}
	else {
		global_update.set(source,direction, opx);
	}
	
	setUpdate(global_update);

	return global_update;
	
}

bool setNode(VINT position, CPM::CELL_ID cell_id) {

	VINT latt_pos = position;
	if ( ! layer->writable_resolve(latt_pos) ) {
		cout << "setNode(): Rejecting write to constant node at " << latt_pos << "." << endl;
		return false;
	}
	
	Update& global_update = getGlobalUpdate();
	global_update.set(position, cell_id); 
	
	if (global_update.valid()) {
		setUpdate(global_update);
		return executeCPMUpdate(global_update);
	}
	else 
		return false;

};

VINT findEmptyNode(VINT min , VINT max) {
	// default behaviour -- any point in the lattice
	if (max == VINT(0,0,0)) max = SIM::lattice().size() - VINT(1,1,1);
	VINT pos;
	for ( int i=0; ; i++) {
		VINT a(0,0,0);
		pos.x= min.x + int(getRandomUint(uint(max.x - min.x)));
		if ( SIM::lattice().getDimensions() > 1) pos.y = min.y +  int(getRandomUint(uint(max.y - min.y)));
		if ( SIM::lattice().getDimensions() > 2) pos.z = min.z +  int(getRandomUint(uint(max.z - min.z)));
		if ( layer->get(pos) == EmptyState  && layer->writable(pos)  ) break;
		if (i==10000) throw string("findEmptyCPMNode: Unable to find empty node for random cell");
	}
	return pos;
}


CELL_ID setCellType(CPM::CELL_ID cell_id, uint celltype)
{
	assert (celltype < celltypes.size());
	uint old_celltype = getCellIndex(cell_id).celltype;
	return celltypes[celltype]->addCell(cell_id);
}

void setUpdate(CPM::Update& update) {
	// we assume that focus source, add_state and remove_state are already set properly
// 	if (update.interaction)
// 		update.interaction->setPosition(update.focus.pos());
// 	update.boundary->setPosition(update.focus.pos());

	// Fwd adding nodes to a supercell to the first subcell
// 	if (update.focusUpdated().cell_index().status == SUPER_CELL ) {
// 		update.add_state.super_cell_id = update.add_state.cell_id;
// 		update.add_state.cell_id = static_cast<const SuperCell&>(getCell(update.add_state.super_cell_id)).getSubCells().front();
// 		update.source = SymbolFocus(update.add_state.cell_id, update.add_state.pos);
// 	}
	
	// Find the proper celltype to notify
// 	update.source_top_ct = update.source.celltype();
// 	update.focus_top_ct =  update.focus.celltype();
	
// 	if ( update.source.cell_index().status == SUB_CELL ) {
// 		// notify the supercell containing the cell
// 		update.source_top_ct = getCellIndex(update.add_state.super_cell_id).celltype;
// 	}
// 
// 	if ( update.focus.cell_index().status == SUB_CELL ) {
// 		// notify the supercell containing the cell
// 		update.focus_top_ct = getCellIndex(update.remove_state.super_cell_id).celltype;
// 	}
	
	if (update.opAdd() && update.opRemove()) {
		if ( update.focus().cell_index().celltype == update.focusUpdated().cell_index().celltype ) {
			celltypes[update.focus().cell_index().celltype] -> set_update(update.selectOp(Update::ADD_AND_REMOVE));
		}
		else {
			celltypes[update.focusUpdated().cell_index().celltype] -> set_update(update.selectOp(Update::ADD));
			celltypes[update.focus().cell_index().celltype] -> set_update(update.selectOp(Update::REMOVE));
		}
	}
	if (update.opNeighborhood()) {
		const vector<StatisticalLatticeStencil::STATS>& neighbor_stats = update.boundaryStencil()->getStatistics();
		for (uint i=0; i<neighbor_stats.size(); i++) {
			if ( (neighbor_stats[i].cell != update.focusStateAfter().cell_id) && (neighbor_stats[i].cell != update.focusStateBefore().cell_id)) {
				CELL_INDEX_STATE state = getCellIndex(neighbor_stats[i].cell).status;
				if ( state != NO_CELL && state != VIRTUAL_CELL)
					CellType::storage.cell(neighbor_stats[i].cell) . setUpdate(update.selectOp(CPM::Update::NEIGHBORHOOD_UPDATE));
			}
		}
	}
	
}

void finish() {
	celltypes.clear();
	celltype_names.clear();
	cpm_sampler.reset();
	layer.reset();
}

}
