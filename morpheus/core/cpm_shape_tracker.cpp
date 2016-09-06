#include "cpm_shape_tracker.h"
#include "Eigen/Eigenvalues"
#include "membranemapper.h"
#include "simulation.h"

/*

 * CPMShapeTracker is the coordinating wrapper to keep current shape and updated shape in synch, according to the 
 * the updates set/applied
 * AdaptiveCPMShapeTracker is the tracker backend that just tracks what is requested from the frontend
 * it needs functionality to initialize a tracker and to apply minimal updates
 */ 

/**
 * We require the current Lattice in order to provide everything in orthogonal coordinates
 */


AdaptiveCPMShapeTracker::AdaptiveCPMShapeTracker(CPM::CELL_ID cell, const CPMShape::Nodes& cell_nodes)
	: cell_id(cell), _nodes(cell_nodes), lattice(SIM::lattice())
{
	boundary_scaling = CPMShape::BoundaryLengthScaling(CPMShape::boundaryNeighborhood);
	if (lattice.getDimensions() == 1)
		Ell_I.resize(1,0.0);
	else if (lattice.getDimensions() == 2)
		Ell_I.resize(3,0.0);
	else if (lattice.getDimensions() == 3)
		Ell_I.resize(6,0.0);
	
	reset();
}

void AdaptiveCPMShapeTracker::reset() {
	tracking.nodes = false;
	tracking.surface = false;
	tracking.interfaces = false;
	tracking.elliptic_approx = false;
	
	n_updates = 0;
	pending_update_op =0;
	
	last_interface_update = -1e9;
	last_surface_update = -1e9;
	last_ellipsoid_update = -1e9;
	last_ellipsoid_init = -1e9;
	last_spherical_update = -1e9;
	scaled_interfaces_valid = false;
	
	initNodes();

}

const CPMShape::Nodes& AdaptiveCPMShapeTracker::surfaceNodes() const
{
	if (last_surface_update == n_updates) {
		return surface_nodes;
	}
	else  if (n_updates - last_surface_update < 3 * (10 + _nodes.size()) ) { 
		tracking.surface = true;
	}
	
	initSurfaceNodes();
	return surface_nodes;
}

const map<CPM::CELL_ID,double>&  AdaptiveCPMShapeTracker::interfaces() const {
	if (last_interface_update!=n_updates) {
		if (n_updates - last_interface_update< 5*(10 + _nodes.size())) {
			tracking.interfaces=true;
// 			cout << "Switching on interface tracking for cell " << cell_id << " after " << n_updates - last_interface_update << " updates."<< endl;
		}
		initInterfaces();
	}
	if (! scaled_interfaces_valid) {
		scaled_interfaces.clear();
		for (const auto& i : _interfaces) {
			scaled_interfaces.insert( make_pair(i.first,double(i.second) / boundary_scaling) );
		}
	}
	
	return scaled_interfaces;
	
};

const EllipsoidShape& AdaptiveCPMShapeTracker::ellipsoidApprox() const {
	if (last_ellipsoid_update != n_updates) {
		if (n_updates - last_ellipsoid_update < 20 ) {
// 			cout << "Switching on ellipsoid tracking for cell " << cell_id << " after " << n_updates - last_ellipsoid_update << " updates."<< endl;
			tracking.elliptic_approx = true;
		}
		initEllipsoidShape();
	}
	return ellipsoid_approx;
}
	
const PDE_Layer& AdaptiveCPMShapeTracker::sphericalApprox() const { 
	if (n_updates == last_spherical_update)
		return spherical_mapper->getData();
	
	if (!spherical_mapper) {
		spherical_mapper = new MembraneMapper(MembraneMapper::MAP_CONTINUOUS);
	}

	// report radius for all surface nodes
// 	cout << "creating sphericalApprox around center " << center()  << endl;
	VDOUBLE c = center();
	spherical_mapper->attachToCenter(c);
	
	for (const auto& pt  : _nodes)
	{
		if (CPM::isSurface(pt)) {
			double distance = lattice.orth_distance(c, lattice.to_orth(pt)).abs();
			spherical_mapper->map(pt,distance);
		}
	}
	// interpolate and return
	spherical_mapper->fillGaps();
 	//spherical_mapper->flatten();
	last_spherical_update = n_updates;
	return spherical_mapper->getData();
} 

