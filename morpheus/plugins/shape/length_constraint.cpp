#include "length_constraint.h"

REGISTER_PLUGIN(LengthConstraint);

#include <complex>

const int USE_OLD_VERSION=0;
const int PARANOID_CHECK=0;

LengthConstraint::LengthConstraint(){
	target.setXMLPath("target");
	strength.setXMLPath("strength");
	registerPluginParameter(target);
	registerPluginParameter(strength);
};


void LengthConstraint::init(const Scope* scope) {
	CPM_Energy::init( scope );

	if( SIM::getLattice()->getDimensions()!=2 && SIM::getLattice()->getDimensions()!=3){
		cerr << "LengthConstraint::init: Length constraint only for 2D or 3D!" << endl;
		exit(-1);
	}
	CellType* p = SIM::getScope()->getCellType();
	
/*	_center 				= p->addProperty<VDOUBLE> ( "_stored_center", "internally stored cell position" );	
	_temp_center 			= p->addProperty<VDOUBLE> ( "_stored_center_temp", "internally stored cell position (temp)" );*/	
	_I  					= p->addProperty<std::vector<double> > ( "_stored_I-vector", "" );
	_temp_I  				= p->addProperty<std::vector<double> > ( "_stored_I-vector_temp", "" );	
	_cell_length 			= p->addProperty<double> ( "_stored_cell_length", "" );
	_temp_cell_length 		= p->addProperty<double> ( "_stored_cell_length_temp", "" );
	_maxIncrementalUpdates	= p->addProperty<double> ( "_stored_maxIncrementalUpdates", "" );
	_incrementalUpdatesLeft	= p->addProperty<double> ( "_stored_incrementalUpdatesLeft", "" );	
}

double LengthConstraint::delta( const SymbolFocus& cell_focus, const CPM::UPDATE& update, CPM::UPDATE_TODO todo) const
{ 
	double s = strength( cell_focus );
	double t = target( cell_focus );
	double dE = s * ( sqr( t - _temp_cell_length(cell_focus.cellID()) ) - sqr( t - _cell_length(cell_focus.cellID())));
	return dE;
}

double LengthConstraint::hamiltonian( CPM::CELL_ID cell_id) const {	  	
	assert( _I(cell_id).size() != 0);
	double s = strength( SymbolFocus( cell_id ) );
	double t = target( SymbolFocus( cell_id ) );
	double dE = s * sqr( long_cell_axis2(cell_id) - t ); // do full calculation, updates everything
	return dE;
}

