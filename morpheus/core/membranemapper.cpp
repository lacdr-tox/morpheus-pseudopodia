#include "membranemapper.h"
#include "cell.h"

const double MembraneMapper::no_distance = 999999;

MembraneMapper::MembraneMapper( MembraneMapper::MODE mode, bool use_cell_shape ) : mapping_mode(mode), use_shape(use_cell_shape)
{
	valid_cell = false;
	cell_id = CPM::NO_CELL;
	cell_shape = NULL;
	membrane_lattice = MembraneProperty::lattice();
	global_lattice = SIM::getLattice();

	const bool spherical = true;
	data_map = shared_ptr<PDE_Layer>(new PDE_Layer(membrane_lattice,true));
	accum_map = shared_ptr<PDE_Layer>(new PDE_Layer(membrane_lattice,true));
	distance_map  = shared_ptr<PDE_Layer>(new PDE_Layer(membrane_lattice,true));
	distance_map->set_boundary_value(Boundary::my,no_distance);
	distance_map->set_boundary_value(Boundary::py,no_distance);
// 	if (mode == MAP_DISTANCE_TRANSFORM)
// 		shape_mapper = shared_ptr<MembraneMapper>(new MembraneMapper(MAP_CONTINUOUS));
	
}

void MembraneMapper::attachToCell ( CPM::CELL_ID cell_id )
{
	cell_center = CPM::getCell(cell_id).getCenter();
	this->cell_id = cell_id;
	valid_cell = true;
	data_map->data = 0;
	accum_map->data = 0;
	distance_map->data = no_distance;

	if (use_shape)
		cell_shape = & (CPM::getCell(cell_id).currentShape().sphericalApprox());
// 	if (mapping_mode == MAP_DISTANCE_TRANSFORM)
// 		shape_mapper->attachToCell(cell_id);
}

void MembraneMapper::attachToCenter ( VDOUBLE center )
{
	cell_id = CPM::NO_CELL;
	valid_cell = true;
	cell_center = center;

	data_map->data = 0;
	accum_map->data = 0;
	distance_map->data = no_distance;
}


void MembraneMapper::map (const VINT& pos, double v)
{
	assert(valid_cell);
	map(global_lattice->to_orth(pos),v);
}

void MembraneMapper::map ( const VDOUBLE& pos, double v)
{
	VINT membrane_pos = getMembranePosition( pos );
	set(membrane_pos,v);
}

VINT MembraneMapper::getMembranePosition( const VDOUBLE& pos_3D){
	assert(valid_cell);

	VDOUBLE orientation = global_lattice->orth_distance(pos_3D, cell_center);
	
	if( rotation_matrices.size() > 0 ){
		for(uint i=0; i<rotation_matrices.size(); i++){
			//cout << "Rotation " << i << ", orientation: " <<  orientation << "\n";
			Eigen::Vector3f vector;
			vector << orientation.x, orientation.y, orientation.z;
			vector = rotation_matrices[i] * vector;
			orientation = VDOUBLE( vector(0), vector(1), vector(2) );
		}
		//cout << "Rotation end, orientation: " <<  orientation << "\n";
	} 
	
	VDOUBLE radial;
	if (membrane_lattice->getDimensions()==1) {
		radial.x = orientation.angle_xy() * double(data_map->l_size.x) / ( 2.0*M_PI );
		radial.x = MOD(radial.x, double(data_map->l_size.x));
	}
	else {
		radial = orientation.to_radial();
		radial.x *= double(data_map->l_size.x) / (2.0*M_PI);
		radial.y *= double(data_map->l_size.y) / M_PI;
		radial.x = MOD(radial.x, double(data_map->l_size.x));
		radial.y = MOD(radial.y, double(data_map->l_size.y));
		radial.z = 0;
	}
	
	VINT membrane_pos(radial);
	return membrane_pos;
}

void MembraneMapper::setRotationMatrix( Eigen::Matrix3f rot_matrix ){
	rotation_matrices.push_back(rot_matrix);
}

void MembraneMapper::resetRotationMatrices( void ){
	rotation_matrices.clear();
}

void MembraneMapper::set ( const VINT& membrane_pos, double v)
{
	assert( membrane_pos.x >= 0); 
	assert( membrane_pos.y >= 0 ); 
	assert( membrane_pos.x < data_map->l_size.x );
	assert( membrane_pos.y < data_map->l_size.y ); 
	uint idx = data_map->get_data_index(membrane_pos);
	if (mapping_mode == MAP_DISCRETE) {
		data_map->data[idx]  = v;
		accum_map->data[idx] = 1;
	}
	else {
		data_map->data[idx]  += v;
		accum_map->data[idx] += 1;
	}
	distance_map->data[idx] = 0;
	
}

