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

#ifndef VOLUMECONSTRAINT_H
#define VOLUMECONSTRAINT_H

#include "core/interfaces.h"
#include "core/plugin_parameter.h"

using namespace SIM;

/** \defgroup VolumeConstraint
\ingroup ML_CellType
\ingroup CellShapePlugins
\brief Penalizes deviations from target cell area (2D) or volume (3D)
\ingroup CPM_EnergyPlugins


The volume constraint models the conservation of area (2D) or volume (3D) of cells.

It puts a quadratic energy penality to deviations of the area (2D) or volume (3D) of a cell \f$ v_{\sigma, t} \f$ from a given target (resting) area or volume  \f$ V_{target} \f$, specified in units of lattice sites. 
The strength of this penalty is modulated by a Langrangian multiplier \f$ \lambda_V \f$ that determines the cell's incompressibility.

The Hamiltonian is given by:

\f$ E_{Volume} = \sum_{\sigma} \lambda_V \cdot ( v_{\sigma, t} - V_{target} )^2 \f$

For each proposed copy attempt \f$ \mathbf{x} \rightarrow \mathbf{x_{neighbor}}\f$, the change in effective energy is computed as:

\f$ \Delta E_{Volume} = \lambda_V \cdot \big( ( v_{\sigma, before} - V_{target} )^2  -  ( v_{\sigma, after} - V_{target} )^2 \big) \f$
where 
- \f$ \lambda_V \f$ is a Lagrangian multiplier given the strength of the constraint, related to the cell's incompressibility
- \f$ v_{\sigma, before} \f$ is the current volume of cell \f$ \sigma \f$ at time \f$ t \f$
- \f$ v_{\sigma, after} \f$ is the projected (if updated would be accepted) volume of cell \f$ \sigma \f$ at time \f$ t \f$
- \f$ V_{target} \f$ is the target volume of cell \f$ \sigma \f$ at time \f$ t \f$

\section Input 
- *target*: Expression describing the target area (2D) or volume (3D) of a cell \f$ V_{target} \f$, in units of lattice sites. This may be a constant (e.g. "100.0"), a symbol (e.g. "Vt"), or an expression (e.g. "V0 * 2.0")
- *strength*: Expression describing the strength of the volume constraint \f$ \lambda_V \f$, in units of energy. This may be a constant (e.g. "100.0"), a symbol (e.g. "S"), or an expression (e.g. "S0 * 2.0")

\section Reference
- Graner, François, and James A. Glazier. "Simulation of biological cell sorting using a two-dimensional extended Potts model." Physical review letters 69:13, 1992. http://dx.doi.org/10.1103/PhysRevLett.69.2013

\section Example
\verbatim
<VolumeConstraint strength="1" target="100"/>

<VolumeConstraint strength="s" target="Vt"/>

<VolumeConstraint strength="S * 2.0" target="V0 * 2.0"/>
\endverbatim
*/

class VolumeConstraint : public CPM_Energy
{
private:
	// required
	PluginParameter2<double, XMLEvaluator, RequiredPolicy> target;
	PluginParameter2<double, XMLEvaluator, RequiredPolicy> strength;
	
public:
	VolumeConstraint();
	DECLARE_PLUGIN("VolumeConstraint");

	double delta( const SymbolFocus& cell_focus, const CPM::Update& update) const override;
	double hamiltonian(CPM::CELL_ID  cell_id ) const override;
};

#endif
