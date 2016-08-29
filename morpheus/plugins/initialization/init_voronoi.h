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

#ifndef INITCELLLATICE_H
#define INITCELLLATICE_H

#include "core/interfaces.h"
#include "core/plugin_parameter.h"

/** \defgroup InitVoronoi
\ingroup InitializerPlugins
\brief Compute cell areas according to the Voronoi tesselation

Computes the Voronoi tesselation of the empty areas and sets cell IDs according to this tesselation. Note that cell positions have to be specified beforehand (with some other plugin is specified earlier).

- Assumes cell positions have already been initialized.
- Only uses non-occupied lattice nodes.
- Respects \ref Domain

\section Example
\verbatim
<InitVoronoi />
\endverbatim
*/


class InitVoronoi : public Population_Initializer
{
private:

	static const float no_distance;
	static const float no_label;
	Neighborhood neighbors;
	vector<double> neighbor_distance;
	int euclideanDistanceTransform( shared_ptr<Lattice_Data_Layer<double>>& distanceMap, shared_ptr<Lattice_Data_Layer<double> >&maskMap);
	int euclideanDistanceTransform( shared_ptr<Lattice_Data_Layer<double>>& distanceMap, shared_ptr<Lattice_Data_Layer<double> >&maskMap, VINT bottomleft, VINT topright);
	int voronoiLabelling( shared_ptr<Lattice_Data_Layer<double> >& distanceMap, shared_ptr<Lattice_Data_Layer<double> >&maskMap, shared_ptr<Lattice_Data_Layer<double> >& labelMap);
	int voronoiLabelling( shared_ptr<Lattice_Data_Layer<double> >& distanceMap, shared_ptr<Lattice_Data_Layer<double> >&maskMap, shared_ptr<Lattice_Data_Layer<double> >& labelMap, VINT bottomleft, VINT topright);
	shared_ptr< Lattice_Data_Layer< double > > createLatticeDouble(shared_ptr<const Lattice> lattice, double default_value);
// 	shared_ptr< Lattice_Data_Layer< bool > > createLatticeBool(VINT size, Lattice::Structure structure, bool default_value=false);
// 	shared_ptr< Lattice_Data_Layer< int> > createLatticeInt(VINT size, Lattice::Structure structure, int default_value=-1);
	
public:
	InitVoronoi(){};
	DECLARE_PLUGIN("InitVoronoi");
	bool init(CellType* ct);
	bool run(CellType* ct);

};

#endif
