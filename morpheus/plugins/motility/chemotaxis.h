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

#ifndef CHEMOTAXIS_H
#define CHEMOTAXIS_H

#include "core/interfaces.h"
#include "core/plugin_parameter.h"

/** \defgroup Chemotaxis
\ingroup CellMotilityPlugins Celltype
\brief Energetically favors updates in direction of increasing concentration in \ref Field
\ingroup CPM_EnergyPlugins


\section Description
Modifies effective energy \f$ \Delta E \f$ to favor updates in the direction of increasing concentration of a gradient field.

In the simplest case (Savill and Hogeweg, 1997):
\f$ \Delta E_{chemotaxis} = \mu \cdot ( f_{\mathbf{x_i}} - f_{\mathbf{x_j}}) \f$

where 
- \f$ f_{\mathbf{x_i}} \f$ is the concentration of field $f$ at position \f$ \mathbf{x_i} \f$
- \f$ \mu \f$ is the chemotactic strength

Chemotaxis with saturation
--------------------------
When saturation is specified, the following generalized formula is used:

\f$ \Delta E_{chemotaxis} = \mu \cdot ( \frac{ f_{\mathbf{x_i}} }{ 1 + s_{\mathbf{x_i}}f_{\mathbf{x_i}}} - \frac{ f_{\mathbf{x_j}} }{ 1 + s_{\mathbf{x_i}}f_{\mathbf{x_j}} }) \f$

where \f$ s_{\mathbf{x_i}} \f$ is the saturation constant, which may be constant or cell- or location specific.

- \b field: Expression describing gradient. This may be a symbol representing a field (e.g. "c", or an expression describing a gradient (e.g. "l.x / size.x").
- \b strength: Expression describing chemotactic strength. 

- \b saturation (default=None): Symbol representing the saturation constant (may refer to cell Property or MembraneProperty)
- \b retraction (default=True): If false, retractions are not considered, only protrusions. 
- \b contact_inhibition (default=False): If true, only protusions/retractions to/from medium yield nonzero energy.

\section Reference
- Savill NJ, Hogeweg P. Modelling morphogenesis: from single cells to crawling slugs. J Theor Biol. 1997;184:229–235.
- Merks, Roeland MH, Erica D. Perryn, Abbas Shirinifard, and James A. Glazier. "Contact-inhibited chemotaxis in de novo and sprouting blood-vessel growth." PLoS computational biology 4, no. 9 (2008): e1000163.

\section Examples

\verbatim
<Chemotaxis field="c" strength="1">
</Chemotaxis>
\endverbatim
*/

class Chemotaxis : public CPM_Energy
{
	private:
		// required
		PluginParameter2<double, XMLEvaluator, RequiredPolicy> field;
		// optional
		PluginParameter2<double, XMLEvaluator, RequiredPolicy> strength;
		PluginParameter2<double, XMLEvaluator, OptionalPolicy> saturation;
		PluginParameter2<bool, XMLValueReader, DefaultValPolicy> retraction; 
		PluginParameter2<bool, XMLValueReader, DefaultValPolicy> contact_inhibition;
		
	public:
		DECLARE_PLUGIN("Chemotaxis");
		Chemotaxis();

		double delta(const SymbolFocus& cell_focus, const CPM::UPDATE& update, CPM::UPDATE_TODO todo) const;
		double hamiltonian(CPM::CELL_ID cell_id) const;
};

#endif
