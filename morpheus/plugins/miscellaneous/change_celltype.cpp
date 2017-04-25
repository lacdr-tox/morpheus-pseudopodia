#include "change_celltype.h"

REGISTER_PLUGIN(ChangeCelltype);

// TODO: What is the proper interface for MCSListeners?
ChangeCelltype::ChangeCelltype() : InstantaneousProcessPlugin( TimeStepListener::XMLSpec::XML_NONE) {

	condition.setXMLPath("Condition/text");
	registerPluginParameter(condition);

	celltype_new.setXMLPath("newCellType");
	registerPluginParameter(celltype_new);
	
};

void ChangeCelltype::loadFromXML(const XMLNode xNode)
{
    InstantaneousProcessPlugin::loadFromXML(xNode);

    triggers = shared_ptr<TriggeredSystem>(new TriggeredSystem);
	 celltype_new.init();
	 SIM::enterScope(celltype_new()->getScope());
    if (xNode.nChildNode("Triggers")) {
        triggers->loadFromXML(xNode.getChildNode("Triggers"));
    }
    SIM::leaveScope();

}

void ChangeCelltype::init(const Scope* scope)
{
	InstantaneousProcessPlugin::init(scope);
	celltype = scope->getCellType();
	
	setTimeStep( CPM::getMCSDuration() );
	is_adjustable = false;

	triggers->init();

}


void ChangeCelltype::executeTimeStep() {

	int celltype_new_ID = celltype_new()->getID();
	vector <CPM::CELL_ID> cells = celltype->getCellIDs();
	for (int i=0; i < cells.size(); i++ ) {
		
		if( celltype_new_ID != celltype->getID()
			&& condition( SymbolFocus(cells[i]) ) ){
			
			auto changed_cell = CPM::setCellType( cells[i], celltype_new_ID);
			triggers->trigger(SymbolFocus(changed_cell));
		
		}
	}
}



