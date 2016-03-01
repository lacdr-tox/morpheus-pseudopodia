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

void NeighborhoodVectorReporter::loadFromXML(const XMLNode xNode)
{    
	ReporterPlugin::loadFromXML(xNode);
}

void NeighborhoodVectorReporter::init(const Scope* scope)
{
	ReporterPlugin::init(scope);
	
	// Reporter input depends on cell position
	registerCellPositionDependency();
	
	celltype = scope->getCellType();
	assert(celltype);
}

void NeighborhoodVectorReporter::report() {

    auto celltypes = CPM::getCellTypes();
	vector<bool> is_medium_type;
	for (auto ct : celltypes ) { is_medium_type.push_back(ct.lock()->isMedium()); }
	
    vector<CPM::CELL_ID> cells = celltype->getCellIDs();
    for (auto cell_id : cells) {
		SymbolFocus focus(cell_id);

        double total_interface = 0; // interface-based: number all cell-cell interfaces; cell-based: number of neighboring cells (excluding medium)
        VDOUBLE total_signal(0.0,0.0,0.0);
//         uint numneighbors = 0;
		const map <CPM::CELL_ID, double >& interfaces = focus.cell().getInterfaceLengths();
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

		output.set( focus, signal );
    }
}
