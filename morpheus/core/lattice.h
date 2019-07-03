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

#ifndef LATTICE_H
#define LATTICE_H

/**
 * @file
 * Definition of all available lattice structures and Boundary handling.
 * The common interface is the Lattice class.
 */

#include "config.h"

#include <algorithm>
#include "vec.h"
#include "xml_functions.h"
#include "domain.h"
// All lattice characteristics are kept in a class derived from the Lattice class. They provide metrics (including topological boundaries), the neighborhood and the lattice extends. All data is stored in Lattice_Data_Layer class, which thus is a template.


class Lattice;

class Neighborhood {
public:
	Neighborhood() :_neighbors({}),  _order(0), _distance(0), _lattice(nullptr) {};
	Neighborhood(const vector<VINT>& neighbors, int order, const Lattice* lattice);
	
	/// Get i_th neighbor, angular sorted in counter-clockwise fashion
	const VINT& operator[](int i) const { return _neighbors[i]; };
	
	/// Get neighbors, angular sorted in counter-clockwise fashion
	const vector<VINT>& neighbors() const { return _neighbors; };
	bool empty() const { return _neighbors.empty(); }
	int size() const { return _neighbors.size(); }
	/// Get order of the neighborhood
	int order() const { return _order; };
	/// Get neigborhood size, i.e. maximum distance from the neigborhood center
	double distance() const { return _distance; };
	const Lattice& lattice() const { if (!_lattice) throw string("Uninitialized lattice in Neighborhood!"); return *_lattice; };
private:
	vector<VINT> _neighbors;
	int _order;
	double _distance;
	const Lattice* _lattice;
// 	friend class Lattice;
};


class Lattice {
public:

	enum Structure { linear, square , hexagonal, cubic } structure;

	virtual string getXMLName() const =0;
	void loadFromXML(const XMLNode xnode);
	XMLNode saveToXML();

	Lattice();  /// Configure a Lattice from a XML node
	struct LatticeDesc {
		Structure structure;
		VINT size;
		uint neighborhood_order = 1;
		map<Boundary::Codes,Boundary::Type> boundaries;
	};
	Lattice(const LatticeDesc& desc);
	static unique_ptr<Lattice> createLattice(const LatticeDesc& desc);
	
	virtual ~Lattice() {};

//  Lattice extend
	const VINT& size() const { return _size;}					 /// Size of the lattice
	bool equal_pos(const VINT& a, const VINT& b) const;	 /// equality operator that respects periodic BCs
	/// Check wether the node referred by @param a is numerically within the range of lattice size
	bool accessible(const VINT& a) const;
	/// Check wether the node referred by @param a is within the lattice, also considers periodic boundaries
	bool inside(const VINT& a) const;
	/// Solve periodic boundaries, i.e. map @param a into the lattice. Returns FALSE if this is not possible.
	bool resolve(VINT& a) const;
	bool resolve(VDOUBLE& a) const;
	bool resolve(VINT& a, Boundary::Codes& b) const;
	
	/// Read the topological BCs defined within the lattice. Valid values are -x,x,+x for x, y, and z respectively.
	Boundary::Type get_boundary_type(Boundary::Codes code) const;
	vector<Boundary::Type> get_boundary_types() const;
	const Domain& getDomain() const { return domain; } 

	/// Number of lattice dimensions
	uint getDimensions() const {return dimensions; }
	Lattice::Structure getStructure() const { return structure; }
	const Neighborhood& getDefaultNeighborhood() const { return default_neighborhood; }
	/// Choose a lattice site at random ...
	VINT getRandomPos() const;

	// metrics that depend on the lattice topology
	VINT node_distance(const VINT& a, const VINT& b) const;		/// distance between lattice nodes
//	VDOUBLE  node_distance(const VDOUBLE & a, const VDOUBLE & b) const;		/// distance measure in lattice coordinates

