#include "diff_eqn.h"

REGISTER_PLUGIN(DifferentialEqn);

void DifferentialEqn::loadFromXML(const XMLNode node, Scope* scope )
{
    Plugin::loadFromXML(node,scope);

	if ( ! node.nChildNode("Expression")) {
		cout << "Undefined DiffEqn/Expression/text. " << endl;
		exit(-1);
	} else {
		getXMLAttribute(node,"Expression/text",expression);
	}

	if( !getXMLAttribute(node, "symbol-ref", symbol_name) ){
		cerr << "DifferentialEqn::loadFromXML: DifferentialEqn with expression '" << expression << "' did not specify a symbol-ref. " << endl;
		exit(-1);
	}

}

void DifferentialEqn::init(const Scope* scope)
{
	Plugin::init(scope);
}
