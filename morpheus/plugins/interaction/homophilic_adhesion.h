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

#ifndef HOMOPHILICADHESION_H
#define HOMOPHILICADHESION_H

#include "core/interfaces.h"
#include "core/plugin_parameter.h"

/** \defgroup ML_HomophilicAdhesion HomophilicAdhesion 
\ingroup  ML_Contact
\ingroup InteractionPlugins  CPM_InteractionPlugins
\brief Homophilic adhesive interaction between neighboring CPM cells.

Change cell-contact energy by factor based on binding between two identical adhesives \f$ a_{\sigma} \f$ on adjacent cells (i.e. cadherins).

- \b adhesive: Expression describing amount of adhesive molecules. This may be a symbol representing a cell or membrane property (e.g. "c") or an expression (e.g. "10 * c").
- \b strength (default="1"): Expression describing strength of adhesion. This may be a symbol representing a cell or membrane property (e.g. "s") or an expression (e.g. "10 * s").
- \b equilibriumConstant (optional): Value describing ratio of binding/unbinding between adhesive molecules at cell membranes. 


If no \e equilibriumConstant is given (default), the full bond saturation is assumed by taking the minimum of adhesive in both cells:

\f$ E = - min(a_{\sigma1},a_{\sigma2}) \cdot s_{\sigma} \f$, with units energy per node length.


If an \e equilibriumConstant is given, the equilibrium concentration of bonds is calculated on the basis of binding / unbinding rate ratio of reaction

\f$ A_1 A_2 \overset{k_b}{\underset{k_{ub}}{\longleftrightarrow}} A_1 + A_2 \f$

\f$ E = - \left[ \frac{a_1+a_2+k_{ub}/k_b}{2} + \sqrt{\frac{1}{4}(a_1+a_2+k_{ub}/k_b)^2 - a_1 a_2} \right] \f$, with units energy per node length.


\section Examples

\verbatim
<HomophilicAdhesion adhesive="c" strength="1" />
\endverbatim

\verbatim
<HomophilicAdhesion adhesive="10*c" />
\endverbatim

*/


/* \ingroup InteractionPlugins 
 * \defgroup HomophilicAdhesionDoc Homophilic Adhesion 
 * \brief Homophilic adhesive interaction between neighboring CPM cells
 * 
 * 
 * 

In case of maximum bonds or low unbinding/binding ratio, bonds energy converges to: 
dEnergy = min(adhesive(cell_1), adhesive(cell_2)) * Strength

 *  Based on the concentration of an adhesive molecule, interaction energies for the CPM hamiltonian
 *  can be defined.
 * 
 *  EquilibriumConstant defines the equilibrium of inter-cellular binding and unbinding. 
 *  If omitted, fully bound adhesives are assumed ( bonds = min(adhesive(cell1), adhesive(cell2)) ) .
 * 
 *  Class: \ref HomophilicAdhesion
 * 
 */

/** Plugin providing configurable homophilic adhesion between CPM cells */
class HomophilicAdhesion : public CPM_Interaction_Addon
{
	private:
		PluginParameter2<double, XMLEvaluator, RequiredPolicy> adhesive;
		PluginParameter2<double, XMLEvaluator, RequiredPolicy> strength;
		PluginParameter2<double, XMLValueReader, OptionalPolicy> binding_ratio;
	public:
		DECLARE_PLUGIN("HomophilicAdhesion");
		HomophilicAdhesion();
		double interaction(const SymbolFocus& cell1, const SymbolFocus& cell2) override;
};

#endif // HOMOPHILICADHESION_H
