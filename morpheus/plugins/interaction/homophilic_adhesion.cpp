#include "homophilic_adhesion.h"

REGISTER_PLUGIN(HomophilicAdhesion);

HomophilicAdhesion::HomophilicAdhesion()
{	
	adhesive.setXMLPath("adhesive");
	strength.setXMLPath("strength");

	binding_ratio.setXMLPath("equilibriumConstant");
	
	registerPluginParameter(adhesive);
	registerPluginParameter(strength);
	registerPluginParameter(binding_ratio);
}

double HomophilicAdhesion::interaction(const SymbolFocus& cell1, const SymbolFocus& cell2)
{
	double dE;
	if (binding_ratio.isDefined()) {
		double a1 = adhesive(cell1); 
		double a2 = adhesive(cell2);
		double p1 = 0.5 * (a1 + a2 + 1.0/binding_ratio());
		double p2 = sqrt(p1 - (a1  * a2) );
		dE = p1 - p2 * strength(cell1);
	} else {
		dE = min( adhesive(cell1), adhesive(cell2) ) * strength(cell1);
	}
	return dE;
}

