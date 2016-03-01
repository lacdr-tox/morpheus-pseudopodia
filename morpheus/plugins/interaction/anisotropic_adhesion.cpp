#include "anisotropic_adhesion.h"


REGISTER_PLUGIN(Anisotropic_Adhesion);


double Anisotropic_Adhesion::interaction(CPM::STATE s1, CPM::STATE s2)
{
	double adhesive1 = computeAdhesive( s1 );
	double adhesive2 = computeAdhesive( s2 );
	return -adhesive1*adhesive2;	
}

double Anisotropic_Adhesion::computeAdhesive(CPM::STATE s) {

	static bool threeDlattice = (SIM::getLattice()->getDimensions() == 3 ? true : false);
		
	uint id = s.cell_id;
	VDOUBLE cell_center = CPM::getCell(id).getCenter();
	double adhesive = 0.0;
  
	
	if(threeDlattice && use_vectors){ // 3D in vectors
		
		//double polarization_strength = ((VDOUBLE)orientation_vec.get( id ).abs();
		double polarization_strength = 1.0;
		SIM::getLattice()->resolve( s.pos );
		VDOUBLE update_vector = ((VDOUBLE)s.pos - cell_center).norm();
		VDOUBLE orientation_vector = orientation_vec.get( id ).norm();
		
		double angle_new = angle_unsigned_3D(update_vector,  orientation_vector);
		
		// set property via symbol
		angle.set(id, angle_new);
		
		// use user-custom function to calculate adhesivity at update position
		double function_output = fct_function->get( id );
		adhesive = strength.get( id ) * polarization_strength * (function_output > 0 ? function_output : 0);		
		//cout << update_vector  << "\t" << orientation_vector << "\t" << angle.get(id) << "\t " << adhesive << endl;		
	}
	
	else if(!threeDlattice && !use_vectors){ // 2D in radials
/*	
		VINT update_pos = s.pos;
		double dotp = dot(vertical, cell_center);
		double perpDot = vertical.x * cell_center.y - vertical.y * cell_center.x;
		double orientation_update = atan2( perpDot, dotp);
		double deviation_from_orientation = abs(orientation_update - orientation_rad.get( id )); 
		
		adhesive = strength.get( id ) * (cos( deviation_from_orientation )); //pow( abs(sin( deviation_from_orientation )), 1) * strength.get(cell2); //0.5 + 0.5*sin((0.75*M_PI+angle)*2.0);
*/
	}
	else if(!threeDlattice &&  use_vectors){ // 2D in vectors
		cerr << "Anisotropic_Adhesion:: 2D in vectors: Not implemented yet!" << endl; 
		exit(-1);
	}
	else if (threeDlattice && !use_vectors){ // 3D in radials
		cerr << "Anisotropic_Adhesion:: 3D in radials: Not possible!" << endl; 
		exit(-1);	
	} else {
		cerr << "Anisotropic_Adhesion:: Hmm... Can only use vectors in 3D" << endl; 
		exit(-1);	
	}
	
	return adhesive;

}

void Anisotropic_Adhesion::init(){
	Interaction_Addon::init();
	
	use_vectors = false;
	orientation_vec = SIM::findSymbol<VDOUBLE>(orientation_str);
	if( orientation_vec.valid() ){
		cout << "Anisotropic_Adhesion::init: Orientation symbol '"<<orientation_str<<"' is a vector." << endl;
		use_vectors = true;
	}
	else{
		cerr << "Anisotropic_Adhesion::init: Orientation symbol is not radial (double), nor a vector. Fatal." << endl;
		exit(-1);
	}

	strength = SIM::findSymbol<double>(strength_str);
	if( !strength.valid() ){
		cerr << "Anisotropic_Adhesion::init: Strength symbol '"<< strength_str << "' was not found." << endl;
		exit(-1);
	}


	// find (global) symbol to a cell property to holds the angle 
	// TODO: In the future, this may be replaced by a local symbol (without the need to define a cell property)
	angle = SIM::findRWSymbol<double>(angle_str);

	// initialize function
	fct_function->init();
	
	
}

void Anisotropic_Adhesion::loadFromXML(const XMLNode xNode)
{
	Interaction_Addon::loadFromXML(xNode);
	
	if( xNode.nChildNode("Orientation") ){
		getXMLAttribute(xNode, "Orientation/symbol-ref",orientation_str); // can be radial (double), or a vector
	}
	if( xNode.nChildNode("Strength") ){
		if(!getXMLAttribute(xNode, "Strength/symbol-ref",strength_str)){
			cerr << "Anisotropic_Adhesion::loadFromXML: Strength/symbol-ref not specified!" << endl; exit(-1);
		}
			
	}
	
	if( xNode.nChildNode("Angle") ){
		if(!getXMLAttribute(xNode, "Angle/symbol-ref",strength_str)){
			cerr << "Anisotropic_Adhesion::loadFromXML: Angle/symbol-ref not specified!" << endl; exit(-1);
		}
	}

	if( xNode.nChildNode("Function") ){
		string expression;
		getXMLAttribute(xNode,"Function/text",expression);
		fct_function = shared_ptr<Function> (new Function() );
		fct_function->setExpr(expression);
	}
}
