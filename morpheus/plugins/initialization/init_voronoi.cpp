#include "init_voronoi.h"

REGISTER_PLUGIN(InitVoronoi);

const float InitVoronoi::no_distance = 999999;
const float InitVoronoi::no_label    = 999999;

vector<CPM::CELL_ID> InitVoronoi::run(CellType* celltype)
{
	shared_ptr<const Lattice> lattice = SIM::getLattice();
	shared_ptr<const CPM::LAYER> cpm = CPM::getLayer();

	neighbors = lattice->getNeighborhoodByOrder( 5 ); // the larger this neighborhood, the more accurate the distance transform
	assert( neighbors.size() < 1024 );
	neighbor_distance.resize( neighbors.size() );
	for (uint i=0; i<neighbors.size(); i++) {
		neighbor_distance[i] = VDOUBLE(neighbors[i]).abs() * SIM::getNodeLength();
		cout << "Node Length = " <<  SIM::getNodeLength() << ", neighbor_distance[" << i << "] = " << neighbor_distance[i] << "\n";
	}
	
	// initialize maps: 
	// - Mask: all writable nodes (within domain and not already occupied)
	// - DistanceMap
	// - Labels
	// TODO: why can I only make lattices of type double?? and not int or bool?
	shared_ptr< Lattice_Data_Layer< double > >	maskMap		= createLatticeDouble(lattice, 0);
	shared_ptr< Lattice_Data_Layer< double > >	distanceMap	= createLatticeDouble(lattice, no_distance);
	shared_ptr< Lattice_Data_Layer< double > > 	labelMap	= createLatticeDouble(lattice, no_label);
	
	VINT pos;
	for(pos.z=0; pos.z<lattice->size().z; pos.z++){
		for(pos.y=0; pos.y<lattice->size().y; pos.y++){
			for(pos.x=0; pos.x<lattice->size().x; pos.x++){
				
				// set mask to true at writable, empty positions (TODO: check whether this respects Domains)
				if( cpm->get(pos) == CPM::getEmptyState() && cpm->writable(pos) ){
					//maskMap->set(pos, true);
					maskMap->set(pos, 1);
					//cout << pos << " : empty and writable" << endl;
				}
				else{
					// set distance to 0.0 at positions where there are cells of this celltype
					const Cell& cell_at_pos = CPM::getCell( cpm->get(pos).cell_id );
					if( cell_at_pos.getCellType() == celltype ){
						distanceMap->set(pos, 0.0);
						labelMap->set(pos, cpm->get(pos).cell_id );
// 						cout << pos << " : " << cpm->get(pos).cell_id << endl;
					} 
				}
			}
		}
	}
	
	//euclideanDistanceTransform(distanceMap, maskMap);
	voronoiLabelling(distanceMap, maskMap, labelMap);
	
	VINT start(0,0,0);
	VINT stop = lattice->size();
	pos = VINT(0,0,0);
	for (pos.z = start.z; pos.z < stop.z; pos.z += 1) {
		for (pos.y = start.y; pos.y < stop.y; pos.y += 1, pos.x=0) {
			int idx = maskMap->get_data_index(pos);
			for (; pos.x < stop.x; pos.x += 1, idx += 1) {
				if (idx <0 || idx> maskMap->shadow_size_size_xyz) {
					cout << " Invalid index " << idx << " at " << pos << " max is " << maskMap->shadow_size_size_xyz<< endl;
					assert(0);
				}
				// Skip all points outside of cell in order to compute the geodesic or constrained distance transform
				if( maskMap->data[idx] == 0){
					continue;
				}
				double label = labelMap->data[idx];
// 				cout << "label at pos " << pos << " = " << label << "\n";
				if( label != no_label ){
					CPM::setNode(pos, uint(label) );
				}
			} // end of x-loop
		}// end of y-loop
	}// end of z-loop
	// does not create cells
	return vector<CPM::CELL_ID>();
}

