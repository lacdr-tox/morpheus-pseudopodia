#include "init_hex_lattice.h"

REGISTER_PLUGIN(InitHexLattice);

void InitHexLattice::loadFromXML(const XMLNode node)
{
	stored_node = node;
	mode.setXMLPath("mode");
	map<string, Direction> conv_map;
	conv_map["left"] = Direction::LEFT;
	conv_map["right"] = Direction::RIGHT;
	mode.setDefault("left");
	mode.setConversionMap(conv_map);
	registerPluginParameter(mode);

	randomness.setXMLPath("randomness");
	randomness.setDefault("0.0");
	registerPluginParameter(randomness);

}

bool InitHexLattice::run(CellType* ct)
{
	shared_ptr<const Lattice> lattice = SIM::getLattice();
	
	if( lattice->getStructure() != Lattice::Structure::hexagonal )
		throw MorpheusException("InitHexLattice: This plugin requires a hexagonal lattice of size (91,91,1) or a multiple thereof.", stored_node);
	if( lattice->size().x % 91 > 1e-6 || lattice->size().y % 91 > 1e-6    )
		throw MorpheusException("InitHexLattice: This plugin requires a hexagonal lattice of size (91,91,1) or a multiple thereof.", stored_node);
		
	
	int nbh_order = 11;
	vector<VINT> nbh = lattice->getNeighborhoodByOrder( nbh_order ).neighbors();
	cout << "nbh_order : " << nbh_order << ", size: " << nbh.size() << endl;
	
	
	VINT origin = VINT(6, 6, 0);
	
	int num_cells_x = floor(lattice->size().x / 16)+1;
	int num_cells_y = floor(lattice->size().y / 12)+2;
	cout << "NUMBER OF CELLS: " << num_cells_x << "\t" << num_cells_y << endl;
	
	for(int x=0; x<num_cells_x; x++){
		for(int y=0; y<num_cells_y; y++){
			VDOUBLE center;
			if( mode() == InitHexLattice::Direction::LEFT )
				center = origin + VINT(16*x-6*y, (12-1)*y+x, 0);
			else
				center = origin + VINT(17*x-5*y, (12-1)*y-x, 0);
			if( randomness.isDefined() ){
				center.x += getRandom01()*randomness();
				center.y += getRandom01()*randomness();
				cout << "Randomness: " << center << endl;
			}
			cout << x << "\t" << y << " --> \t" << center << endl;
			CPM::CELL_ID id1 = makeCell(center, nbh, ct);
		}
	}

	VINT origin2;
	if( mode() == InitHexLattice::Direction::LEFT )
		origin2 = origin + VINT(5, 6, 0);
	else
		origin2 = origin + VINT(6, 5, 0);
	
	cout << "NUMBER OF CELLS: " << num_cells_x << "\t" << num_cells_y << endl;
	for(int x=0; x<num_cells_x; x++){
		for(int y=0; y<num_cells_y; y++){
			VDOUBLE center;
			if( mode() == InitHexLattice::Direction::LEFT )
				center = origin2 + VINT(16*x-6*y, (12-1)*y+x, 0);
			else
				center = origin2 + VINT(17*x-5*y, (12-1)*y-x, 0);
			if( randomness.isDefined() )
				center.x += getRandom01()*randomness();
				center.y += getRandom01()*randomness();
			cout << x << "\t" << y << " --> \t" << center << endl;
			CPM::CELL_ID id1 = makeCell(center, nbh, ct);
		}
	}

	// remove empty cells
	vector<CPM::CELL_ID> cellids = ct->getCellIDs();
	for(auto &id : cellids){
		if( CPM::getCell( id ).getNodes().size() == 0 )
			ct->removeCell( id );
	}

	return true;
// 	center += VINT(6, 5, 0);
// 	CPM::CELL_ID id2 = makeCell(center, nbh, ct);
// 	
// 	const map<CPM::CELL_ID,uint>& interfaces = CPM::getCell( id1 ).getInterfaces();
// 	uint interface_length = 0;
// 	for( auto i : interfaces )
// 		interface_length += i.second;
// 	cout << "INTERFACE LENGTH = " << interface_length << endl;

		
// 	uint cell_count = 0;
// 	for(int y=0; y < lattice->size().y; y++){
// 		for(int x=0; x < lattice->size().x; x++){
// 			CPM::CELL_ID ID = ct->createCell();
// 			cout << ID << ":\t" << x << "\t" << y << "\n";
// 			//if( CPM::setNode(VINT(x,y,z), ID) ){
// 				//cout << "InitHexLattice: Cell " <<  ID << " -> " << VINT(x,y,z) << "\n";
// 				cell_count++;
// 			//}
// 		}
// 	}
// 	cout << "InitHexLattice: " << cell_count << " cells created." << endl;
}

CPM::CELL_ID InitHexLattice::makeCell(VINT pos, vector<VINT> nbh, CellType* ct){
	CPM::CELL_ID id = ct->createCell();
	CPM::setNode(pos, id);
	for(VINT nb : nbh)
		CPM::setNode(pos+nb, id);
	return id;
}
