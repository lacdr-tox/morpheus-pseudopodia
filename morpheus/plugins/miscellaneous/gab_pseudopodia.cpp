// include the header file
#include "gab_pseudopodia.h"
// macro to register plugin in framework
REGISTER_PLUGIN(Pseudopodia);

Pseudopodia::Pseudopodia() : InstantaneousProcessPlugin(TimeStepListener::XMLSpec::XML_NONE) {

    maxGrowthTime.setXMLPath("max-growth-time");
    maxGrowthTime.setDefault("100");
    registerPluginParameter(maxGrowthTime);

    maxPseudopods.setXMLPath("max-pseudopods");
    maxPseudopods.setDefault("0");
    registerPluginParameter(maxPseudopods);

    field.setXMLPath("field");
    field.setGlobalScope();
    registerPluginParameter(field);

    movingDirection.setXMLPath("moving-direction");
    registerPluginParameter(movingDirection);
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

    //TODO addProperty for the pseudopods instead of map?
}

// called periodically during simulation
void Pseudopodia::executeTimeStep() {
    auto cells = celltype->getCellIDs();
    call_once(initPseudopods, [&]() {
        for (auto &cellId : cells) {
            pseudopods.insert(make_pair(cellId,
                                        vector<Pseudopod>((size_t) maxPseudopods(),
                                                          Pseudopod((unsigned int) maxGrowthTime(), cpmLayer.get(),
                                                                    cellId, &movingDirection, &field))));
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
    return 0;
}

double Pseudopodia::hamiltonian(CPM::CELL_ID cell_id) const {
    return 0;
};

bool Pseudopodia::update_check(CPM::CELL_ID cell_id, const CPM::Update &update) {
    // We want to block removal of cells in the neighborhood of the actin bundles
    if (!update.opRemove()) {
        return true;
    }

    auto cellPos = update.focusStateBefore().pos;
    auto latticeStencil = update.surfaceStencil();
    auto states = latticeStencil->getStates();
    auto stencil = latticeStencil->getStencil();
    auto inCellNeighbors = vector<VINT>({cellPos});
    for(auto it = states.begin(); it != states.end(); ++it) {
        if(*it == cell_id) {
            inCellNeighbors.push_back(cellPos + stencil.at(distance(states.begin(), it)));
        }
    }
    for(auto &pos: inCellNeighbors) {
        // Check if there is actin in the field for any of these
        if(field.get(pos) > 0) {
            cout << "blocked!" << endl;
            return false;
        }
    }

    return true;
}
