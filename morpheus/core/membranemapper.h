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

#ifndef MEMBRANEMAPPER_H
#define MEMBRANEMAPPER_H

#include "simulation.h"
#include "membrane_property.h"
#include "cell_membrane_accessor.h"
#include "Eigen/Eigenvalues"


class MembraneMapper
{
public:
	/** Type of values that should be mapped.
	 *  MAP_BOOLEAN means you report 0 or 1. In case of concurrency, the majority votes, or is eventually selected by chance.
	 *  MAP_DISCRETE means you report discrete values, that cannot be interpolated. In case of concurrency, one is chosen by chance.
	 *  MAP_CONTINUOUS means you report continuous data that can be interpolated.
	 **/

	enum MODE { MAP_DISCRETE, MAP_BOOLEAN, MAP_CONTINUOUS, MAP_DISTANCE_TRANSFORM };
private:
	shared_ptr<PDE_Layer> data_map, accum_map, distance_map;
// 	shared_ptr<MembraneMapper> shape_mapper;
	bool use_shape;
	const PDE_Layer* cell_shape;
	VDOUBLE cell_center;
	CPM::CELL_ID cell_id;
	bool valid_cell;
	MODE mapping_mode;
	static const double no_distance;

	shared_ptr<const Lattice> membrane_lattice;
	shared_ptr<const Lattice> global_lattice;

	vector< Eigen::Matrix3f > rotation_matrices;
	
public:
	MembraneMapper(MembraneMapper::MODE mode, bool use_cell_shape = false);
	/// Attach to cell @ cell_id. This resets all data stored so far.
	void attachToCell(CPM::CELL_ID cell_id);
	void attachToCenter(VDOUBLE center);

	/// Map global position @pos to the membrane lattice and set particular value @v.
	void map( const VINT& pos, double v); /// Assumes pos to be in the global Lattice coordinates
	/// Map global position @pos to the membrane lattice and set particular value @v.
	void map( const VDOUBLE& pos, double v); /// Assumes pos to be in orthogonal coordinates
	/// Set value @v at position @membrane_pos of the membrane.
	void set( const VINT& membrane_pos, double v ); /// Assumes pos to be in orthogonal coordinates
	/// Normalize all information reported into the membrane and fill all gaps that have been left during reporting
	void fillGaps();

	/// Converts position in 3D space into position in theta/phi space of 2D membrane 
	VINT getMembranePosition( const VDOUBLE& pos_3D );
	
	/// Set rotation matrix to rotate the membrane map relative to the 3D Cartesian coordinate system
	void setRotationMatrix( Eigen::Matrix3f rotation_matrix );
	/// Clear the set of rotation matrices
	void resetRotationMatrices( void );
	
	
	void ComputeDistance();
	
	void flatten();
	/// Accessor
	MembraneMapper::MODE mode() { return mapping_mode; };

	/// The data being reported
	const PDE_Layer& getData();
	void copyData(PDE_Layer* membrane);
	/// The amount of data being reported
	const PDE_Layer& getAccum();
	void copyAccum(PDE_Layer* membrane);
	/// The distance matrix created during fillGaps()
	const PDE_Layer& getDistance();  /// Distance from information source ...
	void copyDistance(PDE_Layer* membrane);
};

#endif // MEMBRANEMAPPER_H
