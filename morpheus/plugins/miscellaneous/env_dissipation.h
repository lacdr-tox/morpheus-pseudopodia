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

#ifndef ENV_DISSIPATION_H
#define ENV_DISSIPATION_H

#include <core/interfaces.h>
#include <core/simulation.h>
#include <core/celltype.h>

/** \defgroup Environment_DissipationPlugin  Environmental Dissipation
 * \ingroup CPM_EnergyPlugins
 */

class Environment_Dissipation : public CPM_Energy
{
	public:
		DECLARE_PLUGIN("Dissipation")

		Environment_Dissipation();
		virtual double hamiltonian ( CPM::CELL_ID cell_id ) const;
		virtual double delta ( CPM::CELL_ID cell_id , const CPM::UPDATE& update, CPM::UPDATE_TODO todo ) const;
		virtual void init ( CellType* ct );

		virtual void loadFromXML ( const XMLNode );
	
	private:
		double saturation_05;
		double sensitivity, high_val, low_val;
		double f_add, f_remove, f_add_rem;
		shared_ptr<const PDE_Layer> pde_layer;
		string pde_layer_name;

};

#endif // SLIME_DISSIPATION_H
