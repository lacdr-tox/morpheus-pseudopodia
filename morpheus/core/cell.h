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

#include <iterator>
#include <numeric>
#include <limits>
#include <vector>
#include <map>
#include <list>
#include <set>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <complex>
#include "Eigen/Eigenvalues"

// #include "simulation.h"
#include "interfaces.h"
#include "cpm_layer.h"
#include "property.h"

class MembraneMapper;

struct EllipsoidShape{
	vector< VDOUBLE > axes;
	vector< double> lengths;
	double eccentricity;
	VDOUBLE center;
	uint volume;
	double timestamp;
};

class CellShape {
public:
	CellShape( const VDOUBLE& cell_center, const set< VINT, less_VINT >& cell_nodes );
	~CellShape();
	const EllipsoidShape& ellipsoidApprox();
	const PDE_Layer& sphericalApprox();
	void invalidate();

private:
	static const int LC_XX=0;
	static const int LC_XY=1;
	static const int LC_YY=2;
	static const int LC_XZ=3;
	static const int LC_YZ=4;
	static const int LC_ZZ=5;

	enum CachePos { ELLIPSOID, SPHERIC };
	valarray<bool> valid_cache;

	EllipsoidShape ellipsoid;
	EllipsoidShape calcLengthHelper3D(const std::vector<double> &I, int N) const;
	EllipsoidShape calcLengthHelper2D(const std::vector<double> &I, int N) const;

	MembraneMapper* spherical_mapper;

	const VDOUBLE& center; /// Reference to the cell center in orthoganal coordinates
	const set<VINT, less_VINT>& nodes; /// Reference to the cell nodes in lattice coordinates
// 	shared_ptr<const Lattice> lattice;
};



/// Interface for all cells, implements basic platform integration
class Cell
{
public:
	typedef set<VINT, less_VINT> Nodes;
// 	typedef unordered_set<VINT,VINThash> Nodes;
	
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
	virtual void setUpdate(const CPM::UPDATE& update, CPM::UPDATE_TODO todo);
	virtual void applyUpdate(const CPM::UPDATE& update, CPM::UPDATE_TODO todo);  ///< apply the requested changes to the cell (either add_state or remove_state are NULL)
	void applyNeighborhoodUpadate(const CPM::UPDATE& update, uint count); 

	CPM::CELL_ID getID() const { return id; };                                 ///< ID that represents the cell in the cpm lattice
	uint getName() const { return id; };                               ///< a unique name that remains constant, no matter of proliferation or cell death or differentiation. It is unique for the whole cpm.
	CellType* getCellType() const { return celltype; };
	VDOUBLE getCenter() const;                         ///< Center of the cell, i.e. nodes average. Values are always in orthoganal coordinates.
	VINT getCenterL() const;                           ///< Center of the cell, i.e. nodes average, in lattice coordinates. The value is always within the lattice size range, in particular under periodic boundary conditions.
	VDOUBLE getCenterOfMassPeriodic() const ;				///< Center of the cell on a periodic 2D lattice;
	VDOUBLE getUpdatedCenter() const  { return updated_center; }; ///< Projects the position of the cell center after executing the update operation given by the parameters.
	VDOUBLE getOrientation(void) ;					///< Gives orientation in radials (taking y-axis as reference), using elliptic approximation
// 	virtual projectCenter(update, todo);
	uint nNodes() const { return nodes.size(); };                                  ///< Number of nodes aka volume, area or whatsoever
	const Nodes& getNodes() const { return nodes; };                    ///< All nodes occupied by the cell. Note, this are lattice coordinates, which are not necessarily orthogonal ... 
	const Nodes& getUpdatedNodes() const { return updated_nodes; };
	const Nodes& getSurface() const; //__attribute__ ((deprecated));
	const Nodes& getUpdatedSurface() const { return updated_surface; } // __attribute__ ((deprecated));
	const map<CPM::CELL_ID,uint>& getInterfaces() const { return interfaces; }; /// List of interfaces with other cells. Note that the count is given in number of neighbors.
	std::map< CPM::CELL_ID, double > getInterfaceLengths() const; /// List of interfaces with other cells. Note that the counts are given in interface length (as getInterfaceLength()).
	double getInterfaceLength() const { return interface_length; };
	const map<CPM::CELL_ID,uint>& getUpdatedInterfaces() const { return updated_interfaces; };
	double getUpdatedInterfaceLength() const { return updated_interface_length; };;
	
	const vector< shared_ptr<AbstractProperty> >& properties;
	const vector< shared_ptr<PDE_Layer> >& membranes;
	void assignMatchingProperties(const vector< shared_ptr<AbstractProperty> > other_properties);
	void assignMatchingMembranes(const vector< shared_ptr<PDE_Layer> > other_membranes);
	
	void disableNodeTracking();
	bool isNodeTracking() const {return track_nodes;};
// 	void setSurfaceTracking(bool state);
	bool isSurfaceTracking() const { return track_surface; };

	const EllipsoidShape& getCellShape() const;
	double  getCellLength() const;
	VDOUBLE getMajorAxis() const;
	VDOUBLE getMinorAxis() const;
	double  getEccentricity() const;
	const PDE_Layer& getSphericalApproximation() const;
// 	int 	getCellVolume();
// 	VDOUBLE getCellCenter();
	
private:
//	bool setID( CPM::STATE );                   ///< Set the ID of a cell (should be never ever ever be used, except when necessary)
	mutable CellShape cell_shape;
	mutable double surface_timestamp;
	double neighbors2length;
// 	void updateSurface() const;

protected:
	const CPM::CELL_ID id;
	CellType* celltype;
	bool track_nodes, track_surface;

/** container that stores the nodes occupied by a cell.
 *  list<> allows constant time insertion and deletion at any position.
 *  however, set<> allows log(N) insert and find methods, which is effectively faster than 
 *  list version of find and insert.
 */
	CPM::UPDATE active_update;
	CPM::UPDATE_TODO active_todo;
	
	Nodes nodes, updated_nodes;                     /// Container storing all occupied nodes
	mutable Nodes surface, updated_surface;                   /// Container storing the nodes at the cell surface
	map<CPM::CELL_ID,uint> interfaces, updated_interfaces;            /// Container storing the length of interfaces with other cells
	double interface_length, updated_interface_length;
	
// 	uint number_of_nodes;
	VINT accumulated_nodes;
	VDOUBLE orth_center, updated_center;                         /// Current cell center in orthogonal coordinates
	VINT lattice_center, updated_lattice_center;                      /// Current cell center in lattice coordinates, within lattice size range.
	
	vector< shared_ptr<AbstractProperty> > p_properties;
	vector< shared_ptr<PDE_Layer> > p_membranes;
	
	void resetUpdatedInterfaces();
	
	friend class CellType;
};
/*
VDOUBLE long_cell_axis(const Cell::Nodes& points)   __attribute__ ((deprecated));            ///< calculates the long axis of a cloud of points based on the tensor of gyration.
double long_cell_axis2(const Cell::Nodes& nodes)	__attribute__ ((deprecated));			///< calculates the length of the long axis*/


#endif


