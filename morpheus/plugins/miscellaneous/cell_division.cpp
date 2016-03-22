#include "cell_division.h"

REGISTER_PLUGIN(CellDivision);

// TODO: is "InstantaneousProcessPlugin" correct as a MCSStepper?
CellDivision::CellDivision() : InstantaneousProcessPlugin( TimeStepListener::XMLSpec::XML_NONE ) {
	
	condition.setXMLPath("Condition/text");
	registerPluginParameter(condition);

	division_plane.setXMLPath("division-plane");
	map<string,CellType::division> conversion_map;
	conversion_map[string("random")] 	= CellType::RANDOM;
	conversion_map[string("minor")] 	= CellType::MINOR;
	conversion_map[string("major")] 	= CellType::MAJOR;
	// TODO: it would be useful to determine specific division plane
	conversion_map[string("oriented")] 	= CellType::ORIENTED; 
	division_plane.setConversionMap( conversion_map );
	registerPluginParameter(division_plane);

	orientation.setXMLPath("orientation");
	registerPluginParameter(orientation);
	
	write_log.setXMLPath("write-log");
	write_log.setDefault("false");
	registerPluginParameter(write_log);

}

void CellDivision::loadFromXML(const XMLNode xNode)
{
	InstantaneousProcessPlugin::loadFromXML( xNode );
	
	if (xNode.nChildNode("Triggers")) {
		XMLNode xTrigger = xNode.getChildNode("Triggers");
		daughterID_symbol = "__daughterID";
		getXMLAttribute(xNode,"daughterID", daughterID_symbol);
		trigger_system = shared_ptr<TriggeredSystem>(new TriggeredSystem);
		trigger_system->loadFromXML(xTrigger);
		
		trigger_system->addLocalSymbol(daughterID_symbol,0);
	}
}

void CellDivision::init(const Scope* scope){
	InstantaneousProcessPlugin::init( scope );
	setTimeStep( CPM::getMCSDuration() );
	is_adjustable = false;
	celltype = scope->getCellType();
	
	registerCellPositionOutput();
	
	if (trigger_system) {
		trigger_system->init();
		// the time-step does not depend on the trigger input symbols registerInputSymbols(trigger_system->getDependSymbols());
		registerOutputSymbols(trigger_system->getOutputSymbols());
		daughterID = trigger_system->getLocalScope()->findRWSymbol<double>(daughterID_symbol);
	}
	
	if (write_log.isDefined() && write_log() ){
		fout.open("CellDivision.log",ios::app);
		cout << "CellDivision: Log cell IDs to file: celldivisions.log\n";
		fout << "Time\tMotherID\tDaughter1ID\tDaughter2ID\n";
	}
	
	if( division_plane() == CellType::ORIENTED && !orientation.isDefined()  ){
		throw MorpheusException("CellDivision: Orientation of cell division plane must be specified.", stored_node);
	}

}

void CellDivision::executeTimeStep() {
	
	vector <CPM::CELL_ID> cells = celltype->getCellIDs();
	for (int i=0; i < cells.size(); i++ ) {
		
		CPM::CELL_ID mother_id = cells[i];

		if( CPM::getCell( mother_id).getNodes().size() < 2.0 )
			continue;
		
		bool divide = condition( SymbolFocus(mother_id) ) >= 1.0;
		if( divide ){ // if condition for proliferation is fulfilled
			
			const Cell& mother = CPM::getCell(cells[i]);
			//CPM::CELL_ID daughter_id = celltype->divideCell( mother_id, divisionPlane ); //CPM::getCellIndex(mother_id).cell );
			
			pair<CPM::CELL_ID,CPM::CELL_ID> daughter_ids;
			if ( division_plane() == CellType::ORIENTED  ){
				if ( orientation.isDefined() )
					daughter_ids = celltype->divideCell2( mother_id, division_plane(), orientation( SymbolFocus(cells[i]) ) );
				else
					throw MorpheusException("CellDivision: Orientation of cell division plane must be specified.", stored_node);
			}
			else
				daughter_ids = celltype->divideCell2( mother_id, division_plane() );
				
			const Cell& daughter1 = CPM::getCell(daughter_ids.first);
			const Cell& daughter2 = CPM::getCell(daughter_ids.second);
			
			// Prevent downstream triggers if something went wrong
			bool found_empty = false;
			if( daughter1.nNodes() == 0){
				CPM::setCellType( daughter1.getID(), CPM::getEmptyCelltypeID() );
				cerr << "CellDivision error: Daughter1 cell is empty! Daughter2 has " << daughter2.nNodes() << " nodes." <<  endl ;
				found_empty = true;
			}
			if( daughter2.nNodes() == 0){
				CPM::setCellType( daughter2.getID(), CPM::getEmptyCelltypeID() );
				cerr << "CellDivision error: Daughter2 cell is empty! Daughter1 has " << daughter1.nNodes() << " nodes." << endl;
				found_empty = true;
			}
			if (!found_empty) {
				if( write_log() ){
					fout << SIM::getTime() << "\t" << mother_id << "\t" << daughter1.getID()<< "\t" << daughter2.getID() << "\n";
				}

				if (trigger_system) {
					daughterID.set(SymbolFocus::global,1);
					trigger_system->trigger(SymbolFocus(daughter1.getID()));

					daughterID.set(SymbolFocus::global,2);
					trigger_system->trigger(SymbolFocus(daughter2.getID()));
				}
			}
		}
	}
}

CellDivision::~CellDivision()
{
	if(fout.is_open())
		fout.close();
}






