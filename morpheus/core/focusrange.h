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

#ifndef CONTEXT_H
#define CONTEXT_H


#include <iterator>
#include <stdexcept>
#include <vector>
#include <map>
#include "symbolfocus.h"
#include "symbol.h"
#include "celltype.h"
// #include "membrane_pde.h"
// #include "simulation.h"

/* Usage :
 * 
 * EXAMPLE: Equation code::
 * 
 * SymbolAccessor<T> sym = SIM::findSymbol() { SIM::getCurrentScope().findSymbol() }
 * 
 * 

 * 
 * range = FocusRange(sym);
 * 
 * #pragma openmp parallel for
 * for (SymbolFocus& focus: range) {
 *    sym.set(focus, function(focus));
 * }
 * 
 * 
 */

class FocusRange; 
enum class FocusRangeAxis; 
class FocusRangeDescriptor {
public:
// 	FocusRangeDescriptor() : spatial_restriction(RESTR_GLOBAL), granularity(SymbolData::UndefGran), domain_enumeration(SIM::lattice().getDomain().domain_enumeration() ){};
	enum SpatialRestriction { RESTR_GLOBAL, RESTR_DOMAIN, RESTR_CELLPOP , RESTR_CELL};
	SpatialRestriction spatial_restriction;
	Granularity granularity;
	enum IterationMode { IT_Cell, IT_CellNodes, IT_CellSurfaceNodes, IT_CellNodes_int, IT_Space, IT_CellMembrane, IT_Domain, IT_Domain_int };
	IterationMode iter_mode;
	vector<CPM::CELL_ID> cell_range;
	vector<uint> cell_sizes;
	VINT pos_range;
	VINT pos_offset;
	int size;
	vector<int> sizes;
	int c_div, x_div, y_div, z_div;
	const vector<VINT>* domain_enumeration;
	vector<VINT> domain_nodes_int;
	vector< const Cell::Nodes* > cell_nodes;
    vector< Cell::Nodes > cell_nodes_int;
	vector<FocusRangeAxis> data_axis;
	set<FocusRangeAxis> spatial_dimensions;
 
};

class FocusRangeIterator : public std::iterator<random_access_iterator_tag, SymbolFocus, int> 
{
public:
	FocusRangeIterator() : data(NULL), idx(0) { /* TODO : you are invalid! -> data == NULL means empty range */}
	FocusRangeIterator(const FocusRangeIterator& other) : FocusRangeIterator(other.data, other.idx) {}

	FocusRangeIterator& operator++();
	FocusRangeIterator& operator--() {setIndex(idx-1); return *this; };
	FocusRangeIterator& operator+=(int n) { if (n==1) return operator++(); setIndex(idx+n); return *this;}
	FocusRangeIterator& operator-=(int n) { setIndex(idx-n); return *this; }
	FocusRangeIterator operator+(int n) const  { return FocusRangeIterator(data,idx+n); }
	FocusRangeIterator operator-(int n) const  { return FocusRangeIterator(data,idx-n); }
	int operator-(const FocusRangeIterator& other) const  { return idx-other.idx; }
	bool operator==(const FocusRangeIterator& other) const { return idx == other.idx; };
	bool operator!=(const FocusRangeIterator& other) const { return idx != other.idx; };
	bool operator<(const FocusRangeIterator& other)  const { return idx <  other.idx; };
	bool operator<=(const FocusRangeIterator& other) const { return idx <= other.idx; };
	bool operator>(const FocusRangeIterator& other)  const { return idx >  other.idx; };
	bool operator>=(const FocusRangeIterator& other) const { return idx >= other.idx; };
	const SymbolFocus& operator*() const {
		if (!data)
			throw std::out_of_range("out of FocusRange");
		return focus;
	}
	
	const SymbolFocus* operator->() const {
		if (!data)
			return NULL;
		return &focus;
	}
	
// 	vector<int> dataIndex() const;
	
private:
	FocusRangeIterator(shared_ptr<const FocusRangeDescriptor> data, uint index = 0);
	
	void setIndex(int index);

	// Range Descriptor
	shared_ptr<const FocusRangeDescriptor> data;
	
	///State
	uint idx; uint cell; VINT pos; 
	Cell::Nodes::const_iterator current_cell_node;
	
