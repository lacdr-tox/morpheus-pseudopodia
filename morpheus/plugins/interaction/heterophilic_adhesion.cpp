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


double HeterophilicAdhesion::interaction(CPM::STATE s1, CPM::STATE s2)
{
	SymbolFocus focus1(s1.cell_id,s1.pos);
	SymbolFocus focus2(s1.cell_id,s1.pos);
	
	double a1c1 = adhesive1( focus1 );
	double a2c1 = adhesive2( focus1 );
	double a1c2 = adhesive1( focus2 );
	double a2c2 = adhesive2( focus2 );

	double dE = ((a1c1*a2c2) + (a2c1*a1c2)) * strength( focus1 );
	return dE;

}

