// include the header file
#include "gab_pseudopodia.h"
// macro to register plugin in framework
REGISTER_PLUGIN(Pseudopodia);

Pseudopodia::Pseudopodia() : InstantaneousProcessPlugin(TimeStepListener::XMLSpec::XML_OPTIONAL) {

    maxGrowthTime.setXMLPath("max-growth-time");
    maxGrowthTime.setDefault("100");
    registerPluginParameter(maxGrowthTime);

    maxPseudopods.setXMLPath("max-pseudopods");
    maxPseudopods.setDefault("0");
    registerPluginParameter(maxPseudopods);

    field.setXMLPath("field");
    field.setGlobalScope();
    registerPluginParameter(field);

};

// called before initialization
void Pseudopodia::loadFromXML(const XMLNode xNode) {
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
    pseudopods.resize(celltype->getCellIDs().size(),
                      vector<Pseudopod>((unsigned int) maxPseudopods(),
                                        Pseudopod((unsigned int) maxGrowthTime(), cpmLayer.get())));
}

// called periodically during simulation
void Pseudopodia::executeTimeStep() {
    cout << "lala" << endl;
}