	SymbolFocus focus;
	friend FocusRange;
// 	friend FocusRangeIterator operator+(int n, const FocusRangeIterator& iter);
// 	friend FocusRangeIterator operator-(int n, const FocusRangeIterator& iter);
};

// enum class FocusRangeRestriction {
// 	Xaxis, Yaxis, Zaxis, CellID, MemXaxis, MemYaxis
// };
	
// {
// public:
// 	enum RestrictionType { Xaxis, Yaxis, Zaxis, CellID, MemXaxis, MemYaxis };
// 	Restriction(RestrictionType type, double value);
// 	RestrictionType getType() { return type; }
// 	double getValue()
// 	
// private: 
// 	RestrictionType type;
// 	double value;
// };

class FocusRange {
public:
	
	/// Create a FocusRange with the given element granularity, restricted to the range of scope provided
	FocusRange(Granularity granularity, const Scope* scope );
	/// Create a FocusRange with the given element granularity, restricted to the cell provided
	FocusRange(Granularity granularity, CPM::CELL_ID cell_id );
	/// Create a FocusRange with the given element granularity, restricted by the provided restrictions map. 
	/// writable_only discards the range out side of a domain, if the lattice is restricted by a domain.
	FocusRange(Granularity granularity = Granularity::Undef, multimap<FocusRangeAxis,int> restrictions = {}, bool writable_only = true );
	
	/// Create a FocusRange through all valid elements, restricted to the range of scope provided
	template <class T, template <class> class AccessPolicy>
	FocusRange(const SymbolAccessorBase<T,AccessPolicy>& sym, const Scope* scope = nullptr) { 
		auto restrictions = sym.getRestrictions();
		if (scope && scope->getCellType()) {
			restrictions.erase(FocusRangeAxis::CellType);
			restrictions.insert( make_pair(FocusRangeAxis::CellType,scope->getCellType()->getID()) );
		}
		init_range(sym.getGranularity(), restrictions, true);
	};
	
	/// Create a FocusRange constrained to the spatial domain of a cell
	template <class T, template <class> class AccessPolicy>
	FocusRange(const SymbolAccessorBase<T,AccessPolicy>& sym, CPM::CELL_ID cell_id) :  FocusRange(sym.getGranularity(), sym.getScope(), cell_id) {};

	size_t size() { if (!data) return 0; else return data->size; };
	SymbolFocus operator[] (size_t index) {
		return *FocusRangeIterator(data,index);
	}
	FocusRangeIterator begin() const { return FocusRangeIterator(data, 0); };
	FocusRangeIterator end() const { return FocusRangeIterator(data, data ? data->size : 0); };

	/// Number of dimensions
	int dimensions() const { if (!data) return 0; else return data->data_axis.size(); };
	/// If the data is organized like a multidimensional array, isRegular returns true
	bool isRegular() const {
		if (!data) throw string("Invalid FocusRange");
		return data->iter_mode != FocusRangeDescriptor::IT_Domain 
		    && data->iter_mode != FocusRangeDescriptor::IT_CellNodes
		    && data->iter_mode != FocusRangeDescriptor::IT_Domain_int
		    && data->iter_mode != FocusRangeDescriptor::IT_CellNodes_int; 
	}
	/// Iteration Axis of the range
	const vector<FocusRangeAxis>& dataAxis() const { if (!data) throw string("Invalid FocusRange"); return data->data_axis;};
	/// Lengh of the Axis of the range
	const vector<int>& dataSizes() const { if (!data) throw string("Invalid FocusRange"); return data->sizes; };
	const vector<CPM::CELL_ID>& cells() const { /*if (data->data_axis[0] == FocusRangeAxis::CELL)*/ return data->cell_range; /*else return vector<CPM::CELL_ID>();*/ }; 
	/// Spatial extend of the focus range
	const set<FocusRangeAxis>& spatialExtends() const { if (!data) throw string("Invalid FocusRange"); return data->spatial_dimensions; }

private:
	shared_ptr<const FocusRangeDescriptor> data;
	void init_range(Granularity granularity, multimap<FocusRangeAxis,int> restrictions, bool writable_only);
};

#include "interfaces.h"

#endif
