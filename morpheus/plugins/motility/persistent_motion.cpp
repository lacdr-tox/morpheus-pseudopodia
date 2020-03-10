#include "persistent_motion.h"

REGISTER_PLUGIN ( PersistentMotion );

PersistentMotion::PersistentMotion() {
	// required
	decaytime.setXMLPath("decay-time");
	registerPluginParameter(decaytime);
	strength.setXMLPath("strength");
	registerPluginParameter(strength);


    persTypeEval.setXMLPath("type");
    map<string, PersistenceType> persTypeMap;
    persTypeMap["default"] = PersistenceType::MORPHEUS;
    persTypeMap["szabo"] = PersistenceType::SZABO;
    persTypeEval.setConversionMap(persTypeMap);
    registerPluginParameter(persTypeEval);

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
	
	celltype = scope->getCellType();
	cell_position_memory= celltype->addProperty<VDOUBLE> ( "stored_center", /*"internally stored cell position",*/  VINT(0,0,0) );
	cell_direction 		= celltype->addProperty<VDOUBLE> ( "stored_direction", /*"internally stored cell direction",*/ VINT(0,0,0) );
	const_cast<Scope*>(scope)->registerSymbol(cell_direction);
	const_cast<Scope*>(scope)->registerSymbol(cell_position_memory);

	ReporterPlugin::registerCellPositionDependency();
	ReporterPlugin::registerOutputSymbol(cell_direction);
	ReporterPlugin::registerOutputSymbol(cell_position_memory);
	CPM_Energy::registerInputSymbol(cell_position_memory);
	CPM_Energy::registerInputSymbol(cell_direction);

	persistenceType = persTypeEval();
    persistence = Persistence::create(persistenceType, *this);
	
	if( !protrusion() && !retraction() ) 
	{
		cerr << "PersistentMotion: init(): Both retraction and protrusion is set to false. Therefore, PersistentMotion has no effect, which is probably not the intended behavior." << endl;
	    exit(EXIT_FAILURE);
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
    return persistence->delta(cell_focus, update);
}

double PersistentMotion::hamiltonian(CPM::CELL_ID cell_id) const {
	return 0; 
};


double PersistentMotion::MorpheusPersistence::delta(const SymbolFocus &cell_focus, const CPM::Update &update) {
    auto update_direction = Persistence::update_direction(cell_focus);
    auto cell_size = cell_focus.cell().nNodes();

    auto s = (update.opAdd() && pmp.protrusion() ) || (update.opRemove() && pmp.retraction() ) ? pmp.strength(cell_focus ) : 0.0 ;
    return -s * cell_size * dot(update_direction, pmp.cell_direction->get(cell_focus.cellID() ) );
}


double PersistentMotion::SzaboPersistence::delta(const SymbolFocus &cell_focus, const CPM::Update &update) {
    auto update_direction = Persistence::update_direction(cell_focus);

    auto s = (update.opAdd() && pmp.protrusion() ) || (update.opRemove() && pmp.retraction() ) ? pmp.strength(cell_focus ) : 0.0 ;
    return -s * dot(update_direction, pmp.cell_direction->get(cell_focus.cellID() ) );
}

VDOUBLE PersistentMotion::Persistence::update_direction(const SymbolFocus &cell_focus) {
    auto cell = cell_focus.cell();
    return cell.getUpdatedCenter() - cell.getCenter();
}

unique_ptr<PersistentMotion::Persistence>
        PersistentMotion::Persistence::create(const PersistentMotion::PersistenceType persistenceType,
                PersistentMotion& pM) {
switch(persistenceType) {
    case PersistenceType::MORPHEUS:
        return make_unique<MorpheusPersistence>(pM);
    case PersistenceType::SZABO:
        return make_unique<SzaboPersistence>(pM);
    }
    __builtin_unreachable();
}

