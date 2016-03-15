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

#ifndef INTERACTIONENERGY_H
#define INTERACTIONENERGY_H

#include "simulation.h"
#include <algorithm>
#include <vector>
#include "interfaces.h"
#include "celltype.h"
#include "function.h"


class InteractionEnergy : public Plugin {

	private:
		// declare flags for storing the results of interaction initialisation
		
// 		struct NEIGHBOR_STAT { CPM::CELL_ID cell; uint count; };
		
		static const int IA_PLAIN      = 0x0; 
		static const int IA_PLUGINS    = 0x1; 
		static const int IA_COLLAPSE   = 0x2; 
		static const int IA_SUPERCELLS = 0x4;
		int interaction_details;
		
		uint n_celltypes;
		vector<VINT> ia_neighborhood;
		vector<int> ia_neighborhood_offsets;
		vector<int> ia_neighborhood_row_offsets;
		XMLNode ia_XMLNode;
		bool negate_interactions;
		shared_ptr<const CPM::LAYER> layer;
// 		mutable vector <NEIGHBOR_STAT> nei_cells;
// 		static const int max_neighbors=40;
		
		double default_interaction;
		vector<double> ia_energies;
		vector< vector< shared_ptr<Plugin> > > plugins;
		vector< shared_ptr<Interaction_Overrider> > ia_overrider;
		vector< vector< shared_ptr<Interaction_Addon> > > ia_addon;
		
		set< SymbolDependency > dependencies;

		uint getInterActionID(uint celltype1, uint celltype2 ) const { assert(celltype1<n_celltypes); assert(celltype2<n_celltypes); return celltype1 * n_celltypes + celltype2; }

	public:
		InteractionEnergy();
		
		string XMLName() const override { return "InteractionEnergy"; }
		void loadFromXML(const XMLNode xNode) override;
		XMLNode saveToXML() const;

		void init(const Scope* scope) override;
		
		const vector<VINT>& getNeighborhood() { return ia_neighborhood; };

		double delta(const CPM::UPDATE& update) const;
		double hamiltonian(const Cell* gc) const;
// 		bool addMatrix(double ** &intMatrix);
};

#endif
