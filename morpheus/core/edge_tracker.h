//////
//
// This file is part of the modelling and simulation framework 'Morpheus',
// and is made available under the terms of the BSD 3-clause license (see LICENSE
// file that comes with the distribution or https://opensource.org/licenses/BSD-3-Clause).
//
// Authors:  Joern Starruss and Walter de Back
// Copyright 2009-2016, Technische Universität Dresden, Germany
//
//////

/*

This file provides a Monte Carlo Sampler that remembers all edges between nodes of unequal states.

*/


#ifndef EDGE_LIST
 #define EDGE_LIST

#include <list>
#include <functional>
#include <algorithm>
#include <limits>
#include "cpm_layer.h"

// get access to the global randomness
extern uint getRandomUint(uint max);
extern bool getRandomBool();
// 	extern gsl_rng* random_engine;

inline unsigned long l_rand(unsigned long max_val) {
	return (rand() | rand() << 15) % max_val;
}

class EdgeTrackerBase {
protected:
	shared_ptr< const CPM::LAYER > cell_layer;
	shared_ptr< const Lattice> lattice;
	vector<VINT> opx_neighbors;
	vector<VINT> surface_neighbors;
public:
	EdgeTrackerBase(shared_ptr<const  CPM::LAYER > p, const vector<VINT>& opx_nei, const vector<VINT>& surface_nei) : cell_layer(p), lattice(p->getLattice()), opx_neighbors(opx_nei), surface_neighbors(surface_nei) {};
	virtual ~EdgeTrackerBase() {};
	/// Number of update trials in a MCS
	virtual uint updates_per_mcs() const  =0;
	/// Pick a random update
	virtual void get_update(VINT& origin, VINT& direction) const =0;
	/// Notify the stepper that an update has occured
	virtual void update_notifier(const VINT& pos, const LatticeStencil& neighborhood) =0;
	/// Reset the stepper, i.e. the cell configuration may have changed completely
	virtual void reset() =0;
	virtual bool has_surface(const VINT& pos) const =0;
	virtual uint n_surfaces(const VINT& pos) const =0;
	/// The update neighborhood used
	const vector<VINT>& getNeighborhood() const { return opx_neighbors; }
};

class NoEdgeTracker : public EdgeTrackerBase {
public:
	NoEdgeTracker(shared_ptr< const CPM::LAYER > p, const vector<VINT>& opx_nei, const vector<VINT>& surface_nei) : EdgeTrackerBase(p, opx_nei, surface_nei)
	{
		max_source = this->lattice->size() - VINT(1,1,1);
		min_source = VINT(0,0,0);
		// Increase the size in case of constant boundaries
		if (this->lattice->get_boundary_type(Boundary::mx) == Boundary::constant) min_source.x -= 1;
		if (this->lattice->get_boundary_type(Boundary::my) == Boundary::constant) min_source.y -= 1;
		if (this->lattice->get_boundary_type(Boundary::mz) == Boundary::constant) min_source.z -= 1;
		
		if (this->lattice->get_boundary_type(Boundary::px) == Boundary::constant) max_source.x += 1;
		if (this->lattice->get_boundary_type(Boundary::py) == Boundary::constant) max_source.y += 1;
		if (this->lattice->get_boundary_type(Boundary::pz) == Boundary::constant) max_source.z += 1;
		
	};
	
	uint updates_per_mcs() const {
		VINT a = this->cell_layer->size();
		return a.x * a.y * a.z;
	};
	
	void get_update(VINT& origin, VINT& direction) const {
		// Just pick a random position and a random neighbor
		while(1) {
			VINT s = max_source-min_source;
			origin.x = getRandomUint(s.x);
			origin.y = getRandomUint(s.y);
			origin.z = getRandomUint(s.z);
			origin += min_source;
			
			direction = this->opx_neighbors[ getRandomUint(this->opx_neighbors.size()-1) ];
			if (! this->cell_layer->writable(origin+direction)) continue;
			break;
		}
	}
	// Nothing to do on lattice updates ...
	void update_notifier(const VINT& pos, const LatticeStencil& neighborhood) {};
	
	virtual void reset() {};
	
	virtual bool has_surface(const VINT& pos) const {
		const CPM::STATE& spin = cell_layer->get(pos);
		// check the boundary neighborhood
		for (const auto& offset : this->surface_neighbors) {
			if (spin != cell_layer->get(pos+offset) )
				return true;
		}
		return false;
	}
	
	virtual uint n_surfaces(const VINT& pos) const {
		const CPM::STATE& spin = this->cell_layer->get(pos);
		// check 1st order neighborhood
		uint n_edges=0;
		for (const auto& offset : this->surface_neighbors) {
			if (spin != cell_layer->get(pos+offset) )
				n_edges++;
		}
		return n_edges;
	}
	
private:
	VINT min_source, max_source;
};


class EdgeListTracker : public EdgeTrackerBase {
private:
	typedef vector<unsigned int> Edge_Id_List;  // the edge id container per node, does not occupy memory when empty
	const unsigned int no_edge;
	const unsigned int no_flux_edge;

	struct Edge {
			bool valid;
			VINT pos_a, pos_b;
			int direction_a2b;
			Edge_Id_List *eid_list_a, *eid_list_b;
	} ;

	vector<Edge> edges;
	vector<uint> invalid_edge_ids;
	Lattice_Data_Layer<Edge_Id_List> *edge_lattice;
	vector<int> inverse_neighbor;
	vector<bool> opx_is_surface;
	Boundary::Type lattice_boundary_type[Boundary::nCodes];	

	void init_edge_list();

public:
	EdgeListTracker(shared_ptr< const CPM::LAYER > p, const vector<VINT> &opx_nei, const vector<VINT>& surface_nei);
	~EdgeListTracker() { delete edge_lattice; }
	virtual uint updates_per_mcs() const {
		static double factor = 2.0 / this->opx_neighbors.size();
		return uint( (edges.size() - invalid_edge_ids.size()) * factor );
	};
	virtual void get_update(VINT& origin, VINT& direction) const;
	virtual void update_notifier(const VINT& pos, const LatticeStencil& neighborhood);
	virtual void reset();
	
	virtual bool has_surface(const VINT& pos) const;
	virtual uint n_surfaces(const VINT& pos) const;

};



#endif // EDGE_LIST




