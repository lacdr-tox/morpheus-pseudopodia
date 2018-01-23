#define SIMULATION_CPP

// #define NO_CORE_CATCH
#ifdef NO_CORE_CATCH
#warning "NO_CORE_CATCH defined. Do not use for productive systems !!"
#endif

#include "simulation_p.h"
#include "edge_tracker.h"

int main(int argc, char *argv[]) {
    return SIM::main(argc,argv);
}

typedef std::normal_distribution<double> RNG_GaussDist;
typedef std::gamma_distribution<double> RNG_GammaDist;

bool getRandomBool() {
	return random_engines[ omp_get_thread_num() ]()<random_engines[ omp_get_thread_num() ].max()/2;
}

double getRandom01() {
	static uniform_real_distribution <double> rnd(0.0,1.0);
	return rnd(random_engines[omp_get_thread_num()]);
}

// random gaussian distribution of stddev s
double getRandomGauss(double s) {
	RNG_GaussDist rnd( 0.0, s);
	return rnd(random_engines_alt[omp_get_thread_num()]);
}

double getRandomGamma(double shape, double scale) {

    RNG_GammaDist rnd( shape );
    return scale*rnd(random_engines_alt[omp_get_thread_num()]);

}

uint getRandomUint(uint max_val) {
	uniform_int_distribution<uint> rnd(0,max_val);
    return rnd(random_engines[omp_get_thread_num()]);
}

namespace CPM {
	
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
	return time_per_mcs();
};

ostream& operator <<(ostream& os, const CPM::STATE& n) {
	os << n.cell_id << " ["<< n.pos << "]";
	// celltypes[getCellIndex(n.cell_id).celltype] -> getName()
	return os;
}

