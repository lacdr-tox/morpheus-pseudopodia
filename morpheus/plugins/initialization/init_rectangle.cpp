#include "init_rectangle.h"

REGISTER_PLUGIN(InitRectangle);

InitRectangle::InitRectangle() {
	num_cells_eval.setXMLPath("number-of-cells");
	registerPluginParameter(num_cells_eval);

	mode_eval.setXMLPath("mode");
	map<string, Mode> mode_map;
	mode_map["regular"] = Mode::REGULAR;
	mode_map["random"] = Mode::RANDOM;
	mode_map["grid"] = Mode::GRID;
	mode_eval.setConversionMap(mode_map);
	registerPluginParameter(mode_eval);

	random_displacement_eval.setXMLPath("random-offset");
	random_displacement_eval.setDefault("0.0");
	registerPluginParameter(random_displacement_eval);

	origin_eval.setXMLPath("Dimensions/origin");
	registerPluginParameter(origin_eval);

	size_eval.setXMLPath("Dimensions/size");
	registerPluginParameter(size_eval);
};

vector<CPM::CELL_ID> InitRectangle::run(CellType *ct) {
	celltype = ct;

	auto global_focus = SymbolFocus();
	num_cells = num_cells_eval(global_focus);
	mode = mode_eval();
	random_displacement = random_displacement_eval(global_focus);
	origin = VINT(origin_eval(global_focus));
	size = VINT(size_eval(global_focus));

	auto lattice = SIM::getLattice();

	cout << "InitRectangle: number of cells = " << num_cells << ", origin = " << origin << ", size = " << size << endl;

	auto clamped_size = clamp(size, {1,1,1}, lattice->size());
	if(size != clamped_size) {
		cout << "InitRectangle: clamped size from " << size.to_stringp() << " to " << clamped_size.to_stringp() << endl;
		size = clamped_size;
	}
	if (mode != Mode::RANDOM &&
		random_displacement > lattice->size().x &&
		random_displacement > lattice->size().y) {
		cerr << "InitRectangle:: Displacement is larger than lattice size, defaulting to uniform random distribution"
			 << endl;
		mode = Mode::RANDOM;
		random_displacement = 0;
	}
	if (mode == Mode::RANDOM &&
		random_displacement > 0) {
		cout << "InitRectangle: random_displacement > 0 only applicable with 'regular' or 'grid', "
			 << "makes no sense in combination with 'random' distribution."
			 << endl;
	}
	auto size_diff = (origin + size - lattice->size()).to_array();
	if (any_of(size_diff.begin(), size_diff.end(), [](int i) {return i > 0;})) {
		cerr << "InitRectangle:: origin " << origin.to_stringp() << " + size " << size.to_stringp()
			 <<	" is larger than lattice " << lattice->size().to_stringp() << endl;
		exit(EXIT_FAILURE);
	}

	vector<CPM::CELL_ID> cells;
	switch (mode) {
		case Mode::RANDOM:
			cells = setRandom();
			break;
		case Mode::REGULAR:
			cells = setRegular();
			break;
		case Mode::GRID:
			cells = setGrid();
			break;
	}

	cout << "successfully created " << cells.size() << " cells." << endl;
	if (cells.size() < num_cells) {
		cout << "failed to create " << num_cells - cells.size() << " cells." << endl;
	}
	return cells;
}


//============================================================================

CPM::CELL_ID InitRectangle::createCell(VINT newPos) {
	if (CPM::getLayer()->get(newPos) == CPM::getEmptyState() && CPM::getLayer()->writable(newPos)) {
		uint newID = celltype->createCell();
		CPM::setNode(newPos, newID);
		return newID;
	} else {
		// position is already occupied
		return CPM::NO_CELL;
	}
}

//============================================================================

vector<CPM::CELL_ID> InitRectangle::setRandom() {
	vector<CPM::CELL_ID> cells;
	for (uint i = 0; i < num_cells; i++) {
		auto newPos = origin + VINT(getRandomUint((uint) size.x - 1),
									getRandomUint((uint) size.y - 1),
									getRandomUint((uint) size.z - 1));
		auto new_cell = createCell(newPos);
		if (new_cell != CPM::NO_CELL) cells.push_back(new_cell);
	}
	return cells;
}

//============================================================================

