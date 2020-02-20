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

#include "core/simulation.h"
#include "core/interfaces.h"
#include "core/cell.h"
#include "core/plugin_parameter.h"

/** \defgroup Haptotaxis
\ingroup ML_CellType
\ingroup CellMotilityPlugins
\ingroup CPM_EnergyPlugins

\section Description
Haptotaxis favors updates in the directions of certain attractants. Unlike \ref Chemotaxis the amount of attractant at the target site is taken into account, not the gradient of the attractor.

\section Example
~~~~~~~~~~~~~~~~{.xml}
<Haptotaxis attractant="10 * agent1" strength="0.1">
</Haptotaxis>
~~~~~~~~~~~~~~~~
*/

class Haptotaxis : virtual public CPM_Energy
{
	private:
		PluginParameter2<double,XMLEvaluator,RequiredPolicy> attractant;
		PluginParameter2<double,XMLEvaluator,RequiredPolicy> strength;
		
	public:
		DECLARE_PLUGIN("Haptotaxis");

		Haptotaxis(); // default values
		void init(const Scope* scope) override;

		double delta(const SymbolFocus& cell_focus, const CPM::Update& update) const override;
		double hamiltonian(CPM::CELL_ID cell_id) const override;
};
