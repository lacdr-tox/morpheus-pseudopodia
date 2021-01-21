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
	
	output_mode.setConversionMap(VectorDataMapper::getModeNames());
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
	auto mapper = VectorDataMapper::create(output_mode());
#pragma omp parallel
	{
#pragma omp for schedule(static)
		for (auto i_node = range.begin(); i_node<range.end(); ++i_node) {
			const auto& node = *i_node;
			
			// Expose local symbols to input
			if (using_local_ns) {
				input.setNameSpaceFocus(local_ns_id, node);
			}
			
			for ( int i_nei = 0; i_nei < neighbors.size(); ++i_nei ) {
				mapper->addVal( input(node.pos() + neighbors[i_nei]) );
			}
			
			output.set( node, mapper->get() );
			mapper->reset();
		}
	}
}

void NeighborhoodVectorReporter::reportCelltype(CellType* celltype) {

    auto celltypes = CPM::getCellTypes();
	vector<bool> is_medium_type;
	for (auto ct : celltypes ) { is_medium_type.push_back(ct.lock()->isMedium()); }
	
    vector<CPM::CELL_ID> cells = celltype->getCellIDs();
	auto mapper = VectorDataMapper::create(output_mode());
    for (auto cell_id : cells) {
		SymbolFocus cell_focus(cell_id);

		// Expose local symbols to input
		if (using_local_ns) {
			input.setNameSpaceFocus(local_ns_id, cell_focus);
		}
		
		// loop through its neighbors
		const map <CPM::CELL_ID, double >& interfaces = cell_focus.cell().getInterfaceLengths();
		for (map<CPM::CELL_ID,double>::const_iterator nb = interfaces.begin(); nb != interfaces.end(); nb++) {

			CPM::CELL_ID nei_cell_id = nb->first;
			SymbolFocus neighbor(nei_cell_id);
			if ( exclude_medium() && is_medium_type[neighbor.celltype()] )
				continue;

			double interfacelength = (scaling() == INTERFACES) ? nb->second : 1;
	// 			numneighbors++;

			mapper->addValW(input.get( neighbor ), interfacelength);
		}
			
		output.set( cell_focus, mapper->get() );
		mapper->reset();
	}
}
