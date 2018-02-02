#include "edge_tracker.h"
#include "lattice_data_layer.cpp"

template class Lattice_Data_Layer<EdgeListTracker::Edge_Id_List>;

void EdgeListTracker::reset() {
	VINT l_size = this->lattice->size();
	VINT pos;
	for (pos.z = 0; pos.z<l_size.z; pos.z++) {
		for (pos.y = 0; pos.y<l_size.y; pos.y++) {
			for (pos.x = 0; pos.x<l_size.x; pos.x++) {
				if ( ! edge_lattice->writable(pos) )
					continue;
				edge_lattice->get_writable(pos).clear();
			}
		}
	}
	edges.clear();
	invalid_edge_ids.clear();
	
	init_edge_list();
}


EdgeListTracker::EdgeListTracker(shared_ptr< const CPM::LAYER >p, const vector<VINT> &opx_nei, const vector<VINT>& surface_nei) :
	EdgeTrackerBase(p, opx_nei, surface_nei), 
	no_edge(numeric_limits<unsigned int>::max()-1),
	no_flux_edge(numeric_limits<unsigned int>::max())
{
	Edge_Id_List emptyList;
	edge_lattice = new Lattice_Data_Layer<Edge_Id_List>( this->lattice,1, emptyList, "edge_list");
	// creating a list of the indices of the neighbors in the opposite direction
	vector<VINT> other_neighbors(this->opx_neighbors);
	for (vector<VINT>::iterator nei = this->opx_neighbors.begin(); nei != this->opx_neighbors.end(); nei++) {
		for (uint o_nei = 0; ; o_nei++) {
			if (o_nei == other_neighbors.size()) {
				cerr << "Edge_List_Stepper:: unsymmetric opx_neighbors !" << endl; exit(-1);
			}
			if (other_neighbors[o_nei] + (*nei) == VINT(0,0,0) ) {
				inverse_neighbor.push_back(o_nei); 
				// remove the neighbor while not changing the indices of the others
				// this is essential when neighbors show up multiple times as in FCHC
				other_neighbors[o_nei]=VINT(0,0,0); 
				break;
			}
		}
		
		opx_is_surface.push_back(find(this->surface_neighbors.begin(),this->surface_neighbors.end(),*nei) != this->surface_neighbors.end());
	}
	
// 	cout << "Edge_List:_Stepper: Checking for constant and noflux boundaries " << endl;
	for (uint i=0; i<Boundary::nCodes; i++) {
		lattice_boundary_type[i] = this->lattice->get_boundary_type((Boundary::Codes)i);
	}
	
	init_edge_list();
};

