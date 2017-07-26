#ifndef GAB_PSEUDOPODIA_H
#define GAB_PSEUDOPODIA_H

// include the plugin interfaces (required) and plugin parameters (very useful) 
#include "core/interfaces.h"
#include "core/plugin_parameter.h"

using namespace SIM;

// Doxygen documentation as it will appear in documentation panel of GUI
/** \defgroup Pseudopodia
\ingroup CellTypePlugins
\brief Brief description of my example plugin

A detailed description of my example plugin that can contain mathematical formulae in LaTex format: \f$ \DeltaA = \sum_{\sigma} a-b*c^2 \f$ and references to publications with <a href="http://dx.doi.org/">links</a>.

\section Example usage
\verbatim
<Pseudopodia threshold="123" expression="a+b*c" />
\endverbatim
*/

VDOUBLE RandomVonMisesPoint(double mu, double kappa, double *theta) {
    double ar;
    VDOUBLE pos;

    do {
        ar = getRandom01();
        *theta = 2 * M_PI * (getRandom01() - 0.5);
    } while (ar > exp(kappa * (cos(*theta - mu) - 1.0)));

    /* convert polar to cartesian coordinates */
    pos.x = cos(*theta);
    pos.y = sin(*theta);

    return pos;

}

// class declaration, and inheritance of plugin interface
class Pseudopodia : InstantaneousProcessPlugin {
private:
    static VDOUBLE RandomVonMisesPoint(double mu, double kappa, double *theta) {
        double ar;
        VDOUBLE pos;

        do {
            ar = getRandom01();
            *theta = 2 * M_PI * (getRandom01() - 0.5);
        } while (ar > exp(kappa * (cos(*theta - mu) - 1.0)));

        /* convert polar to cartesian coordinates */
        pos.x = cos(*theta);
        pos.y = sin(*theta);

        return pos;

    }

    class Pseudopod {
    private:
        enum class State {
            INIT, GROWING, RETRACTING
        };
        int nextBundlePositionNumber;
        vector<VDOUBLE> bundlePositions;
        int timeLeftForGrowth;
        unsigned int _maxGrowthTime;
        unsigned int noExtensionTime;
        double polarisationDirection;
        double targetDirection;
        double kappa;
        State state;
        CPM::CELL_ID cellId;
        const CPM::LAYER* _cpm_layer;
        PluginParameter2<double, XMLReadWriteSymbol, RequiredPolicy> *field;
    public:
        Pseudopod(unsigned int maxGrowthTime, const CPM::LAYER* cpm_layer) :
                _maxGrowthTime(maxGrowthTime),
                _cpm_layer(cpm_layer) {
            bundlePositions.resize(maxGrowthTime);
            reset();
        };

        void reset() {
            state = State::INIT;
            timeLeftForGrowth = _maxGrowthTime;
            noExtensionTime = 0;

        }

        void startNewBundle() {
            auto cell_center = VINT(CPM::getCell(cellId).getCenter());
            //TODO periodic boundary conditions
            if (_cpm_layer->get(cell_center).cell_id != cellId) {
                //com is not in the cell, return
                return;
            }

            field->set(cell_center, field->get(cell_center) + 1);
            state = State::GROWING;

            //Set polarisation direction based on cell target direction
            RandomVonMisesPoint(targetDirection, kappa, &polarisationDirection);
        }

        void growBundle() {

        }

        void retractBundle() {

        }

        void timeStep() {
            switch (state) {
                case State::INIT:
                    startNewBundle();
                    break;
                case State::GROWING:
                    growBundle();
                    break;
                case State::RETRACTING:
                    retractBundle();
                    break;
            }
        }
    };

    enum class RetractionMethod {
        FORWARD, BACKWARD, WEIRD
    };
    RetractionMethod retractionMethod = RetractionMethod::BACKWARD;
    // parameters that are specified in XML (as values, strings or symbolic expressions)
    PluginParameter2<double, XMLReadWriteSymbol, RequiredPolicy> field;
    PluginParameter2<double, XMLValueReader, DefaultValPolicy> maxGrowthTime;
    PluginParameter2<unsigned int, XMLValueReader, DefaultValPolicy> maxPseudopods;

    // auxiliary plugin-internal variables and functions can be declared here.
    shared_ptr<const CPM::LAYER> cpmLayer;
    CellType* celltype;
    vector<vector<Pseudopod>> pseudopods;
public:
    // constructor
    Pseudopodia();
    // macro required for plugin integration
    DECLARE_PLUGIN("Pseudopodia");

    // the following functions are inherited from plugin interface and are overwritten
    // - load parameters from XML, called before initialization
    void loadFromXML(const XMLNode);

    // - initialize plugin, called during initialization
    void init(const Scope *);

    // - execute plugin, called periodically during simulation (automatically scheduled)
    void executeTimeStep();
};

#endif