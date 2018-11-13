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
	output_mode.setConversionMap(out_map);
	output_mode.setXMLPath("Output/mapping");
	registerPluginParameter(output_mode);
}

void NeighborhoodVectorReporter::loadFromXML(const XMLNode xNode, Scope* scope)
{   
	for (uint i=0; i<xNode.getChildNode("Input").nChildNode("ExposeLocal"); i++) {
		ExposeSpec e;
		e.local_symbol->setXMLPath("Input/ExposeLocal/symbol-ref");
		e.symbol->setXMLPath("Input/ExposeLocal/symbol");
		registerPluginParameter(e.local_symbol);
		registerPluginParameter(e.symbol);
		exposed_locals.push_back(e);
	}
	
	ReporterPlugin::loadFromXML(xNode, scope);
}

void NeighborhoodVectorReporter::init(const Scope* scope)
{
	// Add variables to be exposed from the local cell
	vector<EvaluatorVariable> locals;
	for (auto & local : exposed_locals) {
		local.symbol->init();
		locals.push_back({local.symbol(),EvaluatorVariable::DOUBLE});
	}
	input.setLocalsTable(locals);
	
	ReporterPlugin::init(scope);
	
	exposed_locals_granularity = Granularity::Global;
	for (auto & local : exposed_locals) {
		exposed_locals_granularity += local.local_symbol->granularity();
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
		valarray<double> l_data(0.0,exposed_locals.size());
#pragma omp for schedule(static)
		for (auto i_node = range.begin(); i_node<range.end(); ++i_node) {
			const auto& node = *i_node;
			// Expose local symbols to input
			for (uint i = 0; i<exposed_locals.size(); i++) {
				l_data[i] = exposed_locals[i].local_symbol(node);
			}
			input.setLocals(&l_data[0]);
			
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
			}

			output.set( node, signal );
		}
	}
}

void NeighborhoodVectorReporter::reportCelltype(CellType* celltype) {

    auto celltypes = CPM::getCellTypes();
	vector<bool> is_medium_type;
	for (auto ct : celltypes ) { is_medium_type.push_back(ct.lock()->isMedium()); }
	
	valarray<double> l_data(0.0,exposed_locals.size());
    vector<CPM::CELL_ID> cells = celltype->getCellIDs();
    for (auto cell_id : cells) {
		SymbolFocus cell_focus(cell_id);

		// Expose local symbols to input
		for (uint i = 0; i<exposed_locals.size(); i++) {
			l_data[i] = exposed_locals[i].local_symbol(cell_focus);
		}
		input.setLocals(&l_data[0]);
		
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
		}

		output.set( cell_focus, signal );
    }
}
