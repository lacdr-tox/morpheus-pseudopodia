#include "vector_equation.h"

// MANUALLY REGISTER THE VectorEquation CLASS FOR KEYWORDS "VectorEquation" and "VectorRULE"
Plugin* VectorEquation ::createInstance() { return new VectorEquation(); }

bool VectorEquation ::factory_registration = PluginFactory::RegisterCreatorFunction("VectorEquation", VectorEquation::createInstance )
									   && PluginFactory::RegisterCreatorFunction("VectorRule", VectorEquation::createInstance );

VectorEquation::VectorEquation(): ReporterPlugin() {
	symbol.setXMLPath("symbol-ref");
	registerPluginParameter(symbol);
	
	spherical.setXMLPath("spherical");
	spherical.setDefault("false");
	registerPluginParameter(spherical);
	
	expression.setXMLPath("Expression/text");
	registerPluginParameter(expression);
}

void VectorEquation::report()
{
	FocusRange range(symbol.accessor().getGranularity(), scope());
	
	if (range.size()>500) {
#pragma omp parallel for
		for (auto focus = range.begin(); focus < range.end(); ++focus ) {
			symbol.set(*focus, spherical() ? VDOUBLE::from_radial(expression(*focus)) : expression(*focus) ) ;
		}
	}
	else {
		for (const auto& focus : range ) {
			symbol.set(focus, spherical() ? VDOUBLE::from_radial(expression(focus)) : expression(focus) ) ;
		}
	}
}
