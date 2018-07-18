// include the header file
#include "gab_pseudopodia.h"
// macro to register plugin in framework
REGISTER_PLUGIN(Pseudopodia);

Pseudopodia::Pseudopodia() : InstantaneousProcessPlugin(TimeStepListener::XMLSpec::XML_NONE) {

    maxGrowthTime.setXMLPath("max-growth-time");
    maxGrowthTime.setDefault("20");
    registerPluginParameter(maxGrowthTime);

    directionalStrengthInit.setXMLPath("init-dir-strength");
    directionalStrengthInit.setDefault("8.0");
    registerPluginParameter(directionalStrengthInit);

    directionalStrengthCont.setXMLPath("cont-dir-strength");
    directionalStrengthCont.setDefault("1e4");
    registerPluginParameter(directionalStrengthCont);

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

    touchBehavior.setXMLPath("touch-behavior");
    touchBehavior.setDefault("nothing");
    map<string, Pseudopod::TouchBehavior> touchBehaviorMap;
    touchBehaviorMap["nothing"] = Pseudopod::TouchBehavior::NOTHING;
    touchBehaviorMap["attach"] = Pseudopod::TouchBehavior::ATTACH;
    touchBehaviorMap["retract"] = Pseudopod::TouchBehavior::RETRACT;
    touchBehavior.setConversionMap(touchBehaviorMap);
    registerPluginParameter(touchBehavior);
}

// called during initialization
void Pseudopodia::init(const Scope *scope) {
    // initialize the plugin
    InstantaneousProcessPlugin::init(scope);

    if (SIM::getLattice()->getStructure() != Lattice::Structure::square) {
        throw MorpheusException("Only works for square lattices", "Pseudopodia");
    }

    setTimeStep(CPM::getMCSDuration());


    cpmLayer = CPM::getLayer();
    cellType = scope->getCellType();

    //TODO addProperty for the pseudopods instead of map?
}

// called periodically during simulation
void Pseudopodia::executeTimeStep() {
    auto cells = cellType->getCellIDs();

    // This is only called the first time to allocate space for pseudopod storage
    call_once(initPseudopods, [&]() {
        for (auto &cellId : cells) {
            auto pseudopod = Pseudopod((unsigned int) maxGrowthTime(), cpmLayer.get(),
                                       cellId, &movingDirection, &field, retractionMethod(), directionalStrengthInit(),
                                       directionalStrengthCont(), touchBehavior());
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
    double change{0};
    /* These bonuses only apply when the medium is involved, otherwise the pseudopods might cause the cells to "drill"
     * in each other
     */
    if(update.focusStateBefore().cell_id == CPM::getEmptyState().cell_id
       || update.focusStateAfter().cell_id == CPM::getEmptyState().cell_id) {
        change += calcNeighboringActinBonus(update);
        change += calcPseudopodTipBonus(cell_focus, update);
    }
    return change;
}

double Pseudopodia::calcNeighboringActinBonus(const CPM::Update &update) const {
    // We are only interested in adding stuff, the rest is unchanged
    if(!update.opAdd()) return 0.0;

    auto pos = update.focus().pos();
    auto neighbors = getLattice()->getNeighborhoodByOrder(2).neighbors();
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

double Pseudopodia::minDistanceToPseudopodTip(const VINT pos, const CPM::CELL_ID& cellId) const {
    auto pseudopods = getPseudopodsForCell(cellId);
    // Remove pseudopods that have no tips (yet)
    pseudopods.erase(
            std::remove_if(
                    pseudopods.begin(),
                    pseudopods.end(),
                    [](const Pseudopod& pseudopod) { return !pseudopod.hasBundleTip(); }
            ),
            pseudopods.end()
    );
    if(pseudopods.empty()) return std::numeric_limits<double>::infinity();
    std::vector<double> distancePosToTip;
    std::transform(pseudopods.begin(), pseudopods.end(), std::back_inserter(distancePosToTip),
                   [&](Pseudopod &pseudopod) -> double {
        return dist(pos, VINT(pseudopod.getBundleTip()));
    });
    return *std::min_element(distancePosToTip.begin(), distancePosToTip.end());

}

double Pseudopodia::calcPseudopodTipBonus(const SymbolFocus &cell_focus, const CPM::Update &update) const {
    auto pos = cell_focus.pos();
    // if update.opAdd() this will be the new cellID, if update.opRemove() it will be the old cellID
    auto cellID = cell_focus.cellID();

    auto isCloseToOwnPseudoPod = FALSE;
    if(minDistanceToPseudopodTip(pos, cellID) <= pseudopodTipBonusMaxDistance) {
        isCloseToOwnPseudoPod = TRUE;
    }
    auto isTouchingNeighbor = FALSE;
    // TODO find better way of getting nearby cells?
    auto neighbors = getLattice()->getNeighborhoodByOrder(2).neighbors();
    for(auto const& neighbor : neighbors) {
        auto neighborPos = pos + neighbor;
        auto neighborCellId = cpmLayer->get(neighborPos).cell_id;
        if(neighborCellId == cellID || neighborCellId == CPM::getEmptyState().cell_id) continue;
        // if neighbor belongs to a different cell and is close to a pseudopod tip -> give bonus
        isTouchingNeighbor = TRUE;
        if(minDistanceToPseudopodTip(pos, neighborCellId) <= pseudopodTipBonusMaxDistance) {
            if(update.opAdd()) {
                // Make more likely
                return (isCloseToOwnPseudoPod ? 2 : 1) * -pseudopodTipBonus;
            } else if(update.opRemove()){
                // Make less likely
                return (isCloseToOwnPseudoPod ? 2 : 1) * pseudopodTipBonus;
            }
        }
    }
    // not close to any neighbor pseudopod, take own pseudopod bonus into account ONLY if touching
    if(isTouchingNeighbor && isCloseToOwnPseudoPod) {
        if(update.opAdd()) {
            // Make more likely
            return -pseudopodTipBonus;
        } else if(update.opRemove()){
            // Make less likely
            return pseudopodTipBonus;
        }
    }
    return 0;
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

vector<Pseudopod> Pseudopodia::getPseudopodsForCell(const CPM::CELL_ID &cell_id) const {
    auto it = pseudopods.find(cell_id);
    return it != pseudopods.end() ? it->second : vector<Pseudopod>();
}

