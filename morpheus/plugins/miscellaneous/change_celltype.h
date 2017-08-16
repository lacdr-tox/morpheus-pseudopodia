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
#include "core/plugin_parameter.h"
#include "core/system.h"

/** \defgroup ChangeCellType
\ingroup ML_CellType
\ingroup MiscellaneousPlugins
\brief Conditionally alters CellType of cell

ChangeCelltype alters CellType of cell upon a predefined Condition.

- \b condition: expression to evaluate to trigger Celltype change.
- \b celltype_new_str: \ref ML_CellType to change to after condition is satisfied.

- \b Tiggers: (optional): a System of Rules that are triggered for both daughter cells after cell division.

\section Example

\verbatim
<ChangeCelltype  condition="..." celltype_new_str="other_ct"/>
\endverbatim

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
	void loadFromXML(const XMLNode) override;

};

#endif // PROLIFERATION_H
