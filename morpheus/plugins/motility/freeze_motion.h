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

#ifndef FREEZEMOTION_H
#define FREEZEMOTION_H

#include "core/interfaces.h"
#include "core/plugin_parameter.h"

/** \defgroup FreezeMotion
\ingroup ML_CellType
\ingroup CellMotilityPlugins

FreezeMotion conditionally prevents all CPM updates, rendering a cell immotile.

\section Input
- *condition*: Expression describing condition under which cell become immotile. True if expression >= 1.0.

\section Examples

Prevent all motility of cells in the cell type
\verbatim
<FreezeMotion condition="1" />
\endverbatim

Prevent motility of cells larger than 100 nodes.
\verbatim
<FreezeMotion condition="cell.volume > 100" />
\endverbatim

Prevent motility after a certain time. Assumes SymbolTime is 'time'.
\verbatim
<FreezeMotion condition="time > 100" />
\endverbatim
*/

class FreezeMotion : public CPM_Check_Update
{
private:
	PluginParameter2<double, XMLEvaluator, RequiredPolicy> condition;

public:
	FreezeMotion();
	DECLARE_PLUGIN("FreezeMotion");
	void init(const Scope* scope) override;
	bool update_check(CPM::CELL_ID  cell_id, const CPM::Update& update) override;
};

#endif // FREEZER_H