void enableEgdeTracking()
{
	// Don't enable the edge tracker when just creating a dependency graph
	if (SIM::generate_symbol_graph_and_exit) return ;
	
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


void loadFromXML(XMLNode xMorph) {
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
				ct->loadFromXML(medium_node, SIM::global_scope.get());
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
			cpm_sampler->loadFromXML(xCPM, SIM::global_scope.get());
			time_per_mcs.set( cpm_sampler->timeStep() );
			update_neighborhood = cpm_sampler->getUpdateNeighborhood();
		}
		
	}
	if ( ! xMorph.getChildNode("CellPopulations").isEmpty()) {
		xCellPop = xMorph.getChildNode("CellPopulations");
	}
	
	if ( ! celltypes.empty()) {
		cout << "Creating cell layer ";
   
		InitialState = EmptyState;
		cout << "with initial state set to CellType \'" << celltypes[EmptyCellType]->getName() << "\'" << endl;

		layer = shared_ptr<LAYER>(new LAYER(SIM::global_lattice, 3, InitialState, "cpm") );
		assert(layer);
		
		// Setting up lattice boundaries
		if (! xCellPop.isEmpty()) {
			layer->loadFromXML( xCellPop, make_shared<BoundaryReader>());
		}
		
		// Setting the initial state
		VINT size = SIM::global_lattice->size();
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
			ct->loadFromXML( xCTNode, SIM::global_scope.get() );
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
			SIM::global_scope->registerSymbol(celltype_name);
 			auto celltype_size = make_shared<CellPopulationSizeSymbol>(string("celltype.") + name + ".size", celltypes.back().get());
			SIM::global_scope->registerSymbol(celltype_size);
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
	if (SIM::generate_symbol_graph_and_exit)
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

// const Neighborhood& getInteractionNeighborhood() {
// 	if (cpm_sampler)
// 		return cpm_sampler->getInteractionNeighborhood();
// 	else
// 		return layer->lattice().getDefaultNeighborhood();
// };


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
	assert(SIM::global_lattice->equal_pos(update.focus().pos(), update.focusStateAfter().pos));
	assert(SIM::global_lattice->equal_pos(update.focusStateBefore().pos, update.focusStateAfter().pos));

	try {
		if (update.opAdd() && update.opRemove()) {
			
			if ( update.focus().celltype() == update.focusUpdated().celltype()) {
				celltypes[update.focus().celltype()] -> apply_update(update.selectOp(Update::ADD_AND_REMOVE));
			}
			else {
				celltypes[update.focusUpdated().celltype()] -> apply_update(update.selectOp(Update::ADD));
				celltypes[update.focus().celltype()] -> apply_update(update.selectOp(Update::REMOVE));
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
	if ( ! layer->writable_resolve(latt_pos) ) {
		cout << "Cannot write to constant node " << latt_pos << ". Rejecting update." << endl;
		global_update.unset();
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
	
/*	global_update.source = VINT(0,0,0);
	
	global_update.add_state.pos = cell_pos;
	global_update.add_state.cell_id = cell_id;
	global_update.remove_state = layer->get(global_update.focus.pos());*/

	if (global_update.valid()) {
		setUpdate(global_update);
		return executeCPMUpdate(global_update);
	}
	else 
		return false;

};

VINT findEmptyNode(VINT min , VINT max) {
	// default behaviour -- any point in the lattice
	if (max == VINT(0,0,0)) max = SIM::global_lattice->size() - VINT(1,1,1);
	VINT pos;
	for ( int i=0; ; i++) {
		VINT a(0,0,0);
		pos.x= min.x + int(getRandomUint(uint(max.x - min.x)));
		if ( SIM::global_lattice->getDimensions() > 1) pos.y = min.y +  int(getRandomUint(uint(max.y - min.y)));
		if ( SIM::global_lattice->getDimensions() > 2) pos.z = min.z +  int(getRandomUint(uint(max.z - min.z)));
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

}


namespace SIM {
// #define NO_CORE_CATCH
int main(int argc, char *argv[]) {
	bool exception = false;
#ifndef NO_CORE_CATCH
	try {
#endif
		
    double init0 = get_wall_time();
	init(argc, argv);
	
	if (generate_symbol_graph_and_exit){
		cout << "Generated symbol dependency graph. Exiting." << endl;
		return 0;
	}
	
    double init1 = get_wall_time();
	size_t initMem = getCurrentRSS();
	
	//  Start Timers
    double wall0 = get_wall_time();
    double cpu0  = get_cpu_time();

	TimeScheduler::compute();
	
    //  Stop timers
    double wall1 = get_wall_time();
    double cpu1  = get_cpu_time();

	finalize();


	cout << "\n=== Simulation finished ===\n";
	string init_time = prettyFormattingTime( init1 - init0 );
	string cpu_time = prettyFormattingTime( cpu1 - cpu0 );
	string wall_time = prettyFormattingTime( wall1 - wall0 );
	size_t peakMem = getPeakRSS();
	
	cout << "Init Time   = " << init_time << "\n";
	cout << "Wall Time   = " << wall_time << "\n";
	cout << "CPU Time    = " << cpu_time  << " (" << numthreads << " threads)\n\n";
	cout << "Memory peak = " << prettyFormattingBytes(peakMem) << "\n";
	
	ofstream fout("performance.txt", ios::out);
    fout << "Threads\tInit(s)\tCPU(s)\tWall(s)\tMem(Mb)\n";
    fout << numthreads << "\t" << (init1-init0) << "\t" << (cpu1-cpu0) << "\t" << (wall1-wall0) << "\t" << (double(peakMem)/(1024.0*1024.0)) << "\n";
	fout.close();

#ifndef NO_CORE_CATCH
	}
	catch (MorpheusException e) {
		exception = true;
		cerr << "\n" << e.what()<< "\n";
		cerr << "\nXMLPath: " << e.where() << endl;
	}
	catch (string e) {
		exception = true;
		cerr << e << endl;
	}
	catch (SymbolError& e) {
// 		switch(e.type()) {
// 			case SymbolError::Type::InvalidDefinition
// 		}
		exception = true;
		cerr << "\n" << e.what()<< "\n";
	}
	catch (...) {
		cerr << "Unknown error while creating the simulation";
		exception = true;
	}
#endif
	if (exception) {
		cerr.flush();
		if (SIM::generate_symbol_graph_and_exit) {
			createDepGraph();
		}
		exit(-1);
	}
	return 0;
}

string prettyFormattingTime( double time_in_sec ) {
	char s[100];
	if( time_in_sec >= 0) {
        int msec = time_in_sec*1000;
        int millisec=(msec)%1000;
        int seconds=(msec/1000)%60;
        int minutes=(msec/(1000*60))%60;
        int hours=(msec/(1000*60*60))%24;
		int days=(msec/(1000*60*60*24));
		
		if (days > 0)
			std::sprintf(s, "%dd %02dh %02dm", days, hours, minutes);
        else if(hours > 0)
            std::sprintf(s, "%dh %02dm", hours, minutes);
        else if( minutes > 0 )
            std::sprintf(s, "%dm %02ds", minutes, seconds);
        else
            std::sprintf(s, "%1ds %03dms", seconds, millisec);
    }
    else
        std::sprintf(s, " -- ");
	return string(s);
}

// Prints to the provided buffer a nice number of bytes (KB, MB, GB, etc)
string prettyFormattingBytes(uint bytes)
{
    vector<string> suffixes;
	suffixes.resize(7);
    suffixes[0] = "B";
    suffixes[1] = "Kb";
    suffixes[2] = "Mb";
    suffixes[3] = "Gb";
    suffixes[4] = "Tb";
    suffixes[5] = "Pb";
    suffixes[6] = "Eb";
    uint s = 0; // which suffix to use
    double count = bytes;
    while (count >= 1024 && s < 7)
    {
        s++;
        count /= 1024;
    }
 	stringstream ss;
	if (count - floor(count) == 0.0)
		ss << (int)count << " " << suffixes[s];
        //sprintf(buf, "%d %s", (int)count, suffixes[s]);
    else
		ss << count << " " << suffixes[s];
        //sprintf(buf, "%.1f %s", count, suffixes[s]);
	return ss.str();
}

const string getTitle() {
	return fileTitle;
}


double getNodeLength()  {
    return node_length();
};

string getLengthScaleUnit() {
	if (node_length.getLengthScaleUnit() == "alu")
		return "alu";
	else
		return "meter";
};

double getLengthScaleValue() {
	return node_length.getLengthScaleValue();
};


string getTimeScaleUnit() {
	if (TimeScheduler::getTimeScaleUnit() == "atu")
		return "atu";
	else
		return "sec";
};

// double getTimeScaleValue() {
// 	return sim_stop_time.getTimeScaleValue();
// };

string getTimeName() {
	return getTimeName(getTime());
}

string getTimeName(double time) {
	
	stringstream sstr;
	sstr << fixed << TimeScheduler::getStopTime();
	int length = sstr.str().find_first_of(".") + (AnalysisPlugin::max_time_precision >0 ? AnalysisPlugin::max_time_precision + 1 : 0);
	length = max(5,length);
	sstr.str("");
	
	sstr << setw(length) << setfill('0') << fixed << setprecision(AnalysisPlugin::max_time_precision) << time;
	return sstr.str();
}

double TimeSymbol::get(const SymbolFocus&) const {	return TimeScheduler::getTime(); }

double getTime() {
	return TimeScheduler::getTime();
};

double getStopTime(){
	return TimeScheduler::getStopTime();	
};

double getStartTime(){
	return TimeScheduler::getStartTime();	
};

string centerText(string in) {
	int pos=(int)((80-in.length())/2);
	string out(pos,' ');
	out.append(in);
	return out;
}

void splash(bool show_usage) {

    time_t t = time(0); // get time now
    struct tm * now = localtime( & t );
    int current_year = now->tm_year + 1900;

	cout << endl;
	cout << centerText("<<  M O R P H E U S  >>") << endl;
	cout << centerText("Modeling environment for multi-scale and multicellular systems biology") << endl;
    stringstream copyright;
    copyright << "Copyright 2009-"<< current_year <<", Technische Universität Dresden, Germany";
    cout << centerText( copyright.str() ) << endl;

	stringstream version;
	version << "Version " << MORPHEUS_VERSION_STRING;
	version << ", Revision " << MORPHEUS_REVISION_STRING;
    cout << centerText( version.str() ) << endl;

    if( show_usage ){
    cout << endl << endl;
    cout << " Usage: "<< endl;
    cout << "  morpheus [OPTIONS] " << endl << endl;
    cout << " Options:  " << endl;
    cout << " -file [XML-FILE]      Run simulator with XML configuration file" << endl;
	cout << " -[KEY]=[VALUE]  		Override the value of Constant symbol" << endl;
    cout << " -version              Show release version" << endl;
    cout << " -revision             Show SVN revision number" << endl;
	cout << " -gnuplot-version      Show version of GnuPlot used" << endl;
	cout << " -gnuplot-path [FILE]  Set the path to the GnuPlot executable" << endl;
    cout << endl << endl;
    }

	cout << " External applications" << endl;
	try {
		cout << "  GnuPlot executable:   " <<  Gnuplot::get_GNUPlotPath() << endl;
		cout << "  GnuPlot version:      " <<  Gnuplot::version() << endl;
	}
	catch (...) {
		cout << "Morpheus cannot find/run GnuPlot executable" << endl;
	}
	cout << endl << endl;
}


void init(int argc, char *argv[]) {

	std::map<std::string, std::string> cmd_line = ParseArgv(argc,argv);

// 	for (map<string,string>::const_iterator it = cmd_line.begin(); it != cmd_line.end(); it++ ) {
// 		cout << "option " << it->first << " -> " << it->second << endl;
// 	}
	if (cmd_line.find("revision") != cmd_line.end()) {
		cout << "Revision: " << MORPHEUS_REVISION_STRING << endl;
		exit(0);
	}

	if (cmd_line.find("version") != cmd_line.end()) {
		cout << "Version: " << MORPHEUS_VERSION_STRING << endl;
		exit(0);
	}

	if (cmd_line.find("gnuplot-path") != cmd_line.end()) {
		Gnuplot::set_GNUPlotPath(cmd_line["gnuplot-path"]);
		cmd_line.erase(cmd_line.find("gnuplot-path"));
	}
	
	if (cmd_line.find("gnuplot-version") != cmd_line.end()) {
		string version;
		try {
			version = Gnuplot::version();
		}
		catch (GnuplotException &e) {
			cout << e.what();
			exit(0);
		}
		cout << version << endl;
		exit(0);
	}

	if (cmd_line.find("symbol-graph") != cmd_line.end()) {
		generate_symbol_graph_and_exit = true;
		cmd_line.erase(cmd_line.find("symbol-graph"));
	}
	else {
		generate_symbol_graph_and_exit = false;
	}

    bool show_usage = false;
	if ( argc  == 1 ) {
        show_usage = true;
        splash( show_usage );
        cout << "No arguments specified." << endl;
        exit(0);
    }
    splash( show_usage );


// TODO Handling missing file( a file parameter must be provided and the file must exist)

	string filename = cmd_line["file"];
	cmd_line.erase(cmd_line.find("file"));

	struct stat filestatus;
	int filenotexists = stat( filename.c_str(), &filestatus );
	if ( filenotexists > 0 || filename.empty() ) {
		cerr << "Error: file '" << filename << "' does not exist." << endl;
		exit(-1);
	}
	else if ( filestatus.st_size == 0 ) {
		cerr << "Error: file '" << filename << "' is empty." << endl;
		exit(-1);
	}

	XMLNode xMorpheusRoot;
	if (filename.size() > 3 and filename.substr(filename.size()-4,3) == ".gz") {
		cerr << "You must unzip the model file before using it\n";
		exit(-1);
	} else {
		xMorpheusRoot = parseXMLFile(filename);
	}

	global_scope = unique_ptr<Scope>(new Scope());
	// Attach global overrides to the global scope
	for (map<string,string>::const_iterator it = cmd_line.begin(); it != cmd_line.end(); it++ ) {
		if (it->first == "file") continue;
		global_scope->value_overrides()[it->first] = it->second;
	}
	current_scope = global_scope.get();
	CPM::EmptyState.cell_id = 0;
	
	loadFromXML(xMorpheusRoot);
	
	
	if (SIM::generate_symbol_graph_and_exit) {
		createDepGraph();
		exit(0);
	}

	// try to match cmd line options with symbol names and adjust values accordingly
	// check that global overrides have been used
	for ( const auto& override: global_scope->value_overrides() ) {
		cout << "Unknown cmd line override " << override.first << "=" << override.second << endl;
	}

	cout.flush();
};

void finalize() {
	TimeScheduler::finish();
}

void setRandomSeeds( const XMLNode xNode ){
	// initialize multiple random engines (one for each thread) and set seed
	// 1. make vector of random engines
	numthreads = 1;		
#pragma omp parallel
	{
		numthreads = omp_get_num_threads();
	}
	random_engines.resize( numthreads );
	random_engines_alt.resize( numthreads );

	random_seed = time(NULL);

    if ( ! xNode.isEmpty() ){
			getXMLAttribute(xNode,"value",random_seed);
        }
    else{
        cout << "Time/RandomSeed not specified, using arbitray seed (based on time)." << endl;
    }
		
		// 2. set random seed of first engine taken from XML
		random_engines[0].seed(random_seed);
		random_engines_alt[0].seed(random_seed);
        cout << "Random seed of master thread = " << random_seed << endl;

		// 3. generate random seeds for other engines using the first engine.
		vector<uint> random_seeds(numthreads,0);
		for(uint i=1; i<numthreads; i++){
// #ifdef USING_CXX0X_TR1
			uniform_int_distribution<> rnd(0,9999999);
// #else
// 			uniform_int<> rnd(0,9999999);
// #endif
			random_seeds[i] = rnd(random_engines[0]);
		}

		// 4. set random seed of other engines (for other threads)
#pragma omp parallel
		{
			uint thread = omp_get_thread_num();
			if( thread > 0){
				random_engines[ thread ].seed(random_seeds[ thread ] );
				random_engines_alt[ thread ].seed(random_seeds[ thread ] );
#pragma omp critical
                cout << "Random seed of thread " << thread << " = " << random_seeds[ thread ] << endl;
			}
		}
// 	}
}

void createDepGraph() {
	shared_ptr<AnalysisPlugin> dep_graph_writer;
	
	for (uint i=0;i<analysers.size();i++) {
		if (analysers[i]->XMLName() == "DependencyGraph") {
			dep_graph_writer = analysers.at(i);
			dep_graph_writer->setParameter("format", dep_graph_format);
			break;
		}
	}
	if (!dep_graph_writer) {
		dep_graph_writer = dynamic_pointer_cast<AnalysisPlugin>(PluginFactory::CreateInstance("DependencyGraph"));
		if (!dep_graph_writer) {
			cerr << "Unable to create instance for DependencyGraph Plugin." << endl;
			return;
		}
		dep_graph_writer->setParameter("format",dep_graph_format);
		dep_graph_writer->init(getGlobalScope());
	}
	dep_graph_writer->analyse(0);
}

void loadFromXML(const XMLNode xNode) {

// Loading simulation parameters
	string nnn;
	/*********************************************/
	/** LOADING XML AND REGISTRATION OF SYMBOLS **/
	/*********************************************/
	
	
	getXMLAttribute(xNode,"version",morpheus_file_version);
	xDescription = xNode.getChildNode("Description");
	getXMLAttribute(xDescription,"Title/text",fileTitle);
	XMLNode xTime = xNode.getChildNode("Time");
	TimeScheduler::loadFromXML(xTime, global_scope.get());
	
	
	getXMLAttribute(xTime,"TimeSymbol/symbol",SymbolBase::Time_symbol);
	global_scope->registerSymbol(make_shared<TimeSymbol>(SymbolBase::Time_symbol));
	
	xSpace = xNode.getChildNode("Space");
	getXMLAttribute(xSpace,"SpaceSymbol/symbol",SymbolBase::Space_symbol);
	global_scope->registerSymbol(make_shared<SpaceSymbol>(SymbolBase::Space_symbol));
	
	setRandomSeeds(xTime.getChildNode("RandomSeed"));
	
	// Loading and creating the underlying lattice
	cout << "Creating lattice"<< endl;
	XMLNode xLattice = xSpace.getChildNode("Lattice");
	if (xLattice.isEmpty()) throw string("unable to read XML Lattice node");
	
	if (xLattice.nChildNode("NodeLength"))
		node_length.loadFromXML(xLattice.getChildNode("NodeLength"), global_scope.get());
	try {
		string lattice_code="cubic";
		getXMLAttribute(xLattice, "class", lattice_code);
		if (lattice_code=="cubic") {
			global_lattice =  shared_ptr<Lattice>(new Cubic_Lattice(xLattice));
		} else if (lattice_code=="square") {
			global_lattice =  shared_ptr<Lattice>(new Square_Lattice(xLattice));
		} else if (lattice_code=="hexagonal") {
			global_lattice =  shared_ptr<Lattice>(new Hex_Lattice(xLattice));
		} else if (lattice_code=="linear") {
			global_lattice =  shared_ptr<Lattice>(new Linear_Lattice(xLattice));
		}
		else throw string("unknown Lattice type " + lattice_code);
		if (! global_lattice)
				throw string("Error creating Lattice type " + lattice_code);
	}
	catch (string e) {
		throw MorpheusException(e,xLattice);
	}
	
	lattice_size_symbol="";
	if (getXMLAttribute(xLattice,"Size/symbol",lattice_size_symbol)) {
		auto lattice_size = SymbolAccessorBase<VDOUBLE>::createConstant(lattice_size_symbol,"Lattice Size", global_lattice->size());
		global_scope->registerSymbol( lattice_size );
	}
	
	MembranePropertyPlugin::loadMembraneLattice(xSpace, global_scope.get());
	
	// Loading global definitions
	if (xNode.nChildNode("Global")) {
		xGlobals = xNode.getChildNode("Global");
		cout << "Loading [" << xGlobals.nChildNode() << "] Global Plugins" <<endl;
		for (int i=0; i<xGlobals.nChildNode(); i++) {
			XMLNode xGlobalChild = xGlobals.getChildNode(i);
			string xml_tag_name(xGlobalChild.getName());
			shared_ptr<Plugin> p = PluginFactory::CreateInstance(xml_tag_name);
			
			if (! p.get())
				throw MorpheusException(string("Unknown Global plugin ") + xml_tag_name, xGlobalChild);
			
			p->loadFromXML(xGlobalChild, global_scope.get());
			global_section_plugins.push_back(p);
		}
	}
	
	// Loading cell types, CPM and CellPopulations
	CPM::loadFromXML(xNode);

	/*****************************************************/
	/** CREATION AND INTERLINKING of the DATA STRUCTURE **/
	/*****************************************************/
	
	// all model constituents are loaded. let's initialize them (i.e. interlink)
	global_scope->init();
	for (auto glob : global_section_plugins) {
#ifdef NO_CORE_CATCH
		glob->init(SIM::getGlobalScope());
#else
		try {
			glob->init(SIM::getGlobalScope());
		}
		catch (string e) {
			string s("Simulation Error in Plugin ");
			s+= glob->XMLName() + "\n" + e;
			throw MorpheusException(s,glob->getXMLNode());
		}
#endif
	}

	CPM::init();

	XMLNode xAnalysis = xNode.getChildNode("Analysis");
	if ( ! xAnalysis.isEmpty() ) {
		cout << "Loading Analysis tools [" << xAnalysis.nChildNode() << "]" <<endl;
		for (int i=0; i<xAnalysis.nChildNode(); i++) {
			XMLNode xNode = xAnalysis.getChildNode(i);
			try {
				string xml_tag_name(xNode.getName());
				shared_ptr<Plugin> p = PluginFactory::CreateInstance(xml_tag_name);
				
				if (! p.get()) 
					throw(string("Unknown analysis plugin " + xml_tag_name));
				
				p->loadFromXML(xNode, SIM::global_scope.get());
				
				if (dynamic_pointer_cast<AnalysisPlugin>(p) ) {
					analysers.push_back( dynamic_pointer_cast<AnalysisPlugin>(p) );
				}
			}
			catch (string er) {
				cout << er << endl;
			}
		}
	}

	// before loading all the Analysis tools that might create some files we should switch the cwd
	for (uint i=0;i<analysers.size();i++) {
		analysers[i]->init(global_scope.get());
	}
	for (uint i=0;i<analysis_section_plugins.size();i++) {
		analysis_section_plugins[i]->init(global_scope.get());
	}
	
	TimeScheduler::init();
	cout << "model is up" <<endl;
};


void saveToXML() {
	XMLNode xMorpheusNode;
	ostringstream filename("");
	filename << fileTitle << setfill('0') << setw(6) << getTimeName() << ".xml.gz";
	cout << "Saving " << filename.str()<< endl;

	xMorpheusNode = XMLNode::createXMLTopNode("MorpheusModel");
	if (!morpheus_file_version.empty())
		xMorpheusNode.addAttribute("version",morpheus_file_version.c_str());

	XMLNode xTimeNode = xMorpheusNode.addChild( TimeScheduler::saveToXML() );

	xMorpheusNode.addChild(xDescription);

	xMorpheusNode.addChild(xSpace);
	
	// saving Field data
	// TODO:: global_scope::saveToXML -> Field / VectorField
	for (auto plugin : global_section_plugins) {
		xGlobals.addChild(plugin->saveToXML());
	}
	
	// saving global_scope
	xMorpheusNode.addChild(xGlobals);

	// saving cell types
	xMorpheusNode.addChild(CPM::saveCellTypes());
	
	// save CPM details (interaction energy and metropolis kinetics)
	xMorpheusNode.addChild(CPM::saveCPM());
	
	if ( ! (analysers.empty() && analysis_section_plugins.empty() )) {
		XMLNode xAnalysis = xMorpheusNode.addChild("Analysis" );
		for (uint i=0; i<analysis_section_plugins.size(); i++ ) {
			xAnalysis.addChild(analysis_section_plugins[i]->saveToXML());
		}
		for (uint i=0; i<analysers.size(); i++ ) {
			xAnalysis.addChild(analysers[i]->saveToXML());
		}
	}

	/****************************/
	/** SAVING SIMULATION DATA **/
	/****************************/

	// cell populations
	xMorpheusNode.addChild(CPM::saveCellPopulations());

	int xml_size;
	XMLSTR xml_data=xMorpheusNode.createXMLString(1,&xml_size);

	gzFile zfile = gzopen(filename.str().c_str(), "w9");
	if (Z_NULL == zfile) {
		cerr<<"Cannot open file " << filename.str()  << endl;
		exit(-1);
	}
	int written = gzwrite(zfile, xml_data, xml_size);
	if ( written != xml_size) {
		cerr<<"Error writing to file " << filename.str()  << " wrote "<< written << " of " << xml_size << endl;
		exit(-1);
	}
	gzclose(zfile);
	free(xml_data);
}


shared_ptr <const Lattice> getLattice() {
	if (!global_lattice) {
		cerr << "Trying to access global lattice, while it's not defined yet!" << endl;
		assert(0);
		exit(-1);
	}
	//cout << "getLattice" << endl;
	return global_lattice;
};

const Lattice& lattice()
{
	return *global_lattice;
}


// shared_ptr<PDE_Layer> findPDELayer(string symbol) {
// 	if (pde_layers.find(symbol) != pde_layers.end()) {
// 		return pde_layers[symbol];
// 	}
// 	else {
// 		throw string("Unable to locate Field \"") + symbol +"\"";
// 		return shared_ptr<PDE_Layer>();
// 	}
// }

// shared_ptr<VectorField_Layer> findVectorFieldLayer(string symbol)
// {
// 	if (vector_field_layers.find(symbol) != vector_field_layers.end()) {
// 		return vector_field_layers[symbol];
// 	}
// 	else {
// 		throw string("Unable to locate VectorField \"") + symbol +"\"";
// 		return shared_ptr<VectorField_Layer>();
// 	}
// }

const Scope* getScope() { return current_scope; }

const Scope* getGlobalScope() { return global_scope.get(); }

Scope* createSubScope(string name, CellType* ct) { if (! current_scope) throw string("Cannot create subscope from empty scope"); return current_scope->createSubScope(name,ct); }

deque<Scope*> scope_stash;
void enterScope(const Scope* scope) { if (!scope) throw(string("Invalid scope in enterScope")); cout << "Entering scope " << scope->getName() << endl; scope_stash.push_back(current_scope); current_scope = const_cast<Scope*>(scope);}

void leaveScope() { if (scope_stash.empty()) throw (string("Invalid scope in leaveScope on empty Stack")); cout << "Leaving scope " << current_scope->getName(); current_scope = scope_stash.back(); scope_stash.pop_back();  cout << ", back to scope " << current_scope->getName() << endl;  }



/*
 * Author:  David Robert Nadeau
 * Site:    http://NadeauSoftware.com/
 * License: Creative Commons Attribution 3.0 Unported License
 *          http://creativecommons.org/licenses/by/3.0/deed.en_US
 */

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(_WIN32)
#include <windows.h>
#include <psapi.h>

#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>
#include <sys/resource.h>

#if defined(__APPLE__) && defined(__MACH__)
#include <mach/mach.h>

#elif (defined(_AIX) || defined(__TOS__AIX__)) || (defined(__sun__) || defined(__sun) || defined(sun) && (defined(__SVR4) || defined(__svr4__)))
#include <fcntl.h>
#include <procfs.h>

#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
#include <stdio.h>

#endif

#else
#error "Cannot define getPeakRSS( ) or getCurrentRSS( ) for an unknown OS."
#endif

	/**
 * Returns the peak (maximum so far) resident set size (physical
 * memory use) measured in bytes, or zero if the value cannot be
 * determined on this OS.
 */
std::size_t getPeakRSS( )
{
#if defined(_WIN32)
	/* Windows -------------------------------------------------- */
	PROCESS_MEMORY_COUNTERS info;
	GetProcessMemoryInfo( GetCurrentProcess( ), &info, sizeof(info) );
	return (std::size_t)info.PeakWorkingSetSize;

#elif (defined(_AIX) || defined(__TOS__AIX__)) || (defined(__sun__) || defined(__sun) || defined(sun) && (defined(__SVR4) || defined(__svr4__)))
	/* AIX and Solaris ------------------------------------------ */
	struct psinfo psinfo;
	int fd = -1;
	if ( (fd = open( "/proc/self/psinfo", O_RDONLY )) == -1 )
		return (std::size_t)0L;		/* Can't open? */
	if ( read( fd, &psinfo, sizeof(psinfo) ) != sizeof(psinfo) )
	{
		close( fd );
		return (std::size_t)0L;		/* Can't read? */
	}
	close( fd );
	return (std::size_t)(psinfo.pr_rssize * 1024L);

#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
	/* BSD, Linux, and OSX -------------------------------------- */
	struct rusage rusage;
	getrusage( RUSAGE_SELF, &rusage );
	
	// HACK WdB: use statm file instead of getrusage (which does not work properly) HACK
	int tSize = 0, resident = 0, share = 0;
	ifstream buffer("/proc/self/statm");
    buffer >> tSize >> resident >> share;
	buffer.close();
	
#if defined(__APPLE__) && defined(__MACH__)
	return (std::size_t)rusage.ru_maxrss;
#else
	//return (std::size_t)(rusage.ru_maxrss * 1024L); // WdB HACK
	return (std::size_t)(resident * 1024L);
#endif

#else
	/* Unknown OS ----------------------------------------------- */
	return (std::size_t)0L;			/* Unsupported. */
#endif
}


/**
 * Returns the current resident set size (physical memory use) measured
 * in bytes, or zero if the value cannot be determined on this OS.
 */
std::size_t getCurrentRSS( )
{
#if defined(_WIN32)
	/* Windows -------------------------------------------------- */
	PROCESS_MEMORY_COUNTERS info;
	GetProcessMemoryInfo( GetCurrentProcess( ), &info, sizeof(info) );
	return (std::size_t)info.WorkingSetSize;

#elif defined(__APPLE__) && defined(__MACH__)
	/* OSX ------------------------------------------------------ */
	/* 10.6.8 ////////////////////////////////////////////////////*/
	struct task_basic_info info; 
    mach_msg_type_number_t infoCount = TASK_BASIC_INFO_COUNT; 
    if ( task_info( mach_task_self( ), TASK_BASIC_INFO, 
		(task_info_t)&info, &infoCount ) != KERN_SUCCESS )
		return (std::size_t)0L;		/* Can't access? */
	return (std::size_t)info.resident_size;
	
	/* 10.7+  /////////////////////////////////////////////////////*/
//    struct mach_task_basic_info info;
//    mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
//    if ( task_info( mach_task_self( ), MACH_TASK_BASIC_INFO,
//		(task_info_t)&info, &infoCount ) != KERN_SUCCESS )
//		return (std::size_t)0L;		/* Can't access? */
//	return (std::size_t)info.resident_size;

#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
	/* Linux ---------------------------------------------------- */
	long rss = 0L;
	FILE* fp = NULL;
	if ( (fp = fopen( "/proc/self/statm", "r" )) == NULL )
		return (std::size_t)0L;		/* Can't open? */
	if ( fscanf( fp, "%*s%ld", &rss ) != 1 )
	{
		fclose( fp );
		return (std::size_t)0L;		/* Can't read? */
	}
	fclose( fp );
	return (std::size_t)rss * (std::size_t)sysconf( _SC_PAGESIZE);

#else
	/* AIX, BSD, Solaris, and Unknown OS ------------------------ */
	return (std::size_t)0L;			/* Unsupported. */
#endif
}

#ifdef __cplusplus
}
#endif

}
