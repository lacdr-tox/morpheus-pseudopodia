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
            if (interfaces.size() == 0) {
              cout << "no neighbors\n";
            }

            //out << CPM::getCell(cell_id).getInterfaceLength() << "\t" << CPM::getCell(cell_id).getSurface() << endl;
            for (auto nb = interfaces.begin(); nb != interfaces.end(); nb++, i++) {
              cout << "interface between with " << nb->first << " = ";
              cout << nb->second << " of " << CPM::getCell(cell_id).getInterfaceLength() << endl;
            }
            // randomly select neighbor or select longest interface
            // new cell type could also include ECM!
            // if new cell != ECM
            //  add cell nodes to new cell (steal from fusion.cpp)
            // else
            //  use default method (below)
            CPM::setCellType(cell_id, CPM::getEmptyCelltypeID());
			  if (dying.find(cell_id) != dying.end())
			  	dying.erase(cell_id);
		}
	}
}

