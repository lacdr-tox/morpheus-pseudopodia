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

#ifndef Lattice_Data_Layer_H
#define Lattice_Data_Layer_H

#include "lattice.h"
#include "config.h"
#include <set>
#include <valarray>

struct span {
	span(uint a, uint b) : low(a), up(b), length(up-low+1) {};
	uint low,up, length;
	span operator +(uint b) { return span(low+b,up+b);}
	span operator -(uint b) { return span(low-b,up-b);}
};


template <class T >
class Lattice_Data_Layer {
public:
	typedef T value_type;
	
	Lattice_Data_Layer(shared_ptr<const Lattice> l, int shadow_width, T def_val, string layer_name  = "");
	~Lattice_Data_Layer() { data.resize(0); }

	XMLNode saveToXML() const;
	void loadFromXML(const XMLNode xNode, T (* converter)(const string&));
	void storeData(ostream& out) const;
	bool restoreData(istream & in,  T (* converter)(istream&));
	shared_ptr<const Lattice> getLattice() const { cout << "LDL getLattice" << endl; return _lattice;};
	const Lattice& lattice() const { return *_lattice; };

	typename TypeInfo<T>::Return get(VINT a) const;
	bool set(VINT a, typename TypeInfo<T>::Parameter b);
	string getName() const { return name; }
	VINT getWritableSize();
// 	vector<const T*>  getBlock(VINT reference, vector<VINT> offsets) const;
	bool accessible(const VINT& a) const;
	bool writable(const VINT& a) const;
	bool writable_resolve(VINT& a) const;
	bool writable_resolve(VINT& a, Boundary::Type& b) const;
	T& get_writable(VINT a) __attribute__ ((deprecated));
	valarray<T> getData() const __attribute__ ((deprecated)) ;
	Boundary::Type getBoundaryType(Boundary::Codes code) const;
	void set_boundary_value(Boundary::Codes code, value_type a) { boundary_values[code] = a; };
	void set_domain_value(value_type a) { domain_value = a; }
	const VINT& size() const  {return l_size;};
// 	const VINT& size_shadow() const  {return shadow_size;};
	vector<VINT> optimizeNeighborhood(const vector<VINT>& ) const;

	void useBuffer(bool);
	const int shadow_base_width;
	bool setBuffer(const VINT& pos, typename TypeInfo<T>::Parameter val);
	void applyBuffer(const VINT& pos);
	void copyDataToBuffer();
	void swapBuffer();

protected:
	string name;
	valarray<value_type> data;
	value_type default_value, default_boundary_value, shit_value;
	
	bool using_buffer;
	valarray<value_type> write_buffer;
	
	bool using_domain;
	valarray<Boundary::Type> domain;

	shared_ptr<const Lattice> _lattice;
	uint dimensions;
	Lattice::Structure structure;
	VINT l_size;
	VINT shadow_size;  // size of the data grid including shadows
	uint shadow_size_size_xyz;  // number of data pints in the grid
	VINT shadow_offset; /// index steps when moving in x,y or z direction 
	VINT shadow_width;  /// shift between the lattice origin and the origin of the shadowed grid
	gslice s_xmb, s_xm ,s_xp, s_xpb, s_xall;
	gslice s_ymb, s_ym ,s_yp, s_ypb, s_yall;
	gslice s_zmb, s_zm ,s_zp, s_zpb, s_zall;
	
	gslice xslice(span a);
	gslice yslice(span a);
	gslice zslice(span a);
	
	
	bool has_reduction;
	Boundary::Codes reduction;

	uint get_data_index(const VINT& a) const;
	static bool less_memory_position(const VINT& a, const VINT& b);
	void allocate();
	
	const vector<Boundary::Type> boundary_types;
	valarray<value_type> boundary_values;
    value_type domain_value;
	void setDomain();
	void reset_boundaries();
	
	friend class Domain;
	friend class LatticeStencil;
	friend class StatisticalLatticeStencil;
	friend class InteractionEnergy;  // Neighborhood per node -- not using the stencil implementation
	friend class MembraneMapper;
	friend class MembraneRules3D;
	friend class InitVoronoi;
};

#endif

