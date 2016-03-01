#include "env_dissipation.h"

REGISTER_PLUGIN(Environment_Dissipation);


Environment_Dissipation::Environment_Dissipation() : high_val(0.0), low_val(0.0), f_add(1.0), f_remove(1.0), f_add_rem(1.0), pde_layer_name("") {};

double Environment_Dissipation::hamiltonian ( CPM::CELL_ID cell_id ) const { return 0; }

double Environment_Dissipation::delta ( CPM::CELL_ID cell_id , const CPM::UPDATE& update, CPM::UPDATE_TODO todo ) const
{
	double weight = 1.0;
	if (todo & CPM::ADD) weight*=f_add;
	if (todo & CPM::REMOVE) weight*=f_remove;
	if (todo == CPM::ADD_AND_REMOVE) weight*=f_add_rem;
	double env = pde_layer->get(update.focus);
	return  low_val + weight * (high_val - low_val) *  env / ( saturation_05 + env) ;
}

void Environment_Dissipation::init ( CellType* ct ) { 
	pde_layer = SIM::findPDELayer(pde_layer_name);
	if (!pde_layer) { exit(-1);}
}


void Environment_Dissipation::loadFromXML ( const XMLNode xNode)
{
	Plugin::loadFromXML(xNode);
	getXMLAttribute(xNode,"layer", pde_layer_name);
	getXMLAttribute(xNode,"saturation", saturation_05);
	getXMLAttribute(xNode,"sensitivity", high_val);
	getXMLAttribute(xNode,"high", high_val);
	low_val=0.0;
	getXMLAttribute(xNode,"low", low_val);
	getXMLAttribute(xNode,"Weights/add", f_add);
	getXMLAttribute(xNode,"Weights/add_remove", f_add_rem);
	getXMLAttribute(xNode,"Weights/remove", f_remove);
}

