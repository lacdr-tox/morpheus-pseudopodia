#include "surface_constraint.h"

REGISTER_PLUGIN(SurfaceConstraint);

SurfaceConstraint::SurfaceConstraint(){
	target.setXMLPath("target");
	strength.setXMLPath("strength");
	target_mode.setXMLPath("mode");
	map<string,TargetMode> mapping;
	mapping["surface"] = TargetMode::SURFACE;
	mapping["aspherity"] = TargetMode::ASPHERITY;
	target_mode.setConversionMap(mapping);
	
	registerPluginParameter(target);
	registerPluginParameter(strength);
	registerPluginParameter(target_mode);

	exponent.setXMLPath("exponent");
	registerPluginParameter(exponent);
};

double SurfaceConstraint::delta ( const SymbolFocus& cell_focus, const CPM::Update& update ) const
{
	double d_surface_length = cell_focus.cell().currentShape().surface();
	double d_new_surface_length = cell_focus.cell().updatedShape().surface();
	
	double s = strength( cell_focus );
	double dE = 0.0;
	double target_surface;
	double new_target_surface;
	if (target_mode() == TargetMode::SURFACE) {
		new_target_surface = target_surface = target( cell_focus ) ;
	}
	else { // (target_mode() == TargetMode::ASPHERITY)
		auto aspherity = target( cell_focus );
		target_surface =  aspherity * targetSurfaceFromVolume(cell_focus.cell().currentShape().size());
		new_target_surface = aspherity * targetSurfaceFromVolume(cell_focus.cell().updatedShape().size());
	}
	if( exponent.isDefined() ){
		dE = s * ( pow(d_new_surface_length - new_target_surface, exponent()) - pow(d_surface_length - target_surface, exponent()) );
	}
	else{
		dE = s * ( sqr(d_new_surface_length - new_target_surface ) - sqr(d_surface_length - target_surface ) );
	}
	
	return dE;
}

double SurfaceConstraint::hamiltonian ( CPM::CELL_ID cell_id ) const
{
	SymbolFocus focus(cell_id);
	int surface_length = focus.cell().currentShape().surface();
	
	double t = target( SymbolFocus(cell_id) );
	if (target_mode() == TargetMode::ASPHERITY)
		t*=targetSurfaceFromVolume(focus.cell().currentShape().size());

	return strength(focus) * sqr( t - surface_length );
}

vector<double> SurfaceConstraint::target_surface_cache;

double SurfaceConstraint::targetSurfaceFromVolume( int volume ) const {
	
	static bool threeDLattice = (SIM::lattice().getDimensions() == 3 ? true : false);

	// create a cache of precalculated target surfaces for each encountered cell volumes
	//static vector<double> target_surface_cache;
	if ( volume >= target_surface_cache.size() ) {
		for (uint i_volume=target_surface_cache.size(); i_volume<= volume; i_volume++ ) {
			double target_surface = 0.0;
			if( ! threeDLattice  ){
				target_surface = (double) (/* ((sqrt(2.0)+1.0)/2.0) **/  2.0 * sqrt(M_PI * i_volume ));
			}
			else {// if 3D lattice
				// TODO: Multiply by a lattice correction prefactor for 3D 
				target_surface = (double) (4.0 *  M_PI * pow(((0.75 * i_volume) / M_PI ), (2.0/3.0)));
			}
			target_surface_cache.push_back(target_surface);
		}
	}
	return target_surface_cache[volume];
}
