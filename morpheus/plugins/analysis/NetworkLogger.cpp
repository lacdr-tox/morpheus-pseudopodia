#include "NetworkLogger.h"

REGISTER_PLUGIN(NetworkLogger);


void NetworkLogger::notify(double time)
{
	Analysis_Listener::notify(time);
		
	fout_nodes.open( to_string("nodes_",time,".log").c_str() , ios::out);
	fout_edges.open( to_string("edges_",time,".log").c_str() , ios::out);
	fout_network.open( to_string("network_",time,".log").c_str() , ios::out);
	fout_linked_cells.open( to_string("links_cellids_",time,".log").c_str() , ios::out);
	
	vector<CPM::CELL_ID> cells = celltype->getCellIDs();
	for(uint c=0; c<cells.size(); c++){
		
		const Cell& cell = CPM::getCell( cells[c] );		
		fout_nodes << cell.getID() << "\t" << cell.getCenter() <<  "\n";
		fout_linked_cells << cell.getID();
		
		const std::map< CPM::CELL_ID, uint >& interfaces = cell.getInterfaces();
		std::map <CPM::CELL_ID, uint >::const_iterator i = interfaces.begin();
		for(i = interfaces.begin(); i != interfaces.end(); i++){
			 CPM::CELL_ID nb_cellid = i->first;
// 			cout << "cell " << cell.getID() << " has interface with cell " << nb_cellid << endl;
			 if ( CPM::getCellIndex( nb_cellid ).celltype == celltype.get()->getID() ) { // if not medium
// 				cout << "not medium?? cell " << cell.getID() << " has interface with cell " << nb_cellid << endl;
				VDOUBLE nb_center = CPM::getCell( i->first ).getCenter();
				fout_network << cell.getCenter() << "\n" << nb_center << "\n";
				fout_edges << cell.getCenter() << "\n" << CPM::getCell( i->first ).getCenter() << "\n\n\n";
				fout_linked_cells << "\t" << nb_cellid << "\t" << i->second;
			}
			
		}
		fout_network << "\n";
		fout_linked_cells << "\n";
	}	
	
	fout_nodes.close();
	fout_edges.close();
	fout_network.close();
	fout_linked_cells.close();
	
	
}

void NetworkLogger::loadFromXML(const XMLNode Node)
{
    Analysis_Listener::loadFromXML( Node );
	getXMLAttribute(Node,"celltype", celltype_str);
}

set< string > NetworkLogger::getDependSymbols()
{
	return set<string>();
}


void NetworkLogger::init(double time){
	
	Analysis_Listener::init(time);
	
	// get cell type (must be given)
	celltype = CPM::findCellType(celltype_str);
	if (!celltype){
		cerr << "Logger::init: Celltype '" << celltype_str << "' does not exist! " << endl;
		exit(-1);
	}
}


string NetworkLogger::to_string(string a, int b, string c){
		stringstream ss;
		ss << a << setfill('0') << setw(4) << b << c;
		return ss.str();
}
