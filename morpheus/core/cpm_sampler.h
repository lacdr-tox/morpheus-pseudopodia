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

#ifndef CPM_SAMPLER_H
#define CPM_SAMPLER_H

#include <map>
#include "interfaces.h"
#include "simulation.h"
#include "plugin_parameter.h"
#include "edge_tracker.h"
#include "interaction_energy.h"

/**
\defgroup ML_CPM CPM
\ingroup MorpheusML ContinuousProcessPlugins

Specifies parameters for a cellular Potts model (CPM) which provides a MonteCarlo sampler that evolves a spatial cell configuration on the basis of a Hamiltonian definition by statistical sampling.

\f$ H = \f$

\f$ P = \f$

\b ShapeSurface specifies the Neighborhood used to estimate the boundary length of CPM Shapes, in particular cells. This estimate is used for computing interaction energies, cell perimeters and interface lengths.
  - \b scaling scaling of number of neighbors to length: \b norm estimate the length in unit of node length (see Magno, Grieneisen and Marée, BMC Biophysics, 2015), \b size neigborhood fraction occupied by other entities, \b none number of neighbors occupied by other entities.
  - \b Neigborhood defines the stencil size to approximate the surface length. Wrt. to shape isotropy some neighborhoods are favourable: 
    - square  -- 6th order corresponding to a distance of 3
    - hexagonal -- 3rd order, corresponding to a distance of 2
    - cubic  -- 7th order  corresponding to a distance of \f$ 2 \sqrt 2 \f$ 

\ref ML_Interaction specifies interaction energies \f$ J_{\sigma, \sigma} \f$ for different inter-cellular \ref ML_Contact. The interaction energy given per length unit as defined in ShapeSurface.


\b MonteCarloSampler
  - \b stepper: \b edgelist chooses updates from a tracked list of lattice sites that can potentially change configuration; \b random sampling chooses lattice site with uniform random distribution over all lattice sites.
  - \b MetropolisKinetics:
    - \b temperature: specifies Boltzmann probability to accept updates that increase energy, required to be homogeneous in space.
    - \b yield: offset for Boltzmann probability distribution representing resistance to membrane deformations (see Kafer, Hogeweg and Maree, PLoS Comp Biol, 2006).
  - \b Neighborhood specifies the neighborhood size used for choosing updates in the modified Metropolis algorithm.
  - \b MCSDuration scales the Monte Carlo Step (MCS) to the simulation time. One MCS is defined as a number of update attempts equal to the number of lattice sites.
    
\section References

Graner, Glazier, 1992

Kafer, Hogeweg and Maree, PLoS Comp Biol, 2006

Magno, Grieneisen and Marée, BMC Biophysics, 2015


\defgroup ML_Interaction Interaction
\ingroup ML_CPM
specifies interaction energies \f$ J_{\sigma, \sigma} \f$ for different inter-cellular \ref ML_Contact. The interaction energy given per length unit as defined in ShapeSurface.

 - \b default: default value for unspecified interactions
 - \b negate: negate all defined interaction values
 

\defgroup ML_Contact Contact
\ingroup ML_Interaction
**/



class CPMSampler : public ContinuousProcessPlugin {
public:
	CPMSampler();
	~CPMSampler() { cout << "Deleting the CPM sampler" << endl; };
	
    virtual void loadFromXML(const XMLNode node, Scope* scope) override;
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
	mutable double cached_temp;
};

#endif
