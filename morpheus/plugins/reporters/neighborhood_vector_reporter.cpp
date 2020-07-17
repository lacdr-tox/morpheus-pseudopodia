#include "neighborhood_vector_reporter.h"

using namespace SIM;

REGISTER_PLUGIN(NeighborhoodVectorReporter);

NeighborhoodVectorReporter::NeighborhoodVectorReporter() : ReporterPlugin() {
	input.setXMLPath("Input/value");
	input.setGlobalScope();
	registerPluginParameter(input);
	
	map<string,InputModes> value_map;
	value_map["cell"] = CELLS;
	value_map["length"] = INTERFACES;
	   scaling.setConversionMap(value_map);
	   scaling.setXMLPath("Input/scaling");
	   scaling.setDefault("length");
	registerPluginParameter(scaling);
	
	exclude_medium.setDefault("false");
	exclude_medium.setXMLPath("Input/exclude-medium");
	registerPluginParameter(exclude_medium);
	
	output.setXMLPath("Output/symbol-ref");
	registerPluginParameter(output);
	
	map<string, OutputMode> out_map;
	out_map["sum"] =  NeighborhoodVectorReporter::SUM;
	out_map["average"] = NeighborhoodVectorReporter::AVERAGE;
	out_map["discrete"] = NeighborhoodVectorReporter::DISCRETE;
	output_mode.setConversionMap(out_map);
	output_mode.setXMLPath("Output/mapping");
	registerPluginParameter(output_mode);
}

void NeighborhoodVectorReporter::loadFromXML(const XMLNode xNode, Scope* scope)
{   
	ReporterPlugin::loadFromXML(xNode, scope);
}

void NeighborhoodVectorReporter::init(const Scope* scope)
{
	// Add a callback to automativally exposed symbols from the local cell
	input.addNameSpaceScope("local",scope);
	
	ReporterPlugin::init(scope);
	
	local_ns_id = input.getNameSpaceId("local");
	
	local_ns_granularity = Granularity::Global;
	using_local_ns = false;
	for (auto & local : input.getNameSpaceUsedSymbols(local_ns_id)) {
		local_ns_granularity += local->granularity();
		using_local_ns = true;
	}
	
	// Reporter input depends on cell position
	registerCellPositionDependency();
	
	celltype = scope->getCellType();
	assert(celltype);
}

void NeighborhoodVectorReporter::report() {
	if (celltype) {
		reportCelltype(celltype);
	}
	else {
		reportGlobal();
	}
}

void NeighborhoodVectorReporter::reportGlobal() {
	FocusRange range(Granularity::Node, SIM::getGlobalScope());
	auto neighbors = SIM::lattice().getDefaultNeighborhood().neighbors();
#pragma omp parallel
	{
#pragma omp for schedule(static)
		for (auto i_node = range.begin(); i_node<range.end(); ++i_node) {
			const auto& node = *i_node;
			
			// Expose local symbols to input
			if (using_local_ns) {
				input.setNameSpaceFocus(local_ns_id, node);
			}
			
			if (output_mode() == NeighborhoodVectorReporter::DISCRETE) {
				map<VDOUBLE,int,less_VDOUBLE> signal_map;
				// loop through its neighbors
				for ( int i_nei = 0; i_nei < neighbors.size(); ++i_nei ) {
					VDOUBLE signal = input(node.pos() + neighbors[i_nei]);
					signal_map[signal]+=1;
				}
				int max_occ=0; int max_occ_cnt=0; VDOUBLE max_occ_value;
				for (const auto& v : signal_map) {
					if (v.second>max_occ) {
						max_occ = v.second;
						max_occ_cnt = 1;
						max_occ_value= v.first;
					}
					else if (v.second == max_occ)
						max_occ_cnt++;
				}
				output.set( node, max_occ_value );
				
			}
			else {
				double total_interface = 0; // interface-based: number all cell-cell interfaces; cell-based: number of neighboring cells
				VDOUBLE total_signal(0.0,0.0,0.0);
		// 	for (auto node : range) { // syntax cannot be used with openMP
				// loop through its neighbors
				for ( int i_nei = 0; i_nei < neighbors.size(); ++i_nei ) {
					VINT nb_pos = node.pos() + neighbors[i_nei];
					total_interface += 1;
					// get value at neighbor node
					total_signal += input(nb_pos);
				}
				
				// final signal stats calculation
				VDOUBLE signal;
				switch (output_mode()) {
					case NeighborhoodVectorReporter::AVERAGE :
						if ( total_interface > 0.0)
							signal = total_signal / total_interface;
						else
							signal = VDOUBLE(0.0,0.0,0.0);
						break;
					case NeighborhoodVectorReporter::SUM:
						signal = total_signal;
						break;
					case NeighborhoodVectorReporter::DISCRETE: break;
				}
				output.set( node, signal );
			}
		}
	}
}

