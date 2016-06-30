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

#ifndef SUPER_CELL_H
#define SUPER_CELL_H


#include "cell.h"
#include <set>
class SuperCT;
// #ifndef CPM_SEGMENTED_CELLS
// 	#error "CPM_SEGMENTED_CELLS needs to be enabled through ccmake prior to using segmented cells!" 
// #endif

///* Basic segmented cell
///
class SuperCell : public Cell // public CellPopulation
{
	private: 
// 		SuperCT* super_celltype;
	protected:
		vector<VDOUBLE> centers, updated_centers;
		map<CPM::CELL_ID , uint> sub_cell_pos;
		vector<CPM::CELL_ID >  sub_cells;
// 		vector<AbstractProperty*> segment_properties_template;
	public:
		SuperCell ( CPM::CELL_ID cell_name, SuperCT* celltype);
		~SuperCell();

		virtual XMLNode saveToXML () const;
		virtual void loadFromXML ( const XMLNode Node );
                                      

// 		virtual Nodes getNodes() const;
/*	Those methods can be directly reused from the cell implementation
		virtual const Cell::Nodes& getSurface() const;
		virtual const Cell::Nodes& getUpdatedSurface() const;
		virtual const map<CPM::STATE,uint>& getInterfaces() const;
		virtual const map<CPM::STATE,uint>& getUpdatedInterfaces() const*/;
		uint getSubCellPosition(CPM::CELL_ID cell_id) const { 
			map <CPM::CELL_ID , uint >::const_iterator iter = sub_cell_pos.find(cell_id);
			if (iter == sub_cell_pos.end()) {
				cout << "Missing subcell " << cell_id << endl;
				for (iter= sub_cell_pos.begin(); iter != sub_cell_pos.end(); iter++) cout << iter->first << " => " << iter->second << " | ";
			}
			assert(iter != sub_cell_pos.end());
			return iter->second;
		}
		const vector<CPM::CELL_ID >& getSubCells() const { return sub_cells; }        ///< the cell segments, which are internally nothing but cpm cells.
		const vector<VDOUBLE> & getSubCenters() const { return centers; };      ///< centers of the subcells, in orthogonal coordinates
		const vector<VDOUBLE> & getUpdatedSubCenters() const { return updated_centers; };      
		CPM::CELL_ID addSubCell( CPM::CELL_ID  cell_id );
		void removeSubCell( CPM::CELL_ID  cell_id );
		
		/**  @brief Set the intermediate cell information (prefixed updated_)
		* 
		*   Here, the platform assures that the subcell already received the notifier for setUpdate (SuperCT::setUpdate).
		*   Thus, all subcell information is already up to date !
		*/
		virtual void setUpdate(const CPM::Update & update);
		/**  @brief Apply the update on the cell
		* 
		*   Here, the platform assures that the subcell already received the notifier for applyUpdate (SuperCT::applyUpdate).
		*   Thus, all subcell information is already up to date !
		*/
		virtual void applyUpdate ( const CPM::Update& update);
// 		void splitLastSegment(VDOUBLE split_plane_normal);
};

#endif // SEGMENTED_CELL_H
