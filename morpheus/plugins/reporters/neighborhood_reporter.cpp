#include "neighborhood_reporter.h"


using namespace SIM;

REGISTER_PLUGIN(NeighborhoodReporter);

NeighborhoodReporter::NeighborhoodReporter() {
	input_mode.setXMLPath("Input/scaling");
	map<string, InputModes> value_map;
	value_map["cell"] = CELLS;
	value_map["length"] = INTERFACES;
	input_mode.setConversionMap(value_map);
	registerPluginParameter(input_mode);
	
	input.setXMLPath("Input/value");
	// Input is required to be valid on global scope
	input.setGlobalScope();
	registerPluginParameter(input);
	
	noflux_cell_medium_interface.setXMLPath("Input/noflux-cell-medium");
	noflux_cell_medium_interface.setDefault("false");
	registerPluginParameter(noflux_cell_medium_interface);
	
}

void NeighborhoodReporter::loadFromXML(const XMLNode xNode)
{    
	map<string, DataMapper::Mode> output_mode_map = DataMapper::getModeNames();
	
	// Define PluginParameters for all defined Output tags
	for (uint i=0; i<xNode.nChildNode("Output"); i++) {
		shared_ptr<OutputSpec> out(new OutputSpec());
		out->mapping.setXMLPath("Output["+to_str(i)+"]/mapping");
		out->mapping.setConversionMap(output_mode_map);
		out->symbol.setXMLPath("Output["+to_str(i)+"]/symbol-ref");
		registerPluginParameter(out->mapping);
		registerPluginParameter(out->symbol);
		output.push_back(out);
	}
	
	// Load all the defined PluginParameters
	ReporterPlugin::loadFromXML(xNode);
}


void NeighborhoodReporter::init(const Scope* scope)
{
    ReporterPlugin::init(scope);
    celltype = scope->getCellType();
		if (celltype) {
		
		cpm_layer = CPM::getLayer();

		// Reporter output value depends on cell position
		registerCellPositionDependency();
		
		bool input_is_halo = input.granularity() == Granularity::MembraneNode || input.granularity() == Granularity::Node;
		if (input_mode.isDefined() && input_is_halo) {
			cout << "NeighborhoodReporter: Input has (membrane) node granularity. Ignoring defined input mode." << endl;
		}
		
		for ( auto & out : output) {
			switch (out->symbol.granularity()) {
				case Granularity::MembraneNode:
					halo_output.push_back(out);
					out->membrane_acc = celltype->findMembrane(out->symbol.name());
					break;
				case Granularity::Global :
				case Granularity::Cell :
					if ( ! out->mapping.isDefined())
						throw MorpheusException(string("NeighborhoodReporter requires a mapping for reporting into Symbol ") + out->symbol.name(), stored_node);
					out->mapper = DataMapper::create(out->mapping());
					
					if (input_is_halo) {
						halo_output.push_back(out);
					}
					else {
						interf_output.push_back(out);
					}
					break;
					
				case Granularity::Node: 
				default:
					throw XMLName() + " can not write to symbol " + out->symbol.name() + " with node granularity.";
					break;
			}
		}
		
		if (!halo_output.empty()) {
			CPM::enableEgdeTracking();
		}
		
		noflux_cell_medium = false;
		if (noflux_cell_medium_interface.isDefined() ){
			cout << "noflux_cell_medium_interface.isDefined()" << endl;
			if (noflux_cell_medium_interface() == true){
				noflux_cell_medium = true;
				cout << "noflux_cell_medium_interface() == true!" << endl;
			}
		}
	}
	else {
		// global scope case
		
		for ( auto & out : output) {
			switch (out->symbol.granularity()) {
				case Granularity::Node: 
					out->mapper = DataMapper::create(out->mapping());
					break;
				default:
					throw MorpheusException( XMLName() + " can not write to symbol " + out->symbol.name() + " without node granularity.", stored_node);
					break;
			}
		}
	}
}


void NeighborhoodReporter::report() {
	if (celltype) {
		reportCelltype(celltype);
	}
	else {
		reportGlobal();
	}
}

void NeighborhoodReporter::reportGlobal() {
	FocusRange range(Granularity::Node, SIM::getGlobalScope());
	auto neighbors = SIM::lattice().getDefaultNeighborhood().neighbors();
//#pragma omp parallel
//{
//#pragma omp for schedule(static)
//	for (auto i_node = range.begin(); i_node<range.end(); ++i_node) {
//		auto node = *i_node;
	
	for (auto node : range) { // syntax cannot be used with openMP
		// loop through its neighbors
		for ( int i_nei = 0; i_nei < neighbors.size(); ++i_nei ) {
			VINT nb_pos = node.pos() + neighbors[i_nei];
			// get value at neighbor node
			double value = input(nb_pos);
			// add value to data mapper
			for (auto const& out : output){
				out->mapper->addVal( value );
			}
		}
		// write mapped values to output symbol at node
		for (auto const& out : output){
			out->symbol.set(node.pos(),out->mapper->get());
			out->mapper->reset();
		}
	}
// }
}


