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

#ifndef MECHANICALLINK_H
#define MECHANICALLINK_H

#include "core/interfaces.h"
#include "core/celltype.h"

/** \defgroup MechanicalLink TODO
 \ingroup ML_CellType
 \ingroup CellMotilityPlugin
 \ingroup CPM_EnergyPlugins

 Oh god, it's alive!

*/

class MechanicalLink : public InstantaneousProcessPlugin, public CPM_Energy
{	
private:
	
	PluginParameter2<double, XMLEvaluator, RequiredPolicy> target_volume;
    PluginParameter2<double, XMLEvaluator, RequiredPolicy> strength;

    PluginParameter2<double, XMLValueReader, RequiredPolicy> link_probability;
    PluginParameter2<double, XMLValueReader, RequiredPolicy> max_bond_stretch;

	CellType* celltype;

	double d_0;
    bool **bonds;
    unsigned int cell_number;
    set<unsigned int> neighbor_set;
	
public:
	MechanicalLink();
	DECLARE_PLUGIN("MechanicalLink")

    void init(const Scope* scope) override;
    void executeTimeStep() override;
	double delta( const SymbolFocus& cell_focus, const CPM::Update& update) const override;
	double hamiltonian(CPM::CELL_ID cell_id) const override;  

};

#endif // MECHANICALLINK_H
