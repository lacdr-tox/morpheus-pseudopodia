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

#ifndef LENGTHCONSTRAINT_H
#define LENGTHCONSTRAINT_H

#include "core/interfaces.h"
#include "core/plugin_parameter.h"
#include "core/cell_property_accessor.h"
using namespace SIM;

/** \defgroup LengthConstraint Length Constraint
\ingroup CellShapePlugins
\brief Penalizes deviations from target cell length
\ingroup CPM_EnergyPlugins

\section Description
The length constraint penalizes deviations of the length of a cell \f$ l_{\sigma, t} \f$ from a given target length \f$ L_{target} \f$, specified in units of lattice sites.

The length of a cell is the length of the semimajor axis of an ellipsoid approximation of the cell shape, using the inertia tensor.

The Hamiltonian is given by: \f$ E_{Length} = \sum_{\sigma} \lambda_L \cdot ( l_{\sigma, t} - L_{target} )^2 \f$

\f$ \Delta E_{Volume} = \lambda_L \cdot \big( ( l_{\sigma, before} - L_{target} )^2  -  ( l_{\sigma, after} - L_{target} )^2 \big) \f$
where 
- \f$ \lambda_L \f$ is the strength of the constraint
- \f$ l_{\sigma, before} \f$ is the current length of cell \f$ \sigma \f$ at time \f$ t \f$
- \f$ l_{\sigma, after} \f$ is the projected length of cell \f$ \sigma \f$ at time \f$ t \f$ (if updated would be accepted)
- \f$ L_{target} \f$ is the target length of cell \f$ \sigma \f$ at time \f$ t \f$


\section Input
- *target*: Expression describing the target cell length. This may be a constant (e.g. "1.0"), a symbol (e.g. "St"), or an expression (e.g. "S0 * 2.0")
- *strength*: Expression describing the strength of the length constraint. This may be a constant (e.g. "2.0"), a symbol (e.g. "Ss"), or an expression (e.g. "S0 * 2.0")

\section Notes

- This constraint is not safe against periodic boundary conditions.
- Use together with "ConnectivityConstraint" plugin to prevent cell breakup.
- This plugin is optimized to determine the cell length using incremental updates, preventing the need to recompute the entire inertia tensor on every call. 
- Acknowledgement: Incremental updating is implemented by Robert Muller (ZIH, TU Dresden)

\section Reference

Zajac M, Jones GL, Glazier JA. Simulating convergent extension by way of anisotropic differential adhesion. J Theor Biol. 222:247–259, 2003.

\section Example
\verbatim
\endverbatim
*/


class LengthConstraint : public CPM_Energy, public CPM_Update_Listener
{
private:
	// required
	PluginParameter2<double, XMLEvaluator, RequiredPolicy> target;
	PluginParameter2<double, XMLEvaluator, RequiredPolicy> strength;
		
	static const int LC_XX=0;
	static const int LC_XY=1;
	static const int LC_YY=2;
	static const int LC_XZ=3;
	static const int LC_YZ=4;
	static const int LC_ZZ=5;
	
// actual + test properties of cell		
// 		CellPropertyAccessor<VDOUBLE> _center;
// 		CellPropertyAccessor<VDOUBLE> _temp_center;
	CellPropertyAccessor<double>  _cell_length;
	CellPropertyAccessor<double>  _temp_cell_length;
	CellPropertyAccessor<std::vector<double > > _I;
	CellPropertyAccessor<std::vector<double > > _temp_I;		
			
// how many incremental updates still sensible
	CellPropertyAccessor<double> _maxIncrementalUpdates;
	CellPropertyAccessor<double> _incrementalUpdatesLeft;
	CellPropertyAccessor<double> i_updated;
	double calcLengthHelper3D(const std::vector<double> &I, int N) const;
	double calcLengthHelper2D(const std::vector<double> &I, int N) const;
public:
	LengthConstraint();
	DECLARE_PLUGIN("LengthConstraint");

	double delta(const SymbolFocus& cell_focus, const CPM::Update& update) const;
	double hamiltonian(CPM::CELL_ID cell_id) const;
    void init(const Scope* scope);
	double long_cell_axis2(CPM::CELL_ID id) const ;
	void set_update_notify( CPM::CELL_ID cell_id, const CPM::Update& update);
	void update_notify(CPM::CELL_ID cell_id, const CPM::Update& update);
};

#endif
