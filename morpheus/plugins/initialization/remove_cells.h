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

#ifndef REMOVECELLS_H
#define REMOVECELLS_H

#include "core/interfaces.h"
#include "core/plugin_parameter.h"


/** \defgroup ML_RemoveCells RemoveCells
 * \ingroup ML_Population
\ingroup InitializerPlugins
\brief Remove cell based on a condition

Induces cell removal upon a predefined condition. 

- \b condition: Expression describing the condition under which a cell will be removed.

\section Example
Remove cells outside of center circle with radius 50
\verbatim
<RemoveCells condition="sqrt( (cell.center.x - size.x/2)^2 + (cell.center.y - size.y/2)^2 ) > 50.0" />
\endverbatim
*/

class RemoveCells : public Population_Initializer
{
private: 
	PluginParameter2<double, XMLEvaluator, RequiredPolicy> condition;	
	CellType* celltype;
	
public:
	DECLARE_PLUGIN("RemoveCells");
	RemoveCells();
	bool run(CellType* ct) override;
};

#endif // APOPTOSIS_H
