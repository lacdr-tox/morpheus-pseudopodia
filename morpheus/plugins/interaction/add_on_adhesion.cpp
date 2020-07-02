#include "add_on_adhesion.h"

REGISTER_PLUGIN(AddonAdhesion);

AddonAdhesion::AddonAdhesion()
{
	adhesive.setXMLPath("adhesive");
	registerPluginParameter(adhesive);
	
	strength.setXMLPath("strength");
	registerPluginParameter(strength);
}

double AddonAdhesion::interaction(const SymbolFocus& cell1, const SymbolFocus& cell2)
{
	double dE = - adhesive( cell1 ) * strength( cell1 );
	dE += - adhesive( cell2 ) * strength( cell2 );
	
	return dE*0.5;
}

