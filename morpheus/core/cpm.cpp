#include "cpm.h"
#include "interaction_energy.h"


CPMSampler::CPMSampler() :
	ContinuousProcessPlugin(MCS,XMLSpec::XML_NONE)
{
	mcs_duration.setXMLPath("MonteCarloSampler/MCSDuration/value");
	registerPluginParameter(mcs_duration);
	mcs_duration_symbol.setXMLPath("MonteCarloSampler/MCSDuration/symbol");
	registerPluginParameter(mcs_duration_symbol);
	
	stepper_type.setXMLPath("MonteCarloSampler/stepper");
	map<string, StepperType> stepper_map;
	stepper_map["edgelist"] = StepperType::EDGELIST;
	stepper_map["random"] = StepperType::RANDOM;
	stepper_type.setConversionMap(stepper_map);
	registerPluginParameter(stepper_type);
	
	metropolis_temperature.setXMLPath("MonteCarloSampler/MetropolisKinetics/temperature");
	registerPluginParameter(metropolis_temperature);
	
	metropolis_yield.setXMLPath("MonteCarloSampler/MetropolisKinetics/yield");
	metropolis_yield.setDefault("0");
	registerPluginParameter(metropolis_yield);
};


void CPMSampler::loadFromXML(const XMLNode node)
{
    ContinuousProcessPlugin::loadFromXML(node);
	
	if (mcs_duration_symbol.isDefined()) {
		auto symbol = Property<double>::createConstantInstance(mcs_duration_symbol.stringVal(),"Monte Carlo Step Duration");
		symbol->set(mcs_duration.get());
		SIM::defineSymbol(symbol);
	}
	
	update_neighborhood = SIM::lattice().getNeighborhoodByDistance(1.9); // 
	XMLNode xNeighborhood = node.getChildNode("MonteCarloSampler").getChildNode("Neighborhood");
	if (!xNeighborhood.isEmpty()) {
		update_neighborhood = SIM::lattice().getNeighborhood(xNeighborhood);
	}
	
	interaction_energy = shared_ptr<InteractionEnergy>(new InteractionEnergy());
	interaction_energy->loadFromXML(node.getChildNode("Interaction"));

	setTimeStep(mcs_duration.get());
	is_adjustable = false;
}

void CPMSampler::init(const Scope* scope)
{
    ContinuousProcessPlugin::init(scope);
	cell_layer = CPM::getLayer();
	lattice = cell_layer->getLattice();
	
	if (stepper_type() == StepperType::EDGELIST) {
		CPM::enableEgdeTracking();
	}
	else if (stepper_type() == StepperType::RANDOM) {
		// That's the default case;
	}
	
	edge_tracker = CPM::cellEdgeTracker();
	
	interaction_energy->init(scope);
	registerInputSymbols( interaction_energy->getDependSymbols() );
	
// 	current_update = CPM::createUpdate(); // we also might require a Stencil for the connectivity constraint.
// 	current_update.boundary = unique_ptr<LatticeStencil> (new LatticeStencil(cell_layer,edge_tracker->getNeighborhood()));
// 	current_update.interaction = unique_ptr<StatisticalLatticeStencil> (new StatisticalLatticeStencil(cell_layer,interaction_energy->getNeighborhood()));

	auto weak_celltypes = CPM::getCellTypes();
	for (auto wct : weak_celltypes) {
		auto ct = wct.lock();
		celltypes.push_back(ct);
		auto dependencies =  ct->cpmDependSymbols();
		for (auto& dep : dependencies) {
			registerInputSymbol( dep.second.name, dep.second.scope );
		}
		registerCellPositionOutput();
	}
}

vector<multimap< Plugin*, SymbolDependency > > CPMSampler::getCellTypeDependencies() const
{
	vector<multimap< Plugin*, SymbolDependency > > res;
	auto weak_celltypes = CPM::getCellTypes();
	for (auto wct : weak_celltypes) {
		auto ct = wct.lock();
		res.push_back(ct->cpmDependSymbols() );
	}
	return res;
}

set< SymbolDependency > CPMSampler::getInteractionDependencies() const 
{
	return interaction_energy->getDependSymbols();
}

const Neighborhood& CPMSampler::getInteractionNeighborhood()
{
	return interaction_energy->getNeighborhood();
}

const Neighborhood& CPMSampler::getUpdateNeighborhood()
{
	return update_neighborhood;
}

void CPMSampler::executeTimeStep()
{
	MonteCarloStep();
}

void CPMSampler::MonteCarloStep() 
{
	int success=0;
	assert(edge_tracker != NULL);
	uint nupdates = edge_tracker->updates_per_mcs();
	for (uint i=0; i < nupdates; ++i) {
		// an update is actually a copy operation of a value at source to position source + direction
		// in "cpm language" source + direction is the "focus" (W. de Back et. al.)
		VINT direction;
		VINT source_pos(-1,-1,0);
		edge_tracker->get_update(source_pos, direction);
		const CPM::Update& current_update = CPM::createUpdate( source_pos, direction, CPM::Update::Operation::Extend);

		if (current_update.focusStateBefore().cell_id == current_update.focusStateAfter().cell_id) continue;
		
		// and we check whether the update should take place
		if ( evalCPMUpdate(current_update) ) {
			success += CPM::executeCPMUpdate(current_update);
		}
	}
}

bool CPMSampler::evalCPMUpdate(const CPM::Update& update)
{
	double dE=0, dInteraction;
	
	// Find the proper celltype to notify
	uint source_ct = update.source().celltype();
	uint focus_ct =  update.focus().celltype();
	
	if ( focus_ct == source_ct ) {
		if ( ! celltypes[focus_ct] -> check_update(update) )
			return false;
	}
	else {
		if ( ! celltypes[source_ct] -> check_update(update.selectOp(CPM::Update::ADD)) )
			return false;
		if ( ! celltypes[focus_ct] -> check_update( update.selectOp(CPM::Update::REMOVE)) )
			return false;
	}
	
	// InteractionEnergy
	dE += interaction_energy -> delta(update);
	// CellType dependend energies
	if ( focus_ct == source_ct ) {
		dE += celltypes[source_ct] -> delta(update);
	}
	else {
		dE += celltypes[source_ct] -> delta( update.selectOp(CPM::Update::ADD));
		dE += celltypes[focus_ct] -> delta( update.selectOp(CPM::Update::REMOVE));
		// TODO crawl through the neighborhood for CPM::Update::Neighborhood energy changes, if any CPMEnergy requires that
	}
	// the magic Metropolis Kinetics with Boltzmann probability ...
	dE += metropolis_yield(); //metropolis_yield(update.focus);

	if (dE <= 0)
		return true;
	
	double p = exp(-dE / metropolis_temperature());
	double rnd = getRandom01();

	return rnd < p;
}

