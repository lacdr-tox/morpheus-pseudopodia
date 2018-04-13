//
// Created by gerhard on 13-4-18.
//

#include "pseudopod.h"
#include "core/cell.h"

// returns a radial in phi, theta, radius. convert with from_radial()
VDOUBLE RandomVonMisesPoint(VDOUBLE mu, double kappa) {
    double ar, theta;
    do {
        ar = getRandom01();
        theta = 2 * M_PI * (getRandom01() - 0.5);
    } while (ar > exp(kappa * (cos(theta - mu.x) - 1.0)));

    return {theta, 0.0, 1.0};
}

Pseudopod::Pseudopod(unsigned int maxGrowthTime, const CPM::LAYER *cpm_layer, CPM::CELL_ID cellId,
          PluginParameter2<double, XMLReadableSymbol, RequiredPolicy> *movingDirection,
          PluginParameter2<double, XMLReadWriteSymbol, RequiredPolicy> *field, RetractionMethod retractionMethod,
          double kappaInit, double kappaCont, bool retractOnTouch) :
        maxGrowthTime_(maxGrowthTime), _cpm_layer(cpm_layer), cellId(cellId),
        movingDirection_(movingDirection), state_(State::INIT), field_(field),
        timeLeftForGrowth_(0), timeNoExtension_(0), paramRetractionMethod_(retractionMethod), currRetractionMethod_(retractionMethod), kappaInit_(kappaInit),
        kappaCont_(kappaCont), retractOnTouch_(retractOnTouch) {
    bundlePositions_.reserve(maxGrowthTime);
}

Pseudopod::State Pseudopod::state() const {
    return state_;
}

void Pseudopod::startNewBundle() {
    auto cellCenter = CPM::getCell(cellId).getCenter();
    if (_cpm_layer->get(VINT(cellCenter)).cell_id != cellId) {
        //com is not in the cell, return
        return;
    }
    // Successful start
    state_ = State::GROWING;
    timeLeftForGrowth_ = maxGrowthTime_;
    timeNoExtension_ = 0;

    addPosToBundle(cellCenter);

    //Set polarisation direction based on cell target direction
    auto preferredDirection = VDOUBLE(movingDirection_->get(SymbolFocus(cellId)), 0, 1);
    polarisationDirection_ = RandomVonMisesPoint(preferredDirection, kappaInit_);
}

void Pseudopod::addPosToBundle(VDOUBLE const &pos) {
    field_->set(VINT(pos), field_->get(VINT(pos)) + 1);
    bundlePositions_.emplace_back(pos);
}

void Pseudopod::setRetracting() {
    state_ = State::RETRACTING;
    if(paramRetractionMethod_ == RetractionMethod::IN_MOVING_DIR) {
        // Implementation JBB
        if(cos(polarisationDirection_.x - movingDirection_->get(SymbolFocus(cellId))) > 0.8) {
            currRetractionMethod_ = RetractionMethod::FORWARD;
        } else {
            currRetractionMethod_ = RetractionMethod::BACKWARD;
        }
    } else {
        currRetractionMethod_ = paramRetractionMethod_;
    }
}

void Pseudopod::growBundle() {
    assert(timeLeftForGrowth_ > 0);
    if (getRandom01() > extendprob_) {
        // No extension this time
        timeNoExtension_++;
        if (timeNoExtension_ > timeNoExtensionLimit_) {
            setRetracting();
        }
        return;
    }

    auto extendDirection = RandomVonMisesPoint(polarisationDirection_, kappaCont_);
    auto orthoOffset = VDOUBLE::from_radial(extendDirection);
    auto newBundlePosition = bundlePositions_.back() + orthoOffset;

    // get cell id of (possible) new bundle position
    auto newPosCellId = _cpm_layer->get(VINT(newBundlePosition)).cell_id;
    if (newPosCellId != cellId) {
        //newBundlePosition is not in this cell
        if(retractOnTouch_ && newPosCellId != CPM::getEmptyCelltypeID()) {
            // cell is touching 'something' else -> retract
            setRetracting();
        }
        return;
    }

    addPosToBundle(newBundlePosition);
    polarisationDirection_ = extendDirection;

    timeNoExtension_ = 0;
    timeLeftForGrowth_--;
    if (timeLeftForGrowth_ == 0) {
        setRetracting();
    }
}

void Pseudopod::decrementActinLevelAt(VINT pos) const {
    auto prevFieldVal = field_->get(pos);
    assert(prevFieldVal > 0);
    field_->set(pos, prevFieldVal - 1);
}


void Pseudopod::retractBundle() {
    if (getRandom01() > retractprob_) {
        //Nothing happens
        return;
    }

    VINT pos;
    if(currRetractionMethod_ == RetractionMethod::BACKWARD) {
        pos = bundlePositions_.back();
    } else if (currRetractionMethod_ == RetractionMethod::FORWARD) {
        pos = bundlePositions_.front();
    }

    decrementActinLevelAt(VINT(pos));

    if(currRetractionMethod_ == RetractionMethod::BACKWARD) {
        bundlePositions_.pop_back();
    } else if (currRetractionMethod_ == RetractionMethod::FORWARD) {
        bundlePositions_.erase(bundlePositions_.begin());
    }

    if (bundlePositions_.empty()) {
        // completely retracted, start over
        state_ = State::INIT;
    }
}

void Pseudopod::timeStep() {
    switch (state_) {
        case State::INIT:
            startNewBundle();
            break;
        case State::GROWING:
            growBundle();
            break;
        case State::RETRACTING:
            retractBundle();
            break;
        case State::INACTIVE:
            break;
    }
}

VDOUBLE Pseudopod::getBundleTip() const {
    switch (state_) {
        case State::RETRACTING:
        case State::GROWING:
            return *bundlePositions_.end();
        default:
            cerr << "Pseudopod::getBundleTip: pseudo in INIT or INACTIVE state, no bundle tip" << endl;
            return {-1, -1, -1};
    }
}