void EdgeListTracker::init_edge_list() {
	// Crawl through the lattice and extract all existing boundaries
	int initial_edges = 0;
	int update_edges = 0;
	VINT l_size = this->lattice->size();
	uint l_dim = this->lattice->getDimensions();
	VINT pos;	
	for (pos.z = 0; pos.z<l_size.z; pos.z++) {
		for (pos.y = 0; pos.y<l_size.y; pos.y++) {
			for (pos.x = 0; pos.x<l_size.x; pos.x++) {
				
				if ( ! edge_lattice->writable(pos) )
					continue;
				
				// Find out if there are boundaries that can recieve updates
				bool has_boundaries = false;
				const CPM::CELL_ID cpm_cell_a = this->cell_layer->get(pos).cell_id;
				for (uint neighbor = 0; neighbor < this->opx_neighbors.size(); neighbor++) {
					// get_writable uses the  topological boundaries to wrap the point into the accessible space, which is saved in edge.pos_b
					VINT nei_pos = pos + this->opx_neighbors[neighbor];

					const CPM::CELL_ID cpm_cell_b = this->cell_layer->get(nei_pos).cell_id;
					// this->cpm_lattice->get(edge.pos_b);
					if ( cpm_cell_a != cpm_cell_b ) {
						has_boundaries = true;
						break;
					}
				}
				
				if (!has_boundaries) {
					continue;
				}
				
				// There are boundaries that can recieve updates
				Edge edge;
				edge.pos_a = pos;
				edge.eid_list_a = &(edge_lattice->get_writable(edge.pos_a));
				if (edge.eid_list_a->empty()) edge.eid_list_a->resize(this->opx_neighbors.size(), no_edge);
				edge.valid = true;
				Boundary::Type bt;
				
				for (uint neighbor = 0; neighbor < this->opx_neighbors.size(); neighbor++) {
					
					if ((*edge.eid_list_a)[neighbor] != no_edge){
						// a neighbor already registered the boundary !
						continue;
					}
					// get_writable uses the  topological boundaries to wrap the point into the accessible space, which is saved in edge.pos_b
					edge.pos_b = edge.pos_a + this->opx_neighbors[neighbor];
					if ( edge_lattice->writable_resolve(edge.pos_b, bt) ) 
						// that neighboring node is writable !!
					{
						const CPM::CELL_ID cpm_cell_b = this->cell_layer->get(pos + this->opx_neighbors[neighbor]).cell_id;
						if ( cpm_cell_a != cpm_cell_b ) {	// there will be an edge
							if (edge.eid_list_a->empty()) edge.eid_list_a->resize(this->opx_neighbors.size(), no_edge);
							
							edge.eid_list_b = &(edge_lattice->get_writable(edge.pos_b));
							if (edge.eid_list_b->empty()) edge.eid_list_b->resize(this->opx_neighbors.size(), no_edge);
							edge.direction_a2b = neighbor; //index of the opx_neighborhood
							uint new_eid = edges.size();
								edges.push_back(edge);
							(*edge.eid_list_a)[neighbor] = new_eid;
							(*edge.eid_list_b)[inverse_neighbor[neighbor]] = new_eid;
							initial_edges++;
							update_edges++;
						}
						
					}
					else if ( bt == Boundary::constant) 
						// that neighboring node is not writable, but can source new nodes 
					{
						const CPM::CELL_ID cpm_cell_b = this->cell_layer->get(pos + this->opx_neighbors[neighbor]).cell_id;
						if ( cpm_cell_a != cpm_cell_b ) {
							
							// not registered in the boundary neighbor
							edge.eid_list_b = NULL;
							edge.direction_a2b = neighbor; //index of the opx_neighborhood

							uint new_eid = edges.size();
							edges.push_back(edge);
							(*edge.eid_list_a)[neighbor] = new_eid;
							initial_edges++;
							update_edges++;
						}
					}
					else if ( bt == Boundary::noflux) 
						// that neighboring node is not writable and can not source new nodes 
					{
						edge.eid_list_a->at(neighbor) = no_flux_edge;
						initial_edges++;
					}
				}
			}
		}
	}
	cout << "EdgeListTracker::init() : Created Tracker with Neighborhood size " << surface_neighbors.size() << endl;
	cout << "EdgeListTracker::init() : Found " << initial_edges << " initial edges, wherof " << update_edges << " can be modified." << endl;
}

void EdgeListTracker::get_update(VINT& origin, VINT& direction) const{

	assert(edges.size() - invalid_edge_ids.size() > 0);
	unsigned int idx;
	uint n_try=0;
	do {
		idx = getRandomUint( edges.size()-1 );
		assert(++n_try < 500);
		if ( ! edges[idx].valid ) continue;

		if ( getRandomBool() ) {
			if ( ! edge_lattice->writable(edges[idx].pos_b) ) continue; // node b, the focus, is not writable ...
			origin = edges[idx].pos_a;
			direction = this->opx_neighbors[edges[idx].direction_a2b];
			return;
		} else {
			if ( ! edge_lattice->writable(edges[idx].pos_a) ) continue;
			origin = edges[idx].pos_b;
			direction = this->opx_neighbors[inverse_neighbor[edges[idx].direction_a2b]];
			return;
		}
	} while (1) ;
}

