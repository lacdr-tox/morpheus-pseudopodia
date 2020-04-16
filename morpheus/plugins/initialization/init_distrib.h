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
#include "core/celltype.h"

/** \defgroup InitDistribute

\ingroup ML_Population
\ingroup InitializerPlugins
\brief Places and initializes cells randomly in space the lattice with certain probability.

\section Description
Places \b number-of-cells cells randomly in space using \b probability as likelyhood bias.


\section Example
\verbatim
<InitDistribute number-of-cells="100" probability="1+sin(0.05*size.x)" />
\endverbatim
*/

class InitDistribute : public Population_Initializer
{
private:

	enum class Mode {
		REGULAR, RANDOM, GRID
	};
	
	PluginParameter2<double, XMLEvaluator, RequiredPolicy> num_cells_eval;
	PluginParameter2<double, XMLEvaluator, RequiredPolicy> probability;
	PluginParameter2<Mode, XMLNamedValueReader, DefaultValPolicy> mode;

	CPM::CELL_ID createCell(CellType* celltype, VINT newPos);


public:
	InitDistribute();
	DECLARE_PLUGIN("InitDistribute");
	vector<CPM::CELL_ID> run(CellType* ct) override;
};

#endif
