#ifndef GAB_PSEUDOPODIA_H
#define GAB_PSEUDOPODIA_H

// include the plugin interfaces (required) and plugin parameters (very useful)
#include "core/interfaces.h"
#include "core/plugin_parameter.h"

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
class Pseudopodia : CPM_Energy, CPM_Check_Update, InstantaneousProcessPlugin {
private:
    Pseudopod::RetractionMethod retractionMethod = Pseudopod::RetractionMethod::BACKWARD;
    // parameters that are specified in XML (as values, strings or symbolic expressions)
    PluginParameter2<double, XMLReadWriteSymbol, RequiredPolicy> field;
    PluginParameter2<double, XMLValueReader, DefaultValPolicy> maxGrowthTime;
    PluginParameter2<unsigned int, XMLValueReader, DefaultValPolicy> maxPseudopods;
    PluginParameter2<double, XMLReadableSymbol, RequiredPolicy> movingDirection;

    once_flag initPseudopods;

    // auxiliary plugin-internal variables and functions can be declared here.
    shared_ptr<const CPM::LAYER> cpmLayer;
    CellType *celltype;
    map<CPM::CELL_ID, vector<Pseudopod>> pseudopods;

    double neighboringActinBonus;

public:
    // constructor
    Pseudopodia();
    // macro required for plugin integration
    DECLARE_PLUGIN("Pseudopodia");

    // - load parameters from XML, called before initialization
    void loadFromXML(XMLNode xNode) override;

    // - initialize plugin, called during initialization
    void init(const Scope *scope) override;

    // - execute plugin, called periodically during simulation (automatically scheduled)
    void executeTimeStep() override;

private:
    double delta(const SymbolFocus &cell_focus, const CPM::Update &update) const override;

    bool update_check(CPM::CELL_ID cell_id, const CPM::Update &update) override;

    double hamiltonian(CPM::CELL_ID cell_id) const override;

};

#endif