void NeighborhoodVectorReporter::reportCelltype(CellType* celltype) {

    auto celltypes = CPM::getCellTypes();
	vector<bool> is_medium_type;
	for (auto ct : celltypes ) { is_medium_type.push_back(ct.lock()->isMedium()); }
	
    vector<CPM::CELL_ID> cells = celltype->getCellIDs();
    for (auto cell_id : cells) {
		SymbolFocus cell_focus(cell_id);

		// Expose local symbols to input
		if (using_local_ns) {
			input.setNameSpaceFocus(local_ns_id, cell_focus);
		}
		
		if (output_mode() == NeighborhoodVectorReporter::DISCRETE) {
			map<VDOUBLE,int,less_VDOUBLE> signal_map;
			// loop through its neighbors
			const map <CPM::CELL_ID, double >& interfaces = cell_focus.cell().getInterfaceLengths();
			for (map<CPM::CELL_ID,double>::const_iterator nb = interfaces.begin(); nb != interfaces.end(); nb++) {

				CPM::CELL_ID nei_cell_id = nb->first;
				SymbolFocus neighbor(nei_cell_id);
				if ( exclude_medium() && is_medium_type[neighbor.celltype()] )
					continue;

				double interfacelength = (scaling() == INTERFACES) ? nb->second : 1;
	// 			numneighbors++;

				signal_map[input.get( neighbor )] += interfacelength;
			}
			
			int max_occ=0; int max_occ_cnt=0; VDOUBLE max_occ_value;
			for (const auto& v : signal_map) {
				if (v.second>max_occ) {
					max_occ = v.second;
					max_occ_cnt = 1;
					max_occ_value= v.first;
				}
				else if (v.second == max_occ)
					max_occ_cnt++;
			}
			output.set( cell_focus, max_occ_value );
			
		}
		else {
			double total_interface = 0; // interface-based: number all cell-cell interfaces; cell-based: number of neighboring cells
			VDOUBLE total_signal(0.0,0.0,0.0);

			const map <CPM::CELL_ID, double >& interfaces = cell_focus.cell().getInterfaceLengths();
			for (map<CPM::CELL_ID,double>::const_iterator nb = interfaces.begin(); nb != interfaces.end(); nb++) {

				CPM::CELL_ID nei_cell_id = nb->first;
				SymbolFocus neighbor(nei_cell_id);
				if ( exclude_medium() && is_medium_type[neighbor.celltype()] )
					continue;

				double interfacelength = (scaling() == INTERFACES) ? nb->second : 1;
	// 			numneighbors++;

				total_interface += interfacelength;
				total_signal +=  interfacelength * input.get( neighbor );
			}
	// 		if (numberneighbors.valid())
	// 			numberneighbors.set ( cells[c], numneighbors );

			VDOUBLE signal;
			// final signal stats calculation
			switch (output_mode()) {
				case NeighborhoodVectorReporter::AVERAGE :
					if ( total_interface > 0.0)
						signal = total_signal / total_interface;
					else
						signal = VDOUBLE(0.0,0.0,0.0);
					break;
				case NeighborhoodVectorReporter::SUM:
					signal = total_signal;
					break;
				case NeighborhoodVectorReporter::DISCRETE: break;
			}

			output.set( cell_focus, signal );
		}
    }
}
