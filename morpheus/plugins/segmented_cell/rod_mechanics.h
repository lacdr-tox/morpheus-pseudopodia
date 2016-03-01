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

#include "core/interfaces.h"
#include "core/super_celltype.h"
#include "core/simulation.h"
#include "core/symbol_accessor.h"


/** \defgroup rod_mechanics  Rod Mechanics
\ingroup segmented_plugins

\section Description
Rod_Mechanics defines a stiffness model for the segments in a segmented cell, assuming a chain-like segment arrangement. The curvature as well as the distance of consecutive segments are penalized with energy constraints. An optimal distance is estimated from the segments target volume. The shape of the potential can be selected via the stiffness model.

\section Example
Put the following xml block into the Definition of a segmented celltype
\verbatim
<RodMechanics model="solid|tube|exponential">
	<Axial stiffness="10" scaling="length" />
	<Bending stiffness="10" scaling="length" exponent="1.3" />
</RodMechanics>
\endverbatim
**/

class Rod_Mechanics : public CPM_Energy, public CPM_Update_Listener, public CPM_Check_Update /*, virtual Celltype_MCS_Listener*/
//, virtual public Cell_Interaction
{
	private:
		enum StiffnessModel { solid, tube, exponential } stiffness_model;
		double axial_stiffness;
		double bending_stiffness;
		double bending_parameter;
		double size_scale;
		double interface_strength;
		double interface_length_factor;
		
		string orientation_symbol_name;
		SymbolRWAccessor<VDOUBLE> orientation;
		string reversed_symbol_name;
		CellPropertyAccessor<double> reversed;
		CellPropertyAccessor<vector<double> > rod_energy;


		double reversal_time;
		bool do_reversal;
		SuperCT* celltype;
    uint lattice_dimensions;

		double hamiltonian(const std::vector< VDOUBLE >& centers, uint first_seg, uint last_seg, double size_scale) const;
		/** Returns the curvature in terms of 1/R², where R is the curvature radius 
		 *  of three consecutive points starting from iterator @param a
		 **/
		double curve3P(vector< VDOUBLE >::const_iterator p) const;

	public:
		Rod_Mechanics();
		DECLARE_PLUGIN("RodMechanics");
		
		virtual void loadFromXML(const XMLNode Node);
		virtual void init();
		
		virtual bool update_check(CPM::CELL_ID cell_id, const CPM::UPDATE& update, CPM::UPDATE_TODO todo);
		virtual double delta(CPM::CELL_ID cell_id, const CPM::UPDATE& update, CPM::UPDATE_TODO todo) const;
		virtual double hamiltonian(CPM::CELL_ID cell_id) const;
		virtual void update_notify(CPM::CELL_ID cell_id, const CPM::UPDATE& update, CPM::UPDATE_TODO todo);
// 		void mcs_notify(uint);
		// double interaction(double base_interaction, const CPM::STATE& State_a, const CPM::STATE& State_b) const;

};

class Rod_Segment_Interaction : virtual public Interaction_Overrider {
private:
	double segment_interaction;
	enum InteractionType { HeadAdhesion, ZeroNeighbors } interaction_type;

public:
	DECLARE_PLUGIN("RodSegmentInteraction");

	virtual void loadFromXML(const XMLNode Node);
	virtual void init(CellType* ) {};
	
	virtual double interaction(CPM::STATE s1, CPM::STATE s2, double base_interaction);
// 	virtual double delta( cell_id, const & update, _TODO todo) const;
// 	virtual double hamiltonian( cell_id) const;

};
