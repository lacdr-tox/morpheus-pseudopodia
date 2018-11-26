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
#include "core/celltype.h"

#define POISSON_PROGRESS_INDICATOR 1
#include "../3rdparty/poisson-disc-generator/PoissonGenerator.h"

/** \defgroup InitPoissonDisc
\ingroup ML_Population
\ingroup InitializerPlugins
\brief Arranges cells approximately equidistantly according to Poisson Disk Sampling

\section Description
Arranges approximately equidistance cells according to Poisson Disk Sampling using Robert Bridson’s algorithm.
Attempts to arrange \b number-of-cells but will generate less cells.

Only applicable to 2D lattices with square structure.

Implementation from: https://github.com/corporateshark/poisson-disk-generator
\section Example

Try to initialize 500 cells in the given lattice:
\verbatim
<InitPoissonDisc number-of-cells="500"/>
\endverbatim
*/

class InitPoissonDisc : public Population_Initializer
{
private:

	PluginParameter2<double, XMLEvaluator, RequiredPolicy> num_cells;
	CellType* celltype;
	CPM::CELL_ID createCell(VINT newPos);
	shared_ptr<const Lattice> lattice;

/*
	PluginParameter2<double, XMLEvaluator, OptionalPolicy> max_dist;
	PluginParameter2<int, XMLValueReader, DefaultValPolicy> new_points_count;

	CellType* celltype;
	Neighborhood nbh;
    double cellSize;
    bool cubic;
	shared_ptr<const Lattice> lattice;
    shared_ptr< Lattice_Data_Layer< double > > grid_x;
    shared_ptr< Lattice_Data_Layer< double > > grid_y;
    shared_ptr< Lattice_Data_Layer< double > > grid_z;

    bool addPointToGrid(VDOUBLE point);
	CPM::CELL_ID createCell(VINT newPos);
    VINT imageToGrid(VDOUBLE p);
	//shared_ptr< Lattice_Data_Layer< double > > createGrid(shared_ptr<const Lattice> lattice, double default_value);
    shared_ptr< Lattice_Data_Layer< double > > createGrid( VINT boxsize, Lattice::Structure structure, double default_value);

    VDOUBLE generateRandomPointAround(VDOUBLE point, double min_dist, double maxdist);
	bool inRectangle(VDOUBLE point, VINT latticeSize);
    bool noNeighbors(VDOUBLE point, double mindist);
*/

public:
	InitPoissonDisc();
	DECLARE_PLUGIN("InitPoissonDisc");
	vector<CPM::CELL_ID> run(CellType* ct) override;
};

#endif
