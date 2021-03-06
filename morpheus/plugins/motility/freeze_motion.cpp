#include "freeze_motion.h"

REGISTER_PLUGIN(FreezeMotion);

FreezeMotion::FreezeMotion(){
	condition.setXMLPath("Condition/text");
	registerPluginParameter(condition);
}

void FreezeMotion::init(const Scope* scope){
	Cell_Update_Checker::init(scope);
}

bool FreezeMotion::update_check(CPM::CELL_ID cell_id, const CPM::Update& update)
{
    if( condition( SymbolFocus(cell_id) ) >= 1.0 ){
        return false;
    }
    return true;
}
