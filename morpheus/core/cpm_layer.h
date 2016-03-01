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

#ifndef LATTICE_STENCIL
#define LATTICE_STENCIL

#include "lattice.h"
#include "lattice_data_layer.h"

class LatticeStencil;

namespace CPM {


	typedef unsigned int CELL_ID;
	
	enum CELL_INDEX_STATE {
		REGULAR_CELL, SUPER_CELL, SUB_CELL, NO_CELL=99999, VIRTUAL_CELL=100000
	};
	
	/**
	 * @brief CPM::STATE represents the information hold in the cpm lattice and identifies a cell that occupies the node.
	 *
	 * In addition, @p pos stores the position of a node in the coordinate system of 
	 * the cell. Calculating the cell center depends on that coordinate system.
	 */
	struct STATE { 
		CELL_ID cell_id;
		CELL_ID super_cell_id;
		VINT pos;
	};
	
	/// @brief Stores the association of a cell, which might change over time
	struct INDEX {
		uint celltype;
		CELL_INDEX_STATE status;
		CELL_ID super_cell_id;
		uint super_celltype;
		uint sub_cell_id;
	};
	
// 	/** @brief Represents a cursor for the retrieving spatial/temporal information in a simulation
// 	 *
// 	 * Resolving symbols during a simulation often demands to determine the context -- a cell or a position or ...
// 	 * Finally, it might be more efficient to just use a single object to fill that gap, which might slenderize the interface for symbol retrieval.
// 	*/
// 	class focus {
// 	public:
// 		focus();
// 		void set(VINT pos);
// 		void set(CPM::CELL_ID cellid);
// 		
// 		CPM::CELL_ID cellid() const;
// 		uint celltype() const;
// 		const VINT& pos() const;
// 		bool writable();
// 	private:
// 		mutable bool have_pos, have_celltype, have_cellid;
// 		mutable VINT cached_pos; uint cached_celltype; CPM::CELL_ID cached_cellid;
// 		const VINT& getPos();
// 		const VINT& getCellID();
// 		const VINT& getCellType();
// 	};
// 	
	/** 
	 * Comparison operator that returns true in the case that both states denote the same cpm cell, false otherwise. 
	 * The given position does not matter.
	 */
	inline bool operator ==(const CPM::STATE &a, const CPM::STATE &b)
	{
		return ( a.cell_id == b.cell_id && a.super_cell_id == b.super_cell_id);
	}
	


	/**
	 * Comparison operator that returns true in the case that both states do not denote the same cpm cell, false otherwise.
	 * The given position does not matter.
	 */
	inline bool operator !=(const CPM::STATE &a,const CPM::STATE &b)  
	{
		return ( a.cell_id != b.cell_id ); 
	}

	inline bool operator <(const CPM::STATE &a,const CPM::STATE &b)
	{
		return (a.cell_id < b.cell_id);
	}
	
	ostream& operator <<(ostream& os, const CPM::STATE& n);         /// Output operator, exclusively used to save values of boundary conditions
	
	typedef Lattice_Data_Layer<CPM::STATE> LAYER;
	
}

/** @brief Extractes Information of a Neighborhood in terms of (CPM::CELL_ID - count) statistics in an efficient way, respecting boundary conditions
 * 
 *  The stencil neighborhood is arbitrary order, sorted for optimal memory access patterns
 */
class StatisticalLatticeStencil  {
	public:
		struct STATS { CPM::CELL_ID cell; uint count; };
		StatisticalLatticeStencil( shared_ptr< const CPM::LAYER > data_layer, const std::vector< VINT >& neighbors );
		void setPosition(VINT pos);
		const vector<VINT>& getStencil() const { return stencil_neighbors; };
		const vector<StatisticalLatticeStencil::STATS>& getStatistics() const { return stencil_statistics; };
		const vector<CPM::CELL_ID>& getStates() const { return stencil_states; };
		
	private:
		void setStencil(const std::vector< VINT >& neighbors);

		shared_ptr< const CPM::LAYER > data_layer;
		vector<VINT> stencil_neighbors;
		vector<int> stencil_offsets;
		vector<int> stencil_row_offsets;
		vector <STATS> stencil_statistics;
		vector<CPM::CELL_ID> stencil_states;
};

class LatticeStencil {
	public:
		
		LatticeStencil( shared_ptr< const CPM::LAYER > data_layer, const std::vector< VINT >& neighbors );
		void setPosition(VINT pos);
		const vector<VINT>& getStencil() const { return stencil_neighbors; };
		const vector<CPM::CELL_ID>& getStates() const { return stencil_states; };
		
	private:
		void setStencil(const std::vector< VINT >& neighbors);
		
		shared_ptr< const CPM::LAYER > data_layer;
		vector<VINT> stencil_neighbors;
		vector<int> stencil_offsets;
		vector<CPM::CELL_ID> stencil_states;
};

#endif //LATTICE_STENCIL

