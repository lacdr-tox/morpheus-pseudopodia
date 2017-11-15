#include "init_circle.h"

REGISTER_PLUGIN(InitCircle);
//============================================================================

InitCircle::InitCircle()							//kreis-initialisierer
{
	celltype = nullptr;
    numcells.setXMLPath("number-of-cells");
	registerPluginParameter( numcells );
	
	mode.setXMLPath("mode");
	map< string, Mode > convmap;
	convmap["regular"] = InitCircle::REGULAR;
	convmap["random"] = InitCircle::RANDOM;
	mode.setConversionMap( convmap );
	registerPluginParameter( mode );
	
	center_eval.setXMLPath("Dimensions/center");
	registerPluginParameter( center_eval );
	
	radius_eval.setXMLPath("Dimensions/radius");
	registerPluginParameter( radius_eval );
	
};

vector<CPM::CELL_ID> InitCircle::run(CellType* ct)					//start & auswahl der initialisierungsmethode
{
	celltype = ct;
	vector<CPM::CELL_ID> cells;
	number_of_cells = numcells( SymbolFocus() );
	center 		= center_eval( SymbolFocus() );
	radius   	= radius_eval( SymbolFocus() );

	switch( mode() ){
		case InitCircle::RANDOM:
			return setRandom();
			break;
		case InitCircle::REGULAR:
			return setRegular();
			break;
		default:
			cout << "no type defined" << endl;
			return vector<CPM::CELL_ID>();
	}
}

vector<CPM::CELL_ID> InitCircle::setRandom()				//zufallsbelegung des bereichs
{
	shared_ptr< const Lattice > lattice = SIM::getLattice();
	int i = 0;
	vector<CPM::CELL_ID> cells;
	int attempts = 100000;
	while(i < number_of_cells and attempts > 0)									//solange maximalzahl der knoten nicht erreicht ist setze neue knoten
	{
		VINT newPos;
		newPos = lattice->getRandomPos();			//rückgabe einer zufallscenterition im gitter
	
		if( lattice->orth_distance( center , lattice->to_orth(newPos)).abs() <= radius )
		{
			auto cell = createNode(newPos);
			if (cell != CPM::NO_CELL) {
				cells.push_back(cell);
				i++;
			}
		}
		attempts--;
		//cout << "i = " << i << endl;
	}
	return cells;
}

//============================================================================

vector<CPM::CELL_ID> InitCircle::setRegular()			//gleichmäßige belegung des bereichs
{
	int i_lines = calculateLines();				//berechnung der bahnkreise
	vector<CPM::CELL_ID> cells;
	double dist_lines = double(radius / i_lines);	//abstand der bahnen
	double allCircuit = 0.0;
	shared_ptr<const Lattice> lattice = SIM::getLattice();

	for(int i = 1; i <= i_lines; i++)
		allCircuit += (2 * M_PI * i * dist_lines);				//berechnung des gesamtumfanges aller kreisbahnen
	double dist_numbers = allCircuit / number_of_cells;
	
	vector<int> cells_per_circuit;
	cells_per_circuit.push_back(1);
	for(int i = 1; i <= i_lines; i++) {
		cells_per_circuit.push_back(round(2 * M_PI * i * dist_lines / dist_numbers) );
	}
	cells_per_circuit.back() += number_of_cells - accumulate(cells_per_circuit.begin(),cells_per_circuit.end(), 0 );

	auto cell = createNode(lattice->from_orth(center));
	if (cell != CPM::NO_CELL)
		cells.push_back(cell);
	
	for(int i = 1; i <= i_lines; i++)			//berechnung der knotencenteritionen auf den kreisbahnen
	{
		double radius = i * dist_lines;
		for (int j=0; j<cells_per_circuit[i]; j++) {
			VDOUBLE newPos = center;
			double alpha = 2 * M_PI * double(j + 0.5*(i % 2))/double(cells_per_circuit[i]) ;
			
			newPos.x += cos(alpha) * radius;
			newPos.y += sin(alpha) * radius;
			cell = createNode(lattice->from_orth(newPos));
			if (cell != CPM::NO_CELL)
				cells.push_back(cell);
		}
	}
	return cells;
}

//============================================================================

CPM::CELL_ID InitCircle::createNode(VINT newPos)
{
	if(CPM::getNode(newPos) == CPM::getEmptyState())
	{
		CPM::CELL_ID newID = celltype->createCell();
		if (!CPM::setNode(newPos, newID))  {
			celltype->removeCell(newID);
			return CPM::NO_CELL;
		}
		return newID;
	}
	else
	{
// 		cout << " - creating cell at " << newPos << " FAILED" << endl;
		return CPM::NO_CELL;
	}
}

//============================================================================

int InitCircle::calculateLines() //methode zum errechnen der benötigten kreisbahnen
{
	vector<double> vec_diff;
	for(int i = 1; i <= radius; i++)
	{
		int i_lines = i;
		double allCircuit = 0.0;
		double dist_lines = double(radius / i_lines);

		for(int j = 1; j <= i_lines; j++)	//errechnen der gesamtumfänge der kreisbahnen
		{
			allCircuit += (j * dist_lines * 2 * M_PI);
		}

		double dist_number = (allCircuit / number_of_cells);
		vec_diff.push_back(abs(dist_lines - dist_number));	//differenzen der bahnenabstände bei verschiedener anzahl an bahnen
	}

	double diff = vec_diff[0];
	int i_index = 0;
	for(int i = 1; i < radius; i++)	//rückgabe der anzahl an bahnen, bei der die differenz am geringsten ist
	{
		if(vec_diff[i] < diff)
		{
			diff = vec_diff[i];
			i_index = i;
		}
	}

	return (i_index+1);
}