	// metrics that depend on the lattice topology in orthogonal coordinates
	virtual VDOUBLE to_orth(const VDOUBLE& a) const =0;   /// convert to orthogonal coordinates
	bool orth_resolve(VDOUBLE& a) const;
	virtual VDOUBLE orth_distance(const VDOUBLE& a, const VDOUBLE& b) const =0; /// distance measure assuming parameters are provided in orthogonal coordinates. Also respects periodic boundary conditions.
	virtual VINT from_orth(const VDOUBLE& a) const = 0; /// convert orthogonal coordinates into a discrete lattice position
	/// Neighborhood identified by @param  name
	Neighborhood getNeighborhood(const std::string name) const;  
	/// Neighborhood up to the order defined by @param  order.
	Neighborhood getNeighborhood(uint order) const { return getNeighborhoodByOrder(order); };
	Neighborhood getNeighborhood(const XMLNode node) const ;
	Neighborhood getNeighborhoodByDistance(const double dist_max) const;
	Neighborhood getNeighborhoodByOrder(const uint order) const;
	
protected:
	const double sin_60 = 0.86602540378443864676;
	uint dimensions;
	VINT _size;
	Boundary::Type boundaries[Boundary::nCodes];
	Domain domain;
	VINT setSize(const VINT&);
	string neighborhood_value, neighborhood_type;
	XMLNode stored_node;

	// access to predefined neighborhoods
	virtual Neighborhood getNeighborhoodByName(std::string name) const { return Neighborhood(); };
	virtual vector<VINT> get_all_neighbors() const =0;
	virtual vector<int> get_all_neighbors_per_order() const =0;
	Neighborhood default_neighborhood;
	void setNeighborhood(const XMLNode node);
	
	friend class Domain;
	
};

// #include "lattice_data_layer.h"



inline bool Lattice::resolve ( VINT& a) const {
	if ( a.z<0 || a.z >= _size.z ) {
		if ( boundaries[Boundary::pz] == Boundary::periodic  ) {
			 a.z = pmod(a.z,_size.z);
		}
		else {
			return false;
		}
	}
	if ( a.y<0 or a.y >= _size.y ) {
		if (boundaries[Boundary::py] == Boundary::periodic ) {
			a.y = pmod(a.y,_size.y); 
		}
		else {
			return false;
		}
	}
	if ( a.x<0 or a.x >= _size.x ) {
		if ( boundaries[Boundary::px] == Boundary::periodic ) {
			a.x = pmod(a.x,_size.x);
		}
		else {
			return false;
		}
	}
	
	return true;
}

inline bool Lattice::resolve ( VDOUBLE& a) const {
	if ( a.z<0 || a.z >= _size.z ) {
		if ( boundaries[Boundary::pz] == Boundary::periodic  ) {
			 a.z = pmod(a.z, double(_size.z));
		}
		else {
			return false;
		}
	}
	if ( a.y<0 or a.y >= _size.y ) {
		if (boundaries[Boundary::py] == Boundary::periodic ) {
			a.y = pmod(a.y, double(_size.y)); 
		}
		else {
			return false;
		}
	}
	if ( a.x<0 or a.x >= _size.x ) {
		if ( boundaries[Boundary::px] == Boundary::periodic ) {
			a.x = pmod(a.x, double(_size.x));
		}
		else {
			return false;
		}
	}
	
	return true;
}

inline bool Lattice::orth_resolve ( VDOUBLE& a) const {
	if ( a.z<0 || a.z >= _size.z ) {
		if ( boundaries[Boundary::pz] == Boundary::periodic  ) {
			 a.z = pmod(a.z,double(_size.z));
		}
		else {
			return false;
		}
	}
	if (structure == hexagonal) {
		double orth_y_size = sin_60 *_size.y;
		if ( a.y<0 or a.y >= orth_y_size ) {
			if (boundaries[Boundary::py] == Boundary::periodic) {
				a.x -= double(_size.y) * 0.5 * pdiv(a.y,orth_y_size);
				a.y  = pmod(a.y,double(orth_y_size));
			}
			else {
				return false;
			}
		}
	}
	else {
		if ( a.y<0 or a.y >= _size.y ) {
			if (boundaries[Boundary::py] == Boundary::periodic) {
				a.y = pmod(a.y,double(_size.y));
			}
			else {
				return false;
			}
		}
	}
	if ( a.x<0 or a.x >= _size.x ) {
		if ( boundaries[Boundary::px] == Boundary::periodic ) {
			a.x = pmod(a.x,double(_size.x));
		}
		else {
			return false;
		}
	}
	
	return true;
}