int InitVoronoi::voronoiLabelling( shared_ptr<Lattice_Data_Layer<double> >& distanceMap, shared_ptr<Lattice_Data_Layer<double> >&maskMap,shared_ptr<Lattice_Data_Layer<double> >& labelMap){
	return voronoiLabelling(distanceMap, maskMap, labelMap, VINT(0,0,0), distanceMap->size());
	
}
int InitVoronoi::voronoiLabelling( shared_ptr<Lattice_Data_Layer<double> >& distanceMap, shared_ptr<Lattice_Data_Layer<double> >&maskMap, shared_ptr<Lattice_Data_Layer<double> >& labelMap, VINT bottomleft, VINT topright){
	bool print_to_file = false;
	bool done = false;
	int iterations=0;
	int dir = 0;
	set<int> indices;
	VINT lsize = distanceMap->size();
	
	/////// ITERATIVE DISTANCE TRANSFORM //////
	// scan (bounding box) lattice is various directions
	// while, for each point, check the shortest distance in the neighborhood, and copy this + distance to the neighbor
	// terminate when no changes occur.
	while (! done ) {
		if(print_to_file){
			// print to file
			std::ofstream d;
			d.open(string( string("label_") + to_str(iterations) + string(".log") ).c_str(), ios_base::trunc);
			VINT p;
			for(p.z=0; p.z<labelMap->size().z; p.z++){
				for(p.y=0; p.y<labelMap->size().y; p.y++){
					for(p.x=0; p.x<labelMap->size().x; p.x++){
						if( labelMap->get( p ) == no_label )
							d << " 0";
						else 
							d << " " << labelMap->get( p );
					}
					d << "\n";
				}
				d << "\n";
			}
			d.close();
			d.open(string( string("distance_") + to_str(iterations) + string(".log") ).c_str(), ios_base::trunc);
			for(p.z=0; p.z<distanceMap->size().z; p.z++){
				for(p.y=0; p.y<distanceMap->size().y; p.y++){
					for(p.x=0; p.x<distanceMap->size().x; p.x++){
						if( distanceMap->get( p ) == no_distance )
							d << " 0";
						else 
							d << " " << distanceMap->get( p );
					}
					d << "\n";
				}
				d << "\n";
			}
			d.close();
		}
		
		iterations++;
		done = true;
		uint changes = 0;
		// 0: Fx Fy Fz
		// 1: Bx Fy Fz
		// 2: Fx By Fz
		// 3: Fx Fy Bz
		// 4: Bx By Fz
		// 5: Fx By Bz
		// 6: Bx Fy Bz
		// 7: Bx By Bz
		bool x_fwd = (dir == 0 || dir == 2 || dir == 3 || dir == 5);
		bool y_fwd = (dir == 0 || dir == 1 || dir == 3 || dir == 6);
		bool z_fwd = (dir == 0 || dir == 1 || dir == 2 || dir == 4);
		VINT start(	x_fwd ? bottomleft.x : topright.x-1,
					y_fwd ? bottomleft.y : topright.y-1,
					z_fwd ? bottomleft.z : topright.z-1 );
		VINT iter(	x_fwd ? +1 : -1,
					y_fwd ? +1 : -1,
					z_fwd ? +1 : -1 );
		VINT stop(	x_fwd ? topright.x : bottomleft.x-1,
					y_fwd ? topright.y : bottomleft.y-1,
					z_fwd ? topright.z : bottomleft.z-1 );
		VINT pos(0,0,0);
		for (pos.z = start.z; pos.z != stop.z; pos.z += iter.z) {
			for (pos.y = start.y; pos.y != stop.y; pos.y += iter.y) {
				pos.x = start.x;
				int idx = maskMap->get_data_index(pos);
				for (; pos.x != stop.x; pos.x += iter.x, idx +=iter.x) {
					if (idx <0 || idx> maskMap->shadow_size_size_xyz) {
						cout << " Invalid index " << idx << " at " << pos << " max is " << maskMap->shadow_size_size_xyz<< endl;
						cout << " Lattice size " << lsize << " bottom " << bottomleft << " top " << topright << endl;
						assert(0);
					}
					// Skip all points outside of cell in order to compute the geodesic or constrained distance transform
					if( maskMap->data[idx] == 0){
						continue;
					}
					
					double orig_dist = distanceMap->data[idx];
					double min_dist = orig_dist;
					double closest_label = labelMap->data[idx];
					for (uint i=0; i<neighbors.size(); i++) {
						//cout << "neighbors" << endl;
						double dist = distanceMap->data[ distanceMap->get_data_index( pos + neighbors[i] )];
						double label_nb = labelMap->data[ labelMap->get_data_index( pos + neighbors[i] )];
						if( dist != no_distance ){
							//cout << "not equal to no_distance" << endl;
							dist += neighbor_distance[i];
							if( dist < min_dist ){
								min_dist = dist;
								closest_label = label_nb;
							}
						}
					}
					if ( min_dist - orig_dist < -10e-6) {
						distanceMap->data[idx] = min_dist;
						labelMap->data[idx] = closest_label;
						indices.insert(idx);
						done = false;
						changes++;
					}
				} // end of x loop
			} // end of y loop
		} // end of z loop
		dir = (dir + 1) % 8;
		//distanceMap->reset_boundaries();
		cout << "Iterations: " << iterations << ", changes: " << changes << endl;
	}
	//std::ofstream d;
	//d.open(string( string("mask_") + to_str(iterations) + string(".log") ).c_str(), ios_base::trunc);
	//distanceMap->write_ascii(d);
	//maskMap->write_ascii(d);
	//d.close();
	return indices.size();
};



