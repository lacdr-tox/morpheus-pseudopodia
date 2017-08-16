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

#include "core/simulation.h"
#include "core/interfaces.h"
#include "core/cell.h"
#include "core/pde_layer.h"
#include "core/pluginparameter.h"
#include <core/symbol_accessor.h>

/** \defgroup MatrixGuidance Matrix Guidance
\ingroup ML_CellType
\ingroup CellMotilityPlugin
*/


class MatrixGuidance : public CPM_Energy
{
	private:
		string layer_strength_symbol;
		string layer_orientation_symbol;
		double degradation;
		double reorientation;
		shared_ptr<PDE_Layer> layer_strength;
		shared_ptr<PDE_Layer> layer_orientation;
	public:
		DECLARE_PLUGIN("MatrixGuidance");

		MatrixGuidance(); // default values
		void init();
		void loadFromXML(const XMLNode);
		virtual set< string > getDependSymbols();
		double delta(CPM::CELL_ID cell_id, const CPM::UPDATE& update, CPM::UPDATE_TODO todo) const;
		double hamiltonian(CPM::CELL_ID cell_id) const;
};
