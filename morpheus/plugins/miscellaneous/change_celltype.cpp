#include "change_celltype.h"

REGISTER_PLUGIN(ChangeCelltype);

// TODO: What is the proper interface for MCSListeners?
ChangeCelltype::ChangeCelltype() : InstantaneousProcessPlugin( TimeStepListener::XMLSpec::XML_OPTIONAL) {

	condition.setXMLPath("Condition/text");
	registerPluginParameter(condition);

	celltype_new.setXMLPath("newCellType");
	registerPluginParameter(celltype_new);
	
};

void ChangeCelltype::loadFromXML(const XMLNode xNode, Scope* scope)
{
	InstantaneousProcessPlugin::loadFromXML(xNode, scope);

	triggers = shared_ptr<TriggeredSystem>(new TriggeredSystem);
	celltype_new.init();

	if (xNode.nChildNode("Triggers")) {
		triggers->loadFromXML(xNode.getChildNode("Triggers"), const_cast<Scope*>(celltype_new()->getScope()));
	}

}

void ChangeCelltype::init(const Scope* scope)
{
	if (CPM::getMCSDuration()>0)
		setTimeStep( CPM::getMCSDuration() );
	
	InstantaneousProcessPlugin::init(scope);
	celltype = scope->getCellType();
	registerInputSymbols(condition.getDependSymbols());

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