int InitVoronoi::euclideanDistanceTransform( shared_ptr<Lattice_Data_Layer<double> >& distanceMap, shared_ptr<Lattice_Data_Layer<double> >&maskMap){
	return euclideanDistanceTransform( distanceMap, maskMap, VINT(0,0,0), distanceMap->size());
	return euclideanDistanceTransform(distanceMap, maskMap, VINT(0,0,0), distanceMap->size());
	
}
int InitVoronoi::euclideanDistanceTransform( shared_ptr<Lattice_Data_Layer<double> >& distanceMap, shared_ptr<Lattice_Data_Layer<double> >&maskMap, VINT bottomleft, VINT topright){
	bool done = false;
	int iterations=0;
	int dir = 0;
	set<int> indices;
	VINT lsize = distanceMap->size();
	
	/////// ITERATIVE DISTANCE TRANSFORM //////
	// scan (bounding box) lattice is various directions
	// while, for each point, check the shortest distance in the neighborhood, and copy this + distance to the neighbor
	// terminate when no changes occur.
	while (! done ) {
		// print to file
		std::ofstream d;
		d.open(string( string("distance_") + to_str(iterations) + string(".log") ).c_str(), ios_base::trunc);
		VINT p;
		for(p.z=0; p.z<distanceMap->size().z; p.z++){
			for(p.y=0; p.y<distanceMap->size().y; p.y++){
				for(p.x=0; p.x<distanceMap->size().x; p.x++){
					if( distanceMap->get( p ) == no_distance )
						d << " 0";
					else 
						d << " " << distanceMap->get( p );
				}
				d << "\n";
			}
			d << "\n";
		}
		d.close();
		
		iterations++;
		done = true;
		uint changes = 0;
		// 0: Fx Fy Fz
		// 1: Bx Fy Fz
		// 2: Fx By Fz
		// 3: Fx Fy Bz
		// 4: Bx By Fz
		// 5: Fx By Bz
		// 6: Bx Fy Bz
		// 7: Bx By Bz
		bool x_fwd = (dir == 0 || dir == 2 || dir == 3 || dir == 5);
		bool y_fwd = (dir == 0 || dir == 1 || dir == 3 || dir == 6);
		bool z_fwd = (dir == 0 || dir == 1 || dir == 2 || dir == 4);
		VINT start(	x_fwd ? bottomleft.x : topright.x-1,
					y_fwd ? bottomleft.y : topright.y-1,
					z_fwd ? bottomleft.z : topright.z-1 );
		VINT iter(	x_fwd ? +1 : -1,
					y_fwd ? +1 : -1,
					z_fwd ? +1 : -1 );
		VINT stop(	x_fwd ? topright.x : bottomleft.x-1,
					y_fwd ? topright.y : bottomleft.y-1,
					z_fwd ? topright.z : bottomleft.z-1 );
		VINT pos(0,0,0);
		for (pos.z = start.z; pos.z != stop.z; pos.z += iter.z) {
			for (pos.y = start.y; pos.y != stop.y; pos.y += iter.y) {
				pos.x = start.x;
				int idx = maskMap->get_data_index(pos);
				for (; pos.x != stop.x; pos.x += iter.x, idx +=iter.x) {
					if (idx <0 || idx> maskMap->shadow_size_size_xyz) {
						cout << " Invalid index " << idx << " at " << pos << " max is " << maskMap->shadow_size_size_xyz<< endl;
						cout << " Lattice size " << lsize << " bottom " << bottomleft << " top " << topright << endl;
						assert(0);
					}
					// Skip all points outside of cell in order to compute the geodesic or constrained distance transform
					if( maskMap->data[idx] == 0){
						continue;
					}
					double orig_dist = distanceMap->data[idx];
					double min_dist = orig_dist;
					for (uint i=0; i<neighbors.size(); i++) {
						double dist = distanceMap->data[ distanceMap->get_data_index( pos + neighbors[i] )];
						if( dist != no_distance ){
							dist += neighbor_distance[i];
							if( dist < min_dist ){
								min_dist = dist;
							}
						}
					}
					if ( min_dist - orig_dist < -10e-6) {
						distanceMap->data[idx] = min_dist;
						indices.insert(idx);
						done = false;
						changes++;
					}
				} // end of x loop
			} // end of y loop
		} // end of z loop
		dir = (dir + 1) % 8;
		//distanceMap->reset_boundaries();
		cout << "Iterations: " << iterations << ", changes: " << changes << endl;
	}
	//std::ofstream d;
	//d.open(string( string("mask_") + to_str(iterations) + string(".log") ).c_str(), ios_base::trunc);
	//distanceMap->write_ascii(d);
	//maskMap->write_ascii(d);
	//d.close();
	return indices.size();
};