void NeighborhoodReporter::reportCelltype(CellType* celltype) {
    vector<CPM::CELL_ID> cells = celltype->getCellIDs();
	if (cells.empty()) return;

	// if the input has membrane node granularity, we need to create and iterate over the cell halo
	// the same is true, if one of the output symbols requires node granularity
	if ( !halo_output.empty()) {
		//  draw in the membrane mapper ...
		vector<VINT> neighbor_offsets = CPM::getSurfaceNeighborhood().neighbors();

#pragma omp parallel
{
		// There might also be boolean input, that we cannot easily handle this way. But works for concentrations and rates, i.e. all continuous quantities.
		MembraneMapper mapper(MembraneMapper::MAP_CONTINUOUS,false);
#pragma omp for schedule(static)
		for ( auto i_cell_id = cells.begin(); i_cell_id<cells.end(); ++i_cell_id) {
//		for ( auto cell_id : cells) {
			// Create halo of nodes surrounding the cell
			auto cell_id = *i_cell_id;
			Cell::Nodes halo_nodes; // holds nodes of neighbors of membrane nodes. Used for <concentration>
			const Cell::Nodes& surface_nodes = CPM::getCell (cell_id).getSurface();
			for ( Cell::Nodes::const_iterator m = surface_nodes.begin(); m != surface_nodes.end(); ++m ) {
				// check the curren boundary neighborhood
				for ( int i_nei = 0; i_nei < neighbor_offsets.size(); ++i_nei ) {
					VINT neighbor_position = ( *m ) + neighbor_offsets[i_nei];
					const CPM::STATE& nb_spin = cpm_layer->get ( neighbor_position );

					if ( cell_id != nb_spin.cell_id ) { // if neighbor is different from me
						
						// HACK: NOFLUX BOUNDARY CONDITIONS when halo is in MEDIUM
						//cout << CPM::getCellIndex( nb_spin.cell_id ).celltype << " != " << CPM::getEmptyCelltypeID() << endl;
						if( noflux_cell_medium && CPM::getCellIndex( nb_spin.cell_id ).celltype == CPM::getEmptyCelltypeID() ){ // if neighbor is medium, add own node in halo 
							halo_nodes.insert ( *m ); // add own membrane node to list of unique neighboring points (used for layers below)
							//cout << *m << "\n";
						}
						else
							halo_nodes.insert ( neighbor_position ); // add neighbor node to list of unique neighboring points (used for layers below)
					}
				}
			}
			if (halo_nodes.empty() ) {
				cout << "MembraneReporter refuses to report on cell " << cell_id << " because it has no surface" << endl;
				cout << "Cell "<< cell_id << " Cell size was " << CPM::getCell ( cell_id ).nNodes() << endl;
				continue;
			}
			
			// Report halo input into membrane mapper, coordinating the transfer into an intermediate membrane property
			mapper.attachToCell(cell_id);
			for ( auto const & i :halo_nodes) {
				SymbolFocus f(i);
				double value = input(SymbolFocus(i));
				if (std::isnan(value)){ 
					mapper.map(i,0);
				}
				else{
					mapper.map(i,value);
				}
			}

			mapper.fillGaps();
			valarray<double> raw_data;
			SymbolFocus cell_focus(cell_id);
			for (auto & out : halo_output) {
				if (out->symbol.granularity() == Granularity::MembraneNode) {
					mapper.copyData(out->membrane_acc.getMembrane(cell_id));
				}
				else {
					if (raw_data.size() == 0) {
						raw_data =  mapper.getData().getData();
					}
					for (uint i=0; i<raw_data.size(); i++) {
						out->mapper->addVal(raw_data[i]);
					}
					if (out->symbol.granularity() == Granularity::Cell){
						out->symbol.set(cell_focus, out->mapper->get());
						out->mapper->reset();
					}
				}
			}
		}
}
		// Post hoc writing of global output 
		for (auto const& out : halo_output) {
			if (out->symbol.granularity() == Granularity::Global) {
				out->symbol.set(SymbolFocus::global, out->mapper->get());
				out->mapper->reset();
			}
		}
	}
	
	if ( !interf_output.empty()) {
		// Assume cell granularity
		
		for (int c=0; c<cells.size(); c++) {
			SymbolFocus cell_focus(cells[c]);
			map <CPM::CELL_ID, double > interfaces = CPM::getCell(cells[c]).getInterfaceLengths();
			
			// Special case cell has no interfaces ...
			if (interfaces.size() == 0) {
				for (auto const& out : interf_output) {
					out->symbol.set(cell_focus, 0.0);
				}
				continue;
			}
			
			double cell_interface = 0;
			double first_val  = input.get(SymbolFocus(interfaces.begin()->first));
			double min = first_val;
			double max = first_val;
			double sum = 0;
			double sq_sum =0;

			uint i=0;

			for (map<CPM::CELL_ID,double>::const_iterator nb = interfaces.begin(); nb != interfaces.end(); nb++, i++) {
				CPM::CELL_ID cell_id = nb->first;
				
				double value = 0.0;
				// HACK: NO-FLUX BOUNDARIES for cell-medium interface
				if( noflux_cell_medium && CPM::getCellIndex( cell_id ).celltype == CPM::getEmptyCelltypeID() ){ // if neighbor is medium, add own value 
					value = input.get( cell_focus ); // value of own cell 
				}
				else
					value = input.get(SymbolFocus(cell_id)); // value of neighboring cell
				double interfacelength = (input_mode() == INTERFACES) ? nb->second : 1;
				
				for (auto & out : interf_output) {
					out->mapper->addVal(value, interfacelength);
				}
			}
			
			for (auto & out : interf_output) {
				if (out->symbol.granularity() == Granularity::Cell) {
					out->symbol.set(cell_focus,out->mapper->get());
					out->mapper->reset();
				}
			}
		}
		// Post hoc writing of global output 
		for (auto const& out : interf_output) {
			if (out->symbol.granularity() == Granularity::Global) {
				out->symbol.set(SymbolFocus::global, out->mapper->get());
				out->mapper->reset();
			}
		}
	}
}




