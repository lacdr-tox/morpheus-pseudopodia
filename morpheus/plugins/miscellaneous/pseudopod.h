//
// Created by gerhard on 28/07/17.
//

#include "core/interfaces.h"
#include "core/plugin_parameter.h"

#ifndef MORPHEUS_PSEUDOPOD_H
#define MORPHEUS_PSEUDOPOD_H

// returns a radial in phi, theta, radius. convert with from_radial()
VDOUBLE RandomVonMisesPoint(VDOUBLE mu, double kappa) {
    double ar, theta;
    do {
        ar = getRandom01();
        theta = 2 * M_PI * (getRandom01() - 0.5);
    } while (ar > exp(kappa * (cos(theta - mu.x) - 1.0)));

    return {theta, 0.0, 1.0};
}

class Pseudopod {
public:
    enum class State {
        INIT,
        GROWING,
        RETRACTING,
        INACTIVE
    };
    enum class RetractionMethod {
        FORWARD,
        BACKWARD,
        WEIRD
    };

private:
    vector<VDOUBLE> bundlePositions_;
    unsigned int timeLeftForGrowth_;
    unsigned int maxGrowthTime_;
    unsigned int timeNoExtension_;
    static constexpr auto timeNoExtensionLimit_ = 20;
    static constexpr auto retractionMethod_ = RetractionMethod::BACKWARD;
    VDOUBLE polarisationDirection_;
    static constexpr auto kappa_ = 4.0;
    static constexpr auto retractprob_ = .3;
    static constexpr auto extendprob_ = .3;
    State state_;
    const CPM::CELL_ID cellId;
    const CPM::LAYER *_cpm_layer;
    PluginParameter2<double, XMLReadWriteSymbol, RequiredPolicy> *field_;
    PluginParameter2<double, XMLReadableSymbol, RequiredPolicy> *movingDirection_;

public:
    Pseudopod(unsigned int maxGrowthTime, const CPM::LAYER *cpm_layer, CPM::CELL_ID cellId,
              PluginParameter2<double, XMLReadableSymbol, RequiredPolicy> *movingDirection,
              PluginParameter2<double, XMLReadWriteSymbol, RequiredPolicy> *field) :
            maxGrowthTime_(maxGrowthTime), _cpm_layer(cpm_layer), cellId(cellId),
            movingDirection_(movingDirection), state_(State::INIT), field_(field),
            timeLeftForGrowth_(0), timeNoExtension_(0) {
        bundlePositions_.reserve(maxGrowthTime);
    }

    State state() const {
        return state_;
    }

    void startNewBundle() {
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
        polarisationDirection_ = RandomVonMisesPoint(preferredDirection, kappa_);
    }

    void addPosToBundle(VDOUBLE const &pos) {
        field_->set(VINT(pos), field_->get(VINT(pos)) + 1);
        bundlePositions_.emplace_back(pos);
    }

    void growBundle() {
        assert(timeLeftForGrowth_ > 0);
        if (getRandom01() > extendprob_) {
            // No extension this time
            timeNoExtension_++;
            if (timeNoExtension_ > timeNoExtensionLimit_) {
                cout << "retracting" << endl;
                state_ = State::RETRACTING;
            }
            return;
        }

        auto extendDirection = RandomVonMisesPoint(polarisationDirection_, kappa_);
        auto orthoOffset = VDOUBLE::from_radial(extendDirection);
        auto newBundlePosition = bundlePositions_.back() + orthoOffset;

        if (_cpm_layer->get(VINT(newBundlePosition)).cell_id != cellId) {
            //newBundlePosition is not in the cell, return
            return;
        }

        addPosToBundle(newBundlePosition);
        polarisationDirection_ = extendDirection;

        timeNoExtension_ = 0;
        timeLeftForGrowth_--;
        if (timeLeftForGrowth_ == 0) {
            state_ = State::RETRACTING;
        }
    }

    void retractBundle() {
        if (getRandom01() > retractprob_) {
            //Nothing happens
            return;
        }

        if(retractionMethod_ != RetractionMethod::BACKWARD){
            throw MorpheusException("Only backward retraction is supported");
        }

        auto &lastBundlePos = bundlePositions_.back();
        auto prevFieldVal = field_->get(VINT(lastBundlePos));
        assert(prevFieldVal > 0);
        field_->set(VINT(lastBundlePos), prevFieldVal - 1);
        bundlePositions_.pop_back();

        if(bundlePositions_.empty()) {
            cout << "done retracting" << endl;
            state_ = State::INACTIVE;
        }
    }

    void timeStep() {
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
};

#endif //MORPHEUS_PSEUDOPOD_H
