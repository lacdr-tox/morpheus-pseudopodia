#include "neighborhood_reporter.h"


using namespace SIM;

REGISTER_PLUGIN(NeighborhoodReporter);

NeighborhoodReporter::NeighborhoodReporter() : ReporterPlugin(TimeStepListener::XMLSpec::XML_OPTIONAL) {
	scaling.setXMLPath("Input/scaling");
	map<string, InputModes> value_map;
	value_map["cell"] = CELLS;
	value_map["length"] = INTERFACES;
	scaling.setConversionMap(value_map);
	registerPluginParameter(scaling);
	
	input.setXMLPath("Input/value");
	// Input is required to be valid on global scope
	input.setGlobalScope();
	registerPluginParameter(input);
	
	   exclude_medium.setXMLPath("Input/noflux-cell-medium");
	   exclude_medium.setDefault("false");
	registerPluginParameter(exclude_medium);
	
}

void NeighborhoodReporter::loadFromXML(const XMLNode xNode, Scope* scope)
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
	ReporterPlugin::loadFromXML(xNode, scope);
}


void NeighborhoodReporter::init(const Scope* scope)
{
	input.addNameSpaceScope("local",scope);

    ReporterPlugin::init(scope);
	
	local_ns_id = input.getNameSpaceId("local");
	
	local_ns_granularity = Granularity::Global;
	using_local_ns = false;
	for (auto & local : input.getNameSpaceUsedSymbols(local_ns_id)) {
		local_ns_granularity += local->granularity();
		using_local_ns = true;
	}
	
    celltype = scope->getCellType();
	if (celltype) {
		
		// Reporter output value depends on cell position
		registerCellPositionDependency();
		
		bool input_is_halo = input.granularity() == Granularity::MembraneNode || input.granularity() == Granularity::Node;
		if (scaling.isDefined() && input_is_halo) {
			cout << "NeighborhoodReporter: Input has (membrane) node granularity. Ignoring defined input mode." << endl;
		}
		
		for ( auto & out : output) {
			switch (out->symbol.granularity()) {
				case Granularity::MembraneNode:
					halo_output.push_back(out);
					out->membrane_acc = dynamic_pointer_cast<const MembranePropertySymbol>(out->symbol.accessor());
					out->mapper = DataMapper::create(out->mapping());
					break;
				case Granularity::Global :
				case Granularity::Cell :
					if ( ! out->mapping.isDefined())
						throw MorpheusException(string("NeighborhoodReporter requires a mapping for reporting into Symbol ") + out->symbol.name(), stored_node);
					out->mapper = DataMapper::create(out->mapping());
					out->mapper->setWeightsAreBuckets(true);
					
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
		
		if (exclude_medium.isDefined() ){
			cout << "noflux_cell_medium_interface.isDefined()" << endl;
			if (exclude_medium() == true){
				cout << "noflux_cell_medium_interface() == true!" << endl;
			}
		}
	}
	else {
		// global scope case
		if (!CPM::getCellTypes().empty()) registerCellPositionDependency();
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
#pragma omp parallel
	{
		int thread = omp_get_thread_num();
#pragma omp for schedule(static)
		for (auto i_node = range.begin(); i_node<range.end(); ++i_node) {
			const auto& node = *i_node;
			
			// Expose local symbols to input
			if (using_local_ns) {
				input.setNameSpaceFocus(local_ns_id, node);
			}
			
	// 	for (auto node : range) { // syntax cannot be used with openMP
			// loop through its neighbors
			for ( int i_nei = 0; i_nei < neighbors.size(); ++i_nei ) {
				VINT nb_pos = node.pos() + neighbors[i_nei];
				// get value at neighbor node
				double value = input(nb_pos);
				// add value to data mapper
				for (auto const&  out: output){
					out->mapper->addVal( value , thread);
				}
			}
			// write mapped values to output symbol at node
			for ( auto const&  out: output ) {
				out->symbol.set(node.pos(), out->mapper->get(thread));
				out->mapper->reset(thread);
			}
		}
	}
}

void NeighborhoodReporter::reportCelltype(CellType* celltype) {
    vector<CPM::CELL_ID> cells = celltype->getCellIDs();
	if (cells.empty()) return;

	// if the input has membrane node granularity, we need to create and iterate over the cell halo
	// the same is true, if one of the output symbols requires node granularity
	if ( !halo_output.empty()) {
		//  draw in the membrane mapper ...
		vector<VINT> neighbor_offsets = CPM::getSurfaceNeighborhood().neighbors();
		ExceptionCatcher exception_catcher;
		
#pragma omp parallel
		{
			// There might also be boolean input, that we cannot easily handle this way. But works for concentrations and rates, i.e. all continuous quantities.
			auto thread = omp_get_thread_num();
			unique_ptr<MembraneMapper> mapper;
			unique_ptr<MembraneMapper> discrete_mapper;
			struct count_data { double val; double count; };
			map<CPM::CELL_ID,count_data> cell_mapper;
			for ( const auto& out : halo_output) {
				out->mapper->reset(thread);
				if (out->membrane_acc && !mapper ) {
					mapper = make_unique<MembraneMapper> (MembraneMapper::MAP_CONTINUOUS,false);
					discrete_mapper = make_unique<MembraneMapper> (MembraneMapper::MAP_DISCRETE,false);
				}
			}
#pragma omp for schedule(static)
			for ( int i=0; i<cells.size(); i++) {
				exception_catcher.Run([&](){
				// Create halo of nodes surrounding the cell
				SymbolFocus cell_focus(cells[i]);
				if ( using_local_ns && local_ns_granularity != Granularity::MembraneNode) {
					// Expose local symbols to input
					input.setNameSpaceFocus(local_ns_id, cell_focus);
				}
				
				Cell::Nodes halo_nodes; // holds nodes of neighbors of membrane nodes. Used for <concentration>
				const Cell::Nodes& surface_nodes = cell_focus.cell().getSurface();
				for ( Cell::Nodes::const_iterator m = surface_nodes.begin(); m != surface_nodes.end(); ++m ) {
					// check the current boundary neighborhood
					for ( int i_nei = 0; i_nei < neighbor_offsets.size(); ++i_nei ) {
						VINT neighbor_position = ( *m ) + neighbor_offsets[i_nei];
						const CPM::STATE& nb_spin = CPM::getNode( neighbor_position );

						if ( cell_focus.cellID() != nb_spin.cell_id ) { // if neighbor is different from me
							// HACK: NOFLUX BOUNDARY CONDITIONS when halo is in MEDIUM
							//cout << CPM::getCellIndex( nb_spin.cell_id ).celltype << " != " << CPM::getEmptyCelltypeID() << endl;
							if ( exclude_medium() ) {
								// if neighbor is medium, add own node in halo 
								if (CPM::getCellIndex( nb_spin.cell_id ).celltype == CPM::getEmptyCelltypeID() )
									halo_nodes.insert ( *m );
							}
							else
								halo_nodes.insert ( neighbor_position );
						}
					}
				}
				if (halo_nodes.empty() ) {
					cout << "MembraneReporter refuses to report on cell " << cell_focus.cellID() << " because it has no surface" << endl;
					cout << "Cell "<< cell_focus.cellID() << " Cell size was " << cell_focus.cell().nNodes() << endl;
					return;
				}
				
				
				if (scaling() == INTERFACES) {
					vector<double> raw_data;
					if (mapper) {
						// Report halo input into membrane mapper, coordinating the transfer into an intermediate membrane property
						mapper->attachToCell(cell_focus.cellID());
						discrete_mapper->attachToCell( cell_focus.cellID() );
						for ( auto const & i :halo_nodes) {
							if ( using_local_ns && local_ns_granularity == Granularity::MembraneNode) {
								// Expose local symbols to input
								cell_focus.setCell(cell_focus.cellID(),i);
								input.setNameSpaceFocus(local_ns_id, cell_focus);
							}
							double value = input(SymbolFocus(i));
							if (std::isnan(value)) {
								mapper->map(i,0);
								discrete_mapper->map(i,0);
							}
							else {
								mapper->map(i, std::isnan(value) ? 0 : value );
								discrete_mapper->map(i, std::isnan(value) ? 0 : value );
								raw_data.push_back(value);
							}
						}
						mapper->fillGaps();
						discrete_mapper->fillGaps();
					}
					else {
						// Report just into a data store
						for ( auto const & i :halo_nodes) {
							if ( using_local_ns && local_ns_granularity == Granularity::MembraneNode) {
								// Expose local symbols to input
								cell_focus.setCell(cell_focus.cellID(),i);
								input.setNameSpaceFocus(local_ns_id, cell_focus);
							}
							double value = input(SymbolFocus(i));
							if (!std::isnan(value)) {
								raw_data.push_back(value);
							}
						}
					}

					for (const auto & out : halo_output) {
						if (out->membrane_acc) {
							if (out->mapping() == DataMapper::DISCRETE)
								discrete_mapper->copyData(out->membrane_acc->getField(cell_focus.cellID()));
							else
								mapper->copyData(out->membrane_acc->getField(cell_focus.cellID()));
						}
						else {
							for (const auto& val : raw_data) {
								out->mapper->addVal(val, thread);
							}
							
							if (out->symbol.granularity() == Granularity::Cell) {
								if (out->mapper->getMode() == DataMapper::SUM) {
									// Rescale to Interface length
									out->symbol.set(cell_focus, out->mapper->get(thread) / raw_data.size() *  cell_focus.cell().getInterfaceLength() );
								}
								else {
									out->symbol.set(cell_focus, out->mapper->get(thread));
								}
								out->mapper->reset(thread);
							}
						}
					}
				}
				else if (scaling() == CELLS) {
					
					cell_mapper.clear();
					for ( const auto & i :halo_nodes) {
						SymbolFocus f(i);
						
						if (using_local_ns) {
							// Expose local symbols to input
							cell_focus.setCell(cell_focus.cellID(),i);
							input.setNameSpaceFocus(local_ns_id, cell_focus);
						}
						
						double value = input(SymbolFocus(i));
						cell_mapper[f.cellID()].val += std::isnan(value) ? 0 : value ;
						cell_mapper[f.cellID()].count ++;
					}
					for ( auto & cell_stat : cell_mapper ) {
						cell_stat.second.val /= cell_stat.second.count;
					}
					
					for ( const auto & out : halo_output) {
						if (out->membrane_acc) {
							mapper->attachToCell(cell_focus.cellID());
							for ( const auto & i :halo_nodes) {
								SymbolFocus focus(i);
								mapper->map(i, cell_mapper[focus.cellID()].val );
							}
							mapper->fillGaps();
							mapper->copyData(out->membrane_acc->getField(cell_focus.cellID()));
						}
						else {
							for ( const auto & cell_stat : cell_mapper ) {
								out->mapper->addVal(cell_stat.second.val, thread);
							}
							if (out->symbol.granularity() == Granularity::Cell) {
								out->symbol.set(cell_focus, out->mapper->get(thread));
								out->mapper->reset(thread);
							}
						}
					}
				}
			
			});
			}
		}
		exception_catcher.Rethrow();
		// Post hoc writing of global output 
		for (auto const& out : halo_output) {
			if (out->symbol.granularity() == Granularity::Global) {
				out->symbol.set(SymbolFocus::global, out->mapper->getCollapsed());
			}
		}
	}
	
	if ( !interf_output.empty()) {
		// Assume cell granularity
		ExceptionCatcher exception_catcher;
#pragma omp parallel
		{
			int thread = omp_get_thread_num();
			for (auto const& out : interf_output) {
				out->mapper->reset(thread);
			}
#pragma omp for schedule(static)
			for (int c=0; c<cells.size(); c++) {
				exception_catcher.Run([&](){
				SymbolFocus cell_focus(cells[c]);
				
				// Expose local symbols to input
				if (using_local_ns) {
					// Expose local symbols to input
					input.setNameSpaceFocus(local_ns_id, cell_focus);
				}
				
				const auto& interfaces = CPM::getCell(cells[c]).getInterfaceLengths();
				
				// Special case cell has no interfaces ...
				if (interfaces.size() == 0) {
					for (auto const& out : interf_output) {
						out->symbol.set(cell_focus, 0.0);
					}
					return;
				}
				
				uint i=0;

				for (auto nb = interfaces.begin(); nb != interfaces.end(); nb++, i++) {
					CPM::CELL_ID cell_id = nb->first;
					SymbolFocus cell_focus(cell_id);
					
					double value = 0.0;
					if ( exclude_medium() && cell_focus.cell().getCellType()->isMedium() ) { 
						// if neighbor is medium, skip value
						continue;
					}

					value = input.get(cell_focus); // value of neighboring cell
					double interfacelength = (scaling() == INTERFACES) ? nb->second : 1;
					
					for (auto & out : interf_output) {
						out->mapper->addValW(value, interfacelength, thread);
					}
				}

				for (auto& out : interf_output) {
					if (out->symbol.granularity() == Granularity::Cell) {
						out->symbol.set(cell_focus, out->mapper->get(thread));
						out->mapper->reset(thread);
					}
				}
			});
			}
		}
		exception_catcher.Rethrow();
		// Post hoc writing of global output 
		for (auto const& out : interf_output) {
			if (out->symbol.granularity() == Granularity::Global) {
				out->symbol.set(SymbolFocus::global, out->mapper->getCollapsed());
			}
		}
	}
}