void LengthConstraint::set_update_notify(CPM::CELL_ID cell_id, const CPM::UPDATE& update, CPM::UPDATE_TODO todo)
{
	std::vector<double> &I=_I(cell_id), 
	                    &temp_I=_temp_I(cell_id);	
	// in case that cell did not store intermediates ...
	if (I.empty()) {
		cout << "LengthConstraint:: Initializing incremental length updates for cell " << cell_id << endl;
		std::vector<double> &temp_I=_temp_I(cell_id); 
		std::vector<double> &I=_I(cell_id); 	  
		_maxIncrementalUpdates(cell_id)=1000*1000; // still very accurate, could be set by user in XML
		_incrementalUpdatesLeft(cell_id)=_maxIncrementalUpdates(cell_id);   
		size_t l=(SIM::getLattice()->getDimensions()-1)*3;	 
		I.resize(l,0.0);
		temp_I.resize(l,0.0);
		long_cell_axis2(cell_id);
	}

	if (USE_OLD_VERSION) { // old version, calculate everthing new
		const Cell::Nodes& nodes = CPM::getCell(cell_id).getNodes();         
		std::vector<double> &I   = _temp_I(cell_id);
		VDOUBLE center          =  CPM::getCell(cell_id).getUpdatedCenter();
		
		fill(I.begin(),I.end(),0);
		
		if(SIM::getLattice()->getDimensions()==3){  
			for (Cell::Nodes::const_iterator pt = nodes.begin(); pt != nodes.end(); pt++)
			{
				VDOUBLE delta = center - *pt;
				I[LC_XX] +=  sqr(delta.y) + sqr(delta.z);
				I[LC_YY] +=  sqr(delta.x) + sqr(delta.z);
				I[LC_ZZ] +=  sqr(delta.x) + sqr(delta.y);
				I[LC_XY] += -delta.x * delta.y;
				I[LC_XZ] += -delta.x * delta.z;
				I[LC_YZ] += -delta.y * delta.z;
			}
			double dir;
			VDOUBLE delta;
			if (todo == CPM::ADD) {
				delta = center - VDOUBLE(update.add_state.pos);
				dir=+1;
			}
			else if (todo == CPM::REMOVE) {
				delta = center - VDOUBLE(update.remove_state.pos); 
				dir=-1;
			}
			else dir = 0;
			I[LC_XX] +=  dir * (sqr(delta.y) + sqr(delta.z));
			I[LC_YY] +=  dir * (sqr(delta.x) + sqr(delta.z));
			I[LC_ZZ] +=  dir * (sqr(delta.x) + sqr(delta.y));
			I[LC_XY] += -dir * delta.x * delta.y;
			I[LC_XZ] += -dir * delta.x * delta.z;
			I[LC_YZ] += -dir * delta.y * delta.z;		  
			_temp_cell_length(cell_id) = calcLengthHelper3D(I,nodes.size()+dir);
		} 
		else if(SIM::getLattice()->getDimensions()==2){
			for (Cell::Nodes::const_iterator pt = nodes.begin(); pt != nodes.end(); pt++)
			{
				VDOUBLE delta = center - VDOUBLE(*pt);
				I[LC_XX] += sqr(delta.y);
				I[LC_YY] += sqr(delta.x);
				I[LC_XY] += -delta.x*delta.y;	   
			}		 	 
			double dir;
			VDOUBLE delta;
			if (todo == CPM::ADD) {
				delta = center - VDOUBLE(update.add_state.pos);
				dir=+1;
			}
			else if (todo == CPM::REMOVE) {
				delta = center - VDOUBLE(update.remove_state.pos); 
				dir=-1;
			} 
			else dir = 0;
			
			I[LC_XX] +=  dir * sqr(delta.y);
			I[LC_YY] +=  dir * sqr(delta.x);
			I[LC_XY] += -dir * delta.x * delta.y;	   
			_temp_cell_length(cell_id) = calcLengthHelper2D(I,nodes.size()+dir);
// 			cout << "cell_length = " << cell_length << endl;
		} else assert(0); 	
	}
	else {
		// incremental update
	
		VDOUBLE delta,deltacom;
		int dir;
		if (todo & CPM::ADD) dir=1;
		else if (todo & CPM::REMOVE) dir=-1;
		else dir=0;
		
		if (dir!=0)
		{
			const Cell::Nodes& nodes = CPM::getCell(cell_id).getNodes();
			VDOUBLE center = CPM::getCell(cell_id).getCenter();
			VDOUBLE updated_center = CPM::getCell(cell_id).getUpdatedCenter();
			
			if (dir==1){
				delta = updated_center - VDOUBLE(update.add_state.pos);
			} 
			else {
				delta = updated_center - VDOUBLE(update.remove_state.pos);
			}
			deltacom = center - updated_center;
// 			cout << " p  " << update.add_state.pos 
// 				 << " cu " << updated_center
// 				 << " c  " << center
// 				 << " dc " << deltacom
// 				 << " d  " << delta << endl;
			if(SIM::getLattice()->getDimensions()==3){
	// old Iij with respect to new center of mass	(Huygens-Steiner)
				temp_I[LC_XX] = I[LC_XX] + double(nodes.size())*(sqr(deltacom.y)+sqr(deltacom.z));
				temp_I[LC_YY] = I[LC_YY] + double(nodes.size())*(sqr(deltacom.x)+sqr(deltacom.z));
				temp_I[LC_ZZ] = I[LC_ZZ] + double(nodes.size())*(sqr(deltacom.x)+sqr(deltacom.y));
				temp_I[LC_XY] = I[LC_XY] - double(nodes.size())*deltacom.x*deltacom.y;
				temp_I[LC_XZ] = I[LC_XZ] - double(nodes.size())*deltacom.x*deltacom.z;
				temp_I[LC_YZ] = I[LC_YZ] - double(nodes.size())*deltacom.y*deltacom.z;
				// new Iij with respect to new com			
				temp_I[LC_XX] += double(dir)*(sqr(delta.y) + sqr(delta.z));
				temp_I[LC_YY] += double(dir)*(sqr(delta.x) + sqr(delta.z));
				temp_I[LC_ZZ] += double(dir)*(sqr(delta.x) + sqr(delta.y));
				temp_I[LC_XY] -= double(dir)*(delta.x*delta.y);
				temp_I[LC_XZ] -= double(dir)*(delta.x*delta.z);
				temp_I[LC_YZ] -= double(dir)*(delta.y*delta.z);
				_temp_cell_length(cell_id) = calcLengthHelper3D(_temp_I(cell_id),nodes.size()+dir) ;
			} 
			else if(SIM::getLattice()->getDimensions()==2){             
	// old Iij with respect to new center of mass	(Huygens-Steiner)		
	//                 for(int i=0;i<3;i++) printf("I[%i]=%f ",i,I[i]); 
				temp_I[LC_XX] = I[LC_XX] + double(nodes.size())*sqr(deltacom.y);
				temp_I[LC_YY] = I[LC_YY] + double(nodes.size())*sqr(deltacom.x);
				temp_I[LC_XY] = I[LC_XY] - double(nodes.size())*deltacom.x*deltacom.y;
		//		 for(int i=0;i<3;i++) printf("temp_I[%i]=%f ",i,temp_I[i]); 
		// new Iij with respect to new com			
				temp_I[LC_XX] += double(dir)*sqr(delta.y);
				temp_I[LC_YY] += double(dir)*sqr(delta.x);
				temp_I[LC_XY] -= double(dir)*(delta.x*delta.y);
				_temp_cell_length(cell_id) =   calcLengthHelper2D(_temp_I(cell_id),nodes.size()+dir) ;
			} else assert(0);
		} 
	}
}
 
