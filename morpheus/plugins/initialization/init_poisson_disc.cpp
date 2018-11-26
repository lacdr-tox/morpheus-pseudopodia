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
		throw MorpheusException("InitPoissonDisc is only implemented for 2D square lattices.");
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

/*	cout << "InitPoissonDisc: run" << endl;
	// http://devmag.org.za/2009/05/03/poisson-disk-sampling/
	SymbolFocus f = SymbolFocus();
	lattice = SIM::getLattice();
    nbh = lattice->getNeighborhood(4);
    cubic = lattice->getDimensions()==3 ? true:false;

    cellSize = min_dist(f) / sqrt(2.0);
	VINT gridSize( lattice->size().x / cellSize,
			   lattice->size().y / cellSize,
               (cubic ? lattice->size().z / cellSize : 0) );

	//grid = createGrid(gridSize, VDOUBLE(-1,-1,-1) );
	grid_x = createGrid(gridSize, lattice->getStructure(), -1 );
	grid_y = createGrid(gridSize, lattice->getStructure(), -1 );
	grid_z = createGrid(gridSize, lattice->getStructure(), -1 );

	deque<VDOUBLE> processList;
	vector<VDOUBLE> samplePoints;

	// generate an initial point
// 	VDOUBLE firstPoint( getRandom01() * lattice->size().x,
// 						getRandom01() * lattice->size().y,
// 						(cubic ? getRandom01() * lattice->size().z: 0));
	VDOUBLE firstPoint( lattice->size().x/2.0,
						lattice->size().y/2.0,
                        (cubic ? lattice->size().z/2.0: 0));

	// add point to grid
	processList.push_back( firstPoint );
	samplePoints.push_back( firstPoint );
    addPointToGrid(firstPoint);

	double maxi_dist = 0.0;
	if( max_dist.isDefined() ){
		if( max_dist(f) <= min_dist(f) ){
			cerr << "InitPoissonDisc: Error: maximum must be larger than minimum!" << endl;
			exit(-1);
		}
		maxi_dist = max_dist(f);
	}
	else
		maxi_dist = 2.0*min_dist(f);


	//generate other points from points in queue
	int iteration=0;
    while( !processList.empty() && samplePoints.size() < 99999){
		iteration++;
		// get random point from processList
		random_shuffle(processList.begin(), processList.end());
		VDOUBLE point = processList.front();
		processList.pop_front();

		for(int i = 0; i < new_points_count(); i++){

            VDOUBLE newPoint = generateRandomPointAround(point, min_dist(f), maxi_dist);
			//cout << newPoint << "\n";
			//check that the point is in the image region
			//and no points exists in the point's neighbourhood
			if( inRectangle(newPoint, lattice->size()) &&
                noNeighbors(newPoint, min_dist(f)))
			{
				//update containers
				processList.push_back(newPoint);
				samplePoints.push_back(newPoint);
				addPointToGrid(newPoint);
				continue;
			}
		}
		cout << "Iteration = " << iteration << ", processList, size: " << processList.size() << endl;
	}
*/

/*	for(uint x=0; x<grid_x->size().x; x++){
		for(uint y=0; y<grid_x->size().y; y++){
			cout << "(" << grid_x->get(VINT(x,y,0)) << ", " << grid_y->get(VINT(x,y,0)) << ")\t";
		}
		cout << endl;
	}

	for(uint i=0; i<samplePoints.size(); i++){
		VINT pos = VINT(samplePoints[i]);
		cout << "samplePoints: " << samplePoints[i] << ", VINT: " << pos << "\t: " << imageToGrid(samplePoints[i]) << endl;
	}
*/

// 	for(uint i=0; i<samplePoints.size(); i++){
// 		VINT pos = VINT( round(samplePoints[i].x), round(samplePoints[i].y), round(samplePoints[i].z));
// 		if( createCell( pos ) ){
// 			//cout << "Cell created at " << pos << "\n";
// 		}
// 	}
/*
	vector<CPM::CELL_ID> cells;
	for(uint z=0; z<grid_x->size().z; z++){
		for(uint y=0; y<grid_x->size().y; y++){
			for(uint x=0; x<grid_x->size().x; x++){
				VDOUBLE pos_double = VDOUBLE(grid_x->get(VINT(x,y,z)), grid_y->get(VINT(x,y,z)), grid_z->get(VINT(x,y,z)) );
				VINT pos = VINT( pos_double );
				auto new_cell = createCell( pos );
				if (new_cell != CPM::NO_CELL)
					cells.push_back(new_cell);
			}
		}
	}
	cout << "InitPoissonDisc: number of cells created : " << cells.size() << endl;
	return cells;
}*/

