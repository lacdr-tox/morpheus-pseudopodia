#include "insert_medium.h"

REGISTER_PLUGIN(InsertMedium);

InsertMedium::InsertMedium() : InstantaneousProcessPlugin( TimeStepListener::XMLSpec::XML_NONE) {
	condition.setXMLPath("Condition/text");
	registerPluginParameter( condition );
	//registerCellPositionOutput(); // TODO: is this necessary? this plugin only set neighboring nodes, not the cell position itself
}


void InsertMedium::init(const Scope* scope)
{
	InstantaneousProcessPlugin::init(scope);

	setTimeStep( CPM::getMCSDuration() );
	celltype 		= scope->getCellType();
	cpm_layer 		= CPM::getLayer();
	neighbor_sites	= cpm_layer->getLattice()->getNeighborhoodByOrder(1).neighbors();
	medium			= CPM::getEmptyState().cell_id;
	
	if( condition.isDefined() )
		condition.init();
}

void InsertMedium::executeTimeStep()
{
	// 0. if condition is defined and not true, do nothing
	// however, if condition is not defined, always insert medium
	if( condition.isDefined() && condition.get( SymbolFocus() ) < 1 ){
		return;
	}
	
	// 1. choose random cell from population
	vector<CPM::CELL_ID> cells = celltype->getCellIDs();
	int random_cell = floor(getRandom01() * cells.size());
	CPM::CELL_ID cell_id = cells[ random_cell ];
	
	// 2. get the cell interface from the cell
	const auto& interfaces =  CPM::getCell(cell_id).getInterfaceLengths();

	// 3. if there are already medium neighbors, inserting medium is senseless, so return
	auto it = interfaces.find(medium);
	if( it != interfaces.end()){
		if( it->second > 0 )
			return;
	}

	// 4. otherwise (if interface does not touch medium), collect the cell's halo (nodes adjacent to cell)
	vector<VINT> halo;
	const Cell::Nodes& membrane = CPM::getCell(cell_id).getSurface();

	for( auto& m : membrane )
	{
		// check the von neumann (4-membered) neighborhood
		// gain the halo of nodes surrounding the cell
		for ( int i_nei = 0; i_nei < neighbor_sites.size(); ++i_nei )
		{
			VINT neighbor_position = m + neighbor_sites[i_nei];
			const CPM::STATE& nb_spin = cpm_layer->get (neighbor_position );

			// if there are already medium neighbors, inserting is useless, so do not continue
			if( nb_spin.cell_id == medium )
				continue;
			
			if( cell_id != nb_spin.cell_id) { // if neighbor is different from me
				halo.push_back( neighbor_position ); // add neighbor to list of unique neighboring points (used for layers below)
			}
		}
	}

	// 4. finally, pick a random node from the halo, and set this to medium
	VINT random_node = halo[ floor(getRandom01() * halo.size()) ];
	CPM::setNode(random_node, medium);
	
}
