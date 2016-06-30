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

#ifndef SURFACECONSTRAINT_H
#define SURFACECONSTRAINT_H

#include "core/interfaces.h"
#include "core/plugin_parameter.h"

/** \defgroup SurfaceConstraint
\ingroup CellShapePlugins
\brief Penalizes deviations from target cell perimeter (2D) or surface area (3D)
\ingroup CPM_EnergyPlugins

The surface constraint penalizes deviations of the cell perimeter (2D) or surface area \f$ s_{\sigma, t} \f$ from a given target \f$ S_{target} \f$.
This models the cell cortex ridigity by specifying the ratio between a cell's surface area to its volume (or ratio between perimeter length are area in 2D).

The Hamiltonian is given by \f$ E_{Surface} = \sum_{\sigma} \lambda_S \cdot ( s_{\sigma, t} - S_{target} )^n \f$

For each proposed copy attempt \f$ \mathbf{x} \rightarrow \mathbf{x_{neighbor}}\f$, the change in effective energy is computed as:

\f$ \Delta E = \lambda \cdot ( ( s_{\sigma, before} - S_{\sigma, t, target} )^n  -  ( s_{\sigma, after} - S_{\sigma, t, target} )^n ) \f$

where 
- \f$ \lambda_s \f$ is strength of the constraint
- \f$ s_{\sigma, before} \f$ is the current surface area of cell \f$ \sigma \f$ at time \f$ t \f$
- \f$ s_{\sigma, after} \f$ is the projected (if updated would be accepted) surface area  of cell \f$ \sigma \f$ at time \f$ t \f$
- \f$ S_{\sigma, t, target} \f$ is the target surface area of cell \f$ \sigma \f$ at time \f$ t \f$.  \f$ S_{\sigma, t, target} < 1 \f$ represent rigid cell.
- \f$ n \f$ is the exponent.


Note that the target surface is normalized to the surface area of a sphere given its cell volume \f$ v_{\sigma, t}\f$: 

2D: \f$ S_{target} =   2\sqrt{  v_{\sigma, t} \pi} \f$

3D: \f$ S_{target} =   4\pi \big( \frac{ \frac{3}{4} v_{\sigma, t}}{ \pi }^{\frac{2}{3}} \big) \f$

\section Input 
Required
--------
- *target*: Expression describing the target perimeter (2D) or surface area (3D) of a cell. This may be a constant (e.g. "1.0"), a symbol (e.g. "St"), or an expression (e.g. "S0 * 2.0")

- *strength*: Expression describing the strength of the surface constraint. This may be a constant (e.g. "2.0"), a symbol (e.g. "Ss"), or an expression (e.g. "S0 * 2.0")

Optional
--------
- *exponent*: Value giving the exponents \f$ n \f$ (default: \f$ n=2 \f$).

\section Reference

- Noriyuki Bob Ouchi, James A. Glazier, Jean-Paul Rieu, Arpita Upadhyayad, Yasuji Sawadae, Improving the realism of the cellular Potts model in simulations of biological cells. Physica A, 329:451--458, 2003.

\section Example
\verbatim
\endverbatim
*/

class SurfaceConstraint : public CPM_Energy
{
private:
	// required
	PluginParameter2<double, XMLEvaluator, RequiredPolicy> target;
	PluginParameter2<double, XMLEvaluator, RequiredPolicy> strength;
	//optional 
	PluginParameter2<double, XMLValueReader, OptionalPolicy> exponent;
	
	static vector<double> target_surface_cache;
	double targetSurfaceFromVolume( int ) const;
public:
	SurfaceConstraint();
	DECLARE_PLUGIN("SurfaceConstraint");

	double hamiltonian ( CPM::CELL_ID  cell_id ) const;
    double delta ( const SymbolFocus& cell_focus, const CPM::Update& update) const;

};

#endif
