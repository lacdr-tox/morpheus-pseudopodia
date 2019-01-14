#include "cell_death.h"

REGISTER_PLUGIN(CellDeath);

CellDeath::CellDeath() : InstantaneousProcessPlugin( TimeStepListener::XMLSpec::XML_NONE ) {
	condition.setXMLPath("Condition/text");
	registerPluginParameter(condition);
	target_volume.setXMLPath("Shrinkage/target-volume");
	registerPluginParameter(target_volume);
    remove_volume.setXMLPath("Shrinkage/remove-volume");
    registerPluginParameter(remove_volume);
}

void CellDeath::init(const Scope* scope)
{
	InstantaneousProcessPlugin::init( scope );
	
	setTimeStep( CPM::getMCSDuration() );
	is_adjustable = false;
	celltype = scope->getCellType();
	if ( target_volume.isDefined() ) {
		mode = SHRINKAGE;
		// determine whether the target_volume symbol has a cellular (spatial) granularity
		if ( target_volume.accessor()->granularity() != Granularity::Cell ){
			throw MorpheusException("CellDeath expects the target-volume symbol to be a cell property!", stored_node);
		}

	}
	else {
		mode = LYSIS;
	}

	// because this plugin changes the cell configuration, 
	//  the cell position need to be registered as output symbol (during init())
	registerCellPositionOutput();
}




void CellDeath::executeTimeStep()
{
	vector<CPM::CELL_ID> cells = celltype->getCellIDs();
	for (int i=0; i<cells.size(); i++ ) {
		bool remove_cell = false;
		CPM::CELL_ID cell_id = cells[i];
		
		if (dying.find(cell_id) != dying.end()) {

			if (CPM::getCell(cell_id).nNodes() <= remove_volume( SymbolFocus(cell_id) ))
				remove_cell = true;
		}
		else {
			bool about_to_die = condition(SymbolFocus(cell_id, VINT(CPM::getCell( cell_id ).getCenterL())) ) >= 1.0;
			if (about_to_die) {
				//cout << "Triggered cell death for cell " << cell_id << " at " << currentTime() << " due to condition" << endl;
				if (mode == LYSIS) {
					remove_cell = true;
				}
				else if (mode == SHRINKAGE) {
					target_volume.set(SymbolFocus(cell_id),0);
					dying.insert(cell_id);
				}
			}
		}
		
		if (remove_cell) {
			cout << "Removing cell " << cell_id << " at " << currentTime() << endl;
            SymbolFocus cell_focus(cell_id);
            const auto& interfaces = CPM::getCell(cell_id).getInterfaceLengths();
            bool to_medium = false;
            CPM::CELL_ID fusion_partner_id;
            double fusion_interface_length = 0;
            if (interfaces.size() == 0) {
              to_medium = true;
            }
            else{
              double p = getRandom01()*CPM::getCell(cell_id).getInterfaceLength();
              double p_run = 0;
              for (auto nb = interfaces.begin(); nb != interfaces.end(); nb++, i++) {
                if ((p > p_run) && (p < (p_run+nb->second))) {
                  fusion_partner_id = nb->first;
                  break;
                }
                p_run += nb->second;
              }

//              for (auto nb = interfaces.begin(); nb != interfaces.end(); nb++, i++) {
//                if (nb->second >= fusion_interface_length){
//                  fusion_partner_id = nb->first;
//                }
//              }
              if (fusion_partner_id == CPM::getEmptyCelltypeID()){
                to_medium = true;
              }
            }
            // randomly select neighbor: nb->second/CPM::getCell(cell_id).getInterfaceLength()
            // or select longest interface
            if (to_medium){
              cout << "add dying pixels to medium\n";
              CPM::setCellType(cell_id, CPM::getEmptyCelltypeID());
              if (dying.find(cell_id) != dying.end())
                dying.erase(cell_id);
            }
            else{
              cout << "add dying pixels to cell\n";
              const Cell::Nodes& dying_nodes = CPM::getCell(cell_id).getNodes();
              while (!dying_nodes.empty())
              {
                CPM::setNode( *(dying_nodes.begin()), fusion_partner_id );
              }
              CPM::setCellType( cell_id, CPM::getEmptyCelltypeID() );
            }


            // new cell type could also include ECM!
            // if new cell != ECM
            //  add cell nodes to new cell (steal from fusion.cpp)
            // else
            //  use default method (below)

		}
	}
}

