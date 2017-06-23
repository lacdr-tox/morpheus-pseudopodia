#include "protrusion.h"

REGISTER_PLUGIN(Protrusion);

Protrusion::Protrusion(): CPM_Energy(), InstantaneousProcessPlugin( TimeStepListener::XMLSpec::XML_OPTIONAL ) {

	/// required symbols
	field.setXMLPath("field");
	field.setGlobalScope();
	registerPluginParameter(field);

	strength.setXMLPath("strength");
	registerPluginParameter(strength);

	maxact.setXMLPath("maximum");
	registerPluginParameter(maxact);

//    cout << "Protrusion" << endl;
	
}

void Protrusion::init(const Scope* scope) {
//    cout << "Protrusion> init" << endl;
    CPM_Energy::init(scope);
	InstantaneousProcessPlugin::init( scope );  

    setTimeStep( CPM::getMCSDuration() );
	this->scope = scope;
    lattice = SIM::getLattice();
	cpm_lattice = CPM::getLayer();
	nbh = CPM::getSurfaceNeighborhood().neighbors();
	//registerCellPositionOutput(); 

};


double Protrusion::local_activity(SymbolFocus f) const{
//    cout << "Protrusion> local_activity" << endl;

    // calculates the geometric mean of the activity values near a focus point inside the cell
	double multiplication = field( f );
	int num_nbs = 0;
	for(VINT nb : nbh){
		VINT nb_pos = nb + f.pos();
		SymbolFocus neighbor(nb_pos);
		if( neighbor.cellID() == f.cellID() ){
			multiplication *= field( neighbor );
			num_nbs++;
		}
	}
	return pow( multiplication, 1.0/((double)num_nbs+1.0));
}

double Protrusion::delta(const SymbolFocus& cell_focus, const CPM::Update& update) const
{
//    cout << "Protrusion> delta" << endl;

// 	if ( todo & CPM::REMOVE )
// 		return 0.0;
	
    if (maxact(cell_focus) == 0){
        return 0.0;
    }
    if (update.source().celltype() == CPM::getEmptyCelltypeID() ) {
 		return 0.0;
 	}

	double act_source = local_activity( update.source() );
	double act_focus  = local_activity( update.focus() );

	double dH = (strength(cell_focus) / maxact(cell_focus) ) * (act_source - act_focus);
	
// 	cout << update.focus.pos() << "\t" << update.source.pos()  << endl;
// 	cout << "strength: " << strength(cell_focus) 
// 		 << "\tmaxact:   " << maxact(cell_focus) 
// 		 << "\tact_source: " << act_source
// 		 << "\tact_focus: " << act_focus
// 		 << "\tdH: " << dH << endl; 
		
	if ( update.opAdd() ) 
		return -dH; // Cell is extended to the focal node (Protrusion)
 	if ( update.opRemove() ) 
 		return dH;  // Cell is removed from the focal node (Retraction)
	return 0;

}

// update the activity field after update
void Protrusion::update_notify(CPM::CELL_ID cell_id, const CPM::Update& update) {
	
	if ( update.opAdd()){
		field.set( update.focus().pos(), maxact( SymbolFocus(cell_id) ));
		if (update.source().valid())
			field.set( update.source().pos(), maxact( SymbolFocus(cell_id) ));
	}
	if ( update.opRemove() ) 
		field.set( update.focus().pos(), 0.0);
	
}


// update the activity field every MCS
void Protrusion::executeTimeStep(){
//    cout << "Protrusion> executeTimeStep" << endl;

// 	cout << "executeTimeStep: " << SIM::getTime()  << endl;

    // substract 1 from all nonzero activity values (using FocusRange Iterators)
    FocusRange range(Granularity::Node, scope);
    for (auto focus : range) {
        double val = field(focus.pos());
        field.set(focus.pos(), (val>0 ? val-1 : 0));
    }

    // substract 1 from all activity values (NOT using FocusRange Iterators, SLOW!)
//    VINT pos;
//    for(pos.z=0; pos.z<lattice->size().z; pos.z++){
//        for(pos.y=0; pos.y<lattice->size().y; pos.y++){
//            for(pos.x=0; pos.x<lattice->size().x; pos.x++){
//                double val = field(pos);
//                field.set(pos, (val>0 ? val-1 : 0));
//            }
//        }
//    }
	
}

double Protrusion::hamiltonian(CPM::CELL_ID cell_id) const {
	return 0.0;
}