void LengthConstraint::update_notify( CPM::CELL_ID cell_id, const CPM::UPDATE& update, CPM::UPDATE_TODO todo){
	assert(_I(cell_id).size()!=0);
// check difference between incremental and absolute calculation
	if (PARANOID_CHECK){ 
		 
		 std::vector<double> tI=_I(cell_id);
		 std::vector<double> ttempI=_temp_I(cell_id);
// 		 VDOUBLE tcenter = CPM::getCell(cell_id).getCenter();
 		 double tlength=_cell_length(cell_id);
		 
		 double doof=long_cell_axis2(cell_id);
		 
		 _I(cell_id)=tI;
//		 _temp_I(cell_id)=ttempI;
// 		 _center(cell_id)=(tcenter);
		 _cell_length(cell_id)=tlength;
		 
		 printf("enter update_:notify center, abs=%f, inc=%f, rel_error= %e\n",doof,_temp_cell_length(cell_id),
			(doof-_temp_cell_length(cell_id))/(doof+1.e-50));
		}      
		
 	if (_incrementalUpdatesLeft(cell_id)) { // we already did the calculation during test update
		_I(cell_id)=_temp_I(cell_id);	 
		_cell_length(cell_id)=_temp_cell_length(cell_id);
// 		_center(cell_id)=_temp_center(cell_id);
		_incrementalUpdatesLeft(cell_id)--;
	} 
	else {
		long_cell_axis2(cell_id);	 // do full calculation, updates everything
		_incrementalUpdatesLeft(cell_id)=_maxIncrementalUpdates(cell_id);
	}
// printf("update_notify: newlength=%f\n",_cell_length(cell_id));
}

double LengthConstraint::long_cell_axis2(CPM::CELL_ID id ) const {
//  cout << "per cell!!: LengthConstraint::long_cell_axis2( id) const \n";
	std::vector<double> &I=_I(id);
	assert( ! I.empty() );
	// set all values to zero
	fill(I.begin(),I.end(),0);
	VDOUBLE center = CPM::getCell(id).getCenter();
	
	const Cell::Nodes& nodes = CPM::getCell(id).getNodes();         
	if(SIM::getLattice()->getDimensions()==3){
		
		for (Cell::Nodes::const_iterator pt = nodes.begin(); pt != nodes.end(); pt++)
		{
			VDOUBLE delta = center - *pt;
			I[LC_XX]+=sqr(delta.y)+sqr(delta.z);
			I[LC_YY]+=sqr(delta.x)+sqr(delta.z);
			I[LC_ZZ]+=sqr(delta.x)+sqr(delta.y);
			I[LC_XY]+=-delta.x*delta.y;
			I[LC_XZ]+=-delta.x*delta.z;
			I[LC_YZ]+=-delta.y*delta.z;		
		}
		_cell_length(id) = calcLengthHelper3D(I,nodes.size());
// 		_center(id) = center;
	} 
	else if(SIM::getLattice()->getDimensions()==2){
		for (Cell::Nodes::const_iterator pt = nodes.begin(); pt != nodes.end(); pt++)
		{
			VDOUBLE delta = center - *pt;
			I[LC_XX] += sqr(delta.y);
			I[LC_YY] += sqr(delta.x);
			I[LC_XY] += -delta.x*delta.y;	   
		} 
		_cell_length(id) = calcLengthHelper2D(I,nodes.size());	 
// 		_center(id) = center;
	} else  { assert(0); exit(-1); }
	return _cell_length(id);
}


