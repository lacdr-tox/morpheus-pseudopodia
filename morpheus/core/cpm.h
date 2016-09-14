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

#ifndef CPM_H
#define CPM_H

#include <map>
#include "interfaces.h"
#include "simulation.h"
#include "plugin_parameter.h"
#include "edge_tracker.h"
#include "interaction_energy.h"

/** \defgroup CPM
\ingroup ContinuousProcessPlugins

\section Description
CPM provides a MonteCarlo sampler, that evolves a spatial cell configuration on the basis of a Hamiltonian definition
by statistical sampling. A MonteCarloStep is corresonds to the exectution of update attempts equal to the number of lattice sites.
Its relation to simulation time is provided trough MCSDuration tag.

The neighborhood used for choosing update neighbors can be selected via Neighborhood tag.

The tendency to accept configuration update increasing the Hamiltonian energy can be adjusted via MetropolisKinetics.

*/

/**
 * 
 */

class CPMSampler : public ContinuousProcessPlugin {
public:
	CPMSampler();
	~CPMSampler() { cout << "Deleting the CPM sampler" << endl; };
	
    virtual void loadFromXML(const XMLNode node) override;
	double MCSDuration() { return mcs_duration(); }
	virtual void prepareTimeStep() override {};
	virtual void executeTimeStep() override ;
	virtual string XMLName() const override { return string("CPM"); };
	
    virtual void init(const Scope* scope) override;
	const Neighborhood& getInteractionNeighborhood();
	const Neighborhood& getUpdateNeighborhood();
	vector< multimap< Plugin*, SymbolDependency > > getCellTypeDependencies() const;
	set< SymbolDependency > getInteractionDependencies() const;
	
private:
	enum class StepperType { EDGELIST, RANDOM };
	PluginParameter2<double,XMLValueReader,RequiredPolicy> mcs_duration;
	PluginParameter2<string,XMLValueReader,OptionalPolicy> mcs_duration_symbol;
	PluginParameter2<StepperType,XMLNamedValueReader,RequiredPolicy> stepper_type;
	PluginParameter2<double,XMLEvaluator,RequiredPolicy> metropolis_temperature;
	PluginParameter2<double,XMLValueReader,DefaultValPolicy> metropolis_yield;
	
// 	CPM::UPDATE current_update;

	Neighborhood update_neighborhood;
	shared_ptr<const EdgeTrackerBase> edge_tracker;
	shared_ptr<InteractionEnergy> interaction_energy;
	
	///  Run one MonteCarloStep, i.e. as many updates as determined by the mcs stepper
	void MonteCarloStep();
	bool evalCPMUpdate(const CPM::Update& update);
	
	shared_ptr<const CPM::LAYER> cell_layer;
	vector <std::shared_ptr <const CellType > > celltypes;
    shared_ptr< const Lattice > lattice;
};

#endif