void MembraneMapper::fillGaps()
{
	valarray<double>& ii_data = data_map->data;
	valarray<double>& ii_distance = distance_map->data;
	valarray<double>& ii_accum = accum_map->data;

	// Normalize the information in the lattice ...
	for (uint i=0; i<data_map->shadow_size_size_xyz; i++ ) {
		if ( (ii_accum[i] != 0.0) ) {
			if (mapping_mode == MAP_BOOLEAN || mapping_mode == MAP_DISTANCE_TRANSFORM) {
				if (ii_data[i] / ii_accum[i] != 0.5)
					ii_data[i] = ii_data[i] / ii_accum[i] > 0.5;
				else
					ii_data[i] =  getRandomBool();
			}
			else {
				ii_data[i] /= ii_accum[i];
			}
			ii_accum[i] = 1.0;
		}
	}
	data_map->reset_boundaries();
	distance_map->reset_boundaries();

	if (membrane_lattice->getDimensions() == 1) {
		// Spread the information ...
		bool done = false;
		bool fwd = true;
		int iterations = 0;
		while (! done ) {
			done = true;
			iterations++;
			int length = data_map->l_size.x;
			VINT start = fwd ? VINT(0,0,0) : VINT(length-1, 0,0);
			int iter = fwd ? 1 : -1;

			int idx =  data_map->get_data_index(start);
			for ( int step=0; step < length; step++, idx+=iter ) {

				if ((ii_distance[idx-1] + 1 < ii_distance[idx]) || (ii_distance[idx+1] + 1 < ii_distance[idx])) {
					if (ii_distance[idx-1] < ii_distance[idx+1]) {
						ii_data[idx] = ii_data[idx-1];
						ii_distance[idx] = ii_distance[idx-1] + 1;
					}
					else if (ii_distance[idx-1] > ii_distance[idx+1]) {
						ii_data[idx] = ii_data[idx+1];
						ii_distance[idx] = ii_distance[idx+1] + 1;
					}
					else {
						if (mapping_mode == MAP_DISCRETE) {
							ii_data[idx] = getRandomBool() ? ii_data[idx-1] : ii_data[idx+1];
						}
						else if (mapping_mode == MAP_BOOLEAN || mapping_mode == MAP_DISTANCE_TRANSFORM ) {
							if (ii_data[idx-1] + ii_data[idx+1] == 1.0)
								ii_data[idx] = getRandomBool();
							else
								ii_data[idx] = (ii_data[idx-1] + ii_data[idx+1]) > 1.0;
						}
						else {
							ii_data[idx] = 0.5 * (ii_data[idx-1] + ii_data[idx+1]);
						}
						ii_distance[idx] = ii_distance[idx+1] + 1;
					}
					done = false;
				}
			}
			data_map->reset_boundaries();
			distance_map->reset_boundaries();
			fwd = ! fwd;
		}
	}
	else if (membrane_lattice->getDimensions() == 2) {
		bool done = false;
		bool fwd = true;
		int iterations=0;
		vector<VINT> neighbors = membrane_lattice->getNeighborhoodByOrder(2).neighbors();
		vector<int> neighbor_offsets(neighbors.size());
		vector<double> neighbor_distance(neighbors.size());
		for (uint i=0; i< neighbors.size(); i++ ) {
			neighbor_offsets[i] = neighbors[i] * data_map->shadow_offset;
		}
		while (! done ) {
			done = true;
			iterations++;

			VINT length(membrane_lattice->size());
			VINT pos( fwd ? 0 :  length.x-1,  fwd ? 0 : length.y-1, 0);
			VINT iter( fwd ? +1 : -1, fwd ? +1 : -1, 0);
			for ( int y_step = 0; y_step<length.y; y_step++, pos.y+=iter.y) {

				VDOUBLE node_dist(1.0,1.0,0);
				if (y_step==0 || y_step==length.y-1)
					node_dist.x=0;  // set x_distance of polar volume elements to zero
				else {
					double theta_y = (double(y_step)+0.5) / double(length.y) * M_PI;
					node_dist.x = sin(theta_y);
				}
				// precompute the distance of the neighbors within the spherical lattice
				for (uint i=0; i<neighbors.size(); i++) {
					neighbor_distance[i] = sqrt( sqr(neighbors[i].x * node_dist.x) + sqr(neighbors[i].y * node_dist.y) );
				}

				int idx = data_map->get_data_index(pos);
				for ( int x_steps=0; x_steps < length.x; x_steps++, idx += iter.x) {

					double min_dist = ii_distance[idx];
					double acc_val=0;
					double acc=0;

					for (uint i=0; i<neighbors.size(); i++) {
						double dist = ii_distance[idx+neighbor_offsets[i]] + neighbor_distance[i];
						if (dist <= min_dist) {
							if (dist == min_dist) {
								acc++;
								acc_val += ii_data[idx+neighbor_offsets[i]];
							}
							else {
								acc = 1;
								acc_val = ii_data[idx+neighbor_offsets[i]];
								min_dist = dist;
							}

						}
					}

					if (min_dist - ii_distance[idx] < -10e-6) {
						if (mapping_mode == MAP_DISCRETE) {
							if (acc>1) {
								vector<double> buffer;
								for (uint i=0; i<neighbors.size(); i++) {
									if (min_dist == ii_distance[idx+neighbor_offsets[i]] + neighbor_distance[i]) {
										buffer.push_back(ii_data[idx+neighbor_offsets[i]]);
										acc +=1;
									}
								}
								ii_data[idx] = buffer[getRandomUint(buffer.size()-1)];
							}
							else
								ii_data[idx] = acc_val;
						}
						else if (mapping_mode == MAP_BOOLEAN || mapping_mode == MAP_DISTANCE_TRANSFORM ) {
							if (acc_val / acc == 0.5)
								ii_data[idx] = getRandomBool();
							else
								ii_data[idx] = (acc_val/acc > 0.5);
						}
						else { // mapping_mode == MAP_CONTINUOUS
								ii_data[idx] = acc_val / acc;
						}
						ii_distance[idx] = min_dist;

						done = false;
					}
				}
			}
			fwd = ! fwd;
			data_map->reset_boundaries();
			distance_map->reset_boundaries();
		}
// 		cout << iterations << " | " << "\t";
	}

	if (mapping_mode == MAP_DISTANCE_TRANSFORM) {
		ComputeDistance();
	}
}


