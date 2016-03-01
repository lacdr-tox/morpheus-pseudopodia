#include "matrix_guidance.h"
#include <core/symbol_accessor.h>
#include <core/interaction_energy.h>

using namespace SIM;

REGISTER_PLUGIN(MatrixGuidance);

MatrixGuidance::MatrixGuidance(){};

void MatrixGuidance::loadFromXML(const XMLNode Node)
{
	CPM_Energy::loadFromXML(Node);
	if ( ! getXMLAttribute(Node,"StrengthLayer/symbol-ref",layer_strength_symbol)) {
		cerr << "MatrixGuidance::loadFromXML:: StrengthLayer symbol-ref not specified!" << endl; 
		exit(-1);
	}

	if ( ! getXMLAttribute(Node,"OrientationLayer/symbol-ref",layer_orientation_symbol)) {
		cerr << "MatrixGuidance::loadFromXML:: OrientationLayer symbol-ref not specified!" << endl; 
		exit(-1);
	}
	
// 	degradation = 0;
// 	if ( ! getXMLAttribute(Node,"degradation",degradation)) {
// 		cout << "MatrixGuidance::loadFromXML:: degradation not specified!" << endl; 
// 	}
// 	reorientation = 0;
// 	if ( ! getXMLAttribute(Node,"reorientation",reorientation)) {
// 		cout << "MatrixGuidance::loadFromXML:: reorientation not specified!" << endl; 
// 	}
	
}

void MatrixGuidance::init()
{
	CPM_Energy::init();
	layer_strength = SIM::findPDELayer(layer_strength_symbol);
	if (! layer_strength) {
		cout << "MatrixGuidance::init PDELayer " << layer_strength << " undefined!" << endl;
		exit(-1);
	}
	layer_orientation = SIM::findPDELayer(layer_orientation_symbol);
	if (! layer_orientation) {
		cout << "MatrixGuidance::init PDELayer " << layer_orientation << " undefined!" << endl;
		exit(-1);
	}
};

set< string > MatrixGuidance::getDependSymbols()
{
    set<string> s=Plugin::getDependSymbols();
	s.insert(layer_strength_symbol);
	s.insert(layer_orientation_symbol);
	return s;
}


double MatrixGuidance::delta(CPM::CELL_ID cell_id, const CPM::UPDATE& update, CPM::UPDATE_TODO todo) const
{
    // orientation of matrix fiber (in layer_orientation). Note: limited to 2D
	double or_fiber_rad = layer_orientation->get( update.focus );
// 	if( or_fiber_rad < 0 || or_fiber_rad > 0.5*M_PI){
// 		cerr << "MatrixGuidance: " << update.focus << " = " <<  or_fiber_rad << endl;
// 		cerr << "MatrixGuidance: Error: Fiber orientation not in [-pi, pi] interval." << endl;
// 		exit(-1);
// 	}
    VDOUBLE or_fiber_vec = VDOUBLE( cos(or_fiber_rad), sin(or_fiber_rad), 0.0);

	
    // orientation of update
    VDOUBLE or_update_vec = VDOUBLE(update.focus - update.source); 

	// TODO: correct for periodc boundary conditions. Something like
	//if( or_update_vec.abs() > max distance of interaction neighborhood ){
	//		change the update.source 
	// }

	// normalize update orientation vector
	or_update_vec = or_update_vec / update.source.abs(); // VDOUBLE(update.source - update.focus).abs(); // unit vector
	
    // acute angle between two vectors (should work in 3D)
    double angle = angle_unsigned_3D(or_update_vec, or_fiber_vec) - 0.5*M_PI;
    double diff_orientation = abs(angle); //(angle < 0 ? -angle : angle);

    double interference = (diff_orientation / (0.5*M_PI));

	double strength = layer_strength->get( update.focus );

// 	cout << "angle: " << angle
//             << "\torient (rad): " << diff_orientation
//             <<"\torient (degr): " << diff_orientation * (180/M_PI)
//             << "\teffect:	" << interference
//             << "\tdelta: " << (strength * sqr(interference)) << "\n";
// 	

    //if(todo & CPM::ADD)
        return (strength * sqr(interference));
	//if(todo & CPM::REMOVE)
    //     return (strength * interference);
}

double MatrixGuidance::hamiltonian(CPM::CELL_ID cell_id) const {
	cout << "MatrixGuidance::hamiltonian MISSING"  << endl;
	return 0.0;
}


