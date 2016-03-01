#include "freeze_motion.h"

REGISTER_PLUGIN(FreezeMotion);

FreezeMotion::FreezeMotion(){
	condition.setXMLPath("Condition/text");
	registerPluginParameter(condition);
}

void FreezeMotion::init(const Scope* scope){
	CPM_Check_Update::init(scope);
	condition.init();
}

bool FreezeMotion::update_check(CPM::CELL_ID cell_id, const CPM::UPDATE& update, CPM::UPDATE_TODO todo )
{
    if( condition( SymbolFocus(cell_id) ) >= 1.0 ){
        return false;
    }
    return true;
}
