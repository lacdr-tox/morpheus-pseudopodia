#include "fusion.h"

REGISTER_PLUGIN(Fusion);

Fusion::Fusion() : TimeStepListener(ScheduleType::MCSListener) {};

void Fusion::executeTimeStep() {
	TimeStepListener::executeTimeStep();
	vector <CPM::CELL_ID> cells = celltype->getCellIDs();
	for (int i=0; i < cells.size(); i++ ) {
		
		if(CPM::getCellIndex(cells[i]).status != CPM::REGULAR_CELL)
			continue;

		CPM::CELL_ID cell_id = cells[i];
		const Cell& cell = CPM::getCell( cell_id );

		// get adjacent cell with whom this cells shares the longest interface
		//  that cell will become the fusion partner
		
		int max_interface_length=0;
		CPM::CELL_ID fusion_partner_id = CPM::NO_CELL;
		const map <CPM::CELL_ID, uint >& interfaces = cell.getInterfaces();
		for(map<CPM::CELL_ID,uint>::const_iterator nb = interfaces.begin(); nb != interfaces.end(); nb++){

			if( CPM::getCellIndex( nb->first ).status != CPM::REGULAR_CELL )
				continue;

			const Cell& touching_cell = CPM::getCell( nb->first );

			// ensure cells are of the same celltype 
			if( celltype != touching_cell.getCellType() ){
// 				cout << "Unequal cell type: " << celltype->getName() << " and "<< touching_cell.getCellType()->getName() << endl;
				continue;
			}
			
			int interfacelength  = nb->second;
// 			cout << "Equal cell type: " << celltype->getName() << " and "<< touching_cell.getCellType()->getName() << "; interface length =  " << interfacelength << endl;

			if( interfacelength > max_interface_length ){
				fusion_partner_id = nb->first;
				max_interface_length = interfacelength;
// 				cout << mcs <<": CellID "<< cell_id << "; CELLID2" << fusion_partner_id << "; interface length = " << interfacelength << endl;
			}
		}
		
		if ( max_interface_length < 1  || fusion_partner_id == CPM::NO_CELL ||  CPM::getCellIndex(fusion_partner_id).status != CPM::REGULAR_CELL ) {
			continue;
		}
		
		// Changing all nodes of fusion partner into my state
		const Cell& fusion_partner = CPM::getCell(fusion_partner_id) ;
		const Cell::Nodes& fusion_nodes = fusion_partner.getNodes();
		uint fusion_nodes_num = fusion_nodes.size();

		uint old_target_volume = targetvolume.get( cell_id );
		uint new_target_volume = old_target_volume + fusion_nodes_num;
		
		targetvolume.set( cell_id, new_target_volume );
		targetvolume.set( fusion_partner_id, 0 );

		while (!fusion_nodes.empty())
		{
			CPM::setNode( *(fusion_nodes.begin()), cell_id );
		}

		CPM::setCellType( fusion_partner_id, CPM::getEmptyCelltype()->getID() );
	}
};


void Fusion::init()
{
	TimeStepListener::init(); 
	celltype = SIM::getScope()->getCellType();
	
 	if( targetvolume_str.size() > 0 ){
 	  targetvolume 	= celltype->findCellProperty<double>(targetvolume_str, true);
 	}	
}

void Fusion::loadFromXML(const XMLNode xNode)
{
	TimeStepListener::loadFromXML(xNode);
 	getXMLAttribute(xNode,"TargetVolume/symbol-ref", targetvolume_str);
}


set< string > Fusion::getDependSymbols()
{
	return set<string>();
}

set< string > Fusion::getOutputSymbols()
{
	set<string> symbols; 
	symbols.insert("cell.center");
	return symbols;
}


