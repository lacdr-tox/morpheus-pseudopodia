#include "connectivity_constraint.h"

REGISTER_PLUGIN(ConnectivityConstraint);

ConnectivityConstraint::ConnectivityConstraint(){
	// this plugin does not have parameters
}

void ConnectivityConstraint::init(const Scope* scope) {
	Cell_Update_Checker::init(scope);
	const Lattice& lattice = SIM::lattice();
	neighbors = CPM::getSurfaceNeighborhood().neighbors();
	max_first_order = 0;
	// TODO Check the neighborhood to be exactly what we need
	//CPM::getBoundaryNeighborhood();
	
	//cout << "ConnectivityConstraint: init(): " << neighbors.size() << " neighbors" << endl;
	
	dimensions = lattice.getDimensions();
	if ( dimensions == 2) {
		for (int i=0; i<neighbors.size(); i++) {
			double distance = lattice.to_orth(neighbors[i]).abs();
			double angle =  lattice.to_orth(neighbors[i]).angle_xy();
			is_first_order.push_back( distance < 1.01);
			max_first_order += distance < 1.01;
			cout << "ConnectivityConstraint: " << neighbors[i] << " a " << angle << " d" << distance << endl;
		}
		if ( lattice.getStructure() == Lattice::hexagonal ) {
			if (neighbors.size() > 12) {
				cerr << "ConnectivityConstraint only available for 1st order surface neighborhood in hexagonal lattices." << endl;
				exit(-1);
			}
		}
		else if ( lattice.getStructure() == Lattice::square ){
			if (neighbors.size() > 8) {
				cerr << "ConnectivityConstraint only available for 1st and 2nd order surfce neighborhood in square lattices." << endl;
				exit(-1);
			}
		}
	}
	else if (dimensions == 3) {
		for (int i=0; i<neighbors.size(); i++) {
			vector<int> neis;
			for (int j=0; j<neighbors.size(); j++) {
				if (i==j) continue;
				if ( (neighbors[i]-neighbors[j]).abs() < 1.01 )
					neis.push_back(j);
			}
			neighbor_neighbors.push_back(neis);
			is_first_order.push_back(neighbors[i].abs() < 1.01);
			max_first_order += neighbors[i].abs() < 1.01;
		}
	}
	cout << "ConnectivityConstraint: Found " << max_first_order << " first order neighbors" << endl;
};

bool ConnectivityConstraint:: update_check( CPM::CELL_ID cell_id , const CPM::Update& update)
{
	const vector<CPM::CELL_ID>& neighbors = update.surfaceStencil()->getStates();
	
	if (dimensions == 3) {
		int n_identicals = 0;
		int n_1st_order = 0;
		for (int i=0; i<neighbors.size(); i++) {
			n_identicals += ( neighbors[i] == cell_id );
			n_1st_order += is_first_order[i] && (neighbors[i] == cell_id) ;
		}
		int n_1st_section = 0;
		if (n_identicals>1) {
			vector<CPM::CELL_ID> nei_copy = neighbors;
			vector<int> neighbor_ids;
			for (int i=0; i<neighbors.size(); i++) {
				// find a first node for the first section
				if (neighbors[i] == cell_id) {
					// accumulate all nodes in this sections
					neighbor_ids.push_back(i);
					
					while (! neighbor_ids.empty()) {
						int i_nei = neighbor_ids.back();
						neighbor_ids.pop_back();
						n_1st_section++;
						nei_copy[i_nei]=-1;
						for (int k=0; k<neighbor_neighbors[i_nei].size(); k++) {
							if (nei_copy[neighbor_neighbors[i_nei][k]] == cell_id)
								neighbor_ids.push_back(neighbor_neighbors[i_nei][k]);
						}
					}
					break;
				}
			}
		}
		else {
			n_1st_section = n_identicals; 
		}
		
		if ( update.opRemove() && ( n_1st_section < n_identicals || n_1st_order == max_first_order ) ) {
// 			cout << "prevented update::remove " << SIM::getTime() << (n_1st_section < n_identicals ? " multiple sections " : "") << ( (n_1st_order == max_first_order) ? " all 1st order occ." : "") << endl;
			return false;
		}
		else if ( update.opAdd() && ( n_1st_section < n_identicals || n_1st_order == 0 )) {
// 			cout << "prevented update::add " << SIM::getTime() << (n_1st_section < n_identicals ? " multiple sections " : "") << ( (n_1st_order == 0) ? " all 1st order empty.": "") << endl;
			return false;
		}
		return true;
	}
	else if (dimensions == 2) {
		// Prohibiting node removals that might break cell connectivity
		int n_sections = 0; // number of sections remaining after the update
		int n_1st_order = 0; // number of 1st order neighbors occupied by other spins,
		CPM::CELL_ID last_neigbor = neighbors.back();
		
		for (int i=0; i<neighbors.size(); i++) {
			n_sections += ( neighbors[i] != cell_id && last_neigbor == cell_id );
			n_1st_order += is_first_order[i] && (neighbors[i] == cell_id) ;
			last_neigbor = neighbors[i];
		}

		if ( update.opRemove() ) {
			// prevent disconnecting chains and prevent hole formation
			if (n_sections > 1 || n_1st_order == max_first_order) return false;
		}
		
		// Prohibiting extension to a node, where a cell has no 1st order neighbor or connects individual branches
		if ( update.opAdd() ) {
			// prevent purely diagonal connections and connecting branches
			if (n_1st_order == 0 || n_sections > 1) return false;
		}
		return true;
	}
	else 
		return true;
};