double LengthConstraint::calcLengthHelper3D(const std::vector<double> &I, int N) const
{
//  cout << "helper: LengthConstraint::calcLengthHelper3D(const std::vector<double> &I, int N)\n";
	if (N==0) return 0;
	if (N==1) return 1; // gives nan otherwise
//	eigenvalues of I (principal moments of inertia) are the
//	roots of characteristic polynome of Iij(cubic equation: http://www.hawaii.edu/suremath/jrootsCubic.html)
//	from principal moments we get the length by assuming the cell was an ellipsoid
	double a=-I[LC_XX] - I[LC_YY] - I[LC_ZZ],
	b=-I[LC_XY]*I[LC_XY] - I[LC_XZ]*I[LC_XZ] - I[LC_YZ]*I[LC_YZ] + I[LC_XX]*I[LC_YY] + I[LC_XX]*I[LC_ZZ] + I[LC_YY]*I[LC_ZZ],
	c=+I[LC_XZ]*I[LC_XZ]*I[LC_YY] + I[LC_XX]*I[LC_YZ]*I[LC_YZ] + I[LC_XY]*I[LC_XY]*I[LC_ZZ] - 2.*I[LC_XY]*I[LC_XZ]*I[LC_YZ] - I[LC_XX]*I[LC_YY]*I[LC_ZZ];
	double q=1./3.*b-1./9.*a*a,
	r=1./6.*(a*b-3.*c)-1./27.*a*a*a;
	std::complex<double> s1=pow(r+sqrt(std::complex<double>(q*q*q)+r*r),1./3.),
                                    s2=pow(r-sqrt(std::complex<double>(q*q*q)+r*r),1./3.);
	double lambda[3];
	lambda[0]=real((s1+s2)-a/3.);
	lambda[1]=real(-0.5*(s1+s2)-a/3.+std::complex<double>(0,0.5*sqrt(3))*(s1-s2));
	lambda[2]=real(-0.5*(s1+s2)-a/3.-std::complex<double>(0,0.5*sqrt(3))*(s1-s2));	
	for(int i=0;i<3;i++) lambda[i]=max(0.,lambda[i]);
	double A[3];
	A[0]=-lambda[0]+lambda[1]+lambda[2];
	A[1]=lambda[0]-lambda[1]+lambda[2];
	A[2]=lambda[0]+lambda[1]-lambda[2];	 
	for(int i=0;i<3;i++) A[i]=sqrt(max(0.,A[i]));	       	 
	for(int flag=1;flag;) { 
		flag=0; for(int i=0;i+1<3;i++)  // sort
		if (A[i]>A[i+1]) {swap(A[i],A[i+1]); swap(lambda[i],lambda[i+1]); flag=1;break;}
	}
	 // normalized axes - useful only for ratio
	 //for(int i=1;i<3;i++) A[i]/=A[0]; A[0]=1;
	 // double ratio=A[2]/(A[0]+1.e-10); 
	 // axes with correct size 
	 double f;
	 if (lambda[2]>0) f=sqrt(lambda[2]*5./(A[0]*A[0]+A[1]*A[1])/double(N)); 
	 else if (lambda[1]>0) f=sqrt(lambda[1]*5./(A[0]*A[0]+A[2]*A[2])/double(N)); 
	 else f=sqrt(lambda[0]*5./(A[1]*A[1]+A[2]*A[2])/double(N));
	 return A[2]*f;   
}

double LengthConstraint::calcLengthHelper2D(const std::vector<double> &I, int N) const
{
//   cout << "helper: LengthConstraint::calcLengthHelper2D(const std::vector<double> &I, int N)\n";
	if (N==0) return 0;
	if (N==1) return 1;
	double lambda_a = 0.5 * (I[LC_XX] + I[LC_YY]) + 0.5 * sqrt( sqr(I[LC_XX] - I[LC_YY]) + 4 * sqr(I[LC_XY]));
	return 4*sqrt(lambda_a/double(N));
}

