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

#ifndef INSERTMEDIUM_H
#define INSERTMEDIUM_H

#include "core/interfaces.h"
#include "core/celltype.h"

/** \defgroup InsertMedium
\ingroup ML_CellType
\ingroup MiscellaneousPlugins 
\ingroup InstantaneousProcessPlugins
\brief Inserts medium nodes randomly along a cell membrane.

Inserts medium nodes randomly along a cell membrane.

In CPM models of continuous cell monolayers, without medium, tissues cannot be disrupted, even when under tension. 
Randomly inserting medium nodes can be used to compensate for this behavior.

Each Monte Carlo step:
- select a random cell
- if it already have medium nodes around it, return
- else, select a random node around the cell
- convert this node into medium

- \b Condition (default = true): expression to evaluate to trigger inserting medium

\section Reference
- Jos Kafer, Takashi Hayashi, Athanasius F.M. Maree, Richard W. Carthew and Francois Graner, Cell adhesion and cortex contractility determine cell patterning in the Drosophila retina, PNAS, 2007.

\section Example
\verbatim
<InsertMedium/>
\endverbatim
*/

class InsertMedium : public InstantaneousProcessPlugin
{
private:
	PluginParameter2<double, XMLEvaluator, OptionalPolicy> condition;

	CellType* celltype;
	vector<VINT> neighbor_sites;
	shared_ptr<const CPM::LAYER> cpm_layer;
	CPM::CELL_ID medium;
	
public:
    InsertMedium();
	DECLARE_PLUGIN("InsertMedium");
	void init(const Scope* scope) override;
	void executeTimeStep() override;
};

#endif // INSERTMEDIUM_H
