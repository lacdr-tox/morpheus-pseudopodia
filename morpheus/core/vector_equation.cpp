#include "vector_equation.h"

// MANUALLY REGISTER THE VectorEquation CLASS FOR KEYWORDS "VectorEquation" and "VectorRULE"
Plugin* VectorEquation ::createInstance() { return new VectorEquation(); }

bool VectorEquation ::factory_registration = PluginFactory::RegisterCreatorFunction("VectorEquation", VectorEquation::createInstance )
									   && PluginFactory::RegisterCreatorFunction("VectorRule", VectorEquation::createInstance );

VectorEquation::VectorEquation(): ReporterPlugin() {
	symbol.setXMLPath("symbol-ref");
	registerPluginParameter(symbol);
	
	notation.setXMLPath("notation");
	notation.setDefault("x,y,z");
	notation.setConversionMap(VecNotationMap());
	registerPluginParameter(notation);
	
	expression.setXMLPath("Expression/text");
	registerPluginParameter(expression);
}

void VectorEquation::report()
{
	FocusRange range(symbol.granularity(), scope());
	
	if (range.size()>100) {
#pragma omp parallel for
		for (auto focus = range.begin(); focus < range.end(); ++focus ) {
			symbol.set(*focus, VDOUBLE::from(expression(*focus), notation())) ;
		}
	}
	else {
		for (const auto& focus : range ) {
			symbol.set(focus, VDOUBLE::from(expression(focus), notation())) ;
		}
	}
}
