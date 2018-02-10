#include "init_rectangle.h"

REGISTER_PLUGIN(InitRectangle);

InitRectangle::InitRectangle() {
    numcells.setXMLPath("number-of-cells");
	registerPluginParameter( numcells );
	
	mode.setXMLPath("mode");
	map< string, Mode > convmap;
	convmap["regular"] = InitRectangle::REGULAR;
	convmap["random"] = InitRectangle::RANDOM;
	convmap["grid"] = InitRectangle::GRID;
	mode.setConversionMap( convmap );
	registerPluginParameter( mode );

	random_displacement.setXMLPath("random-offset");
	random_displacement.setDefault("0.0");
	registerPluginParameter( random_displacement );
	
	origin_eval.setXMLPath("Dimensions/origin");
	registerPluginParameter( origin_eval );
	
	size_eval.setXMLPath("Dimensions/size");
	registerPluginParameter( size_eval );
};

vector<CPM::CELL_ID> InitRectangle::run(CellType* ct)
{
	vector<CPM::CELL_ID> cells;
	SymbolFocus global_focus = SymbolFocus();
	number_of_cells = numcells( global_focus );
	origin = origin_eval( global_focus );
	size   = size_eval( global_focus );
	shared_ptr<const Lattice> lattice = SIM::getLattice();
	
	
	cout << "InitRectangle: number of cells = " << number_of_cells << ", origin = " << origin << ", size = " << size << endl;

	if( mode() == InitRectangle::REGULAR && 
		random_displacement( global_focus ) > lattice->size().x && 
		random_displacement( global_focus ) > lattice->size().y )
	{
		cerr << "InitRectangle:: Displacement is larger than lattice size, defaulting to uniform random distribution" << endl;
		exit(-1);
	}
	if (mode() == InitRectangle::RANDOM &&
		random_displacement(global_focus) > 0)
	{
		cout << "InitRectangle: random_displacement > 0 only applicable with 'regular'. Makes no sense in combination with 'random' distribution." << endl;
	}
	if (mode() == InitRectangle::GRID &&
		lattice->getStructure() == Lattice::Structure::hexagonal) {
		//TODO test for hexagonal
		cerr << "Only tested with non-hexagonal lattices" << endl;
		exit(-1);
	}

	celltype = ct;

	if(size.x > lattice->size().x || size.y > lattice->size().y || size.z > lattice->size().z){
		cerr << "InitRectangle:: Dimensions/size (" << size << ") is larger than lattice  (" << lattice->size() << ")" << endl;
		exit(-1);
	}
	
	switch( mode() ){
		case InitRectangle::RANDOM:
			return setRandom();
			break;
		case InitRectangle::REGULAR:
			return setRegular();
			break;
		case InitRectangle::GRID:
			return setGrid();
			break;
		default:
			cout << "no type defined" << endl;
			return vector<CPM::CELL_ID>();
	}
}


//============================================================================

CPM::CELL_ID InitRectangle::createCell(VINT newPos)
{
	if(CPM::getLayer()->get(newPos) == CPM::getEmptyState() && CPM::getLayer()->writable(newPos))
	{
		uint newID = celltype->createCell();
		CPM::setNode(newPos, newID);
		//cout << " - creating cell " <<  ct->getCell(newID).getName() << endl;
		return newID;
	}
	else // position is already occupied
	{
		return CPM::NO_CELL;
	}
}

//============================================================================

vector<CPM::CELL_ID> InitRectangle::setRandom()			//zufallsbelegung des angegebenen bereichs
{
	vector<CPM::CELL_ID> cells;
	for(uint i=0; i < number_of_cells; i++){

		int rnd_x = origin.x +  getRandomUint( size.x );
		int rnd_y = origin.y +  getRandomUint( size.y );
		int rnd_z = origin.z +  getRandomUint( size.z );
		
		VINT newPos(rnd_x, rnd_y, rnd_z);
		//= SIM::findEmptyCPMNode(origin,size); // zufallige leere Position im Gitter
		auto new_cell = createCell(newPos);
		if ( new_cell != CPM::NO_CELL)
			cells.push_back(new_cell);
	
	}
	cout << "successfully created " << cells.size() << " cells." << endl;
	if (cells.size() < number_of_cells) {
		cout << "failed to create " << number_of_cells - cells.size() << " cells." << endl;
		
	}
	return cells;
}

//============================================================================

