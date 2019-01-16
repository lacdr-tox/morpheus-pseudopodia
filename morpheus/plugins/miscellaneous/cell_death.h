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

#ifndef CELLDEATH_H
#define CELLDEATH_H

#include "core/interfaces.h"
#include "core/celltype.h"


/** \defgroup CellDeath
\ingroup ML_CellType
\ingroup MiscellaneousPlugins InstantaneousProcessPlugins
\brief Remove cell based on a condition

Induces cell removal upon a predefined condition. 

- \b condition: Expression describing the condition under which a cell will be removed.

- \b target_volume: Symbol referring to the target volume as used in VolumeConstraint. 
If no target volume is specified, the cell will be removed and replaced by medium immediately upon fulfilling the specified condition, modeling lysis.
When a target volume is specified, the target volume is set to zero upon fulfilling of the specified condition and the cell will be removed as soon as it's volume shrunk to 1 node (\f$ v_{\sigma, t} = 1 \f$).


\section Example
Stochastically removing cells through lysis (immediate removal).
\verbatim
<CellDeath condition="rand_uni(0,1) < p_death" />
\endverbatim


Stochastically removing cells through shrinkage (removal after cell has shrunk to volume = 1).
\verbatim
<CellDeath condition="rand_uni(0,1) < p_death" target_volume="Vt" />
\endverbatim
*/

class CellDeath : public InstantaneousProcessPlugin
{
private:
    enum Mode{LYSIS, SHRINKAGE};
    enum class ReplaceMode { RANDOM_NB, LONGEST_IF, MEDIUM };
	PluginParameter2<double, XMLEvaluator, RequiredPolicy> condition;
	PluginParameter2<double, XMLReadWriteSymbol, OptionalPolicy> target_volume;
    PluginParameter2<double, XMLEvaluator, OptionalPolicy> remove_volume;
    PluginParameter2<ReplaceMode, XMLNamedValueReader, RequiredPolicy> replace_mode;

	CellType* celltype;
	Mode mode;
	ReplaceMode replace;
	set<uint> dying;

public:
	DECLARE_PLUGIN("CellDeath");
	CellDeath();
	void init(const Scope* scope) override;
	void executeTimeStep() override;
};

#endif // APOPTOSIS_H
