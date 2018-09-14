//
// Created by gerhard on 13-4-18.
//

#include "pseudopod.h"
#include "core/cell.h"

// returns a radial in phi, theta, radius. convert with from_radial()
static VDOUBLE RandomVonMisesPoint(VDOUBLE mu, double kappa) {
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
          double kappaInit, double kappaCont, TouchBehavior touchBehavior) :
        maxGrowthTime_(maxGrowthTime), _cpm_layer(cpm_layer), cellId(cellId),
        movingDirection_(movingDirection), state_(State::INACTIVE), field_(field),
        timeLeftForGrowth_(0), timeNoExtension_(0), paramRetractionMethod_(retractionMethod), currRetractionMethod_(retractionMethod), kappaInit_(kappaInit),
        kappaCont_(kappaCont), touchBehavior_(touchBehavior) {
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

void Pseudopod::setRetracting(RetractionMethod retractionMethod) {
    state_ = State::RETRACTING;
    if(retractionMethod == RetractionMethod::IN_MOVING_DIR) {
        // Implementation JBB
        if(cos(polarisationDirection_.x - movingDirection_->get(SymbolFocus(cellId))) > 0.8) {
            currRetractionMethod_ = RetractionMethod::FORWARD;
        } else {
            currRetractionMethod_ = RetractionMethod::BACKWARD;
        }
    } else {
        currRetractionMethod_ = retractionMethod;
    }
}

void Pseudopod::growBundle() {
    assert(timeLeftForGrowth_ > 0);
    if (getRandom01() > extendprob_) {
        // No extension this time
        timeNoExtension_++;
        if (timeNoExtension_ > timeNoExtensionLimit_) {
            setRetracting(paramRetractionMethod_);
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
        if(newPosCellId != CPM::getEmptyState().cell_id) {
            // cell is touching 'something' else -> do touch behavior
            switch(touchBehavior_) {
                case TouchBehavior::NOTHING:
                    break;
                case TouchBehavior::RETRACT:
                    setRetracting(paramRetractionMethod_);
                    break;
                case TouchBehavior::ATTACH:
                    state_ = State::TOUCHING;
                    break;
            }
        }
        return;
    }

    addPosToBundle(newBundlePosition);
    polarisationDirection_ = extendDirection;

    timeNoExtension_ = 0;
    timeLeftForGrowth_--;
    if (timeLeftForGrowth_ == 0) {
        setRetracting(paramRetractionMethod_);
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
        pos = VINT(bundlePositions_.back());
    } else if (currRetractionMethod_ == RetractionMethod::FORWARD) {
        pos = VINT(bundlePositions_.front());
    }

    decrementActinLevelAt(pos);

    if(currRetractionMethod_ == RetractionMethod::BACKWARD) {
        bundlePositions_.pop_back();
    } else if (currRetractionMethod_ == RetractionMethod::FORWARD) {
        bundlePositions_.erase(bundlePositions_.begin());
    }

    if (bundlePositions_.empty()) {
        // completely retracted, start over
        state_ = State::INACTIVE;
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
        case State::TOUCHING:
            if (getRandom01() < touch_retractprob) {
                setRetracting(RetractionMethod::FORWARD);
            }
            break;
        case State::RETRACTING:
            retractBundle();
            break;
        case State::INACTIVE:
            if(getRandom01() < time_between_extensions) {
                state_ = State::INIT;
            }
            break;
    }
}

VDOUBLE Pseudopod::getBundleTip() const {
    if(!hasBundleTip()) {
        cerr << "Pseudopod::getBundleTip: pseudo in INIT or INACTIVE state, no bundle tip" << endl;
        throw MorpheusException("No bundle tip", "Pseudopod::getBundleTip");
    }
    return bundlePositions_.back();
}

bool Pseudopod::hasBundleTip() const {
    return !bundlePositions_.empty();
}
