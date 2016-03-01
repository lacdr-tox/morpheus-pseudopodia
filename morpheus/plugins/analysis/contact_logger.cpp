#include "contact_logger.h"

REGISTER_PLUGIN(ContactLogger);

ContactLogger::ContactLogger(){
	celltype.setXMLPath("celltype");
	registerPluginParameter(celltype);
}

void ContactLogger::init(const Scope* scope){
	AnalysisPlugin::init(scope);
	stringstream ss;
	ss << "contact_logger_" << celltype()->getName() << ".txt";
	fout.open(ss.str().c_str(), fstream::out );
	if( !fout.is_open() ){
		cout << "Error opening file " << ss.str() << endl;
		return;
	}
	// header
	fout << "Cell" << "\t" << "Neighbor" << "\t" << "Contact" << "\n";
}

void ContactLogger::loadFromXML(const XMLNode node){
	AnalysisPlugin::loadFromXML(node);
}

void ContactLogger::analyse(double time)
{
	vector<CPM::CELL_ID> cells = celltype()->getCellIDs();
	for(uint c=0; c<cells.size(); c++){
		CPM::CELL_ID cellid = cells[c];
		const Cell& cell = CPM::getCell( cellid );

		const std::map< CPM::CELL_ID, uint >& interfaces = cell.getInterfaces();
		std::map <CPM::CELL_ID, uint >::const_iterator i = interfaces.begin();
		for(i = interfaces.begin(); i != interfaces.end(); i++){
			 CPM::CELL_ID nb_cellid = i->first;
			 if ( CPM::getCellIndex( nb_cellid ).celltype == celltype.get()->getID() ) { // if not medium
		 		fout << cellid << "\t" << nb_cellid << "\t" << i->second << "\n";
			}
		}
		fout << "\n";
	}
}


void ContactLogger::finish(){
	if( fout.is_open() )
		fout.close();
}
