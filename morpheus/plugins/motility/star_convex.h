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

#ifndef STAR_CONVEX_H
#define STAR_CONVEX_H

#include "core/interfaces.h"
#include "core/celltype.h"

/** \defgroup StarConvex
\ingroup ML_CellType
\ingroup CellMotilityPlugins
\ingroup CPM_EnergyPlugins

\section Description



For each proposed copy attempt \f$ \mathbf{x} \rightarrow \mathbf{x_{neighbor}} \f$ in direction \f$ \vec{s} \f$ , the change in effective energy is computed as:

\f$ \Delta E = - \mu \cdot v_{\sigma,t} \cdot ( \vec{o} \cdot \vec{s} ) \f$

The direction may be provided through a fixed value or via a cell property.


\section Input
Required
--------
- *membrane*: Symbol representing vector (property or function).

- *strength*: Expression describing the strength of the bias. This may be a constant (e.g. "2.0"), a symbol (e.g. "s"), or an expression (e.g. "s0 * 2.0")

Optional
--------
- *protrusions* (default=true)

- *retractions* (default=true)

\section References

\section Example
\verbatim
<StarConvex direction="d" strength="s" [ protrusions="true" | retractions="true" ] />
\endverbatim
*/


class StarConvex : public CPM_Energy
{
private:
	// required
	PluginParameter2<double, XMLEvaluator, RequiredPolicy> strength;
	PluginParameter2<double, XMLEvaluator, RequiredPolicy> membrane;
	// optional
	PluginParameter2<bool, XMLValueReader, DefaultValPolicy> protrusion;
	PluginParameter2<bool, XMLValueReader, DefaultValPolicy> retraction;

public:
	DECLARE_PLUGIN("StarConvex");
	StarConvex();
    void init(const Scope* scope) override;
	double delta (const SymbolFocus& cell_focus, const CPM::Update& update) const override;
	double hamiltonian (CPM::CELL_ID) const override;

};

#endif // STAR_CONVEX_H
