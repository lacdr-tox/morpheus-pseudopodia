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
#include "core/pde_layer.h"
#include "core/plugin_parameter.h"

/** \defgroup Haptotaxis
\ingroup motility_plugins
\ingroup CPM_EnergyPlugins

\section Description
Haptotaxis is an Energy that puts an energetic bias in the hamiltonian depending on the gradient of an underlying PDE_Layer.

\section Example
\verbatim
<Haptotaxis [layer="agent1" saturation="0.1"]>
	<Layer symbol="agent1" />
	<Strength [symbol="S" | value="0.1"] />
</Haptotaxis>
\endverbatim
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
		void loadFromXML(const XMLNode node) override;

		double delta(const SymbolFocus& cell_focus, const CPM::UPDATE& update, CPM::UPDATE_TODO todo) const;
		double hamiltonian(CPM::CELL_ID cell_id) const;
};
