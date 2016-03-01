#include "add_on_adhesion.h"

REGISTER_PLUGIN(AddonAdhesion);

AddonAdhesion::AddonAdhesion()
{
	adhesive.setXMLPath("adhesive");
	registerPluginParameter(adhesive);
	
	strength.setXMLPath("strength");
	registerPluginParameter(strength);
}

double AddonAdhesion::interaction(CPM::STATE s1, CPM::STATE s2)
{
	SymbolFocus focus = SymbolFocus( s1.cell_id,s1.pos );
	double dE = -adhesive( focus ) * strength( focus );
	return dE;
}