vector<CPM::CELL_ID> InitRectangle::setGrid()
{
	vector<CPM::CELL_ID> cells;

	shared_ptr<const Lattice> l = SIM::getLattice();

	if (l->getDimensions() == 1) {
		return setRegular();
	}
	if (l->getDimensions() == 2) {
		auto gridSize = computeGridSize(l->getDimensions(), size, number_of_cells);
		//TODO Why no integer division of VINT?
		auto gridDistance = VDOUBLE(size.x / gridSize.x, size.y / gridSize.y, size.z / gridSize.z);

		VDOUBLE newPos;
		for (auto z = 0; z < gridSize.z; z++) {
			for (auto y = 0; y < gridSize.y; y++) {
				for (auto x = 0; x < gridSize.x; x++) {
					auto gridPos = VDOUBLE(x, y, z);
					//TODO VDOUBLE multiplication confusing, following line doesn't work, but would be nice
					//newPos = origin + (gridPos + 0.5) * gridDistance;
					newPos.x = origin.x + (gridPos.x + 0.5) * gridDistance.x;
					newPos.y = origin.y + (gridPos.y + 0.5) * gridDistance.y;
					newPos.z = l->getDimensions() != 3 ? 0 : origin.z + (gridPos.z + 0.5) * gridDistance.z;

					newPos = addRandomDisplacement(newPos, random_displacement(SymbolFocus()), VINT(1, 1, 0));

					auto new_cell = createCell(l->from_orth(newPos));
					if (new_cell != CPM::NO_CELL) cells.push_back(new_cell);
				}
			}
		}
	}
	if (l->getDimensions() == 3) {
		return setRegular();
	}

	cout << "successfully created " << cells.size() << " cells." << endl;
	if (cells.size() < number_of_cells) {
		cout << "failed to create " << number_of_cells - cells.size() << " cells." << endl;
	}
	return cells;
}
vector<CPM::CELL_ID> InitRectangle::setRegular()
{
	vector<CPM::CELL_ID> cells;
	vector<double> vec_ids, vec_lines;	//koordinaten der knoten
	int i_lines, i_hlines, i_dlines, zeile, spalte, reihe;	//anzahl der linien, nummer der zeile, reihe und spalte

	shared_ptr<const Lattice> l = SIM::getLattice();

	if (l->getDimensions() == 1) {
		double cell_distance = double (size.x) / (number_of_cells);
		for(int i = 0; i < number_of_cells; i++)
		{
			int line_position  =  ( double(i + 0.5) * cell_distance );
			VDOUBLE newPos;						//neue knoten an position anlegen
			newPos.x = origin.x + line_position;
			newPos.y = 0;
			newPos.z = 0;

			if(random_displacement( SymbolFocus() ) != 0.0){
                newPos.x += getRandom01() * random_displacement( SymbolFocus() ) - random_displacement( SymbolFocus() )/2 ;
			}
			
			auto new_cell = createCell(l->from_orth(newPos));
			if ( new_cell != CPM::NO_CELL)
				cells.push_back(new_cell);
		}
	}
	if (l->getDimensions() == 2)	// wenn "2D", dann...
	{
		i_lines = int(sqrt((size.y * number_of_cells) / size.x) + 0.5);	//berechnung der linienanzahl
		i_lines = max(1, i_lines);
		double total_length  = i_lines * size.x;
		double cell_distance = total_length / (number_of_cells);
		double line_distance = (double)(size.y)/ (i_lines);
		line_distance = l->to_orth(VDOUBLE(0,line_distance,0)).y;
		
		for(int i = 0; i < number_of_cells; i++)
		{
			int line_position  =  ( double(i) * cell_distance );
			VDOUBLE newPos;						//neue knoten an position anlegen
			newPos.x = origin.x + line_position % size.x + 0.25;
			newPos.y = origin.y + (double (line_position /  size.x) ) * line_distance;
			newPos.z = 0;
			
			if(random_displacement( SymbolFocus() )  != 0.0){
                newPos.x += getRandom01() * random_displacement( SymbolFocus() ) - random_displacement( SymbolFocus() ) / 2 ;
                newPos.y += getRandom01() * random_displacement( SymbolFocus() ) - random_displacement( SymbolFocus() ) / 2 ;
                newPos.z += 0;
			}
			
			auto new_cell = createCell(l->from_orth(newPos));
			if ( new_cell != CPM::NO_CELL)
				cells.push_back(new_cell);
		}

	}

	if (l->getDimensions() == 3)		//wenn "3D", dann...
	{
		if (size.x < 1) size.x=1;
		if (size.y < 1) size.y=1;
		if (size.z < 1) size.z=1;
		double cell_distance = pow((double(size.y * size.z * size.x) / number_of_cells), (1.0 / 3.0));		//abstand der linien

		i_dlines = max ( 1, int((size.z / cell_distance) + 0.5));
		
		cell_distance = sqrt(double(size.y * size.x) / i_dlines / number_of_cells);
		i_hlines = max ( 1, int((size.y / cell_distance) + 0.5));
		
		i_lines = i_hlines * i_dlines;	//anzahl der linien
		cell_distance = double(i_lines * size.x) / number_of_cells;	//erneuter abstand der linien (geändert durch rundung der linienanzahl)

		for(int i = 0; i < number_of_cells; i++)	//knotenpositionen festlegen
		{vec_ids.push_back(i * cell_distance);}
		
		VINT offset = VINT( int(0.5*cell_distance),int(0.5 * cell_distance),0);
		for(int i = 0; i < number_of_cells; i++)
		{
			zeile = int(vec_ids[i] / size.x);
			spalte = int((int)vec_ids[i] % int(size.x));
			reihe = int(zeile / i_hlines);
			zeile = int((int)zeile % i_hlines);

			VINT newPos;					//neue knoten an positionen anlegen
			newPos.y = int(origin.y + (zeile * cell_distance));
			newPos.x = int(origin.x + spalte);
			newPos.z = int(origin.z + (reihe * cell_distance));

            if(random_displacement( SymbolFocus() ) != 0.0){
                newPos.x += getRandom01() * random_displacement( SymbolFocus() ) - random_displacement( SymbolFocus() )/2 ;
                newPos.y += getRandom01() * random_displacement( SymbolFocus() ) - random_displacement( SymbolFocus() )/2 ;
                newPos.z += 0;
            }

			auto new_cell = createCell(l->from_orth(newPos + offset));
			if ( new_cell != CPM::NO_CELL)
				cells.push_back(new_cell);
		}
	}
	
	cout << "successfully created " << cells.size() << " cells." << endl;
	if (cells.size() < number_of_cells) {
		cout << "failed to create " << number_of_cells - cells.size() << " cells." << endl;
	}
	return cells;
}

