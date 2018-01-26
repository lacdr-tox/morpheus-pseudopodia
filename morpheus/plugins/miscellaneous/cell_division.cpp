#include "cell_division.h"

REGISTER_PLUGIN(CellDivision);
map<string, shared_ptr<ofstream> >  CellDivision::log_files;

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
	map<string,CellDivision::logmode> conversion_map_log;
	conversion_map_log[string("none")] 	= CellDivision::NONE;
	conversion_map_log[string("dot")] 	= CellDivision::DOT;
	conversion_map_log[string("celltype-dot")] 	= CellDivision::CT_DOT;
	conversion_map_log[string("csv")] 	= CellDivision::CSV;
	conversion_map_log[string("celltype-csv")] 	= CellDivision::CT_CSV;
	conversion_map_log[string("newick")]= CellDivision::NEWICK;
	write_log.setDefault("none");
	write_log.setConversionMap( conversion_map_log );
	registerPluginParameter(write_log);
}

void CellDivision::loadFromXML(const XMLNode xNode, Scope* scope)
{
	InstantaneousProcessPlugin::loadFromXML( xNode, scope );
	
	if (xNode.nChildNode("Triggers")) {
		XMLNode xTrigger = xNode.getChildNode("Triggers");
		daughterID_symbol = "__daughterID";
		getXMLAttribute(xNode,"daughterID", daughterID_symbol);
		trigger_system = shared_ptr<TriggeredSystem>(new TriggeredSystem);
		trigger_system->loadFromXML(xTrigger, scope);
		
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
	
	if (write_log.isDefined()){
		log_file_name = "cell_division";
		switch ( write_log() ) {
			case CellDivision::CT_CSV :
				log_file_name += string("_") + celltype->getName();
			case CellDivision::CSV : 
				log_file_name += ".txt";
				// if logfile was not created yet, create it with a custom deleter, write the header and put it in the static log_files map
				if (! log_files.count(log_file_name)) {
					auto logfile = shared_ptr<ofstream>(new ofstream(log_file_name,ios::out), [=] (ofstream* f) {
						f->close();
						delete f;
					});
					cout << "CellDivision: Log cell IDs to file: " << log_file_name << "\n";
					*logfile << "Time\tMotherID\tDaughter1ID\tDaughter2ID" << endl;
					log_files[log_file_name] = logfile;
				}
				break;
			case CellDivision::CT_DOT :
				log_file_name += string("_") + celltype->getName();
			case CellDivision::DOT :
				log_file_name += ".dot";
				// if logfile was not created yet, create it with a custom deleter, write the header and put it in the static log_files map
				if (! log_files.count(log_file_name)) {
					auto logfile = shared_ptr<ofstream>(new ofstream(log_file_name,ios::out), [=] (ofstream* f) {
						*f << "\n}\n";
						f->close();
						delete f;
					});
					cout << "CellDivision: Log cell IDs to file: cell_division.dot\n";
					*logfile << "digraph{\n";
					logfile->flush();
					log_files[log_file_name] = logfile;
				}
				break;
			default :
				break;
		}
		
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
				//cout << "write_log(): " << (write_log()?"true":"false") << endl;
				switch ( write_log() ) {
					case CellDivision::CSV :
					case CellDivision::CT_CSV :
//	 					cout << "write to " << log_file_name << endl;
						*log_files[log_file_name] << SIM::getTime() << "\t" << mother_id << "\t" << daughter1.getID()<< "\t" << daughter2.getID() << endl;
						break;
					case CellDivision::DOT :
					case CellDivision::CT_DOT :
						*log_files[log_file_name] << mother_id << " -> " << daughter1.getID() << "\n"; 
						*log_files[log_file_name] << mother_id << " -> " << daughter2.getID() << endl;
						break;
					case CellDivision::NEWICK : 
					{
						// Generate NEWICK tree
						//string mstr = "\""+to_string(mother_id)+"\"";
						string mstr = to_string(mother_id);
						bool mother_found=false;
						for(int i=0; i < newicks.size(); i++){
							std::size_t found = newicks[i].find(mstr);
							if (found!=std::string::npos){ // if found, replace mother with mother and daughter sub-tree
								mother_found = true;
								// string with time: newick format 3 (but has problems)
								//string mstr2 = string("(\""+to_string(daughter1.getID())+"\":"+to_string(SIM::getTime())+",\""+
								//						    to_string(daughter2.getID())+"\":"+to_string(SIM::getTime())+")"+mstr);
								string mstr2 = string("("+to_string(daughter1.getID())+","+to_string(daughter2.getID())+")"+mstr);
								newicks[i].replace(found,mstr.length(),mstr2);
							}
						}
						if( !mother_found ){ // add a new newick tree and file
							//cout << "Not FOUND" << endl;
							// string with time: newick format 3 (but has problems)
							//string mstr2 = string("(\""+to_string(daughter1.getID())+"\":"+to_string(SIM::getTime())+",\""+
							//						    to_string(daughter2.getID())+"\":"+to_string(SIM::getTime())+")"+mstr);
							string mstr2 = string("("+to_string(daughter1.getID())+","+to_string(daughter2.getID())+")"+mstr);
							newicks.push_back( mstr2 );
						}
						// verbose: print newick tree to stdout
						//for(string newick : newicks){
						//	cout << newick << ";"<< endl;
						//}
						int num=0;
						for(string newick : newicks){
							ofstream fout_newick;
							fout_newick.open("cell_division_newick_"+celltype->getName()+"_"+to_string(num++)+".txt",ios::out);
							fout_newick << newick << ";" << endl;
							fout_newick.close();
						}
					}
						break;
					default:
						break;
				}

				if (trigger_system) {
					daughterID->set(SymbolFocus::global,1);
					trigger_system->trigger(SymbolFocus(daughter1.getID()));

					daughterID->set(SymbolFocus::global,2);
					trigger_system->trigger(SymbolFocus(daughter2.getID()));
				}
			}
		}
	}
}

CellDivision::~CellDivision()
{
	if( write_log() == CellDivision::NEWICK){
		int num=0;
		for(string newick : newicks){
			ofstream fout_newick;
			fout_newick.open("cell_division_newick_"+celltype->getName()+"_"+to_string(num++)+".txt",ios::out);
			fout_newick << newick << ";" << endl;
			fout_newick.close();
		}
	}

}