void AdaptiveCPMShapeTracker::reset(AdaptiveCPMShapeTracker* other) {
	
	if (other->cell_id != cell_id)
		assert(0);
// 	assert(other->n_updates<n_updates);
	
	node_sum = other->node_sum;
	node_count = other->node_count;
	_interface_length = other->_interface_length;
// 		long int current_updates = n_updates;
	n_updates = other->n_updates;
	pending_update_op = 0;
// 	pending_update_op = other->n_updates;
// 	pending_update_node = other->pending_update_node;
	
	if (last_interface_update != other->last_interface_update) {
		last_interface_update = other->last_interface_update;
		if (last_interface_update == n_updates ) {
			// copy cached interfaces
			bool brute_force_copy = false;
			if (other->_interfaces.size() == _interfaces.size()) {
				auto ui = other->_interfaces.begin();
				auto i = _interfaces.begin();
				for (; ui != other->_interfaces.end(); ++ui,++i) {
					if (ui->first == i->first) {
						i->second = ui->second;
						++i;
					}
					else {
						brute_force_copy = true;
						break;
					}
				}
			}
			else brute_force_copy = true;
			
			if (brute_force_copy) { 
				_interfaces = other->_interfaces;
// 				_interfaces.clear();
// 				map <CPM::CELL_ID, uint >::iterator i, ui;
// 				auto i = _interfaces.begin();
// 				for (auto ui = other->_interfaces.begin();  ui != other->_interfaces.end(); ui++) {
// 					i=interfaces.insert(i,*ui);
// 				}
			}
		}
	}
	
	if (last_ellipsoid_update != other->last_ellipsoid_update) {
		last_ellipsoid_update = other->last_ellipsoid_update;
		if ( last_ellipsoid_update == n_updates ) {
			ellipsoid_approx = other->ellipsoid_approx;
			Ell_I = other->Ell_I;
// 			assert(0);
		}
// 		else if (tracking.elliptic_approx) {
// 			initEllipsoidShape();
// 		}
	}
	
	if (last_spherical_update != other->last_spherical_update) {
		last_spherical_update = other->last_spherical_update;
		if ( last_spherical_update == n_updates ) {
			// copy cached sphericalApprox approx
			assert(0);
			this->spherical_mapper = other->spherical_mapper;
		}
	}
}


void AdaptiveCPMShapeTracker::apply(const CPM::Update& update) {
	/// The referred node container does not have the update applied yet !!!
	if (update.opAdd()) {
		pending_update_op = 1;
		pending_update_node = update.focusStateAfter().pos;
		addNode(update);
	}
	else if (update.opRemove())  {
		pending_update_op = -1;
		pending_update_node = update.focusStateBefore().pos;
		removeNode(update);
	}
	else if (update.opNeighborhood()) {
		pending_update_op =0;
		neighborNode(update);
	}
}

