#include "init_poisson_disc.h"

REGISTER_PLUGIN(InitPoissonDisc);

InitPoissonDisc::InitPoissonDisc() {
	num_cells.setXMLPath("number-of-cells");
	registerPluginParameter( num_cells );
};

vector<CPM::CELL_ID> InitPoissonDisc::run(CellType* ct)
{
	cout << "InitPoissonDisc: run" << endl;

    celltype = ct;
	lattice = SIM::getLattice();

	if(lattice->getDimensions() != 2){
		throw MorpheusException(string("InitPoissonDisc is only implemented for 2D square lattices."),stored_node);
	}

	double magnification = 1.0;
	double aspect_ratio = 1.0;
	if(lattice->size().x > lattice->size().y){
		magnification = lattice->size().x;
		aspect_ratio = lattice->size().x / lattice->size().y;
	}
	else{
		magnification = lattice->size().y;
		aspect_ratio = lattice->size().y / lattice->size().x;
	}


	PoissonGenerator::DefaultPRNG PRNG;
	cout << "aspect_ratio*num_cells(SymbolFocus()) = " << aspect_ratio*num_cells(SymbolFocus()) << endl;
	const auto Points = PoissonGenerator::GeneratePoissonPoints( aspect_ratio*num_cells(SymbolFocus()), PRNG );

	cout << "Number of points = " << Points.size() << endl;

	vector<CPM::CELL_ID> cells;
	for ( auto i = Points.begin(); i != Points.end(); i++ )
	{
		VINT pos = VINT(int( i->x * magnification ), int( i->y * magnification ), 0.0);
		if(pos.x < lattice->size().x && pos.y < lattice->size().y){
			cout << pos << endl;
			auto new_cell = createCell( pos );
			if (new_cell != CPM::NO_CELL)
				cells.push_back(new_cell);
		}
	}
	cout << "InitPoissonDisc: number of cells created : " << cells.size() << endl;
	return cells;
}

//============================================================================
CPM::CELL_ID InitPoissonDisc::createCell(VINT newPos)
{
    if(CPM::getLayer()->get(newPos) == CPM::getEmptyState() && CPM::getLayer()->writable(newPos))
    {
        uint newID = celltype->createCell();
        CPM::setNode(newPos, newID);
        return newID;
    }
    else // position is already occupied
    {
        return CPM::NO_CELL;
    }
}
