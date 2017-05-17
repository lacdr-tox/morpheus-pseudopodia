#include "contact_logger.h"

REGISTER_PLUGIN(ContactLogger);

ContactLogger::ContactLogger() : InstantaneousProcessPlugin( TimeStepListener::XMLSpec::XML_NONE ) {
	celltype.setXMLPath("celltype");
	registerPluginParameter(celltype);
	ignore_medium.setXMLPath("ignore-medium");
	ignore_medium.setDefault("true");
	registerPluginParameter(ignore_medium);
	log_duration.setXMLPath("log-duration");
	registerPluginParameter(log_duration);
	
}

void ContactLogger::init(const Scope* scope){
	AnalysisPlugin::init(scope);
	InstantaneousProcessPlugin::init( scope );
	InstantaneousProcessPlugin::setTimeStep( CPM::getMCSDuration() );

	
	// open file
	stringstream ss;
	ss << "contact_logger";
	if( celltype.isDefined() )
		ss << "_" << celltype()->getName();
	ss << ".log";
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

void ContactLogger::loadFromXML(const XMLNode node){
	AnalysisPlugin::loadFromXML(node);
	InstantaneousProcessPlugin::loadFromXML(node);
}

void ContactLogger::executeTimeStep(){
	if( !log_duration.isDefined() )
		return;

	vector< weak_ptr<const CellType> > celltypes;
	if( celltype.isDefined() ){
		celltypes.push_back( celltype() );
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
				if( celltype.isDefined() && (nb_ct_id != ct_id) )
					continue;
				
				if( ignore_medium.isDefined() && (nb_ct_id == CPM::getEmptyCelltypeID() ))
					continue;

				double length = i->second;

				std::pair<CPM::CELL_ID, CPM::CELL_ID> cid_pair;
				if( cellid < nb_cellid )
					cid_pair = std::make_pair( cellid, nb_cellid );
				else
					cid_pair = std::make_pair( nb_cellid, cellid );
				
				if ( map_contact_duration.find( cid_pair ) == map_contact_duration.end() ) { // if cell pair not in map
					if( length > 0.0 ){
						map_contact_duration[ cid_pair ] = InstantaneousProcessPlugin::timeStep();
					}
				} 
				else { // if cell pair already in map
					if( length > 0.0 ){
						map_contact_duration[ cid_pair ] += InstantaneousProcessPlugin::timeStep();
					}
					else{ // if cell pair not in contact (anymore), remove key
						map_contact_duration.erase( cid_pair );
					}
				}
			}
		}
	}
}

void ContactLogger::analyse(double time)
{
	
	vector< weak_ptr<const CellType> > celltypes;
	if( celltype.isDefined() ){
		celltypes.push_back( celltype() );
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
				
				if( celltype.isDefined() && (nb_ct_id != ct_id) )
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
					fout << "\t" << map_contact_duration[cid_pair] / 2.0; // factor 2.0 because duration will be added twice (once for each interaction pair)
				}
				fout << "\n";
			}
		}
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