vector<CPM::CELL_ID> InitRectangle::setGrid() {
	auto l = SIM::getLattice();
	auto latticeDims = l->getDimensions();
	auto gridSize = VDOUBLE(size);
	if (l->getStructure() == Lattice::Structure::hexagonal) {
		// Correction for hexagonal grid height by multiplying with sin of 60 degrees
		gridSize.y *= sin(M_PI / 3.0);
	}
	auto coordMask = VDOUBLE(1, latticeDims > 1, latticeDims > 2);

	auto gridShape = computeGridShape(latticeDims, VINT(gridSize), num_cells);
	auto gridDistance = gridSize / gridShape; // distance between items on the grid

	vector<CPM::CELL_ID> cells;
	int cellsCreated = 0;
	for (auto z = 0; z < gridShape.z; z++) {
		for (auto y = 0; y < gridShape.y; y++) {
			for (auto x = 0; x < gridShape.x; x++) {
				auto newPos = origin + (VDOUBLE(x, y, z) + 0.5) * gridDistance;

				// Add random displacement
				newPos += random_displacement * (getRandomVDOUBLE() - 0.5);

				// Apply mask to avoid non-zero coordinates in 1D and 2D cases
				// TODO alternative is overriding from_orth for Linear and Square lattices
				newPos *= coordMask;

				auto new_cell = createCell(l->from_orth(newPos));
				if (new_cell != CPM::NO_CELL) cells.push_back(new_cell);

				if (++cellsCreated == num_cells) return cells;
			}
		}
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
		double cell_distance = double (size.x) / (num_cells);
		for(int i = 0; i < num_cells; i++)
		{
			int line_position  =  ( double(i + 0.5) * cell_distance );
			VDOUBLE newPos;						//neue knoten an position anlegen
			newPos.x = origin.x + line_position;
			newPos.y = 0;
			newPos.z = 0;

			if(random_displacement != 0.0){
                newPos.x += getRandom01() * random_displacement - random_displacement / 2 ;
			}
			
			auto new_cell = createCell(l->from_orth(newPos));
			if ( new_cell != CPM::NO_CELL)
				cells.push_back(new_cell);
		}
	}
	if (l->getDimensions() == 2)	// wenn "2D", dann...
	{
		i_lines = int(sqrt((size.y * num_cells) / size.x) + 0.5);	//berechnung der linienanzahl
		i_lines = max(1, i_lines);
		double total_length  = i_lines * size.x;
		double cell_distance = total_length / (num_cells);
		double line_distance = (double)(size.y)/ (i_lines);
		line_distance = l->to_orth(VDOUBLE(0,line_distance,0)).y;
		
		for(int i = 0; i < num_cells; i++)
		{
			int line_position  =  ( double(i) * cell_distance );
			VDOUBLE newPos;						//neue knoten an position anlegen
			newPos.x = origin.x + line_position % size.x + 0.25;
			newPos.y = origin.y + (double (line_position /  size.x) ) * line_distance;
			newPos.z = 0;
			
			if(random_displacement  != 0.0){
                newPos.x += getRandom01() * random_displacement - random_displacement / 2 ;
                newPos.y += getRandom01() * random_displacement - random_displacement / 2 ;
                newPos.z += 0;
			}
			
			auto new_cell = createCell(l->from_orth(newPos));
			if ( new_cell != CPM::NO_CELL)
				cells.push_back(new_cell);
		}

	}

	if (l->getDimensions() == 3)		//wenn "3D", dann...
	{
		double cell_distance = pow((double(size.y * size.z * size.x) / num_cells), (1.0 / 3.0));		//abstand der linien

		i_dlines = max ( 1, int((size.z / cell_distance) + 0.5));
		
		cell_distance = sqrt(double(size.y * size.x) / i_dlines / num_cells);
		i_hlines = max ( 1, int((size.y / cell_distance) + 0.5));
		
		i_lines = i_hlines * i_dlines;	//anzahl der linien
		cell_distance = double(i_lines * size.x) / num_cells;	//erneuter abstand der linien (geändert durch rundung der linienanzahl)

		for(int i = 0; i < num_cells; i++)	//knotenpositionen festlegen
		{vec_ids.push_back(i * cell_distance);}
		
		VINT offset = VINT( int(0.5*cell_distance),int(0.5 * cell_distance),0);
		for(int i = 0; i < num_cells; i++)
		{
			zeile = int(vec_ids[i] / size.x);
			spalte = int((int)vec_ids[i] % int(size.x));
			reihe = int(zeile / i_hlines);
			zeile = int((int)zeile % i_hlines);

			VINT newPos;					//neue knoten an positionen anlegen
			newPos.y = int(origin.y + (zeile * cell_distance));
			newPos.x = int(origin.x + spalte);
			newPos.z = int(origin.z + (reihe * cell_distance));

            if(random_displacement != 0.0){
                newPos.x += getRandom01() * random_displacement - random_displacement / 2 ;
                newPos.y += getRandom01() * random_displacement - random_displacement / 2 ;
                newPos.z += 0;
            }

			auto new_cell = createCell(l->from_orth(newPos + offset));
			if ( new_cell != CPM::NO_CELL)
				cells.push_back(new_cell);
		}
	}
	
	return cells;
}

