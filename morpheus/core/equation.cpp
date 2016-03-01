#include "equation.h"

// REGISTER_PLUGIN(Equation);

// REGISTER THE EQUATION CLASS FOR KEYWORDS "EQUATION" and "RULE"
Plugin* Equation ::createInstance() { return new Equation (); }

bool Equation ::factory_registration = PluginFactory::RegisterCreatorFunction("Equation", Equation::createInstance )
									   && PluginFactory::RegisterCreatorFunction("Rule", Equation::createInstance );

Equation::Equation(): ReporterPlugin() {
	symbol.setXMLPath("symbol-ref");
	registerPluginParameter(symbol);
}


void Equation::loadFromXML(const XMLNode node )
{
    ReporterPlugin::loadFromXML(node);

	if ( ! node.nChildNode("Expression")) {
		cerr << "Undefined Expression/text for equation." << endl;
		exit(-1);
	} else {
		getXMLAttribute(node,"Expression/text",expression);
	}
}

void Equation::init(const Scope* scope)
{
	ReporterPlugin::init(scope);
	evaluators = shared_ptr<ThreadedExpressionEvaluator<double> >(new ThreadedExpressionEvaluator<double>(expression));
	evaluators->init(scope);
	registerInputSymbols(evaluators->getDependSymbols());
}


void Equation::report()
{
	FocusRange fr(symbol.accessor(),scope());

	for (auto f: fr) {
		symbol.set(f,evaluators->get(f));
	}
}