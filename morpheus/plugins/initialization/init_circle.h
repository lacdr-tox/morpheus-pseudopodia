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

#ifndef INITCIRCLE_H
#define INITCIRCLE_H

#include "core/interfaces.h"
#include "core/plugin_parameter.h"

/** \defgroup InitCircle
\ingroup ML_Population
\ingroup InitializerPlugins
\brief Initializes cells as single nodes arranged in a circle 

\section Description
InitCircle puts a number of cells in an either circular region, depending on the lattice structure.

The Dimensions tag determines the center and the radius of the circular region.

Cells are seeded randomly, or in a regular structure. 
In case of a regular structure, deviations from this regularity can be induced by using a uniform random displacement.

\section Examples
100 cells in regular intervals around origin
\verbatim
<InitCircle number-of-cells="100" mode="regular">
	<Dimensions	center="0, 0, 0" radius="10"/>
</InitCircle>
\endverbatim

Almost regular placement of cells in a circular region in center of lattice
\verbatim
<InitCircle number-of-cells="rand_norm(100,10)" mode="regular" random_displacement="0.05*size.x" >
	<Dimensions	center="size.x/2, size.y/2, 0" radius="size.x/2"/>
</InitCircle>
\endverbatim

Random placement of cells in a circular region at the origin of the lattice
\verbatim
<InitCircle number-of-cells="0.01 * (size.x * size.y)" mode="random" >
	<Dimensions	center="0, 0, 0" radius="size.x/3"/>
</InitCircle>
\endverbatim
*/

class InitCircle : public Population_Initializer
{
private:
	
	enum Mode{ REGULAR, RANDOM };

	PluginParameter2<double, XMLEvaluator, RequiredPolicy> numcells;
	PluginParameter2<Mode, XMLNamedValueReader, RequiredPolicy> mode;
	
	PluginParameter2<VDOUBLE, XMLEvaluator, RequiredPolicy> center_eval;
	PluginParameter2<double, XMLEvaluator, RequiredPolicy> radius_eval;

	CellType* celltype;
	uint number_of_cells;
	VDOUBLE center;
	double radius;

	vector<CPM::CELL_ID> setRandom();
	vector<CPM::CELL_ID> setRegular();

	void convertPos(string str_Pos);
	CPM::CELL_ID createNode(VINT newPos);
	int calculateLines();

public:
	InitCircle();
	DECLARE_PLUGIN("InitCircle");
	vector<CPM::CELL_ID> run(CellType* ct) override;

};

#endif