shared_ptr< Lattice_Data_Layer< double > > InitVoronoi::createLatticeDouble(shared_ptr<const Lattice> lattice, double default_value){
	return shared_ptr< Lattice_Data_Layer< double > >(new Lattice_Data_Layer< double >(lattice, 2, default_value));
};

/*
shared_ptr< Lattice_Data_Layer< bool > > InitVoronoi::createLatticeBool( VINT boxsize, Lattice::Structure structure, bool default_value ){
	XMLNode xLattice = XMLNode::createXMLTopNode("Lattice");
	xLattice.addChild("Size").addAttribute("value",to_cstr(boxsize));
	XMLNode xLatticeBC = xLattice.addChild("BoundaryConditions");
	XMLNode xLatticeBCC1 = xLatticeBC.addChild("Condition");
	xLatticeBCC1.addAttribute("boundary",to_cstr("x"));
	xLatticeBCC1.addAttribute("type","constant");
	XMLNode xLatticeBCC2 = xLatticeBC.addChild("Condition");
	xLatticeBCC2.addAttribute("boundary",to_cstr("-x"));
	xLatticeBCC2.addAttribute("type","constant");
	XMLNode xLatticeBCC3 = xLatticeBC.addChild("Condition");
	xLatticeBCC3.addAttribute("boundary",to_cstr("y"));
	xLatticeBCC3.addAttribute("type","constant");
	XMLNode xLatticeBCC4 = xLatticeBC.addChild("Condition");
	xLatticeBCC4.addAttribute("boundary",to_cstr("-y"));
	xLatticeBCC4.addAttribute("type","constant");
	XMLNode xLatticeBCC5 = xLatticeBC.addChild("Condition");
	xLatticeBCC5.addAttribute("boundary",to_cstr("z"));
	xLatticeBCC5.addAttribute("type","constant");
	XMLNode xLatticeBCC6 = xLatticeBC.addChild("Condition");
	xLatticeBCC6.addAttribute("boundary",to_cstr("-z"));
	xLatticeBCC6.addAttribute("type","constant");
	
	shared_ptr<const Lattice> lattice;
	
	if( structure == Lattice::Structure::linear )
        lattice = shared_ptr<const Lattice>(new Linear_Lattice(xLattice));
	else if( structure == Lattice::Structure::hexagonal)
        lattice = shared_ptr<const Lattice>(new Hex_Lattice(xLattice));
	else if( structure == Lattice::Structure::square)
        lattice = shared_ptr<const Lattice>(new Square_Lattice(xLattice));
	else if( structure == Lattice::Structure::cubic)
        lattice = shared_ptr<const Lattice>(new Cubic_Lattice(xLattice));

	return shared_ptr< Lattice_Data_Layer<bool> >(new Lattice_Data_Layer<bool>(lattice, 2, default_value));
}

shared_ptr< Lattice_Data_Layer< int > > InitVoronoi::createLatticeInt(VINT size, Lattice::Structure structure, int default_value){
	
	// create temporary lattice to hold distance values
	XMLNode xLattice = XMLNode::createXMLTopNode("Lattice");
	xLattice.addChild("Size").addAttribute("value",to_cstr(size));
	XMLNode xLatticeBC = xLattice.addChild("BoundaryConditions");
	XMLNode xLatticeBCC1 = xLatticeBC.addChild("Condition");
	xLatticeBCC1.addAttribute("boundary",to_cstr("x"));
	xLatticeBCC1.addAttribute("type","constant");
	XMLNode xLatticeBCC2 = xLatticeBC.addChild("Condition");
	xLatticeBCC2.addAttribute("boundary",to_cstr("-x"));
	xLatticeBCC2.addAttribute("type","constant");
	XMLNode xLatticeBCC3 = xLatticeBC.addChild("Condition");
	xLatticeBCC3.addAttribute("boundary",to_cstr("y"));
	xLatticeBCC3.addAttribute("type","constant");
	XMLNode xLatticeBCC4 = xLatticeBC.addChild("Condition");
	xLatticeBCC4.addAttribute("boundary",to_cstr("-y"));
	xLatticeBCC4.addAttribute("type","constant");
	XMLNode xLatticeBCC5 = xLatticeBC.addChild("Condition");
	xLatticeBCC5.addAttribute("boundary",to_cstr("z"));
	xLatticeBCC5.addAttribute("type","constant");
	XMLNode xLatticeBCC6 = xLatticeBC.addChild("Condition");
	xLatticeBCC6.addAttribute("boundary",to_cstr("-z"));
	xLatticeBCC6.addAttribute("type","constant");
	
    shared_ptr<const Lattice> lattice;
	
	if( structure == Lattice::Structure::linear )
        lattice = shared_ptr<const Lattice>(new Linear_Lattice(xLattice));
	else if( structure == Lattice::Structure::hexagonal)
        lattice = shared_ptr<const Lattice>(new Hex_Lattice(xLattice));
	else if( structure == Lattice::Structure::square)
        lattice = shared_ptr<const Lattice>(new Square_Lattice(xLattice));
	else if( structure == Lattice::Structure::cubic)
        lattice = shared_ptr<const Lattice>(new Cubic_Lattice(xLattice));

	return shared_ptr< Lattice_Data_Layer< int > >(new Lattice_Data_Layer< int >(lattice, 2, default_value));
};*/

