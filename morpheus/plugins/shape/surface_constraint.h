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
#include "core/celltype.h"

/** \defgroup SurfaceConstraint
\ingroup ML_CellType
\ingroup CellShapePlugins
\brief Penalizes deviations from target cell perimeter (2D) or surface area (3D)

\ingroup CPM_EnergyPlugins

The surface constraint penalizes deviations of the cell perimeter (2D) or surface area \f$ s_{\sigma, t} \f$ from a given target \f$ S_{target} \f$.

This models the cell cortex ridigity by specifying the ratio between a cell's surface area to its volume (or ratio between perimeter length to area in 2D).

The target can be defined explicitly in \b surface mode, or implicitely in \b aspherity mode as a multiple of the surface of a sphere of equal volume.

The Hamiltonian is given by \f$ E_{Surface} = \sum_{\sigma} \lambda_S \cdot ( s_{\sigma, t} - S_{target} )^n \f$

For each proposed copy attempt \f$ \mathbf{x} \rightarrow \mathbf{x_{neighbor}}\f$, the change in effective energy is computed as:

\f$ \Delta E = \lambda \cdot ( ( s_{\sigma, before} - S_{\sigma, t, target} )^n  -  ( s_{\sigma, after} - S_{\sigma, t, target} )^n ) \f$

where 
- \f$ \lambda_s \f$ is strength of the constraint
- \f$ s_{\sigma, before} \f$ is the current surface area of cell \f$ \sigma \f$.
- \f$ s_{\sigma, after} \f$ is the projected surface area  of cell \f$ \sigma \f$ (if updated would be accepted).
- \f$ S_{\sigma, t, target} \f$ is the target surface area of cell \f$ \sigma \f$.
- \f$ n \f$ is the exponent.


\section Input 
Required
--------
- *mode*: Selects the *target* to be either
  - \b surface : The length/surface \f$ S_{target} \f$ in [node]/[node²]
  - \b aspherity : <br/>
     2D: The Multiple of the perimeter of a circle of the same area as the cell σ, i.e.  \f$ S_{target} =  target * 2\sqrt{  a_{\sigma} \pi} \f$<br/>
     3D: Multiple of the surface of a sphere of the same volume as the cell σ, i.e. \f$ S_{target} =  target * 4\pi \big( \frac{ \frac{3}{4} v_{\sigma}}{ \pi }^{\frac{2}{3}} \big) \f$

- *target*: Expression describing the target perimeter (2D) or surface area (3D) of a cell.

- *strength*: Expression describing the strength of the surface constraint.

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
	enum class TargetMode {SURFACE, ASPHERITY};
	// required
	PluginParameter2<TargetMode, XMLNamedValueReader, RequiredPolicy> target_mode;
	PluginParameter2<double, XMLEvaluator, RequiredPolicy> target;
	PluginParameter2<double, XMLEvaluator, RequiredPolicy> strength;
	//optional 
	PluginParameter2<double, XMLValueReader, OptionalPolicy> exponent;
	
	static vector<double> target_surface_cache;
	double targetSurfaceFromVolume( int ) const;
public:
	SurfaceConstraint();
	DECLARE_PLUGIN("SurfaceConstraint");

	double hamiltonian ( CPM::CELL_ID  cell_id ) const override;
    double delta ( const SymbolFocus& cell_focus, const CPM::Update& update) const override;

};

#endif