inline bool Lattice::resolve ( VINT& a, Boundary::Codes& b) const {
// try to solve potential topological boundaries
	if ( a.z<0 || a.z >= _size.z ) {
		if ( boundaries[Boundary::pz] == Boundary::periodic  ) {
			 a.z = pmod(a.z,_size.z);
		}
		else if (a.z<0) {
			b = Boundary::mz;
			return false; // outside = false;
		}
		else { 
			b = Boundary::pz;
			return false;
		}
	}
	if ( a.y<0 or a.y >= _size.y ) {
		if (boundaries[Boundary::py] == Boundary::periodic ) {
			a.y = pmod(a.y,_size.y); 
		}
		else if (a.y<0) {
			b = Boundary::my;
			return false; // outside = false;
		}
		else { 
			b = Boundary::py;
			return false;
		}
	}
	if ( a.x<0 or a.x >= _size.x ) {
		if ( boundaries[Boundary::px] == Boundary::periodic ) {
			a.x = pmod(a.x,_size.x);
		}
		else if (a.x<0) {
			b = Boundary::mx;
			return false; // outside = false;
		}
		else { 
			b = Boundary::px;
			return false;
		}
	}

	return true;
}

inline bool Lattice::inside(const VINT& a) const {
	if ( boundaries[Boundary::px] != Boundary::periodic  && (a.x < 0 or a.x >= _size.x) ) return false;
	if ( boundaries[Boundary::py] != Boundary::periodic  && (a.y < 0 or a.y >= _size.y) ) return false;
	if ( boundaries[Boundary::pz] != Boundary::periodic  && (a.z < 0 or a.z >= _size.z) ) return false;
	return true;
};

// inline bool Lattice::insideAndResolve(VINT& a) const {
// // 	if (accessible(a)) return true;
// 	// TODO check all additional shape constraint shrinking the accessible lattice
// 	return resolve(a);
// };

class Hex_Lattice: public Lattice {
	public:
	Hex_Lattice(const XMLNode xnode);
	Hex_Lattice(const LatticeDesc& desc) : Lattice(desc) { dimensions=2; structure = hexagonal;  _size.z=1; };
	string getXMLName() const override { return "hexagonal"; };
	VDOUBLE to_orth(const VDOUBLE& a) const override;
	VDOUBLE orth_distance(const VDOUBLE& a, const VDOUBLE& b) const override;
	VINT from_orth(const VDOUBLE& a) const override;
private:
	VDOUBLE orth_size;
	vector<VINT> get_all_neighbors() const override;
	vector<int> get_all_neighbors_per_order() const override;
};


// boundary is a constraint and a value T, returns the value if constraint()
// typedef enum topo
class Orth_Lattice: public Lattice {
public:
	Orth_Lattice(const XMLNode a) : Lattice() {};
	Orth_Lattice(const LatticeDesc& desc) : Lattice(desc) {};
	VDOUBLE to_orth(const VDOUBLE& a) const override;
	VDOUBLE orth_distance(const VDOUBLE& a, const VDOUBLE& b) const override;
	VINT from_orth(const VDOUBLE& a) const override;
};

class Cubic_Lattice: public Orth_Lattice {
public:
	Cubic_Lattice(const XMLNode xnode);
	Cubic_Lattice(const LatticeDesc& desc) : Orth_Lattice(desc) { dimensions=3; structure = cubic; };
	string getXMLName() const override { return "cubic"; };
	Neighborhood getNeighborhoodByName(std::string name) const override;
private:
	vector<VINT> get_all_neighbors() const override;
	vector<int> get_all_neighbors_per_order() const override;
};


class Square_Lattice: public Orth_Lattice {
public:
       static Square_Lattice* create(VINT resolution, bool spherical);
	Square_Lattice(const XMLNode xNode);
	Square_Lattice(const LatticeDesc& desc) : Orth_Lattice(desc) { dimensions=2; structure = square; _size.z=1; };
	string getXMLName() const override { return "square"; };
private:
	vector<VINT> get_all_neighbors() const override;
	vector<int> get_all_neighbors_per_order() const override;
};



class Linear_Lattice: public Orth_Lattice {
public:
	static Linear_Lattice* create(VINT length, bool periodic);
	Linear_Lattice(const XMLNode xNode);
	Linear_Lattice(const LatticeDesc& desc) : Orth_Lattice(desc) { dimensions=1; structure = linear; _size.y=1; _size.z=1; };
	string getXMLName() const override { return "linear"; };
private:
	vector<VINT> get_all_neighbors() const override;
	vector<int> get_all_neighbors_per_order() const override;
};

#endif

