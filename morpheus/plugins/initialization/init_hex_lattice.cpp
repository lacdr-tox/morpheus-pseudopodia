#include "init_hex_lattice.h"

REGISTER_PLUGIN(InitHexLattice);

InitHexLattice::InitHexLattice()
{
// 	stored_node = node;
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

vector<CPM::CELL_ID> InitHexLattice::run(CellType* ct)
{
	shared_ptr<const Lattice> lattice = SIM::getLattice();
	
	if( lattice->getStructure() != Lattice::Structure::hexagonal )
		throw MorpheusException("InitHexLattice: This plugin requires a hexagonal lattice of size (91,91,1) or a multiple thereof.", stored_node);
	if( lattice->size().x % 91 > 1e-6 || lattice->size().y % 91 > 1e-6    )
		throw MorpheusException("InitHexLattice: This plugin requires a hexagonal lattice of size (91,91,1) or a multiple thereof.", stored_node);
		
	vector<CPM::CELL_ID> cells;
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
// 			cout << x << "\t" << y << " --> \t" << center << endl;
			CPM::CELL_ID id1 = makeCell(center, nbh, ct);
			cells.push_back(id1);
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
			if( randomness.isDefined() ) {
				center.x += getRandom01()*randomness();
				center.y += getRandom01()*randomness();
			}
			cout << x << "\t" << y << " --> \t" << center << endl;
			CPM::CELL_ID id1 = makeCell(center, nbh, ct);
			cells.push_back(id1);
		}
	}

	// remove empty cells
	for(auto &id : cells){
		if( CPM::getCell( id ).getNodes().size() == 0 ){
			ct->removeCell( id );
			cells.erase(std::remove(cells.begin(), cells.end(), id));
		}
	}

	return cells;
}

CPM::CELL_ID InitHexLattice::makeCell(VINT pos, vector<VINT> nbh, CellType* ct){
	CPM::CELL_ID id = ct->createCell();
	CPM::setNode(pos, id);
	for(VINT nb : nbh)
		CPM::setNode(pos+nb, id);
	return id;
}
