#include "contact_logger.h"

REGISTER_PLUGIN(ContactLogger);

ContactLogger::ContactLogger() {
	celltype_from.setXMLPath("celltype-from");
	registerPluginParameter(celltype_from);
	celltype_to.setXMLPath("celltype-to");
	registerPluginParameter(celltype_to);
	
	ignore_medium.setXMLPath("ignore-medium");
	ignore_medium.setDefault("true");
	registerPluginParameter(ignore_medium);
	log_duration.setXMLPath("log-duration");
	registerPluginParameter(log_duration);
	logging_time_step.setXMLPath("time-step");
	registerPluginParameter(logging_time_step);
	
}

void ContactLogger::init(const Scope* scope){
	AnalysisPlugin::init(scope);
	if( log_duration.isDefined() ) // TODO: does this result in optmized performance by only calling executeTimeStep when applicable? Or is this handled by the schedule?
		setTimeStep( CPM::getMCSDuration() > 0 ? CPM::getMCSDuration() : logging_time_step() );

	if( celltype_to.isDefined() && !celltype_from.isDefined() )
		throw MorpheusException("Cannot specify 'celltype_to' without speficying 'celltype_from'.", stored_node);
	
	// open file
	stringstream ss;
	ss << "contact_logger";
	if( celltype_from.isDefined() ) {
		ss << "_" << celltype_from()->getName();
		registerInputSymbol(celltype_from()->getScope()->findSymbol(SymbolBase::CellPosition_symbol));
	} 
	else {
		registerCellPositionDependency();
	}

	if( celltype_to.isDefined() ) {
		ss << "_" << celltype_to()->getName();
		registerInputSymbol(celltype_to()->getScope()->findSymbol(SymbolBase::CellPosition_symbol));
	}
	else {
		registerCellPositionDependency();
	}
		
	ss << ".txt";
	fout.open(ss.str().c_str(), fstream::out );
	if( !fout.is_open() ){
		cout << "Error opening file " << ss.str() << endl;
		return;
	}
	// header
	fout << "time"  << "\t" << "cell.id" << "\t" << "cell.type" 
					<< "\t" << "neighbor.id" << "\t" << "neighbor.type" 
					<< "\t" << "length";
	if( log_duration.isDefined() ){
		fout << "\t" << "duration";
	}
	fout << "\n";
}

void ContactLogger::loadFromXML(const XMLNode node, Scope* scope){
	AnalysisPlugin::loadFromXML(node, scope);
}

void ContactLogger::reportContacts(){
	vector< weak_ptr<const CellType> > celltypes;
	if( celltype_from.isDefined() ){
		celltypes.push_back( celltype_from() );
	}
	else{
		celltypes = CPM::getCellTypes();
	}
	
	for(uint i=0; i<celltypes.size(); i++){
		auto ct = celltypes[i].lock();
		vector<CPM::CELL_ID> cells = ct->getCellIDs();
		
		for(uint c=0; c<cells.size(); c++){
			CPM::CELL_ID cellid = cells[c];
			const Cell& cell = CPM::getCell( cellid );

			const auto& interfaces = cell.getInterfaceLengths();
			for( auto i = interfaces.begin(); i != interfaces.end(); i++){
				uint ct_id = ct.get()->getID();
				CPM::CELL_ID nb_cellid = i->first;
				
				uint nb_ct_id = CPM::getCellIndex( nb_cellid ).celltype;
				if( celltype_to.isDefined() && (nb_ct_id != celltype_to().get()->getID() ) )
					continue;
				
				if( ignore_medium.isDefined() && (nb_ct_id == CPM::getEmptyCelltypeID() ))
					continue;

				double length = i->second;

				if( cellid < nb_cellid ) { // Only one way registration of contacts
					std::pair<CPM::CELL_ID, CPM::CELL_ID> cid_pair;
					cid_pair = std::make_pair( cellid, nb_cellid );
				
					if ( map_contact_duration.find( cid_pair ) == map_contact_duration.end() ) { // if cell pair not in map
						if( length > 0.0 ){
							map_contact_duration[ cid_pair ] = 0;
						}
					} 
					else { // if cell pair already in map
						if( length > 0.0 ){
							map_contact_duration[ cid_pair ] += timeStep();
						}
						else{ // if cell pair not in contact (anymore), remove key
							map_contact_duration.erase( cid_pair );
						}
					}
				}
			}
		}
	}
}

void ContactLogger::analyse(double time)
{
	if( log_duration.isDefined() )
		reportContacts();
	
	if (time >= last_logging + logging_time_step() || time == SIM::getStartTime()) {
		
		vector< weak_ptr<const CellType> > celltypes;
		if( celltype_from.isDefined() ){
			celltypes.push_back( celltype_from() );
		}
		else{
			celltypes = CPM::getCellTypes();
		}
		
		for(uint i=0; i<celltypes.size(); i++){
			auto ct = celltypes[i].lock();
			vector<CPM::CELL_ID> cells = ct->getCellIDs();
			
			for(uint c=0; c<cells.size(); c++){
				CPM::CELL_ID cellid = cells[c];
				const Cell& cell = CPM::getCell( cellid );

				const auto& interfaces = cell.getInterfaceLengths();
				for( auto i = interfaces.begin(); i != interfaces.end(); i++){
					uint ct_id = ct.get()->getID();
					CPM::CELL_ID nb_cellid = i->first;
					uint nb_ct_id = CPM::getCellIndex( nb_cellid ).celltype;
					
					//if( celltype_to.isDefined() && (nb_ct_id != ct_id) )
					if( celltype_to.isDefined() && (nb_ct_id != celltype_to().get()->getID() ) )
						continue;
					
					if( ignore_medium.isDefined() && (nb_ct_id == CPM::getEmptyCelltypeID() ))
						continue;
					
					double length = i->second;
					fout << time << "\t" << cellid << "\t" << ct_id << "\t" << nb_cellid << "\t" << nb_ct_id  << "\t" << length;
					if( log_duration.isDefined() ){
						std::pair<CPM::CELL_ID, CPM::CELL_ID> cid_pair;
						if( cellid < nb_cellid )
							cid_pair = std::make_pair( cellid, nb_cellid );
						else
							cid_pair = std::make_pair( nb_cellid, cellid );
						fout << "\t" << map_contact_duration[cid_pair];
					}
					fout << "\n";
				}
			}
		}
		
		if (time == SIM::getStartTime())
			last_logging = time;
		else
			last_logging = last_logging+ logging_time_step();
	}

	
/*	if( celltype.isDefined() ){
		vector<CPM::CELL_ID> cells = celltype()->getCellIDs();
		for(uint c=0; c<cells.size(); c++){
			CPM::CELL_ID cellid = cells[c];
			const Cell& cell = CPM::getCell( cellid );

			const auto& interfaces = cell.getInterfaceLengths();
			for( auto i = interfaces.begin(); i != interfaces.end(); i++){
				CPM::CELL_ID nb_cellid = i->first;
				if ( CPM::getCellIndex( nb_cellid ).celltype == celltype.get()->getID() ) { // if not medium
					fout << time << "\t" << cellid << "\t" << nb_cellid << "\t" << i->second << "\n";
				}
			}
			fout << "\n";
		}
	}
*/
}


void ContactLogger::finish(){
	if( fout.is_open() )
		fout.close();
}
