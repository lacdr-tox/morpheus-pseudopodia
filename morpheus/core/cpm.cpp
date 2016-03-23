#include "cpm.h"
#include "interaction_energy.h"


CPMSampler::CPMSampler() :
	ContinuousProcessPlugin(MCS,XMLSpec::XML_NONE)
{
	mcs_duration.setXMLPath("MCSDuration/value");
	registerPluginParameter(mcs_duration);
	mcs_duration_symbol.setXMLPath("MCSDuration/symbol");
	registerPluginParameter(mcs_duration_symbol);
	
	stepper_type.setXMLPath("MetropolisKinetics/stepper");
	map<string, StepperType> stepper_map;
	stepper_map["edgelist"] = StepperType::EDGELIST;
	stepper_map["random"] = StepperType::RANDOM;
	stepper_type.setConversionMap(stepper_map);
	registerPluginParameter(stepper_type);
	
	metropolis_temperature.setXMLPath("MetropolisKinetics/temperature");
	registerPluginParameter(metropolis_temperature);
	
	metropolis_yield.setXMLPath("MetropolisKinetics/yield");
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
	
	boundary_neigbhborhood = SIM::lattice().getDefaultNeighborhood();
	XMLNode xNeighborhood = node.getChildNode("MetropolisKinetics").getChildNode("Neighborhood");
	if (!xNeighborhood.isEmpty()) {
		boundary_neigbhborhood = SIM::lattice().getNeighborhood(xNeighborhood);
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
	
	current_update.boundary = unique_ptr<LatticeStencil> (new LatticeStencil(cell_layer,edge_tracker->getNeighborhood()));
	current_update.interaction = unique_ptr<StatisticalLatticeStencil> (new StatisticalLatticeStencil(cell_layer,interaction_energy->getNeighborhood()));

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

const vector< VINT >& CPMSampler::getInteractionNeighborhood()
{
	return interaction_energy->getNeighborhood();
}

const vector< VINT >& CPMSampler::getBoundaryNeighborhood()
{
	return boundary_neigbhborhood;
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
		VINT source_pos, focus_pos;
		edge_tracker->get_update(source_pos, direction);
		lattice->resolve(source_pos); // wrapping topological boundaries, if necessary
		focus_pos = source_pos + direction;
		lattice->resolve(focus_pos);  // wrapping topological boundaries, if necessary
		
		current_update.add_state = cell_layer->get(source_pos);
		current_update.source.setCell(current_update.add_state.cell_id, source_pos);
		if (! lattice->equal_pos(current_update.add_state.pos, current_update.source.pos()))
			current_update.add_state.pos = current_update.source.pos();
		
		// now the position tracker of add_state gets moved to the "focus"
		current_update.add_state.pos += direction;
		
		current_update.remove_state = cell_layer->get(focus_pos);
		current_update.focus.setCell(current_update.remove_state.cell_id, focus_pos);
		
		assert(current_update.add_state.cell_id < CPM::NO_CELL);
		assert(current_update.remove_state.cell_id < CPM::NO_CELL);
		if (current_update.remove_state.cell_id == current_update.add_state.cell_id) continue;
		
		CPM::setUpdate(current_update);
		
		// and we check whether the update should take place
		if ( evalCPMUpdate(current_update) ) {
			success += CPM::executeCPMUpdate(current_update);
		}
	}
}

bool CPMSampler::evalCPMUpdate(CPM::UPDATE& update)
{
	double dE=0, dInteraction;
	
	// Find the proper celltype to notify
	uint source_ct = update.source_top_ct;
	uint focus_ct =  update.focus_top_ct;
	
	if ( focus_ct == source_ct ) {
		if ( ! celltypes[focus_ct] -> check_update(update, CPM::ADD_AND_REMOVE) )
			return false;
	}
	else {
		if ( ! celltypes[source_ct] -> check_update(update, CPM::ADD) )
			return false;
		if ( ! celltypes[focus_ct] -> check_update( update, CPM::REMOVE) )
			return false;
	}
	
	// InteractionEnergy
	dE += interaction_energy -> delta(update);
	// CellType dependend energies
	if ( focus_ct == source_ct ) {
		dE += celltypes[source_ct] -> delta(update, CPM::ADD_AND_REMOVE);
	}
	else {
		dE += celltypes[source_ct] -> delta( update,  CPM::ADD );
		dE += celltypes[focus_ct] -> delta( update, CPM::REMOVE );
	}
	// the magic Metropolis Kinetics with Boltzmann probability ...
	dE += metropolis_yield(); //metropolis_yield(update.focus);

	if (dE <= 0)
		return true;
	
	double p = exp(-dE / metropolis_temperature());
	double rnd = getRandom01();

	return rnd < p;
}

