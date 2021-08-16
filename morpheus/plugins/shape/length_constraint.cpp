#include "length_constraint.h"

REGISTER_PLUGIN(LengthConstraint);

#include <complex>

const int USE_OLD_VERSION=0;
const int PARANOID_CHECK=0;

LengthConstraint::LengthConstraint(){
	target.setXMLPath("target");
	strength.setXMLPath("strength");

	target_mode.setXMLPath("mode");
	map<string,TargetMode> mapping;
	mapping["length"] = TargetMode::TARGET_LENGTH;
	mapping["eccentricity"] = TargetMode::TARGET_ECCENTRICITY;
	target_mode.setConversionMap(mapping);
	
	registerPluginParameter(target_mode);
	registerPluginParameter(target);
	registerPluginParameter(strength);
};


double LengthConstraint::delta( const SymbolFocus& cell_focus, const CPM::Update& update) const
{ 
	double s = strength( cell_focus );
	double t = target( cell_focus );
	double dE;
	if (target_mode() == TargetMode::TARGET_LENGTH)
		dE = s * ( sqr( t - cell_focus.cell().updatedShape().ellipsoidApprox().lengths[0] ) - sqr( t - cell_focus.cell().currentShape().ellipsoidApprox().lengths[0]));
	else 
		dE = s * ( sqr( t - cell_focus.cell().updatedShape().ellipsoidApprox().eccentricity ) - sqr( t - cell_focus.cell().currentShape().ellipsoidApprox().eccentricity));
// 	cout << cell_focus.cell().updatedShape().ellipsoidApprox().eccentricity << " " << cell_focus.cell().currentShape().ellipsoidApprox().eccentricity << endl;
	return dE;
}

double LengthConstraint::hamiltonian( CPM::CELL_ID cell_id) const {	  	
	double s = strength( SymbolFocus( cell_id ) );
	double t = target( SymbolFocus( cell_id ) );
	double dE = s * sqr( SymbolFocus( cell_id ).cell().currentShape().ellipsoidApprox().lengths[0] - t ); // do full calculation, updates everything
	return dE;
}

