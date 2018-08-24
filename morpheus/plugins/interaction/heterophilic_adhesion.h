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

#ifndef HETEROPHILICADHESION_H
#define HETEROPHILICADHESION_H

#include "core/interfaces.h"
#include "core/plugin_parameter.h"

/** \defgroup ML_HeterophilicAdhesion  HeterophilicAdhesion 
\ingroup ML_Contact InteractionPlugins CPM_InteractionPlugins

\brief Heterophilic adhesive interaction between neighboring CPM cells.

Increases adhesion (i.e. decreases cell-contact energy) between neighboring CPM cells based on heterophilic binding, represented in cell or membrane properties.

- \b adhesive1/2: Expression describing the amount of both adhesive molecules.
- \b strength (default="1"): Expression describing strength of adhesive bonds.
- \b equilibriumConstant (optional): Value describing ratio of binding/unbinding rates between adhesive molecules at cell membranes. If omitted, defaults to saturated binding.

Saturated binding equation:
\f$ \Delta E = s \cdot \big( min( a_{\sigma1}^{1}, a_{\sigma2}^{2} ) + min( a_{\sigma1}^{2}, a_{\sigma2}^{1} )  \big) \f$

*/


class HeterophilicAdhesion : public CPM_Interaction_Addon
{
	private:
		PluginParameter2<double, XMLEvaluator, RequiredPolicy> adhesive1;
		PluginParameter2<double, XMLEvaluator, RequiredPolicy> adhesive2;
		PluginParameter2<double, XMLEvaluator, RequiredPolicy> strength;
		PluginParameter2<double, XMLValueReader, OptionalPolicy> binding_ratio;
	public:
		DECLARE_PLUGIN("HeterophilicAdhesion");
		HeterophilicAdhesion();
		double interaction(CPM::STATE s1, CPM::STATE s2) override;
};

#endif // HETEROPHILICADHESION_H
