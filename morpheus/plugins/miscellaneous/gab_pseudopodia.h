#ifndef GAB_PSEUDOPODIA_H
#define GAB_PSEUDOPODIA_H

// include the plugin interfaces (required) and plugin parameters (very useful)
#include "core/interfaces.h"
#include "core/plugin_parameter.h"
#include "core/cell.h"
#include "core/celltype.h"

#include "pseudopod.h"

#include <mutex>

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

// class declaration, and inheritance of plugin interface
class Pseudopodia : CPM_Energy, Cell_Update_Checker, InstantaneousProcessPlugin {
private:
    // parameters that are specified in XML (as values, strings or symbolic expressions)
    PluginParameter2<double, XMLValueReader, DefaultValPolicy> neighboringActinBonus;
    PluginParameter2<double, XMLValueReader, DefaultValPolicy> pseudopodTipBonus;
    PluginParameter2<double, XMLValueReader, DefaultValPolicy> pseudopodTipBonusMaxDistance;
    PluginParameter2<double, XMLReadWriteSymbol, RequiredPolicy> field;
    PluginParameter2<double, XMLValueReader, DefaultValPolicy> maxGrowthTime;
    PluginParameter2<double, XMLValueReader, DefaultValPolicy> directionalStrengthInit;
    PluginParameter2<double, XMLValueReader, DefaultValPolicy> directionalStrengthCont;
    PluginParameter2<double, XMLEvaluator, DefaultValPolicy> maxPseudopods;
    PluginParameter2<unsigned int, XMLValueReader, DefaultValPolicy> timeBetweenExtensions;
    PluginParameter2<double, XMLReadableSymbol, RequiredPolicy> movingDirection;
    PluginParameter2<Pseudopod::RetractionMethod, XMLNamedValueReader, DefaultValPolicy> retractionMethod;
    PluginParameter2<Pseudopod::TouchBehavior, XMLNamedValueReader, DefaultValPolicy> touchBehavior;
    PluginParameter2<bool, XMLValueReader, DefaultValPolicy> persistenceBonus;
    PluginParameter2<double, XMLValueReader, DefaultValPolicy> pullStrength;

    once_flag initPseudopods;

    // auxiliary plugin-internal variables and functions can be declared here.
    shared_ptr<const CPM::LAYER> cpmLayer;
    CellType *cellType;
    map<CPM::CELL_ID, vector<Pseudopod>> pseudopods;

public:
    // constructor
    Pseudopodia();
    // macro required for plugin integration
    DECLARE_PLUGIN("Pseudopodia");

    // - initialize plugin, called during initialization
    void init(const Scope *scope) override;

    // - execute plugin, called periodically during simulation (automatically scheduled)
    void executeTimeStep() override;

private:
    double delta(const SymbolFocus &cell_focus, const CPM::Update &update) const override;

    bool update_check(CPM::CELL_ID cell_id, const CPM::Update &update) override;

    double hamiltonian(CPM::CELL_ID cell_id) const override;

    double calcNeighboringActinBonus(const CPM::Update &update) const;

    double calcPseudopodTipBonus(const SymbolFocus &cell_focus, const CPM::Update &update) const;

    vector<Pseudopod> getPseudopodsForCell(const CPM::CELL_ID &cell_id) const;

    double minDistanceToPseudopodTip(VINT pos, const CPM::CELL_ID &cellId) const;

    double calcPersistenceBonus(const SymbolFocus &cell_focus, const CPM::Update &update) const;
};

#endif