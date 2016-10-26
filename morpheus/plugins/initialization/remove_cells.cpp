#include "remove_cells.h"

REGISTER_PLUGIN(RemoveCells);

RemoveCells::RemoveCells() {
	condition.setXMLPath("Condition/text");
	registerPluginParameter(condition);
}

bool RemoveCells::run(CellType* celltype){

	vector<CPM::CELL_ID> cells = celltype->getCellIDs();
	for (int i=0; i<cells.size(); i++ ) {
		
		CPM::CELL_ID cell_id = cells[i];

		// check condition
		bool remove = condition( SymbolFocus(cell_id, CPM::getCell( cell_id ).getCenterL()) ) >= 1.0;
		
		// remove cell if condition is true
		if (remove){
			CPM::setCellType(cell_id, CPM::getEmptyCelltypeID());
		}
	}
	return true;
}