void MembraneMapper::ComputeDistance()
{

	valarray<double>& ii_data = data_map->data;
	valarray<double>& ii_distance = distance_map->data;
	valarray<double>& ii_accum = accum_map->data;

// 	cout  << "ComputeDistance: sum " << ii_data.sum() << "\n";
	const valarray<double>& ii_shape = cell_shape->data;
	
	double cell_spherical_radius = pow( CPM::getCell( cell_id ).nNodes() / ((4.0/3.0) * M_PI) , 1.0/3.0); // used to scale unit sphere to cell volume
	
	// Reset distances for no contact nodes and set 0 distance for contact nodes after imputation ...
	for (uint i=0; i<distance_map->shadow_size_size_xyz; i++ ) {
		if ( ii_data[i] == 0.0 )
			ii_distance[i] = no_distance;
		else{
			ii_distance[i] = 0.0;
		}
	}
	
	distance_map->reset_boundaries();

	if (membrane_lattice->getDimensions() == 1) {
		bool done = false;
		bool fwd = true;
		int iterations = 0;
		while (! done ) {
			done = true;
			iterations++;
			int length = membrane_lattice->size().x;
			VINT start = fwd ? VINT(0,0,0) : VINT(length-1, 0,0);
			int iter = fwd ? 1 : -1;

			int idx = distance_map->get_data_index(start);
			for ( int step=0; step < length; step++, idx+=iter ) {
				double d_left = ii_distance[idx-1] + sqrt(sqr(1 / membrane_lattice->size().x * min(ii_shape[idx], ii_shape[idx-1]))  + sqr(ii_shape[idx]-ii_shape[idx-1]) );
				double d_right = ii_distance[idx+1] + sqrt(sqr(1 / membrane_lattice->size().x  * min(ii_shape[idx], ii_shape[idx+1]))+ sqr(ii_shape[idx]-ii_shape[idx+1]) );
				if (( d_left < ii_distance[idx]) || (d_right < ii_distance[idx])) {
					if (ii_distance[idx-1] < ii_distance[idx+1]) {
						ii_distance[idx] = d_left;
					}
					else {
						ii_distance[idx] = d_right;
					}
					done = false;
				}
			}
			distance_map->reset_boundaries();
			fwd = ! fwd;
		}
	}
	else if (membrane_lattice->getDimensions() == 2) {
		bool done = false;
// 		bool fwd = true;
		uint dir = 0;
		int iterations=0;
		vector<VINT> neighbors = membrane_lattice->getNeighborhoodByOrder(2).neighbors();
		vector<int> neighbor_offsets(neighbors.size());
		vector<double> neighbor_distance(neighbors.size());
		for (uint i=0; i< neighbors.size(); i++ ) {
			neighbor_offsets[i] = neighbors[i] * distance_map->shadow_offset;
		}
		while (! done ) {
			done = true;
			iterations++;
			bool x_fwd = (dir == 0 || dir == 3);
			bool y_fwd = (dir == 0 || dir == 2);
			VINT length(membrane_lattice->size());
			VINT pos( x_fwd ? 0 :  length.x-1,  y_fwd ? 0 : length.y-1, 0);
			VINT iter( x_fwd ? +1 : -1, y_fwd ? +1 : -1, 0);

			for ( int y_step = 0; y_step<length.y; y_step++, pos.y+=iter.y) {

				// precompute the node distance in a unit sphere
				VDOUBLE node_dist(1.0,1.0,0);
				if (pos.y==0 || pos.y==length.y-1)
					node_dist.x=0;  // set x_distance of polar volume elements to zero
				else {
					double theta_y = (double(pos.y)+0.5) / double(length.y) * M_PI;
					node_dist.x = sin(theta_y);
				}
				// precompute the distance of the neighbors within the spherical lattice
				for (uint i=0; i<neighbors.size(); i++) {
					neighbor_distance[i] = sqrt( sqr(neighbors[i].x * node_dist.x) + sqr(neighbors[i].y * node_dist.y) ) * (2.0 * M_PI / double(membrane_lattice->size().x)) ;
				}

				int idx = distance_map->get_data_index(pos);
				for ( int x_steps=0; x_steps < length.x; x_steps++ , idx+=iter.x ) {
					double min_dist = ii_distance[idx];
					for (uint i=0; i<neighbors.size(); i++) {
						double dist = ii_distance[idx+neighbor_offsets[i]];
						if( dist == no_distance)
						  continue;
						
						dist += (use_shape ? 
							  sqrt( sqr(neighbor_distance[i] * min(ii_shape[idx], ii_shape[idx+neighbor_offsets[i]]) ) + sqr(ii_shape[idx] - ii_shape[idx+neighbor_offsets[i]])) 
							  : neighbor_distance[i] * cell_spherical_radius );
						if (dist < min_dist) {
							min_dist = dist;
						}
					}

					if (min_dist - ii_distance[idx] < -10e-6) {
						ii_distance[idx] = min_dist;
						done = false;
					}
				}
			}
// 			if(cell_id == 39183 || cell_id == 33664){
// 				ofstream d;
// 				d.open(string("distance_") + to_str(iterations) + ".log", ios_base::trunc);
// 				distance_map->write_ascii(d);
// 				d.close();
// 			}
// 			fwd = ! fwd;
			dir = (dir + 1) % 4;
// 				distance_map;
// 			cout << endl << endl;
			distance_map->reset_boundaries();
// 			if (iterations == 8) break;
		}
// 		cout << iterations << " | " << "\t";

		
	}

	
}


