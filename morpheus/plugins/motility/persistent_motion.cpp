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
	protrusionEval.setXMLPath("protrusion");
	protrusionEval.setDefault("true");
	registerPluginParameter(protrusionEval);
	retractionEval.setXMLPath("retraction");
	retractionEval.setDefault("false");
	registerPluginParameter(retractionEval);
	overrideTypeDefaults.setXMLPath("override-type-defaults");
	overrideTypeDefaults.setDefault("false");
	registerPluginParameter(overrideTypeDefaults);

}


void PersistentMotion::init(const Scope* scope) {
	ReporterPlugin::init(scope);
	CPM_Energy::init(scope);
	
	celltype = scope->getCellType();
	cell_position_memory = celltype->addProperty<VDOUBLE> ( "stored_center", /*"internally stored cell position",*/  VINT(0,0,0) );
	cell_direction = celltype->addProperty<VDOUBLE> ( "stored_direction", /*"internally stored cell direction",*/ VINT(0,0,0) );
	const_cast<Scope*>(scope)->registerSymbol(cell_direction);
	const_cast<Scope*>(scope)->registerSymbol(cell_position_memory);

	ReporterPlugin::registerCellPositionDependency();
	ReporterPlugin::registerOutputSymbol(cell_direction);
	ReporterPlugin::registerOutputSymbol(cell_position_memory);
	CPM_Energy::registerInputSymbol(cell_position_memory);
	CPM_Energy::registerInputSymbol(cell_direction);

	persistenceType = persTypeEval();
	persistence = Persistence::create(persistenceType, protrusionEval(), retractionEval(), overrideTypeDefaults(), *this);
}

void PersistentMotion::report(){
    persistence->report();
}

double PersistentMotion::delta ( const SymbolFocus& cell_focus, const CPM::Update& update) const {
    return persistence->delta(cell_focus, update);
}

double PersistentMotion::hamiltonian(CPM::CELL_ID cell_id) const {
	return 0; 
}


double PersistentMotion::MorpheusPersistence::delta(const SymbolFocus &cell_focus, const CPM::Update &update) {
    if(!applyPersistence(update)) return 0.0;

    auto update_direction = Persistence::update_direction(cell_focus);
    auto target_direction = Persistence::target_direction(cell_focus);
    auto cell_size = cell_focus.cell().nNodes();

    return -pmp.strength(cell_focus) * cell_size * dot(update_direction, target_direction);
}

void PersistentMotion::MorpheusPersistence::report() {
    for (auto cell_id : pmp.celltype->getCellIDs()) {
        auto decay_rate = this->decay_rate(cell_id);

        auto new_center = CPM::getCell(cell_id).getCenter();
        auto shift = new_center - pmp.cell_position_memory->get(cell_id);

        auto new_direction = pmp.cell_direction->get( cell_id ) * (1 - decay_rate) + decay_rate * shift.norm();

        pmp.cell_direction->set(cell_id, new_direction);
        pmp.cell_position_memory->set(cell_id, new_center);
    }
}


double PersistentMotion::SzaboPersistence::delta(const SymbolFocus &cell_focus, const CPM::Update &update) {
    if(!applyPersistence(update)) return 0.0;

    auto update_direction = Persistence::update_direction(cell_focus);
    auto target_direction = Persistence::target_direction(cell_focus);

    return -pmp.strength(cell_focus) * dot(update_direction, target_direction.norm());
}

void PersistentMotion::SzaboPersistence::report() {

    for (auto cell_id : pmp.celltype->getCellIDs()) {

        auto new_center = CPM::getCell(cell_id).getCenter();
        auto shift = new_center - pmp.cell_position_memory->get(cell_id);

        auto new_direction = pmp.cell_direction->get( cell_id ) * (1 - decay_rate(cell_id)) + shift;

        pmp.cell_direction->set(cell_id, new_direction);
        pmp.cell_position_memory->set(cell_id, new_center);
    }
}

PersistentMotion::SzaboPersistence::SzaboPersistence(bool protrusion, bool retraction, bool defaultOverride,
        PersistentMotion &pM): Persistence(true, true, pM) {
    if(!defaultOverride) {
        std::cout << "PersistentMotion::SzaboPersistence: Ignoring specified protrusion/retraction" << std::endl;
    } else {
        std::cout << "PersistentMotion::SzaboPersistence: "
                     "Overriding defaults with specified protrusion/retraction" << std::endl;
        this->protrusion = protrusion;
        this->retraction = retraction;
        exitWhenPointless();
    }
}

VDOUBLE PersistentMotion::Persistence::update_direction(const SymbolFocus &cell_focus) {
    const auto& cell = cell_focus.cell();
    return cell.getUpdatedCenter() - cell.getCenter();
}

VDOUBLE PersistentMotion::Persistence::target_direction(const SymbolFocus &cell_focus) const {
    return pmp.cell_direction->get(cell_focus.cellID());
}

unique_ptr<PersistentMotion::Persistence>
        PersistentMotion::Persistence::create(const PersistentMotion::PersistenceType persistenceType,
                bool protrusion, bool retraction, bool typeOverride, PersistentMotion& pM) {
switch(persistenceType) {
    case PersistenceType::MORPHEUS:
        return make_unique<MorpheusPersistence>(protrusion, retraction, pM);
    case PersistenceType::SZABO:
        return make_unique<SzaboPersistence>(protrusion, retraction, typeOverride, pM);
    }
    __builtin_unreachable();
}

PersistentMotion::Persistence::Persistence(bool protrusion, bool retraction, PersistentMotion &pM) :
pmp(pM), protrusion(protrusion), retraction(retraction) {
    exitWhenPointless();
}

void PersistentMotion::Persistence::exitWhenPointless() {
    if(!protrusion && !retraction)
    {
        std::cerr << "PersistentMotion::Persistence::exitWhenPointless(): "
                     "Both retraction and protrusion is set to false. Therefore, PersistentMotion has no effect, "
                     "which is probably not the intended behavior." << endl;
        exit(EXIT_FAILURE);
    }
}

double PersistentMotion::Persistence::decay_rate(CPM::CELL_ID cellId) const {
    auto decay_rate = CPM::getMCSDuration() / pmp.decaytime(SymbolFocus(cellId));
    return min(decay_rate, 1.0);
}

bool PersistentMotion::Persistence::applyPersistence(const CPM::Update &update) const {
    return (update.opAdd() && protrusion) || (update.opRemove() && retraction);
}