void EdgeListTracker::update_notifier(const VINT& pos, const LatticeStencil& neighborhood) {
	Edge edge;
	edge.pos_a=pos;
	edge.valid = true;

	// pos is in cpm_lattice space. we have to map it before we can use it ...
	this->lattice->resolve(edge.pos_a);
	const CPM::CELL_ID cpm_cell_a = this->cell_layer->get(edge.pos_a).cell_id;

	edge.eid_list_a = &(edge_lattice->get_writable(edge.pos_a));
	if (edge.eid_list_a->empty()) edge.eid_list_a->resize(this->opx_neighbors.size(), no_edge);
	
	Boundary::Type bt;
	// appending new meaningful edges and deleting meaningless
	for (uint neighbor = 0; neighbor < this->opx_neighbors.size(); neighbor++) {
		// get_writable uses the  topological boundaries to wrap the point into the accessible space, which is saved in edge.pos_b
		edge.pos_b = edge.pos_a + this->opx_neighbors[neighbor];
		if ( edge_lattice->writable_resolve(edge.pos_b, bt) ) 
			// that neighboring node is writable !!
		{
			const CPM::CELL_ID cpm_cell_b = neighborhood.getStates()[neighbor];
			// this->cpm_lattice->get(edge.pos_b);
			if ( cpm_cell_a != cpm_cell_b ) {	// there will be an edge
				if ((*edge.eid_list_a)[neighbor] == no_edge) {  // there was no edge before

					edge.eid_list_b = &(edge_lattice->get_writable(edge.pos_b));
					if (edge.eid_list_b->empty()) edge.eid_list_b->resize(this->opx_neighbors.size(), no_edge);
					edge.direction_a2b = neighbor; //index of the opx_neighborhood

					uint new_eid;
					if ( invalid_edge_ids.empty() )
					{
						new_eid = edges.size();
						edges.push_back(edge);
					}
					else  {
						new_eid = invalid_edge_ids.back(); 
						invalid_edge_ids.pop_back();
						edges[new_eid] = edge;
					}

					(*edge.eid_list_a)[neighbor] = new_eid;
					(*edge.eid_list_b)[inverse_neighbor[neighbor]] = new_eid;
				}
				else {
					// else the boundary is just conserved ...
				}
				
			}
			else {		// there will be no edge
				unsigned int remove_edge_id  = edge.eid_list_a->at(neighbor);
				if ( remove_edge_id != no_edge) {
					Edge& redge = edges[remove_edge_id];
					(*redge.eid_list_a)[ redge.direction_a2b ] = no_edge;
					(*redge.eid_list_b)[ inverse_neighbor[redge.direction_a2b] ] = no_edge;
					redge.valid = false;
					if (remove_edge_id == edges.size()-1) {
						edges.pop_back();
						while (!edges.back().valid)  {
							auto i = std::find(invalid_edge_ids.begin(), invalid_edge_ids.end(), edges.size()-1);
							if (i == invalid_edge_ids.end())
								throw string("EdgeListTracker:: Unable to remove invalid edge ID");
							invalid_edge_ids.erase(i);
							edges.pop_back();
						}
					}
					else
						invalid_edge_ids.push_back(remove_edge_id);
				} 
				else {
					cerr << "EdgeListTracker:: Error while removing a boundary at " << edge.pos_a << " which is already 'no_edge'."<< endl; assert(0); exit(0);
				}
			}
		} 
		else if ( bt == Boundary::constant) 
			// that neighboring node is not writable, but can source new nodes 
		{
			const CPM::CELL_ID cpm_cell_b = neighborhood.getStates()[neighbor];
			if ( cpm_cell_a != cpm_cell_b ) {
				if ((*edge.eid_list_a)[neighbor] == no_edge) {  // there was no edge before
					// edge.eid_list_b = &(edge_lattice->get_writable(edge.pos_b));
					// if (edge.eid_list_b->empty()) edge.eid_list_b->resize(this->opx_neighbors.size(), no_edge);
					edge.eid_list_b = NULL;
					edge.direction_a2b = neighbor; //index of the opx_neighborhood

					uint new_eid;
					if ( invalid_edge_ids.empty() )
					{
						new_eid = edges.size();
						edges.push_back(edge);
					}
					else  {
						new_eid = invalid_edge_ids.back(); 
						invalid_edge_ids.pop_back();
						edges[new_eid] = edge;
					}

					(*edge.eid_list_a)[neighbor] = new_eid;
				}
				else {
					// else the boundary is just conserved ...
				}
				
			}
			else {		// there will be no edge
				unsigned int remove_edge_id  = edge.eid_list_a->at(neighbor);
				if ( remove_edge_id != no_edge) {
					Edge& redge = edges[remove_edge_id];
					(*redge.eid_list_a)[ redge.direction_a2b ] = no_edge;
					// redge.eid_list_b->at( inverse_neighbor[redge.direction_a2b] ) = no_edge;
					redge.valid = false;
					if (remove_edge_id == edges.size()-1) {
						edges.pop_back();
						while (!edges.back().valid)  {
							auto i = std::find(invalid_edge_ids.begin(), invalid_edge_ids.end(), edges.size()-1);
							if (i == invalid_edge_ids.end())
								throw string("EdgeListTracker:: Unable to remove invalid edge ID");
							invalid_edge_ids.erase(i);
							edges.pop_back();
						}
					}
					else
						invalid_edge_ids.push_back(remove_edge_id);
				} 
				else { 
					cerr << "EdgeListTracker:: Error while removing a boundary at " << edge.pos_a << " which is already 'no_edge'."<< endl; assert(0); exit(0);
				}
			}
		}
		else
		// that neighboring node is not writable and cannot source new nodes  (i.e. noflux)
		{
			edge.eid_list_a->at(neighbor) = no_flux_edge; 
		}
	}
	if (double (invalid_edge_ids.size()) / (edges.size()+100) > 0.08) {
		cout << "EdgeListTracker: Running defragmentation " << endl;
		cout << getStatInfo();
		defragment();
	}

};