void AdaptiveCPMShapeTracker::apply(const CPM::Update& update, AdaptiveCPMShapeTracker* other)
{
	if (other->cell_id != cell_id)
		assert(0);
	
	if (! (other->n_updates>n_updates) )
		return apply(update); // In case just this change is not tracked in 'other'
	
	node_sum = other->node_sum;
	node_count = other->node_count;
	_interface_length = other->_interface_length;
// 		long int current_updates = n_updates;
	n_updates = other->n_updates;
	pending_update_op = 0;
// 	bool adding = update.focusStateAfter().cell_id == cell_id;
	
	if ( tracking.interfaces ) {
		if ( other->last_interface_update == n_updates) {
			/// copy from cached
			bool brute_force_copy = false;
			if (other->_interfaces.size() == _interfaces.size()) {
				auto ui = other->_interfaces.begin();
				auto i = _interfaces.begin();
				for (; ui != other->_interfaces.end(); ++ui,++i) {
					if (ui->first == i->first) {
						i->second = ui->second;
						++i;
					}
					else {
						brute_force_copy = true;
						break;
					}
				}
			}
			else brute_force_copy = true;
			if (brute_force_copy) { 
				_interfaces = other->_interfaces;
// 				_interfaces.clear();
// 				map <CPM::CELL_ID, uint >::iterator i, ui;
// 				auto i = _interfaces.begin();
// 				for (auto ui = other->_interfaces.begin();  ui != other->_interfaces.end(); ui++) {
// 					i=interfaces.insert(i,*ui);
// 				}
			}
		}
		else {
			/// appy tracking manually
			if (update.opAdd()) {
				for (const auto& stat : update.boundaryStencil()->getStatistics()) {
					if (stat.cell == cell_id) {
						if ((_interfaces[update.focusStateBefore().cell_id] -= stat.count) ==0 ) {
							_interfaces.erase(update.focusStateBefore().cell_id);
						}
// 						_interface_length -= stat.count;
					}
					else {
						_interfaces[stat.cell] += stat.count;
// 						_interface_length += stat.count;
					}
				}
			}
			else if (update.opRemove()){
				for (const auto& stat : update.boundaryStencil()->getStatistics()) {
					if (stat.cell == cell_id) {
						_interfaces[update.focusStateAfter().cell_id] += stat.count;
// 						_interface_length += stat.count;
					}
					else {
						if ( (_interfaces[stat.cell] -= stat.count) == 0) {
							_interfaces.erase(stat.cell);
						}
// 						_interface_length -= stat.count;
					}
				}
			}
			else if (update.opNeighborhood()) {
				for (const auto& stat : update.boundaryStencil()->getStatistics()) {
					if (stat.cell == cell_id) {
						if ( (_interfaces[update.focusStateBefore().cell_id] -= stat.count) == 0) {
							_interfaces.erase(update.focusStateBefore().cell_id);
						}
						_interfaces[update.focusStateAfter().cell_id] += stat.count;
					}
				}
			}
		}
		last_interface_update = n_updates;
		scaled_interfaces_valid = false;
	}
	
	if (other->last_ellipsoid_update == n_updates) {
		ellipsoid_approx = other->ellipsoid_approx;
		Ell_I = other->Ell_I;
		last_ellipsoid_update = n_updates;
	}
	else {
		if (tracking.elliptic_approx) {
			if (update.opAdd())
				trackEllipse(update.focusStateAfter().pos,1.0);
			else if (update.opRemove())
				trackEllipse(update.focusStateBefore().pos,-1.0);
			last_ellipsoid_update = n_updates;
		}
	}
	
	
	
// 	if (last_spherical_update != other->last_spherical_update) {
// 		last_spherical_update = other->last_spherical_update;
// 		if ( last_spherical_update == n_updates ) {
// 			// copy cached sphericalApprox approx
// 			assert(0);
// 			this->spherical_approx = other->spherical_approx;
// 		}
// 	}
	
}
	
void AdaptiveCPMShapeTracker::addNode(const CPM::Update& update) {
	n_updates++;
	node_sum += update.focusStateAfter().pos;
	node_count++;
	
	if (tracking.interfaces) {
		scaled_interfaces_valid = false;
		for (const auto& stat : update.boundaryStencil()->getStatistics()) {
			if (stat.cell == cell_id) {
				_interfaces[update.focusStateBefore().cell_id] -= stat.count;
				_interface_length -= stat.count;
			}
			else {
				_interfaces[stat.cell] += stat.count;
				_interface_length += stat.count;
			}
		}
		last_interface_update = n_updates;
	}
	else {
		// only track the total surface
		for (const auto& stat : update.boundaryStencil()->getStatistics()) {
			_interface_length += (stat.cell == cell_id) ? - double(stat.count) : double(stat.count);
			
		}
	}
	
	if (tracking.surface) {
		assert(0);
	}
	
	if (tracking.elliptic_approx) {
		trackEllipse(update.focusStateAfter().pos,+1);
		last_ellipsoid_update = n_updates;
	}
}
		
void AdaptiveCPMShapeTracker::removeNode(const CPM::Update& update) {
	
	n_updates++;
	node_sum -= update.focusStateBefore().pos;
	node_count--;
	
	if (tracking.interfaces) {
		scaled_interfaces_valid = false;
		for (const auto& stat : update.boundaryStencil()->getStatistics()) {
			if (stat.cell == cell_id) {
				_interfaces[update.focusStateAfter().cell_id] += stat.count;
				_interface_length += stat.count;
			}
			else {
				_interfaces[stat.cell] -= stat.count;
				_interface_length -= stat.count;
			}
		}
		last_interface_update = n_updates;
	}
	else {
		// only track the total surface
		for (const auto& stat : update.boundaryStencil()->getStatistics()) {
			_interface_length += (stat.cell == cell_id) ? double(stat.count) : - double(stat.count);
		}

	}
	
	if (tracking.surface) {
		assert(0);
	}
	
	
	if (tracking.elliptic_approx) {
		trackEllipse(update.focusStateBefore().pos,-1);
		last_ellipsoid_update = n_updates;
	}

}

