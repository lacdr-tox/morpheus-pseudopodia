#include "volume_constraint.h"

REGISTER_PLUGIN(VolumeConstraint);

VolumeConstraint::VolumeConstraint(){
	target.setXMLPath("target");
	strength.setXMLPath("strength");
	registerPluginParameter(target);
	registerPluginParameter(strength);
};

double VolumeConstraint::delta( const SymbolFocus& cell_focus, const CPM::Update& update) const
{
	double s = strength( cell_focus );
	double t = target( cell_focus );

	// Vb = volume before update
	int Vb = cell_focus.cell().nNodes(); 
	// Va = volume after update
	int Va =  Vb + (update.opAdd()) - (update.opRemove());
	
	double dE = s * ( sqr(t - Va) - sqr(t - Vb) );
	return dE;
}

double VolumeConstraint::hamiltonian( CPM::CELL_ID cell_id ) const {
	double V = double(CPM::getCell(cell_id).nNodes());
	double s = strength( SymbolFocus( cell_id ) );
	double t = target( SymbolFocus( cell_id ) );
	return s * sqr(t - V);
}
