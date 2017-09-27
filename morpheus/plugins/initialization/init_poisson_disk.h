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

#ifndef INITPOISSONDISK_H
#define INITPOISSONDISK_H

#include "core/interfaces.h"
#include "core/plugin_parameter.h"

/** \defgroup InitPoissonDisk
\ingroup ML_Population
\ingroup InitializerPlugins
\brief Arranges cells approximately equidistantly according to Poisson Disk Sampling

\section Description
Arranges approximately equidistance cells according to Poisson Disk Sampling

If \b mode is 'regular'. Cells are seeded randomly, or in a regular structure. 
In case of a regular structure, deviations from this regularity can be induced by using a uniform random offset.

The \b Dimensions tag determines the origin of the left lower corner and the size of the region.


\section Example
\verbatim

\endverbatim
*/

class InitPoissonDisk : public Population_Initializer
{
private:
	
	PluginParameter2<double, XMLEvaluator, RequiredPolicy> min_dist;
	PluginParameter2<double, XMLEvaluator, OptionalPolicy> max_dist;
	PluginParameter2<int, XMLValueReader, DefaultValPolicy> new_points_count;
	
	CellType* celltype;
	vector<VINT> nbh;
    double cellSize;
    bool cubic;
	shared_ptr<const Lattice> lattice;
    shared_ptr< Lattice_Data_Layer< double > > grid_x;
    shared_ptr< Lattice_Data_Layer< double > > grid_y;
    shared_ptr< Lattice_Data_Layer< double > > grid_z;

    bool addPointToGrid(VDOUBLE point);
	bool createCell(VINT newPos);
    VINT imageToGrid(VDOUBLE p);
	//shared_ptr< Lattice_Data_Layer< double > > createGrid(shared_ptr<const Lattice> lattice, double default_value);
    shared_ptr< Lattice_Data_Layer< double > > createGrid( VINT boxsize, Lattice::Structure structure, double default_value);
	
    VDOUBLE generateRandomPointAround(VDOUBLE point, double min_dist, double maxdist);
	bool inRectangle(VDOUBLE point, VINT latticeSize);
    bool noNeighbors(VDOUBLE point, double mindist);

public:
	InitPoissonDisk();
	DECLARE_PLUGIN("InitPoissonDisk");
	bool run(CellType* ct) override;
};

#endif
