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

#ifndef ADDONADHESION_H
#define ADDONADHESION_H

#include "core/interfaces.h"
#include "core/plugin_parameter.h"

/** \defgroup ML_AddOnAdhesion AddOnAdhesion
\ingroup ML_Contact
\ingroup InteractionPlugins  CPM_InteractionPlugins
\brief Increases adhesion between neighboring CPM cells based on cell or membrane property

Increases adhesion (i.e. decreases cell-contact energy) between neighboring CPM cells based on cell property or membrane property.

Changes cell-contact energy depending on the amount of adhesive \f$ a_{\sigma} \f$ present in one of the cells (additive).

\f$ E = -a_{\sigma} \cdot s_{\sigma} \f$ with units energy per node length.

- \b adhesive: Expression describing amount of adhesive molecules. This may be a symbol representing a cell or membrane property (e.g. "c") or an expression (e.g. "10 * c").
- \b strength: (default=1): Expression describing strength of adhesion. This may be a symbol representing a cell or membrane property (e.g. "s") or an expression (e.g. "10 * s").


\section Examples
Using cell or membrane property 'c' as adhesive (strength = 1.0 by default)
\verbatim
<AddonAdhesion adhesive="c" />
\endverbatim

Both adhesive and strength can be provided as expression.
\verbatim
    <AddonAdhesion adhesive="10.0 * c" strength="t / 10.0" />
\endverbatim
*/

/** Plugin providing configurable adhesion between CPM cells */
class AddonAdhesion: public CPM_Interaction_Addon
{
	private:
		PluginParameter2<double, XMLEvaluator, RequiredPolicy> adhesive;
		PluginParameter2<double, XMLEvaluator, RequiredPolicy> strength;
		
	public:
		DECLARE_PLUGIN("AddonAdhesion");
		AddonAdhesion();
		double interaction(CPM::STATE s1, CPM::STATE s2) override;
};

#endif // ADDONADHESION_H