void AdaptiveCPMShapeTracker::neighborNode(const CPM::Update& update) {
	n_updates++;
	
	if (tracking.interfaces) {
		scaled_interfaces_valid = false;
		for (const auto& stat : update.boundaryStencil()->getStatistics()) {
			if (stat.cell == cell_id) {
				_interfaces[update.focusStateBefore().cell_id] -= stat.count;
				_interfaces[update.focusStateAfter().cell_id] += stat.count;
			}
		}
		last_interface_update = n_updates;
	}
	if (tracking.elliptic_approx) {
		last_ellipsoid_update = n_updates;
	}
}

void AdaptiveCPMShapeTracker::trackEllipse(const VINT pos, int dir) {
	VDOUBLE delta,deltacom;
	
	if (n_updates - last_ellipsoid_init > 1e4 || (n_updates-last_ellipsoid_update) != 1)
		initEllipsoidShape();
	else  {
// 		cout << "TrackEllipse " << ellipsoid_approx.lengths[0];
		if (dir!=0)
		{
			double updated_node_count = node_count;
			VDOUBLE updated_center = lattice.to_orth(node_sum) / updated_node_count;
			double old_node_count = node_count - dir;
			VDOUBLE old_center = lattice.to_orth(node_sum - dir *pos) / old_node_count;
			
			deltacom = old_center - updated_center;
			delta = updated_center - lattice.to_orth(pos);
	// 			cout << " p  " << update.add_state.pos 
	// 				 << " cu " << updated_center
	// 				 << " c  " << center
	// 				 << " dc " << deltacom
	// 				 << " d  " << delta << endl;
			if (lattice.getDimensions()==3){
	// old Iij with respect to new center of mass	(Huygens-Steiner)
				Ell_I[Ell_XX] += double(old_node_count)*(sqr(deltacom.y)+sqr(deltacom.z));
				Ell_I[Ell_YY] += double(old_node_count)*(sqr(deltacom.x)+sqr(deltacom.z));
				Ell_I[Ell_ZZ] += double(old_node_count)*(sqr(deltacom.x)+sqr(deltacom.y));
				Ell_I[Ell_XY] -= double(old_node_count)*deltacom.x*deltacom.y;
				Ell_I[Ell_XZ] -= double(old_node_count)*deltacom.x*deltacom.z;
				Ell_I[Ell_YZ] -= double(old_node_count)*deltacom.y*deltacom.z;
				// new Iij with respect to new com			
				Ell_I[Ell_XX] += double(dir)*(sqr(delta.y) + sqr(delta.z));
				Ell_I[Ell_YY] += double(dir)*(sqr(delta.x) + sqr(delta.z));
				Ell_I[Ell_ZZ] += double(dir)*(sqr(delta.x) + sqr(delta.y));
				Ell_I[Ell_XY] -= double(dir)*(delta.x*delta.y);
				Ell_I[Ell_XZ] -= double(dir)*(delta.x*delta.z);
				Ell_I[Ell_YZ] -= double(dir)*(delta.y*delta.z);
				ellipsoid_approx = computeEllipsoid3D(Ell_I, updated_node_count) ;
			}                                                    
			else if(lattice.getDimensions()==2){            
	// old Iij with respect to new center of mass	(Huygens-Steiner)		
	//                 for(int i=0;i<3;i++) printf("I[%i]=%f ",i,I[i]); 
				Ell_I[Ell_XX] += double(old_node_count)*sqr(deltacom.y);
				Ell_I[Ell_YY] += double(old_node_count)*sqr(deltacom.x);
				Ell_I[Ell_XY] -= double(old_node_count)*deltacom.x*deltacom.y;
		//		 for(int i=0;i<3;i++) printf("temp_I[%i]=%f ",i,temp_I[i]); 
		// new Iij with respect to new com			
				Ell_I[Ell_XX] += double(dir)*sqr(delta.y);
				Ell_I[Ell_YY] += double(dir)*sqr(delta.x);
				Ell_I[Ell_XY] -= double(dir)*(delta.x*delta.y);
				ellipsoid_approx =  computeEllipsoid2D(Ell_I, updated_node_count) ;
			} else assert(0);
// 			cout << " -> " << ellipsoid_approx.lengths[0] << " n0 " << old_center <<  " n1 " << updated_center << endl;
		}
	}
	
}
	
	
EllipsoidShape AdaptiveCPMShapeTracker::computeEllipsoid3D(const valarray<double> &I, int N) {
	EllipsoidShape es;
//  cout << "helper: LengthConstraint::calcLengthHelper3D(const std::vector<double> &I, int N)\n";
	if(N<=1) {
		for (uint i=0; i<3; i++) {
			es.lengths.push_back(N);
			es.axes.push_back(VDOUBLE(0,0,0));
 		}
		return es;
	} // gives nan otherwise
	
	// From of the inertia tensor (principal moments of inertia) we compute the eigenvalues and
	// obtain the cell length by assuming the cell was an ellipsoid
	Eigen::Matrix3f eigen_m;
	eigen_m << I[Ell_XX], I[Ell_XY], I[Ell_XZ],
	           I[Ell_XY], I[Ell_YY], I[Ell_YZ],
	           I[Ell_XZ], I[Ell_YZ], I[Ell_ZZ];
	Eigen::SelfAdjointEigenSolver<Eigen::Matrix3f> eigensolver(eigen_m);
	if (eigensolver.info() != Eigen::Success) {
		cerr << "Cell::calcLengthHelper3D: Computing eigenvalues was not successfull!" << endl;
	}

	Eigen::Vector3f eigen_values = eigensolver.eigenvalues();
	Eigen::Matrix3f EV = eigensolver.eigenvectors();
	Eigen::Matrix3f Am;
	Am << -1,  1,  1,
	       1, -1,  1,
	       1,  1, -1;
	Eigen::Array3f axis_lengths = ((Am * eigen_values).array() * (2.5/double(N))).sqrt();
	Eigen::Vector3i sorted_indices;
	for (uint i=0; i<3; i++) {
		es.lengths.push_back(axis_lengths(i));
		es.axes.push_back(VDOUBLE(EV(0,i),EV(1,i),EV(2,i)).norm());
	}
	// sorting axes by length
	bool done=false;
	while (!done) {
		for (uint i=0;1;i++) {
			if (es.lengths[i] < es.lengths[i+1]) {
				swap(es.lengths[i],es.lengths[i+1]);
				swap(es.axes[i],es.axes[i+1]);
			}
			if (i==2) {
				done=true;
				break;
			}
		}
	} 
	 // axes with correct size 
// 	double f[3];
// 	if (lambda[2]>0) 
// 		 f[2]=sqrt(lambda[2]*5./(A[0]*A[0]+A[1]*A[1])/double(N)); 
// 	else if (lambda[1]>0) 
// 		 f[1]=sqrt(lambda[1]*5./(A[0]*A[0]+A[2]*A[2])/double(N)); 
// 	else 
// 		 f[0]=sqrt(lambda[0]*5./(A[1]*A[1]+A[2]*A[2])/double(N));
// 	 //double l = A[2]*f; 
// 	 
// 	// NOTE: test whether f's are correct
// 	es.lengths.push_back( A[2]*f[2] );
// 	es.lengths.push_back( A[1]*f[1] );
// 	es.lengths.push_back( A[0]*f[0] );

	return es;
};
	
