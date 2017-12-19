// include the header file
#include "gab_pseudopodia.h"
// macro to register plugin in framework
REGISTER_PLUGIN(Pseudopodia);

Pseudopodia::Pseudopodia() : InstantaneousProcessPlugin(TimeStepListener::XMLSpec::XML_NONE) {

    maxGrowthTime.setXMLPath("max-growth-time");
    maxGrowthTime.setDefault("20");
    registerPluginParameter(maxGrowthTime);

    directionalStrength.setXMLPath("dir-strength");
    directionalStrength.setDefault("8.0");
    registerPluginParameter(directionalStrength);

    maxPseudopods.setXMLPath("max-pseudopods");
    maxPseudopods.setDefault("0");
    registerPluginParameter(maxPseudopods);

    field.setXMLPath("field");
    field.setGlobalScope();
    registerPluginParameter(field);

    movingDirection.setXMLPath("moving-direction");
    registerPluginParameter(movingDirection);

    retractionMethod.setXMLPath("retraction-mode");
    retractionMethod.setDefault("backward");
    map<string, Pseudopod::RetractionMethod> retractionMethodMap;
    retractionMethodMap["backward"] = Pseudopod::RetractionMethod::BACKWARD;
    retractionMethodMap["forward"] = Pseudopod::RetractionMethod::FORWARD;
    retractionMethodMap["in-moving-direction"] = Pseudopod::RetractionMethod::IN_MOVING_DIR;
    retractionMethod.setConversionMap(retractionMethodMap);
    registerPluginParameter(retractionMethod);

    retractOnTouch.setXMLPath("retract-on-touch");
    retractOnTouch.setDefault("false");
    registerPluginParameter(retractOnTouch);
}


// called before initialization
void Pseudopodia::loadFromXML(const XMLNode xNode) {
    if (SIM::getLattice()->getStructure() != Lattice::Structure::square) {
        throw MorpheusException("Pseudopodia: Only works for square lattices", xNode);
    }
    // plugin loads parameters according to the XML paths set in constructor
    InstantaneousProcessPlugin::loadFromXML(xNode);
}

// called during initialization
void Pseudopodia::init(const Scope *scope) {
    // initialize the plugin
    InstantaneousProcessPlugin::init(scope);
    setTimeStep(CPM::getMCSDuration());


    cpmLayer = CPM::getLayer();
    celltype = scope->getCellType();
    neighboringActinBonus = 10000.0;

    //TODO addProperty for the pseudopods instead of map?
}

// called periodically during simulation
void Pseudopodia::executeTimeStep() {
    auto cells = celltype->getCellIDs();

    // This is only called the first time to allocate space for pseudopod storage
    call_once(initPseudopods, [&]() {
        for (auto &cellId : cells) {
            auto pseudopod = Pseudopod((unsigned int) maxGrowthTime(), cpmLayer.get(),
                                       cellId, &movingDirection, &field, retractionMethod(), directionalStrength(),
                                       retractOnTouch());
            pseudopods.insert(make_pair(cellId, vector<Pseudopod>((size_t) maxPseudopods(), pseudopod)));
        }
    });
    assert(pseudopods.size() == cells.size()); // We don't handle cell death or proliferation
    for (auto &it : pseudopods) {
        assert(CPM::cellExists(it.first));
        if (CPM::getCell(it.first).getNodes().empty()
            //FIXME HACK 0.0 is the default, we want to wait for a real moving direction
            || movingDirection(SymbolFocus(it.first)) == 0.0)
            continue;
        int test = 0;

        for (auto &pseudopod : it.second) {
            pseudopod.timeStep();
            test++;
        }
    }
}

double Pseudopodia::delta(const SymbolFocus &cell_focus, const CPM::Update &update) const {
    // We are only interested in adding stuff, the rest is unchanged
    if(!update.opAdd()) return 0.0;

    auto pos = update.focus().pos();
    auto neighbors = SIM::getLattice()->getNeighborhoodByOrder(2).neighbors();
    for(auto const& neighbor : neighbors) {
        auto neighborPos = pos + neighbor;
        // if neighbor belongs to the same cell and has positive actin level -> give bonus
        if(cpmLayer->get(neighborPos).cell_id == update.source().cellID()
            && field.get(neighborPos) > 0) {
            return -neighboringActinBonus;
        }

    }
    // no change
    return 0.0;
}

double Pseudopodia::hamiltonian(CPM::CELL_ID cell_id) const {
    return 0;
};

// We want to block removal of cells in the neighborhood of the actin bundles
bool Pseudopodia::update_check(CPM::CELL_ID cell_id, const CPM::Update &update) {
    // If not removal continue
    if (!update.opRemove()) {
        return true;
    }

    auto pos = update.focusStateBefore().pos;
    auto latticeStencil = update.surfaceStencil();
    auto states = latticeStencil->getStates();
    auto stencil = latticeStencil->getStencil();
    for(size_t i = 0; i < states.size(); ++i) {
        // if Neighbor has the same cell && has an actin bundle
        if(states.at(i) == cell_id && field.get(pos + stencil.at(i)) > 0) {
            return false;
        }
    }

    return true;
}
