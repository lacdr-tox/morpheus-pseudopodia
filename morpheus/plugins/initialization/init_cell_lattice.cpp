#include "init_cell_lattice.h"

REGISTER_PLUGIN(InitCellLattice);

bool InitCellLattice::run(CellType* ct)
{
	shared_ptr<const Lattice> lattice = SIM::getLattice();
	uint cell_count = 0;
	for(int z=0; z < lattice->size().z; z++){
		for(int y=0; y < lattice->size().y; y++){
			for(int x=0; x < lattice->size().x; x++){
				CPM::CELL_ID ID = ct->createCell();
				if( CPM::setNode(VINT(x,y,z), ID) ){
					//cout << "InitCellLattice: Cell " <<  ID << " -> " << VINT(x,y,z) << "\n";
					cell_count++;
				}
			}
		}
	}
	cout << "InitCellLattice: " << cell_count << " cells created." << endl;
}