void EdgeListTracker::defragment() {
	while ( ! invalid_edge_ids.empty()) {
		if (edges.back().valid) {
			auto new_id = invalid_edge_ids.back();
			
			auto& edge = edges[new_id] = edges.back();
			(*edge.eid_list_a)[edge.direction_a2b] = new_id;
			if (edge.eid_list_b)
				(*edge.eid_list_b)[inverse_neighbor[edge.direction_a2b]] = new_id;
			
			edges.pop_back();
			invalid_edge_ids.pop_back();
		}
		else {
			auto invalid_id = edges.size()-1;
			edges.pop_back();
			auto i = std::find(invalid_edge_ids.begin(), invalid_edge_ids.end(), invalid_id);
			if (i == invalid_edge_ids.end())
				throw string("EdgeListTracker:: Unable to remove invalid edge ID");
			*i = invalid_edge_ids.back();
			invalid_edge_ids.pop_back();
		}
	}
}


bool EdgeListTracker::has_surface(const VINT& pos) const{
	VINT k(pos);
	if ( ! this->lattice->resolve(k) ) return true;
	const vector<unsigned int> & edge_list = edge_lattice->get(k);
	for (uint i = 0; i< edge_list.size(); i++) {
		if (opx_is_surface[i] &&  edge_list[i] != no_edge ) return true;
	}
	return false;
}

uint EdgeListTracker::n_surfaces(const VINT& pos) const{
	VINT k(pos);
	if ( ! this->lattice->resolve(k) ) return 1;
	const vector<unsigned int> & edge_list = edge_lattice->get(pos);
	uint n_edges(0);
	for (uint i = 0; i< edge_list.size(); i++) {
		n_edges += (opx_is_surface[i] &&  edge_list[i] != no_edge );
	}
	return n_edges;
}


string EdgeListTracker::getStatInfo() const {
	stringstream info;
	info << "EdgeListTracker: EdgeList size " << edges.size() << ", invalid " << invalid_edge_ids.size() << endl;
	info << "                 Select ratio is " <<  double (edges.size() -  invalid_edge_ids.size()) / edges.size() << endl;
	info << "                 Opx Neighborhood is " << this->opx_neighbors.size() << endl;
	return info.str(); 
	
}
