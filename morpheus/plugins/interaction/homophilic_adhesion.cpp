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

double HomophilicAdhesion::interaction(CPM::STATE s1, CPM::STATE s2)
{
	double dE;
	SymbolFocus focus1( s1.cell_id, s1.pos );
	SymbolFocus focus2( s2.cell_id, s2.pos );

	if (binding_ratio.isDefined()) {
		double a1 = adhesive(focus1); 
		double a2 = adhesive(focus2);
		double p1 = 0.5 * (a1 + a2 + 1.0/binding_ratio());
		double p2 = sqrt(p1 - (a1  * a2) );
		dE = p1 - p2 * strength(focus1);
	} else {
		dE = min( adhesive(focus1), adhesive(focus2) ) * strength(focus1);
	}
	return dE;
}

