#include "haptotaxis.h"

using namespace SIM;

REGISTER_PLUGIN(Haptotaxis);

Haptotaxis::Haptotaxis() {
	strength.setXMLPath("strength");
	registerPluginParameter(strength);
	attractant.setXMLPath("attractant");
	registerPluginParameter(attractant);
};


void Haptotaxis::loadFromXML(const XMLNode Node)
{
	CPM_Energy::loadFromXML(Node);
}

void Haptotaxis::init(const Scope* scope) {
	CPM_Energy::init(scope);
};

double Haptotaxis::delta( const SymbolFocus& cell_focus, const CPM::UPDATE& update, CPM::UPDATE_TODO todo) const
{
	double attraction = strength(cell_focus);
	
	if ( attraction == 0.0 ) return 0.;
	double c_neighbor = attractant(update.focus);  // concentration at site of which state is being copied from
	
	attraction *= c_neighbor;
	
	if ( todo & CPM::ADD 	) return -attraction;   
	if ( todo & CPM::REMOVE ) return  attraction;  
	return 0.0;
}

double Haptotaxis::hamiltonian(CPM::CELL_ID cell_id) const {
	cout << "Haptotaxis::hamiltonian MISSING"  << endl;
	return 0.0;
}


