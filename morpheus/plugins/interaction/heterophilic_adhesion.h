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
\ingroup ML_Contact InteractionPlugins
\brief Heterophilic adhesive interaction between neighboring CPM cells.

Increases adhesion (i.e. decreases cell-contact energy) between neighboring CPM cells based on heterophilic binding, represented in cell or membrane properties.

\f$ \Delta E = s \cdot \big( a_{\sigma1}^{1} \cdot a_{\sigma2}^{2} + a_{\sigma1}^{2} \cdot a_{\sigma2}^{1}  \big) \f$

*/


class HeterophilicAdhesion : public Interaction_Addon
{
	private:
		PluginParameter2<double, XMLEvaluator, RequiredPolicy> adhesive1;
		PluginParameter2<double, XMLEvaluator, RequiredPolicy> adhesive2;
		PluginParameter2<double, XMLEvaluator, RequiredPolicy> strength;
	public:
		DECLARE_PLUGIN("HeterophilicAdhesion");
		HeterophilicAdhesion();
		double interaction(CPM::STATE s1, CPM::STATE s2) override;
};

#endif // HETEROPHILICADHESION_H
