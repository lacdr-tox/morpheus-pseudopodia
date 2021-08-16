#include "heterophilic_adhesion.h"

REGISTER_PLUGIN(HeterophilicAdhesion);

HeterophilicAdhesion::HeterophilicAdhesion()
{
	adhesive1.setXMLPath("adhesive1");
	adhesive2.setXMLPath("adhesive2");
	strength.setXMLPath("strength");
	
	registerPluginParameter( adhesive1 );
	registerPluginParameter( adhesive2 );
	registerPluginParameter( strength );
}


double HeterophilicAdhesion::interaction(const SymbolFocus& cell1, const SymbolFocus& cell2)
{
	
	double a1c1 = adhesive1( cell1 );
	double a2c1 = adhesive2( cell1 );
	double a1c2 = adhesive1( cell2 );
	double a2c2 = adhesive2( cell2 );
	double dE;

	if (binding_ratio.isDefined()) {
		double p1 = a1c1 + a2c2 + 1.0/binding_ratio();
		double p2 = a2c1 + a1c2 + 1.0/binding_ratio();
		double bonds = p1 - sqrt(p1*p1 - a1c1*a2c2) + p2 - sqrt(p2*p2 - a2c1*a1c2);
		dE = bonds * strength( cell1 );
	}
	else {
		// full saturation
		dE = (min(a1c1,a2c2) + min(a2c1,a1c2)) * strength( cell1 );
	}
	return dE;

}