VDOUBLE
InitRectangle::addRandomDisplacement(VDOUBLE pos, double maxRandomDisplacement, VINT addRandomDisplacement) const {
	if (maxRandomDisplacement == 0.0) {
		return pos;
	}
	auto newPos = pos;
	if (addRandomDisplacement.x) newPos.x += (getRandom01() - 0.5) * maxRandomDisplacement;
	if (addRandomDisplacement.y) newPos.y += (getRandom01() - 0.5) * maxRandomDisplacement;
	if (addRandomDisplacement.z) newPos.z += (getRandom01() - 0.5) * maxRandomDisplacement;
	return newPos;
}

VINT InitRectangle::computeGridSize(int latticeDims, VINT latticeSize, int numberOfCells) const {
	switch (latticeDims) {
		case 2: {
			// Source: https://stackoverflow.com/a/2480939/1439843
			auto latticeRatio = (float) latticeSize.x / latticeSize.y;
			auto nCols = sqrt(numberOfCells * latticeRatio);
			auto nRows = sqrt(numberOfCells / latticeRatio);
			vector<pair<int, int>> possibleGridSizes = {
					{int(floor(nRows)), int(ceil(nCols))},
					{int(ceil(nRows)),  int(floor(nCols))},
					{int(ceil(nRows)),  int(ceil(nCols))}
			};
			// Remove combinations that result in too few grid positions
			possibleGridSizes.erase(
					std::remove_if(
							possibleGridSizes.begin(),
							possibleGridSizes.end(),
							[&](const std::pair<int, int> &pair) {
								return pair.first * pair.second < numberOfCells;
							}
					),
					possibleGridSizes.end()
			);
			// Find row column configuration that results fewest excess grid positions
			auto tightestGridSize = std::min_element(begin(possibleGridSizes), end(possibleGridSizes),
													 [](const std::pair<int, int> &left,
														const std::pair<int, int> &right) {
														 return left.first * left.second < right.first * right.second;
													 });
			return {tightestGridSize->second, tightestGridSize->first, 1};
		}
		default: {
			throw (string("InitRectangle::computeGridSize: not implemented for lattice dimensions == ") +
				   std::to_string(latticeDims));
		}
	}
}

//============================================================================
