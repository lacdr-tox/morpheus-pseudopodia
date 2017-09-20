#include "init_cell_lattice.h"

REGISTER_PLUGIN(InitCellLattice);

vector<CPM::CELL_ID> InitCellLattice::run(CellType* ct)
{
	cout << "InitCellLattice: Initializing cells on lattice (each dot is a created cell)" << endl;
	vector<CPM::CELL_ID> cells;
	FocusRange range(Granularity::Node, SIM::getGlobalScope());
	for (auto focus : range) {
		//cout << "cell count: " << cell_count << "\n";
		CPM::CELL_ID ID = ct->createCell();
		if( CPM::setNode( focus.pos() , ID) ){
			cout << ".";
			cells.push_back(ID);
		}
	}
	cout << endl;
	cout << "InitCellLattice: " << cells.size() << " cells created." << endl;
	return cells;
}