VDOUBLE InitRectangle::getRandomVDOUBLE() {
	return {getRandom01(), getRandom01(), getRandom01()};
}

VINT InitRectangle::computeGridShape(uint latticeDims, VINT gridSize, int numberOfCells) {
	if (latticeDims == 1) {
		return {numberOfCells, 1, 1};
	}

	vector<VINT> possibleGridSizes;
	if (latticeDims == 2) {
		// Source: https://stackoverflow.com/a/2480939/1439843
		auto gridRatio = (float) gridSize.x / gridSize.y;
		auto nCols = sqrt(numberOfCells * gridRatio);
		auto nRows = sqrt(numberOfCells / gridRatio);

		// Check for weird aspect ratios, in which case we fallback to a 1D grid
		if (nCols <= 1) return {1, numberOfCells, 1};
		if (nRows <= 1) return {numberOfCells, 1, 1};

		// No weird grid sizes, since we test floor and ceil of rows and columns we have
		// 2^2 - 1 possibilities (floor of both will be < number of cells)
		possibleGridSizes = {
				{int(floor(nCols)), int(ceil(nRows)),  1},
				{int(ceil(nCols)),  int(floor(nRows)), 1},
				{int(ceil(nCols)),  int(ceil(nRows)),  1}
		};
	} else { // latticeDims == 3
		// Own adaptation from 2D case
		auto gridRatioXY = (float) gridSize.x / gridSize.y;
		auto gridRatioXZ = (float) gridSize.x / gridSize.z;
		auto gridRatioYZ = (float) gridSize.y / gridSize.z;

		auto nCols = cbrt(numberOfCells * gridRatioXY * gridRatioXZ);
		auto nRows = cbrt(numberOfCells / gridRatioXY * gridRatioYZ);
		auto nSlices = cbrt(numberOfCells / gridRatioXZ / gridRatioYZ);

		// Check for weird aspect ratios, in which case we fallback to a 2D or even 1D grid
		if (nCols <= 1) {
			auto grid2D = computeGridShape(2, VINT(gridSize.y, gridSize.z, 1), numberOfCells);
			return {1, grid2D.x, grid2D.y};
		}
		if (nRows <= 1) {
			auto grid2D = computeGridShape(2, VINT(gridSize.x, gridSize.z, 1), numberOfCells);
			return {grid2D.x, 1, grid2D.y};
		}
		if (nSlices <= 1) {
			auto grid2D = computeGridShape(2, VINT(gridSize.x, gridSize.y, 1), numberOfCells);
			return {grid2D.x, grid2D.y, 1};
		}

		// No weird grid sizes, let's see what our options are
		possibleGridSizes = {
				{int(ceil(nCols)),  int(floor(nRows)), int(floor(nSlices))},
				{int(floor(nCols)), int(ceil(nRows)),  int(floor(nSlices))},
				{int(ceil(nCols)),  int(ceil(nRows)),  int(floor(nSlices))},
				{int(floor(nCols)), int(floor(nRows)), int(ceil(nSlices))},
				{int(ceil(nCols)),  int(floor(nRows)), int(ceil(nSlices))},
				{int(floor(nCols)), int(ceil(nRows)),  int(ceil(nSlices))},
				{int(ceil(nCols)),  int(ceil(nRows)),  int(ceil(nSlices))}
		};
	}

	// Remove combinations that result in too few grid positions
	possibleGridSizes.erase(
			std::remove_if(
					possibleGridSizes.begin(),
					possibleGridSizes.end(),
					[&](const VINT &gridSize) {
						return gridSize.x * gridSize.y * gridSize.z < numberOfCells;
					}
			),
			possibleGridSizes.end()
	);

	// Find configuration that results fewest excess grid positions
	return *std::min_element(begin(possibleGridSizes), end(possibleGridSizes),
							 [](const VINT &left,
								const VINT &right) {
								 return left.x * left.y * left.z < right.x * right.y * right.z;
							 });
}

//============================================================================