EllipsoidShape AdaptiveCPMShapeTracker::computeEllipsoid2D(const valarray<double> &I, int N) {
	EllipsoidShape es;
//	cout << "helper: LengthConstraint::calcLengthHelper2D(const std::vector<double> &I, int N)\n";
	
	if (N<2){
		if (N==1) {
			es.lengths.push_back( 1 );
			es.lengths.push_back( 1 );
			es.axes.push_back(VDOUBLE());
			es.axes.push_back(VDOUBLE());
			es.eccentricity = 0;
			return es;
		}
		else {
			es.lengths.push_back( 0 );
			es.lengths.push_back( 0 );
			es.axes.push_back(VDOUBLE());
			es.axes.push_back(VDOUBLE());
			es.eccentricity = 0;
			return es;
		}
		
	}
	
	// long axis
	double lambda_b = 0.5 * (I[Ell_XX] + I[Ell_YY]) + 0.5 * sqrt( sqr(I[Ell_XX] - I[Ell_YY]) + 4 * sqr(I[Ell_XY]));
    double lambda_a = 0.5 * (I[Ell_XX] + I[Ell_YY]) - 0.5 * sqrt( sqr(I[Ell_XX] - I[Ell_YY]) + 4 * sqr(I[Ell_XY]));
    
	// TODO: is this the radius or diameter ?
	VDOUBLE major_axis = VDOUBLE(I[Ell_XY], lambda_a-I[Ell_XX], 0);
	double major_length = 4*sqrt(lambda_b/double(N));

	// short axis
	
	// TODO: is this the radius or diameter?
	VDOUBLE minor_axis = VDOUBLE(I[Ell_XY], lambda_b-I[Ell_XX], 0);
	double minor_length = 4*sqrt(lambda_a/double(N));
	
	double eccentricity = sqrt( 1 - (lambda_a / lambda_b) );
	
    es.axes.push_back( major_axis );
    es.axes.push_back( minor_axis );
	
	es.lengths.push_back( major_length );
	es.lengths.push_back( minor_length );
	es.eccentricity = eccentricity;

	return es;
}

