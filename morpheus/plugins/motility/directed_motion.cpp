#include "directed_motion.h"

REGISTER_PLUGIN(DirectedMotion);

DirectedMotion::DirectedMotion() : CPM_Energy()
{
	direction.setXMLPath("direction");
	registerPluginParameter(direction);
	strength.setXMLPath("strength");
	registerPluginParameter(strength);
	
	protrusion.setXMLPath("protrusion");
	protrusion.setDefault("true");
	registerPluginParameter(protrusion);
	retraction.setXMLPath("retraction");
	retraction.setDefault("true");
	registerPluginParameter(retraction);
	
}

void DirectedMotion::init(const Scope* scope){
	CPM_Energy::init( scope );

	if( !protrusion() && !retraction() ) 
	{
		cerr << "DirectedMotion: init(): Both retraction and protrusion is set to false. Therefore, DirectedMotion has no effect, which is probably not the intended behavior." << endl;
	    exit(-1);
	}
}

double DirectedMotion::delta(const SymbolFocus& cell_focus, const CPM::UPDATE& update, CPM::UPDATE_TODO todo ) const
{
	const Cell& cell = cell_focus.cell();
	VDOUBLE update_direction = cell.getUpdatedCenter() - cell.getCenter();
	VDOUBLE preferred_direction = direction( cell_focus ).norm();
	double cell_volume = cell.nNodes();
	
	double s = 0.0;
	if( (todo & CPM::ADD) && protrusion() || ( (todo & CPM::REMOVE) && retraction() ))
		s = strength( update.focus );
	
	double dE = -s * cell_volume * ( update_direction * preferred_direction );

	return dE;
}

double DirectedMotion::hamiltonian(CPM::CELL_ID) const
{
  return 0;
}
