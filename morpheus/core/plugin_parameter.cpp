#include "plugin_parameter.h"

template<>
void XMLReadableSymbol< double, DefaultValPolicy >::init() {
    if (! DefaultValPolicy::isMissing()) {
        stringstream s(DefaultValPolicy::stringVal());
        double default_val;
        s >> default_val;
        if (scope)
            _accessor = scope->findSymbol<double>(symbol_name,default_val);
        else if (require_global_scope)
            _accessor = SIM::getGlobalScope()->findSymbol<double>(symbol_name,default_val);
        else
            _accessor = SIM::getScope()->findSymbol<double>(symbol_name,default_val);
    }
}

template<>
void XMLReadableSymbol< VDOUBLE, DefaultValPolicy >::init() {
    if (! DefaultValPolicy::isMissing()) {
        stringstream s(DefaultValPolicy::stringVal());
        VDOUBLE default_val;
        s >> default_val;
        if (scope)
            _accessor = scope->findSymbol<VDOUBLE>(symbol_name,default_val);
        else if (require_global_scope)
            _accessor = SIM::getGlobalScope()->findSymbol<VDOUBLE>(symbol_name,default_val);
        else
            _accessor = SIM::getScope()->findSymbol<VDOUBLE>(symbol_name,default_val);
    }
}