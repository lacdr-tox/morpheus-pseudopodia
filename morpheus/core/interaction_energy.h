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
/**

\defgroup ML_Interaction Interaction
\ingroup ML_CPM
specifies interaction energies \f$ J_{\sigma_1, \sigma_2} \f$ for different inter-cellular \ref ML_Contact. The interaction energy is given per length unit as defined in \ref ML_CPM ShapeSurface.

 - \b default: default value for unspecified interactions
 - \b negate: negate all defined interaction values
 
\defgroup ML_Contact Contact
\ingroup ML_Interaction

\brief Inter-celltype contact energies per length unit as defined in \ref ML_CPM ShapeSurface. 
Contact energies can be constant values or expressions of global symbols or symbols of the involved cell types.

 - \b type1: name of one celltype involved in the interaction
 - \b type2: name of the other celltype involved in the interaction
 - \b value: Contact energy. Expression based on \b global symbols and symbols defined in cell type 1 -- prefixed by namespace \b cell1 (e.g. cell1.adhesive)  and symbols defined in cell type 2 -- prefixed by namespace \b cell2.

*/

#ifndef INTERACTIONENERGY_H
#define INTERACTIONENERGY_H

#include "simulation.h"
#include <algorithm>
#include <vector>
#include "interfaces.h"
#include "celltype.h"
#include "function.h"
// #include "symbolfocus.h"

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
// 		Neighborhood ia_neighborhood;
		double boundaryLenghScaling;
		vector<VINT> ia_neighborhood;
		vector<int> ia_neighborhood_offsets;
		vector<int> ia_neighborhood_row_offsets;
		XMLNode ia_XMLNode;
		bool negate_interactions;
		shared_ptr<const CPM::LAYER> layer;
// 		mutable vector <NEIGHBOR_STAT> nei_cells;
// 		static const int max_neighbors=40;
		
		double default_interaction;
		struct ContactValue { shared_ptr<ThreadedExpressionEvaluator<double> > value; bool celltypes_swapped; bool subscope_use;};
		vector<ContactValue> ia_energies2;
// 		vector<double> ia_energies;
		vector< vector< shared_ptr<Plugin> > > plugins;
		vector< shared_ptr<CPM_Interaction_Overrider> > ia_overrider;
		vector< vector< shared_ptr<CPM_Interaction_Addon> > > ia_addon;
		
		set< SymbolDependency > dependencies;

		uint getInterActionID(uint celltype1, uint celltype2 ) const { assert(celltype1<n_celltypes); assert(celltype2<n_celltypes); return celltype1 * n_celltypes + celltype2; }
		double getBaseInteraction(const SymbolFocus& cell1, const SymbolFocus& cell2) const;

	public:
		InteractionEnergy();
		
		string XMLName() const override { return "InteractionEnergy"; }
		void loadFromXML(const XMLNode xNode, Scope* scope) override;
		XMLNode saveToXML() const override;

		void init(const Scope* scope) override;
		
		const Neighborhood& getNeighborhood() { return CPM::getBoundaryNeighborhood(); };

		double delta(const CPM::Update& update) const;
		double hamiltonian(const Cell* gc) const;
// 		bool addMatrix(double ** &intMatrix);
};

#endif
