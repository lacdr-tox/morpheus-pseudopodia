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

- \b Condition: Expression describing the condition under which a cell will be removed.

- \b ReplaceVolume: Rules how to replace the cell's volume at death.
the remaining nodes are replaced with either medium,
the neighbor with the longest interface, or a random neighbor. For random neighbors, all neighbors may have
the same probability, or the probabilities are scaled to the interfaces.

- \b Shrinkage: If no shrinkage is specified, the cell will removed immediately upon fulfilling of the specified condition, modeling lysis.
If \b Shrinkage is defined, the symbol specified by \b target-volume is set to zero upon fulfilling of the specified condition. The cell is then removed if it eventually reaches \b remove-volume.

  - \b target_volume: Symbol referring to the target volume as used in \ref ML_VolumeConstraint . 
  - \b remove-volume: cell area (2D) or volume (3D) at which the cell is removed


\section Example
Stochastically removing cells through lysis (immediate removal).
\verbatim
<CellDeath>
    <Condition>rand_uni(0,1) < p_death</Condition>
</CellDeath>
\endverbatim


Stochastically removing cells through shrinkage (removal after cell has shrunk to volume = 3).
\verbatim
 <CellDeath>
    <Condition>rand_uni(0,1) < p_death</Condition>
    <Shrinkage target-volume="Vt"/>
</CellDeath>
\endverbatim


Stochastically removing cells through shrinkage and replacing the remaining pixel with a random neighbor
<CellDeath>
  <Condition>rand_uni(0,1) < p_death</Condition>
  <Shrinkage remove-volume="1" target-volume="target_volume" replace-with="random neighbor"/>
</CellDeath>

*/

class CellDeath : public InstantaneousProcessPlugin
{
private:
    enum Mode{LYSIS, SHRINKAGE};
    enum class ReplaceMode { RANDOM_NB, RANDOM_NBW, LONGEST_IF, MEDIUM };
	PluginParameter2<double, XMLEvaluator, RequiredPolicy> condition;
	PluginParameter2<double, XMLReadWriteSymbol, OptionalPolicy> target_volume;
    PluginParameter2<double, XMLEvaluator, OptionalPolicy> remove_volume;
    PluginParameter2<ReplaceMode, XMLNamedValueReader, DefaultValPolicy> replace_mode;

	CellType* celltype;
	Mode mode;
	ReplaceMode replace;
	set<uint> dying;

public:
	DECLARE_PLUGIN("CellDeath");
	CellDeath();
	void init(const Scope* scope) override;
	void executeTimeStep() override;
    CPM::CELL_ID getRandomFusionPartner(std::map<CPM::CELL_ID,double> p_map);
};

#endif // APOPTOSIS_H
