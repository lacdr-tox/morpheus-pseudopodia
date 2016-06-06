#include "init_cell_lattice.h"

REGISTER_PLUGIN(InitCellLattice);

bool InitCellLattice::run(CellType* ct)
{
	cout << "InitCellLattice: Initializing cells on lattice (each dot is a created cell)" << endl;
	uint cell_count = 0;
	FocusRange range(Granularity::Node, SIM::getGlobalScope());
	for (auto focus : range) {
		//cout << "cell count: " << cell_count << "\n";
		CPM::CELL_ID ID = ct->createCell();
		if( CPM::setNode( focus.pos() , ID) ){
			cout << ".";
			//cout << "InitCellLattice: Cell " <<  ID << " -> " << focus.pos() << "\n";
			cell_count++;
		}
	}
	cout << endl;
	cout << "InitCellLattice: " << cell_count << " cells created." << endl;

// 	shared_ptr<const Lattice> lattice = SIM::getLattice();
// 	uint cell_count = 0;
// 	for(int z=0; z < lattice->size().z; z++){
// 		for(int y=0; y < lattice->size().y; y++){
// 			for(int x=0; x < lattice->size().x; x++){
// 				
// 				if( !lattice->accessible( VINT(x,y,z) ) ) 
// 					continue;
// 				
// 				CPM::CELL_ID ID = ct->createCell();
// 				if( CPM::setNode(VINT(x,y,z), ID) ){
// 					//cout << "InitCellLattice: Cell " <<  ID << " -> " << VINT(x,y,z) << "\n";
// 					cell_count++;
// 				}
// 			}
// 		}
// 	}
}