/*

VINT InitPoissonDisc::imageToGrid(VDOUBLE point)
{
  int gridX = (int)(point.x / cellSize);
  int gridY = (int)(point.y / cellSize);
  int gridZ = (int)(point.z / cellSize);
  if(!cubic)
      gridZ = 0;
  //cout << "InitPoissonDisc::imageToGrid: " << point << " cellsize " << cellSize << ", gridpos: " << VINT(gridX, gridY, gridZ) << endl;
  return VINT(gridX, gridY, gridZ);
}


//============================================================================
bool InitPoissonDisc::addPointToGrid(VDOUBLE point){
    VINT gridpos = imageToGrid(point);
	if( grid_x->get( gridpos ) != -1 ){
		cout << "WARNING: InitPoissonDisc::addPointToGrid: " << gridpos << " is NOT empty!!" << endl;
		//exit(-1);
		return false;
	}
    grid_x->set(gridpos, point.x);
    grid_y->set(gridpos, point.y);
    grid_z->set(gridpos, point.z);
    //cout << "Created at pos = " << gridpos  << ": " << grid_x->get(gridpos) << endl;
    return true;
}

//============================================================================
bool InitPoissonDisc::noNeighbors(VDOUBLE p1, double mindist)
{
    VINT pos = imageToGrid(p1);

	// add focal point (self) to neighborhood
   	Neighborhood nbh_incl = nbh;
	//nbh_incl.insert(nbh_incl.begin(), VINT(0,0,0)); // add to first position

//     cout << "\n\nnoNeighbors: p1  " << p1 << " @ " << pos << endl;
	//look in the neighbourhood
	for(int i=0; i<nbh_incl.size();i++){
		VINT nb = nbh_incl[i]+pos;

		VDOUBLE p2;
		if(!cubic)
			p2 = VDOUBLE(grid_x->get( nb ), grid_y->get( nb ), -1);
		else
			p2 = VDOUBLE(grid_x->get( nb ), grid_y->get( nb ), grid_z->get( nb ));

		//if( p2 != VDOUBLE(-1,-1,-1) ){ // is not empty
		if( p2.x != -1 || p2.y != -1 || p2.z != -1 ){ // is not empty
            double distance = 0.0;
            if(cubic)
                distance = sqrt( sqr( p1.x - p2.x ) + sqr( p1.y - p2.y ) + sqr( p1.z - p2.z ) );
            else
                distance = sqrt( sqr( p1.x - p2.x ) + sqr( p1.y - p2.y ) );
//             double distance = (p1 - p2).abs();
//			double distance = lattice->orth_distance(p1, p2).abs();
			if( distance < mindist ) {
		//                cout << "distance " << distance << " < " << mindist << endl;
// 				cout << "noNeighbors: nb " << nb << " IS occupied" << endl;
				return false;
			}
		}
// 		cout << "noNeighbors: nb " << nb << " is not occupied" << endl;
	}
//     cout << "Point " << p1 << " does not have any neighbors " << endl;
    return true;
}

//============================================================================
bool InitPoissonDisc::inRectangle(VDOUBLE point, VINT latticeSize){

    if( point.x >= latticeSize.x ) return false;
	else if( point.x < 0.0 ) return false;
    if( point.y >= latticeSize.y ) return false;
	else if( point.y < 0.0 ) return false;
    if( point.z >= latticeSize.z ) return false;
	else if( point.z < 0.0 ) return false;
	return true;
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

//============================================================================
VDOUBLE InitPoissonDisc::generateRandomPointAround(VDOUBLE point, double mindist, double maxdist){

	VDOUBLE newPoint;
    if( !cubic ){ // 2D
		//non-uniform, favours points closer to the inner ring, leads to denser packings
		double r1 = getRandom01(); //random point between 0 and 1
		double r2 = getRandom01();
		//random radius between mindist and 2 * mindist
		double radius = mindist + r1*(maxdist-mindist);
		//double radius = mindist * (r1+1.0);
		//random angle
		double angle = 2.0 * M_PI * r2;
		//the new point is generated around the point (x, y)
		double newX = point.x + radius * cos(angle);
		double newY = point.y + radius * sin(angle);
        newPoint = VDOUBLE( newX, newY, 0 );
	}
	else // 3D
	{
		//non-uniform, favours points closer to the inner ring, leads to denser packings
		double r1 = getRandom01(); //random point between 0 and 1
		double r2 = getRandom01();
		double r3 = getRandom01();
		//random radius between mindist and 2 * mindist
		//double radius = mindist * (r1 + 1.0);
		double radius = mindist + r1*(maxdist-mindist);
		//random angle
		double angle1 = 2.0 * M_PI * r2;
		double angle2 = 2.0 * M_PI * r3;
		//the new point is generated around the point (x, y)
		double newX = point.x + radius * cos(angle1) * sin(angle2);
		double newY = point.y + radius * sin(angle1) * cos(angle2);
		double newZ = point.z + radius * cos(angle2);
		newPoint = VDOUBLE( newX, newY, newZ );
	}
    //cout << point << " --> " << newPoint << ", dist: "<< (point - newPoint).abs() << endl;
	return newPoint;
}

// shared_ptr< Lattice_Data_Layer< double > > InitPoissonDisc::createGrid(shared_ptr<const Lattice> lattice, double default_value){
// 	return shared_ptr< Lattice_Data_Layer< double > >(new Lattice_Data_Layer< double >(lattice, 2, default_value));
// };

shared_ptr< Lattice_Data_Layer< double > > InitPoissonDisc::createGrid( VINT boxsize, Lattice::Structure structure, double default_value){

	// create temporary lattice to hold distance values
	XMLNode xLattice = XMLNode::createXMLTopNode("Lattice");
	xLattice.addChild("Size").addAttribute("value",to_cstr(boxsize));
	XMLNode xLatticeBC = xLattice.addChild("BoundaryConditions");
	XMLNode xLatticeBCC1 = xLatticeBC.addChild("Condition");
	xLatticeBCC1.addAttribute("boundary","x");
	xLatticeBCC1.addAttribute("type","constant");
	XMLNode xLatticeBCC2 = xLatticeBC.addChild("Condition");
	xLatticeBCC2.addAttribute("boundary","-x");
	xLatticeBCC2.addAttribute("type","constant");
	XMLNode xLatticeBCC3 = xLatticeBC.addChild("Condition");
	xLatticeBCC3.addAttribute("boundary","y");
	xLatticeBCC3.addAttribute("type","constant");
	XMLNode xLatticeBCC4 = xLatticeBC.addChild("Condition");
	xLatticeBCC4.addAttribute("boundary","-y");
	xLatticeBCC4.addAttribute("type","constant");
	XMLNode xLatticeBCC5 = xLatticeBC.addChild("Condition");
	xLatticeBCC5.addAttribute("boundary","z");
	xLatticeBCC5.addAttribute("type","constant");
	XMLNode xLatticeBCC6 = xLatticeBC.addChild("Condition");
	xLatticeBCC6.addAttribute("boundary","-z");
	xLatticeBCC6.addAttribute("type","constant");

	shared_ptr<const Lattice> lattice;

	if( structure == Lattice::Structure::linear )
        lattice = shared_ptr<const Lattice>(new Linear_Lattice(xLattice));
	else if( structure == Lattice::Structure::hexagonal)
        lattice = shared_ptr<const Lattice>(new Hex_Lattice(xLattice));
	else if( structure == Lattice::Structure::square)
        lattice = shared_ptr<const Lattice>(new Square_Lattice(xLattice));
	else if( structure == Lattice::Structure::cubic)
        lattice = shared_ptr<const Lattice>(new Cubic_Lattice(xLattice));
    return shared_ptr< Lattice_Data_Layer< double > >(new Lattice_Data_Layer< double >(lattice, 3, default_value));
}

*/
