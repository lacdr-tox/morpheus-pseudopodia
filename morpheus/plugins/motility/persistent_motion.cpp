#include "persistent_motion.h"

REGISTER_PLUGIN ( PersistentMotion );

PersistentMotion::PersistentMotion() {
	// required
	decaytime.setXMLPath("decay-time");
	registerPluginParameter(decaytime);
	strength.setXMLPath("strength");
	registerPluginParameter(strength);
	
	// optional
	protrusion.setXMLPath("protrusion");
	protrusion.setDefault("true");
	registerPluginParameter(protrusion);
	retraction.setXMLPath("retraction");
	retraction.setDefault("false");
	registerPluginParameter(retraction);

}

void PersistentMotion::init(const Scope* scope) {
	ReporterPlugin::init(scope);
	CPM_Energy::init(scope);
	// add "cell.center" as output(!) symbol  (not depend symbol)
// 	registerCellPositionOutput();
	// The CPM_Energy implementation requires our reported displacement in time steps of MCSDuration 
	ReporterPlugin::updateSinkTS(CPM::getMCSDuration());
	registerCellPositionDependency();
	
	celltype = scope->getCellType();
	uint property_id = celltype->default_properties.size();
	
	cell_position_memory= celltype->addProperty<VDOUBLE> ( "stored_center", /*"internally stored cell position",*/  VINT(0,0,0) );
	cell_direction 		= celltype->addProperty<VDOUBLE> ( "stored_direction", /*"internally stored cell direction",*/ VINT(0,0,0) );
	
	if( !protrusion() && !retraction() ) 
	{
		cerr << "PersistentMotion: init(): Both retraction and protrusion is set to false. Therefore, PersistentMotion has no effect, which is probably not the intended behavior." << endl;
	    exit(-1);
	}
	

}

void PersistentMotion::report(){
	
	vector<CPM::CELL_ID> cell_ids = celltype->getCellIDs();
	for ( uint i=0; i<cell_ids.size(); i++ ) {
		
		CPM::CELL_ID cell_id = cell_ids[i];
		// adjust decay rate
		double decay_rate = CPM::getMCSDuration() / decaytime( SymbolFocus(cell_id) );
		decay_rate = min(decay_rate, 1.0);

		VDOUBLE new_center		= CPM::getCell ( cell_id ).getCenter(); 
		VDOUBLE shift			= new_center - cell_position_memory->get( cell_id );
		VDOUBLE new_direction	= cell_direction->get( cell_id ) * (1 - decay_rate) + decay_rate * shift.norm();

		cell_direction->set( cell_id, new_direction );
		cell_position_memory->set( cell_id, new_center );
	}
}

double PersistentMotion::delta ( const SymbolFocus& cell_focus, const CPM::Update& update) const
{
	const Cell& cell = cell_focus.cell();
	VDOUBLE update_direction = cell.getUpdatedCenter() - cell.getCenter();
	double cell_size = cell.nNodes();

	double s = (update.opAdd() && protrusion() ) || ( update.opRemove() && retraction() ) ? strength( cell_focus ) : 0.0 ; 
	return -s * cell_size * ( update_direction * cell_direction->get( cell_focus.cellID() ) );
}

double PersistentMotion::hamiltonian(CPM::CELL_ID cell_id) const {
	return 0; 
};