void MembraneMapper::flatten()
{
 	if (mapping_mode == MAP_CONTINUOUS) {
		data_map->setDiffusionRate(0.1);
		data_map->updateNodeLength(1);
 		data_map->doDiffusion(1.0);
 	}
 	cout << "MembraneMapper::flatten " << endl;
}


const PDE_Layer& MembraneMapper::getData() {
	if (mapping_mode == MAP_DISTANCE_TRANSFORM)
		return getDistance();
	return *data_map.get();
}

void MembraneMapper::copyData(PDE_Layer* membrane) {
	assert(valid_cell);
	assert(membrane->data.size() == data_map->data.size());
	if (mapping_mode == MAP_DISTANCE_TRANSFORM)
		copyDistance(membrane);
	else
		membrane->data = data_map->data;
}

const PDE_Layer& MembraneMapper::getAccum()
{
	return *accum_map.get();
}


void MembraneMapper::copyAccum(PDE_Layer* membrane) {
	assert(valid_cell);
	assert(membrane->data.size() == accum_map->data.size());
// 	if (mapping_mode == MAP_DISTANCE_TRANSFORM)
// 		shape_mapper->copyData(membrane);
// 	else
	membrane->data = accum_map->data;
}

const PDE_Layer& MembraneMapper::getDistance()
{
	return *distance_map.get();
}


void MembraneMapper::copyDistance(PDE_Layer* membrane) {
	assert(valid_cell);
	assert(membrane->data.size() == distance_map->data.size());
	if( distance_map->data.min() == 999999)
		membrane->data = 0.0;
	else
		membrane->data = distance_map->data;
}


