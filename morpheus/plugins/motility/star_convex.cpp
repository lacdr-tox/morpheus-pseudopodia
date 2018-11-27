#include "star_convex.h"

REGISTER_PLUGIN(StarConvex);

StarConvex::StarConvex() : CPM_Energy()
{
	membrane.setXMLPath("membrane");
	registerPluginParameter(membrane);
	strength.setXMLPath("strength");
	registerPluginParameter(strength);

	protrusion.setXMLPath("protrusion");
	protrusion.setDefault("true");
	registerPluginParameter(protrusion);
	retraction.setXMLPath("retraction");
	retraction.setDefault("true");
	registerPluginParameter(retraction);

}

void StarConvex::init(const Scope* scope){
	CPM_Energy::init( scope );

	if( !protrusion() && !retraction() )
	{
		cerr << "StarConvex: init(): Both retraction and protrusion is set to false. Therefore, DirectedMotion has no effect, which is probably not the intended behavior." << endl;
	    exit(-1);
	}
}

double StarConvex::delta(const SymbolFocus& focus, const CPM::Update& update) const
{
/*	const Cell& cell = focus.cell();
	VDOUBLE update_direction = focus.pos() - cell.getCenter();
	VDOUBLE preferred_direction = update.source().pos() - cell.getCenter();

    VDOUBLE preferred_direction(0,0,0);
    if ( membrane( focus ) < 0.0 )
        preferred_direction = cell.getUpdatedCenter() - cell.getCenter();
    else
        preferred_direction = cell.getCenter() - cell.getUpdatedCenter();

    preferred_direction *= VDOUBLE(membrane( focus ),membrane( focus ),membrane( focus ));
	double cell_volume = cell.nNodes();
	double s = strength( focus );
	double dE = -s * cell_volume * dot( update_direction, preferred_direction );
*/

    double dE = 0.0;
    //cout << "focus = " << focus.pos() << ", mem = " <<  membrane( focus ) << endl;
    if( update.opAdd() && membrane( focus ) > 0 && protrusion())
        dE -= strength( focus ) * membrane( focus );
    if( update.opRemove() && membrane( focus.pos() ) < 0 && retraction())
        dE += strength( focus ) * membrane( focus );
    //cout << "dE = " << dE << endl;
 	return dE;
}

double StarConvex::hamiltonian(CPM::CELL_ID) const
{
  return 0;
}
