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

#ifndef CELL_H
#define CELL_H

// #include <iterator>
// #include <numeric>
// #include <limits>
#include <vector>
// #include <map>
// #include <list>
// #include <set>
// #include <algorithm>
#include <iostream>
#include <sstream>
// #include <complex>
// #include "Eigen/Eigenvalues"

// #include "simulation.h"
#include "interfaces.h"
// #include "cpm_layer.h"
#include "cpm_shape.h"
#include "cpm_shape_tracker.h"
#include "property.h"
/// Interface for all cells, implements basic platform integration
class Cell
{
public:
	typedef CPMShape::Nodes Nodes;
	
	Cell(CPM::CELL_ID cell_name, CellType* celltype);
	Cell( Cell& other_cell, CellType* celltype );  // this looks like a copy constructor
	~Cell();

	virtual XMLNode saveToXML() const;                  ///Save the cell state to an XMLNode
	virtual void loadFromXML(const XMLNode Node);       ///< Loading the cell state from an XMLNode
	void init();										/// Initialize the cell after attaching it to the celltype
	
/**
 * Reflect the changes in the cell due to an update in the cpm lattice
 * @param update provides the whole udate story
 * @param todo selects which changes contribute to the cell
 */
	virtual void setUpdate(const CPM::Update& update);
	virtual void applyUpdate(const CPM::Update& update);  ///< apply the requested changes to the cell (either add_state or remove_state are NULL)
// 	void applyNeighborhoodUpadate(const CPM::Update& update, uint count); 

	CPM::CELL_ID getID() const { return id; };                                 ///< ID that represents the cell in the cpm lattice
	uint getName() const { return id; };                               ///< a unique name that remains constant, no matter of proliferation or cell death or differentiation. It is unique for the whole cpm.
	CellType* getCellType() const { return celltype; };
	const AdaptiveCPMShapeTracker& currentShape() const { return shape_tracker.current(); }
	const AdaptiveCPMShapeTracker& updatedShape() const { return shape_tracker.updated(); }
	VDOUBLE getCenter() const { return SIM::lattice().to_orth(VDOUBLE(node_sum) / nodes.size() ); };                         ///< Center of the cell, i.e. nodes average. Values are always in orthoganal coordinates.
	VDOUBLE getCenterL() const { return VDOUBLE(node_sum) / nodes.size(); };                        ///< Center of the cell, i.e. nodes average, in lattice coordinates. The value is always within the lattice size range, in particular under periodic boundary conditions.
	VDOUBLE getUpdatedCenter() const  { return shape_tracker.updated().center(); }; ///< Projects the position of the cell center after executing the update operation given by the parameters.
	VDOUBLE getOrientation(void) const;					///< Gives orientation in radials (taking y-axis as reference), using elliptic approximation
// 	virtual projectCenter(update, todo);
	uint nNodes() const { return nodes.size(); };                                  ///< Number of nodes aka volume, area or whatsoever
	const Nodes& getNodes() const { return nodes; };                    ///< All nodes occupied by the cell. Note, this are lattice coordinates, which are not necessarily orthogonal ... 
// 	const Nodes& getUpdatedNodes() const { return updated_nodes; };
	const Nodes& getSurface() const { return shape_tracker.current().surfaceNodes();}
// 	const Nodes& getUpdatedSurface() const { return shape_tracker.updated().surfaceNodes(); } // __attribute__ ((deprecated));
// 	const map<CPM::CELL_ID,uint>& getInterfaces() const { return shape_tracker.current().interfaces(); }; /// List of interfaces with other cells. Note that the count is given in number of neighbors.
	std::map< CPM::CELL_ID, double > getInterfaceLengths() const { return shape_tracker.current().interfaces(); };; /// List of interfaces with other cells. Note that the counts are given in interface length (as getInterfaceLength()).
	double getInterfaceLength() const { return  shape_tracker.current().surface(); };
	double getUpdatedInterfaceLength() const { return shape_tracker.updated().surface(); };
	const map<CPM::CELL_ID,double>& getUpdatedInterfaceLengths() const { return shape_tracker.updated().interfaces(); };
	
	const vector< shared_ptr<AbstractProperty> >& properties;
	const vector< shared_ptr<PDE_Layer> >& membranes;
	void assignMatchingProperties(const vector< shared_ptr<AbstractProperty> > other_properties);
	void assignMatchingMembranes(const vector< shared_ptr<PDE_Layer> > other_membranes);
	
	bool isNodeTracking() const {return track_nodes;};
	void disableNodeTracking();
	bool isShapeTracking() const { return track_shape; };
	void setShapeTracking(bool state);

	const EllipsoidShape& getEllipsoidShape() const;
	double  getCellLength() const;
	VDOUBLE getMajorAxis() const;
	VDOUBLE getMinorAxis() const;
	double  getEccentricity() const;
	const PDE_Layer& getSphericalApproximation() const;
	
protected:
	const CPM::CELL_ID id;
	CellType* celltype;
	bool track_nodes, track_shape;
	Nodes nodes;                /// Container storing all occupied nodes
	VINT node_sum;
	CPMShapeTracker shape_tracker;

/** container that stores the nodes occupied by a cell.
 *  list<> allows constant time insertion and deletion at any position.
 *  however, set<> allows log(N) insert and find methods, which is effectively faster than 
 *  list version of find and insert.
 */
	vector< shared_ptr<AbstractProperty> > p_properties;
	vector< shared_ptr<PDE_Layer> > p_membranes;
	
// 	void resetUpdatedInterfaces();
	
	friend class CellType;
};
/*
VDOUBLE long_cell_axis(const Cell::Nodes& points)   __attribute__ ((deprecated));            ///< calculates the long axis of a cloud of points based on the tensor of gyration.
double long_cell_axis2(const Cell::Nodes& nodes)	__attribute__ ((deprecated));			///< calculates the length of the long axis*/


#endif


