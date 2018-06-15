#include "chemotaxis.h"

REGISTER_PLUGIN(Chemotaxis);

Chemotaxis::Chemotaxis(): CPM_Energy() {

	/// required symbols
	field.setXMLPath("field");
	field.setGlobalScope();
	registerPluginParameter(field);

	/// optional symbols
	strength.setXMLPath("strength");
	registerPluginParameter(strength);
	
	saturation.setXMLPath("saturation");
	registerPluginParameter(saturation);

	retraction.setXMLPath("retraction");
	retraction.setDefault("True");
	registerPluginParameter(retraction);

	contact_inhibition.setXMLPath("contact-inhibition");
	contact_inhibition.setDefault("False");
	registerPluginParameter(contact_inhibition);
}


double Chemotaxis::delta(const SymbolFocus& cell_focus, const CPM::Update& update) const
{

	// with contact inhibition, only protrusions and retraction to/from medium have nonzero energy change
	if ( contact_inhibition() == true){
// TODO
// 		CellType * ct =CPM::getCellTypes()[update.add_celltype].isMedium();
// 		ct->isMedium();
		if (update.focus().celltype() != CPM::getEmptyCelltypeID() 
			&& update.source().celltype() != CPM::getEmptyCelltypeID() ){ // if not medium
			return 0.0;
		}
	}
	
	// get chemotactic strength of cell at position
	// note that we need distinguish the cases of protrusion and retraction
	double c_strength = strength( cell_focus );
	if ( c_strength == 0.0  ) 
		return 0.0;

	double conc_focus	= field( update.focus()  );	// concentration at site being copied into
	double conc_source	= field( update.source() );	// concentration at site of which state is being copied from

//	cout << "c_strength: " <<  c_strength << ", conc_focus: " << conc_focus << ", conc_source: " << conc_source << endl;

	double dE = 0.;
	if ( saturation.isDefined() ){
		double c_saturation = saturation( cell_focus );
		dE = c_strength * ( (conc_focus/(1.0 + c_saturation * conc_focus)) - (conc_source/(1.0 + c_saturation * conc_source)));
	}
	else{
		dE = c_strength * (conc_focus - conc_source);
	}

	return -dE;
//	if ( todo & CPM::ADD ) 
//		return -dE;   // Cell is extended to the focal node (Protrusion)
//	if ( retraction() && (todo & CPM::REMOVE) ) 
//		return -dE;  // Cell is removed from the focal node (Retraction): Glazier does not penalize this kind of operation, Hogeweg does. Speeds up the reaction on chemical gradients.

	return 0.0;

}

double Chemotaxis::hamiltonian(CPM::CELL_ID cell_id) const
{
	return 0.0;
}


