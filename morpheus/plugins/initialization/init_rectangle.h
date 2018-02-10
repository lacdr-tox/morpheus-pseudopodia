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

#ifndef INITRECTANGLE_H
#define INITRECTANGLE_H

#include "core/interfaces.h"
#include "core/plugin_parameter.h"

/** \defgroup InitRectangle
 * \ingroup ML_Population
\ingroup InitializerPlugins
\brief Initializes cells as single nodes arranged in a rectangle

\section Description
Arranges a number of cells in an either a rectangular (or cubic) region.

If \b mode is 'regular'. Cells are seeded randomly, or in a regular structure. 
In case of a regular structure, deviations from this regularity can be induced by using a uniform random offset.

The \b Dimensions tag determines the origin of the left lower corner and the size of the region.


\section Example
\verbatim
<InitRectangle number-of-cells="100" mode="regular">
	<Dimensions	origin="0 0 0" size="100 100 1"/>
</InitRectangle>

<InitRectangle number-of-cells="rand_norm(100,10)" mode="regular" random-offset="0.05*size.x" >
	<Dimensions	origin="0, 0, 0" size="size.x/2, size.y, size.z"/>
</InitRectangle>

<InitRectangle number-of-cells="0.01 * (size.x * size.y)" mode="random" >
	<Dimensions	origin="size.x/4, size.y/4, size.z/2" size="size.x/2, size.y/2, size.z/2"/>
</InitRectangle>
\endverbatim
*/

class InitRectangle : public Population_Initializer
{
private:
	
	enum Mode{ REGULAR, RANDOM };
	
	PluginParameter2<double, XMLEvaluator, RequiredPolicy> numcells;
	PluginParameter2<Mode, XMLNamedValueReader, RequiredPolicy> mode;
	PluginParameter2<double, XMLEvaluator, DefaultValPolicy> random_displacement;
	
	PluginParameter2<VDOUBLE, XMLEvaluator, RequiredPolicy> origin_eval;
	PluginParameter2<VDOUBLE, XMLEvaluator, RequiredPolicy> size_eval;
	
	
	VINT origin;
	VINT size;
	int number_of_cells;
	CellType* celltype;

	vector<CPM::CELL_ID> setRandom();
	vector<CPM::CELL_ID> setRegular();
	vector<CPM::CELL_ID> setRegularAlternative();
	vector<int> calculateUniformPos();
	CPM::CELL_ID createCell(VINT newPos);

public:
	InitRectangle();
	DECLARE_PLUGIN("InitRectangle");
	vector<CPM::CELL_ID> run(CellType* ct) override;
};

#endif
