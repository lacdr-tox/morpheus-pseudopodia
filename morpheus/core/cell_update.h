#ifndef CELL_UPDATE_H
#define CELL_UPDATE_H

#include "cpm_layer.h"
#include "symbolfocus.h"


namespace CPM {
	/** @brief Stores details of an update in the cellular potts model.
	 */
		
	struct UpdateData {
		class SymbolFocus focus, source, focus_updated;
		STATE add_state;
		STATE remove_state;
		shared_ptr<LatticeStencil> update;
		shared_ptr<LatticeStencil> surface;
		shared_ptr<StatisticalLatticeStencil> boundary;
		uint source_top_ct;
		uint focus_top_ct;
	}; 
	
	/** @brief Enumerator type used to traverse update tasks through the cell population interfaces.
	 *
	 *  Use (todo & CPM::ADD) or (todo & CPM::REMOVE) to find out if a method shall take care of add or remove operation respectively. 
	 *  Use (todo == CPM::ADD_AND_REVERSE) to find out if a method are responsible for both.
	 */
	
	
	
	
	class Update {
	public:
		Update(UpdateData* data, shared_ptr<CPM::LAYER> layer);
		enum class Operation { Extend, Move };
		enum AtomicOperation { NONE=0x0, ADD=0x1, REMOVE=0x2,  ADD_AND_REMOVE=0x3, MOVE=0x4, NEIGHBORHOOD_UPDATE=0x8 };
		bool valid() const { return operation!=NONE; };
		class SymbolFocus source() const { return d->source; };
		class SymbolFocus focus() const { return d->focus; };
		class SymbolFocus focusUpdated() const{ return d->focus_updated; };
		
		const STATE& focusStateBefore() const { return d->remove_state; };
		const STATE& focusStateAfter() const { return d->add_state; };
		
		// NeighborhoodStencils
		
		/// Neighborhood used for the update operation selection, order is angualar
		shared_ptr< const LatticeStencil> updateStencil() const { return d->update; };
		/// Neighborhood used to determine surface nodes, as used for the connectivity constraint, order is angular
		shared_ptr< const LatticeStencil> surfaceStencil() const { return d->surface; };
		/// Neighborhood used to compute anything derived from shape boundary length.
		/// Most prominently the Interactions, Perimeter, Perimeter constraints, 
		shared_ptr< const StatisticalLatticeStencil> boundaryStencil() const { return d->boundary; };
		
		int op() const { return operation; };
		/// Operation includes addition of Node focus to Cell of focusStateAfter()
		bool opAdd() const { return operation & ADD; };
		/// Operation includes removal of Node focus from focusStateBefore()
		bool opRemove() const { return operation & REMOVE; };
		/// Operation includes move of Node 
		bool opMove() const { return operation & MOVE; };
		/// Operation just notifies about the update in the Neighborhood
		bool opNeighborhood() const { return operation & NEIGHBORHOOD_UPDATE; };
		/// Select an atomic suboperation of the update.
		const Update selectOp(AtomicOperation op) const;
		const Update removeOp(AtomicOperation op) const;
		
		
		/// Set the update to perform operation @p opx from @p source to @p dest
		void set(VINT source, VINT direction, Update::Operation opx);
		/// Set the update to place a certain @p cell at position @p dest
		void set(VINT dest, CELL_ID cell);
		
		void unset() { operation=NONE; };
		
	private:
		UpdateData* d;
		shared_ptr<CPM::LAYER> layer;
		int operation;
	};
	
	inline bool operator == (const CPM::Update &a,const CPM::Update &b ) {
		return (a.focusStateBefore() == b.focusStateBefore() && a.focusStateAfter() == b.focusStateAfter() && a.source().pos() == b.source().pos() &&  a.focus().pos() == b.focus().pos());
	}
	
	inline ostream& operator <<(ostream& os, const CPM::Update& n) { os << n.focusStateBefore() << " | " << n.focusStateAfter() << " | " << n.focus().pos() << " | " << n.source().pos() << endl; return os;}

	
}


#endif