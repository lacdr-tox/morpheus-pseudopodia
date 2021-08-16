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

#ifndef CHANGECELLTYPE_H
#define CHANGECELLTYPE_H

#include "core/interfaces.h"
#include "core/celltype.h"
#include "core/system.h"

/** \defgroup ChangeCellType
\ingroup ML_CellType
\ingroup MiscellaneousPlugins InstantaneousProcessPlugins
\brief Conditionally alters CellType of cell

ChangeCelltype alters the CellType of a cell upon a condition.

- \b newCellType: \ref ML_CellType to change to after condition is satisfied.
- \b time-step [optional]\b: The time step adjusts by default to 1 Monte Carlo step (MCS) of the CPM. The time step be overridden by setting the \b time-step attribute.

- \b Condition: expression that triggers the celltype change.
- \b Tiggers: (optional): a System of Rules that are triggered for both daughter cells after cell division.

\section Example

~~~~~~~~~~~~~~.xml
<ChangeCellType newCellType="ct_differentiated_A" time-step>
	<Condition> 
		cell_fate_morphogen_A >= 1
	</Condition>
<ChangeCellType>
~~~~~~~~~~~~~~

**/

class ChangeCelltype : public InstantaneousProcessPlugin
{
private:
	PluginParameter2<double, XMLEvaluator, RequiredPolicy> condition;
	PluginParameterCellType<RequiredPolicy> celltype_new;
	
	// variable and functions declared here
	CellType* celltype;
// 	uint celltype_new_ID;
	shared_ptr<TriggeredSystem> triggers;

public:
	ChangeCelltype();
	DECLARE_PLUGIN("ChangeCellType");

	void init(const Scope* scope) override;
	void executeTimeStep() override;
	void loadFromXML(const XMLNode, Scope* scope) override;

};

#endif // PROLIFERATION_H
