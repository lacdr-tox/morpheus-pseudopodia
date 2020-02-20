#include "haptotaxis.h"

using namespace SIM;

REGISTER_PLUGIN(Haptotaxis);

Haptotaxis::Haptotaxis() {
	strength.setXMLPath("strength");
	registerPluginParameter(strength);
	attractant.setXMLPath("attractant");
	registerPluginParameter(attractant);
};


void Haptotaxis::init(const Scope* scope) {
	CPM_Energy::init(scope);
};

double Haptotaxis::delta( const SymbolFocus& cell_focus, const CPM::Update& update) const
{
	double attraction = strength(cell_focus);
	
	if ( attraction == 0.0 ) return 0.;
	double c_neighbor = attractant(update.focus());  // concentration at site of which state is being copied from
	
	attraction *= c_neighbor;
	
	if ( update.opAdd() ) return -attraction;   
	if ( update.opRemove() ) return  attraction;  
	return 0.0;
}

double Haptotaxis::hamiltonian(CPM::CELL_ID cell_id) const {
	cout << "Haptotaxis::hamiltonian MISSING"  << endl;
	return 0.0;
}