void AdaptiveCPMShapeTracker::initNodes() const {
	node_sum = VINT(0,0,0);
	node_count = _nodes.size();
	_interface_length=0;
	if (_nodes.size()>0) {
		StatisticalLatticeStencil boundaryStencil(CPM::getLayer(),CPMShape::boundaryNeighborhood.neighbors());
		for (auto node : _nodes) {
			node_sum += node;
			lattice.resolve(node);
			boundaryStencil.setPosition(node);
			for (const auto& stat : boundaryStencil.getStatistics()) {
				if (stat.cell != cell_id) {
					_interface_length += stat.count;
				}
			}
		}
	}
}

void AdaptiveCPMShapeTracker::initInterfaces() const {
	_interfaces.clear();
	_interface_length=0;
	if (node_count>0) {
		StatisticalLatticeStencil boundaryStencil(CPM::getLayer(),CPMShape::boundaryNeighborhood.neighbors());
		for (auto node : _nodes) {
			lattice.resolve(node);
			boundaryStencil.setPosition(node);
			for (const auto& stat : boundaryStencil.getStatistics()) {
				if (stat.cell != cell_id) {
					_interfaces[stat.cell] += stat.count;
					_interface_length += stat.count;
				}
			}
		}
		if (pending_update_op) {
			auto node = pending_update_node;
			lattice.resolve(node);
			boundaryStencil.setPosition(node);
			for (const auto& stat : boundaryStencil.getStatistics()) {
				if (stat.cell != cell_id) {
					_interfaces[stat.cell] += pending_update_op * stat.count;
					_interface_length += pending_update_op * stat.count;
				}
			}
		}
	}
	last_interface_update = n_updates;
	scaled_interfaces_valid = false;
}

void AdaptiveCPMShapeTracker::initSurfaceNodes() const {
	surface_nodes.clear();
	for (const auto& node : _nodes) {
		if (pending_update_op == -1 && pending_update_node == node) 
			continue;
		if (CPM::isSurface(node)) {
			surface_nodes.insert(node);
		}
	}
	if (pending_update_op == +1 && CPM::isSurface(pending_update_node)) {
		surface_nodes.insert(pending_update_node);
	}
	last_surface_update = n_updates;
}

