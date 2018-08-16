#ifndef CPM_H
#define CPM_H

/**
 
 
 
 */ 

#include "cpm_layer.h"
#include "cell_update.h"

class EdgeTrackerBase;

namespace CPM {
	
	
	/// Is a CPM model
	bool isEnabled();
	/// Duration of a Monte Carlo Step
	double getMCSDuration();
	// Temperature for Metropolis Kinetics
	// 	double getTemperature();
    
	/**  
	 * Returns shared pointer to the CPM date layer
	 * Modifying the layer is only possible through setNode and executeCPMUpdate
	 */
	shared_ptr<const CPM::LAYER> getLayer();
	
	/// Get the CPM state at position @pos
	const CPM::STATE& getNode(const VINT& pos);
	/// Get the "empty" CPM state, representing the medium
	const CPM::STATE& getEmptyState();
	/**
	 * Get the position of a random empty node in the sub-lattice provided by the 
	 * range @param min , @param mx. If no range is provided, the whole lattice
	 * is parsed.
	 * This method throws an exception when it is unable to find an empty node
	 * in the given range
	 */
	VINT findEmptyNode(VINT min = VINT(0,0,0) , VINT max = VINT(0,0,0));
	
	/// Set CPM state at @position to be occupied by cell @cell_id
	bool setNode(VINT position, CELL_ID cell_id);

	/**
	 * Create an Update encoding the operation described by @sourece, @direction, @opx
	 * 
	 * No actual action is performed
	 * 
	 * Interface used by the MonteCarloSampler
	 */
	const CPM::Update& createUpdate(VINT source, VINT direction, Update::Operation opx);
	
	/**
	 * Set an Update to be current
	 * 
	 * Notifies the cells involved, such that they provide proper information in the Cell::updatedShape()
	 * and Cell::updatedInterfaces() slots.
	 * 
	 * Cell_Update_Listener plugins of the cells involved are notified in the set_update_notify() slot
	 * Interface used by the MonteCarloSampler
	 */
	void setUpdate(CPM::Update& update);
	
	/**
	 * Execute an Update
	 * 
	 * Cell_Update_Listener plugins of the cells involved are notified in the update_notify() slot
	 */
	bool executeCPMUpdate(const CPM::Update& update);
	
	/// Get an index of cached information about cell @cell_id
	const CPM::INDEX& getCellIndex(const CELL_ID cell_id);
	
	/// Get the Cell object associated with @cell_id
	const Cell& getCell(CELL_ID cell_id);
	/// Check wether a Cell object is associated with @cell_id
	bool cellExists(CELL_ID cell_id);
	
	/** 
	 * Get the array of celltypes
	 * 
	 * Indices in the vector correspond to the celltype ids
	 * Usually you should not store a shared_ptr to the celltypes, since they should not be owned by plugins (actually, it's the other way round)
	 */
	vector< weak_ptr<const CellType> > getCellTypes();
	
	/** 
	 * Get a name -> celltype map
	 * 
	 * Usually you should not store a shared_ptr to the celltypes, since they should not be owned by plugins (actually, it's the other way round)
	 */
	map<string, weak_ptr<const CellType> > getCellTypesMap();
	
	/// Id of the "Empty" celltype
	uint getEmptyCelltypeID();
	/// The "Empty" celltype. Don't store the shared_ptr
	weak_ptr<const CellType> getEmptyCelltype();
	/// Find the celltype named @p name. Don't store the shared_ptr
	weak_ptr<const CellType> findCellType(string name); 
	
	/// Set a cell's celltype
	CELL_ID setCellType(CELL_ID cell_id, uint celltype);
	
	/// Manually enable edge tracking. Usually this is done automatically.
	void enableEgdeTracking();
	
	/// Get the current edgeTracker
	shared_ptr<const EdgeTrackerBase> cellEdgeTracker();
	
	/// 
	bool isSurface(const VINT& pos);
	uint nSurfaces(const VINT& pos);
	void setInteractionSurface(bool enabled = true);
	const Neighborhood& getBoundaryNeighborhood(); /// Returns the Neighborhood of a node boundary, sorted counterclockwise
	const Neighborhood& getSurfaceNeighborhood(); /// Returns the Neighborhood, that designates a node to be surface node, sorted counterclockwise
	
}


#endif

