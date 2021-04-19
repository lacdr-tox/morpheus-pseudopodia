//////
//
// This file is part of the modelling and simulation framework 'Morpheus',
// and is made available under the terms of the BSD 3-clause license (see LICENSE
// file that comes with the distribution or https://opensource.org/licenses/BSD-3-Clause).
//
// Authors:  Joern Starruss
// Copyright 2009-2021, Technische Universität Dresden, Germany
//
//////

#ifndef MECHANICALLINK_H
#define MECHANICALLINK_H

#include "core/interfaces.h"
#include "core/celltype.h"

/** \defgroup ML_MechanicalLink MechanicalLink
 \ingroup ML_CellType
 \ingroup CellMotilityPlugin
 \ingroup CPM_EnergyPlugins

 Spring-like mechanics excerted by links between cells. Equilibrium link distance is reached when the distance of their centers equals the sum of their radii assuming circular / spherical cell shape. Links always detach with a certain probability given by an expression, but may form only when cells are in touch. All expressions provide the scopes of the involved cells via the named scopes \b cell1 and \b cell2 ( property 'k1' from the first cell is thus \b cell1.k1 ).
 
 - \b strength: Spring konstant of a link that determines the springs energy when multiplied with the relative stretch of the spring link.
 - \b link-probability: Probability to establish a link with a touching cell.
 - \b unlink-probability: Probability to release an established link. Local variables \b stretch.abs and \b stretch.rel provide the absolute and relative stretch of the respective bond.

 The implementation is inspired by the paper below, but allows for much broader parameterisations. Use the \b CellLinks plot option of the \ref Gnuplotter to vizualize the link data. Currently, only links amoung cells of the same cell type may be created. Future versions will drop this limitation.
 
 András Szabó, Manuela Melchionda, Giancarlo Nastasi, Mae L. Woods, Salvatore Campo, Roberto Perris, Roberto Mayor; <i>In vivo confinement promotes collective migration of neural crest cells.</i> J Cell Biol 6 June 2016; 213 (5): 543–555. doi: https://doi.org/10.1083/jcb.201602083.
 
*/

class MechanicalLink : public InstantaneousProcessPlugin, public CPM_Energy
{
public: 
	typedef vector<CPM::CELL_ID> LinkType;
private:
	PluginParameter2<double, XMLEvaluator, RequiredPolicy> strength;
	PluginParameter2<double, XMLEvaluator, RequiredPolicy> link_probability;
	PluginParameter2<double, XMLEvaluator, RequiredPolicy> unlink_probability;
	
	int ns1_id, ns2_id;
	bool use_ns_strength, use_ns_link, use_ns_unlink;

	CellType* celltype;
	CellType::PropertyAccessor<LinkType> bonds;
	
	bool insertBond(const SymbolFocus& cell_a, const SymbolFocus& cell_b) const;
	bool removeBond(const SymbolFocus& cell_a, const SymbolFocus& cell_b) const;

public:
	MechanicalLink();
	DECLARE_PLUGIN("MechanicalLink")
	void loadFromXML(const XMLNode node, Scope * scope) override;
	void init(const Scope* scope) override;
	void executeTimeStep() override;
	double delta( const SymbolFocus& cell_focus, const CPM::Update& update) const override;
	double hamiltonian(CPM::CELL_ID) const override;  
};

#endif // MECHANICALLINK_H
