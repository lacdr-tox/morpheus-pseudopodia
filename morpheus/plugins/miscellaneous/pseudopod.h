//
// Created by gerhard on 28/07/17.
//

#ifndef MORPHEUS_PSEUDOPOD_H
#define MORPHEUS_PSEUDOPOD_H

#include "core/interfaces.h"
#include "core/plugin_parameter.h"

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
        IN_MOVING_DIR
    };

private:
    vector<VDOUBLE> bundlePositions_;
    unsigned int timeLeftForGrowth_;
    unsigned int maxGrowthTime_;
    unsigned int timeNoExtension_;
    static constexpr auto timeNoExtensionLimit_ = 20;
    RetractionMethod paramRetractionMethod_;
    RetractionMethod currRetractionMethod_;
    VDOUBLE polarisationDirection_;
    double kappaInit_;
    double kappaCont_;
    static constexpr auto retractprob_ = .3;
    static constexpr auto extendprob_ = .3;
    State state_;
    const CPM::CELL_ID cellId;
    const CPM::LAYER *_cpm_layer;
    PluginParameter2<double, XMLReadWriteSymbol, RequiredPolicy> *field_;
    PluginParameter2<double, XMLReadableSymbol, RequiredPolicy> *movingDirection_;
    bool retractOnTouch_;

    void startNewBundle();
    void retractBundle();
    void decrementActinLevelAt(VINT pos) const;
    void growBundle();
    void setRetracting();
    void addPosToBundle(const VDOUBLE &pos);


public:
    Pseudopod(unsigned int maxGrowthTime, const CPM::LAYER *cpm_layer, CPM::CELL_ID cellId,
              PluginParameter2<double, XMLReadableSymbol, RequiredPolicy> *movingDirection,
              PluginParameter2<double, XMLReadWriteSymbol, RequiredPolicy> *field, RetractionMethod retractionMethod,
              double kappaInit, double kappaCont, bool retractOnTouch);

    State state() const;
    void timeStep();
    VDOUBLE getBundleTip() const;

};

#endif //MORPHEUS_PSEUDOPOD_H