void AdaptiveCPMShapeTracker::initEllipsoidShape() const {
	Ell_I = 0;
	
	uint num_nodes = this->node_count;
	VINT nodes_sum = this->node_sum;
	VDOUBLE c = lattice.to_orth(nodes_sum) / num_nodes;

	if (lattice.getDimensions()==2){
		Ell_I[Ell_XX] = 0;
		Ell_I[Ell_YY] = 0;
		Ell_I[Ell_XY] = 0;
		for (const auto& node : _nodes)
		{
			VDOUBLE delta = c - lattice.to_orth(node);
			Ell_I[Ell_XX] += sqr(delta.y);
			Ell_I[Ell_YY] += sqr(delta.x);
			Ell_I[Ell_XY] += -delta.x*delta.y;  
		}
		// Just add the node which is missing in the nodes container
		if (pending_update_op!=0) {
			VDOUBLE delta = c - lattice.to_orth(pending_update_node);
			Ell_I[Ell_XX] += pending_update_op * sqr(delta.y);
			Ell_I[Ell_YY] += pending_update_op * sqr(delta.x);
			Ell_I[Ell_XY] += - pending_update_op * delta.x*delta.y;  
		}
		ellipsoid_approx = computeEllipsoid2D(Ell_I,num_nodes);
	} 
	else if (lattice.getDimensions()==3){
		for (const auto& node : _nodes)
		{
			VDOUBLE delta = c - lattice.to_orth(node);
			Ell_I[Ell_XX]+=  sqr(delta.y) + sqr(delta.z);
			Ell_I[Ell_YY]+=  sqr(delta.x) + sqr(delta.z);
			Ell_I[Ell_ZZ]+=  sqr(delta.x) + sqr(delta.y);
			Ell_I[Ell_XY]+= -delta.x*delta.y;
			Ell_I[Ell_XZ]+= -delta.x*delta.z;
			Ell_I[Ell_YZ]+= -delta.y*delta.z;
		}
		// Just add the node which is missing in the nodes container
		if (pending_update_op!=0) {
			VDOUBLE delta = c - lattice.to_orth(pending_update_node);
			Ell_I[Ell_XX]+= pending_update_op * sqr(delta.y)+sqr(delta.z);
			Ell_I[Ell_YY]+= pending_update_op * sqr(delta.x)+sqr(delta.z);
			Ell_I[Ell_ZZ]+= pending_update_op * sqr(delta.x)+sqr(delta.y);
			Ell_I[Ell_XY]+= - pending_update_op * delta.x*delta.y;
			Ell_I[Ell_XZ]+= - pending_update_op * delta.x*delta.z;
			Ell_I[Ell_YZ]+= - pending_update_op * delta.y*delta.z; 
		}
		ellipsoid_approx = computeEllipsoid3D(Ell_I,num_nodes);
	} 
	else assert(0);
	
	ellipsoid_approx.center = c;
	ellipsoid_approx.volume = num_nodes;
	
	last_ellipsoid_update = n_updates;
	last_ellipsoid_init = n_updates;
}


CPMShapeTracker::CPMShapeTracker(CPM::CELL_ID cell_id, const CPMShape::Nodes& cell_nodes)
: updated_shape(cell_id, cell_nodes), current_shape(cell_id, cell_nodes) {
	updated_is_current = true;
};

void CPMShapeTracker::setUpdate(const CPM::Update& update) { 
	if (update.opAdd() && update.opRemove()) return;
	
	if (!updated_is_current) updated_shape.reset(&current_shape); 
	
	updated_shape.apply(update);
	updated_is_current = false;
};

void CPMShapeTracker::reset()
{
	current_shape.reset();
	updated_shape.reset();
}


void CPMShapeTracker::applyUpdate(const CPM::Update& update)
{
	if (update.opAdd() && update.opRemove()) return;
	
	if (update.opNeighborhood()) {
		// Neighborhood updates are not tracked in updated_shape so far
		current_shape.apply(update);
		updated_is_current = false;
	}
	else {
		current_shape.apply(update, &updated_shape);
		updated_is_current = true;
	}
}

/*
class NewCell {
public:
	VINT lattPosition();
	VDOUBLE position();
	bool devide(pair<CellID,CellID>& new_cells);

	void setUpdate(const CELL_UPDATE);
	void applyUpdate(CELL_UPDATE);
	
	SpatialType spatialtype(); // returns CPM, POINT, (LGCA)?
	const AdaptiveCPMShapeTracker& cpm_shape() { return shapeTracker.current(); }
	const AdaptiveCPMShapeTracker& updated_cpm_shape() { return shapeTracker.updated(); }
private:
	vector<AbstractProperty> properties;  // --> could we get the membranes in here too ?
	vector<CellFields> fields;
	vector<CellMembrane> membranes;
	set<VINT> nodes;
	CPMShapeTracker shapeTracker;
};*/
