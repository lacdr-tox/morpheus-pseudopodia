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

#ifndef FLIPCELLMOTION_H
#define FLIPCELLMOTION_H

#include "core/interfaces.h"
#include "core/celltype.h"
#include "core/focusrange.h"

/** \defgroup FlipCellMotion
\ingroup ML_CellType
\ingroup CellMotilityPlugins
\brief Exchanges cell positions (spin flips). For CA-like models.

Randomly exchanges cell positions. Used to model cell motility in CA-like models.

- \b Condition: evaluated condition for a spin flip.

- \b neighborhood: size of the neighborhood between which cell positions are exchanged, given as distance in units of lattice sites.


\section References

\section Example
\verbatim
<FlipCellMotion neighborhood="2" time-step="0.1">
	<Condition>
		rand_uni(0,1) < p
	</Condition>
</FlipCellMotion>
\endverbatim
*/
class FlipCellMotion: public InstantaneousProcessPlugin {

private:
	PluginParameter2<double, XMLEvaluator, RequiredPolicy> condition;
	PluginParameter2<int, XMLValueReader, DefaultValPolicy> neighborhood;
	
	shared_ptr <const Lattice > lattice;
// 	shared_ptr <const CPM::LAYER > cpm_lattice;
	const CellType* celltype;
	uint nbh_order;
	vector<VINT> nbh;
	
public:
	DECLARE_PLUGIN("FlipCellMotion");
	FlipCellMotion();
	void init(const Scope* scope) override;
	void executeTimeStep() override;
};

#endif // FLIPCELLS_H
