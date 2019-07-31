#include "equation.h"

// REGISTER_PLUGIN(Equation);

// REGISTER THE EQUATION CLASS FOR KEYWORDS "EQUATION" and "RULE"
Plugin* Equation ::createInstance() { return new Equation (); }

bool Equation ::factory_registration = PluginFactory::RegisterCreatorFunction("Equation", Equation::createInstance )
									   && PluginFactory::RegisterCreatorFunction("Rule", Equation::createInstance );

Equation::Equation(): ReporterPlugin() {
	symbol.setXMLPath("symbol-ref");
	registerPluginParameter(symbol);
	expression.setXMLPath("Expression/text");
	registerPluginParameter(expression);
}

void Equation::report()
{
	FocusRange fr(symbol.accessor(),scope());
	
	if (fr.size()>500) {
		auto end = fr.end();
#pragma omp parallel for
		for (auto f=fr.begin(); f<fr.end(); ++f) {
			symbol.set(*f,expression(*f));
		}
	}
	else {
		for (const auto& f: fr) {
			symbol.set(f,expression(f));
			cout << "Setting " << symbol.stringVal() << "=" << expression(f) << endl;
		}
	}
	// Make equations always run
	prepared_time_step = SIM::getTime() - valid_time;
	if (prepared_time_step<0) prepared_time_step = 0;
}
