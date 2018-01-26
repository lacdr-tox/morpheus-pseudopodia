#include "margolus_diffusion.h"

REGISTER_PLUGIN ( MargolusDiffusion );

MargolusDiffusion::MargolusDiffusion() : InstantaneousProcessPlugin( TimeStepListener::XMLSpec::XML_OPTIONAL ) {
	iterations.setXMLPath("iterations");
	iterations.setDefault("1");
	registerPluginParameter(iterations);
	
};

void MargolusDiffusion::init( const Scope* scope ) {
	InstantaneousProcessPlugin::init( scope );
	celltype = scope->getCellType();
	// add "cell.center" as output(!) symbol  (not depend symbol)
	registerCellPositionOutput();

	firstRun = true;
	shifted = false;
	shared_ptr <const Lattice > lattice = SIM::getLattice();

	if (   lattice->getDimensions() != 2 
		|| lattice->getStructure() != Lattice::square 
		|| lattice->get_boundary_type ( Boundary::mx ) != Boundary::periodic 
		|| lattice->get_boundary_type ( Boundary::my ) != Boundary::periodic ) {
		cerr << "Margolus diffusion on applicable to 2D square lattices with periodic boundary conditions" << endl;
		cerr << "Dimensions: " << lattice->getDimensions() << endl;
		cerr << "Structure: "  << lattice->getStructure() << endl;
		cerr << "Boundary x: " << lattice->get_boundary_type ( Boundary::mx ) << endl;
		cerr << "Boundary y: " << lattice->get_boundary_type ( Boundary::my ) << endl;
		exit ( -1 );
	}

	// make Margolus partitioning of lattice:
	// divide lattice into pieces of 2x2
	vector< VINT > partition;
	partition.resize ( 4 ); // each partition contains 4 (= 2x2) cells

	for ( int x=0; x<lattice->size().x; x+=2 ) {
		for ( int y=0; y<lattice->size().y; y+=2 ) {
			partition[0] = ( VINT ( x  , y  ,0 ) );		// origin (bottom left)
			partition[1] = ( VINT ( x  , y+1,0 ) );		// top left
			partition[2] = ( VINT ( x+1, y+1,0 ) );		// top right
			partition[3] = ( VINT ( x+1, y  ,0 ) );		// bottom right
			margolus_partitions_normal.push_back ( partition );
			cout << x << ", " << y << endl;
		}
	}
// 	cout << "Number of coordinates = " << margolus_partitions_normal.size() << endl;

	// create second partitioning that is shifted diagonally by one cell
	for ( int x=1; x<lattice->size().x; x+=2 ) {
		for ( int y=1; y<lattice->size().y; y+=2 ) {
			partition[0] = ( VINT ( x  , y  ,0 ) );		// origin (bottom left)
			partition[1] = ( VINT ( x  , y+1,0 ) );		// top left
			partition[2] = ( VINT ( x+1, y+1,0 ) );		// top right
			partition[3] = ( VINT ( x+1, y  ,0 ) );		// bottom right
			margolus_partitions_shifted.push_back ( partition );
		}
	}
	
};

void MargolusDiffusion::executeTimeStep () {

	if ( firstRun ){
		shared_ptr <const Lattice > lattice = SIM::getLattice();
		if ( celltype->getCellIDs().size() != ( lattice->size().x * lattice->size().y ) ) {
			cerr << "Margolus diffusion is only applicable on CA lattices that contain no empty cells. Population size should be equal to lattice size."<< endl;
			cerr << "Population size = " << celltype->getCellIDs().size() << endl;
			cerr << "Lattice size = " << lattice->size() << endl;
			exit ( -1 );
		}
		firstRun = false;
	}
	
	for ( uint i=0; i<iterations(); i++ ) {
		margolus ();
	}
};

void MargolusDiffusion::margolus () {

//      cout << "Margolus : shifted ? " << (shifted?"yes":"no") << endl;

	for ( uint i=0; i < margolus_partitions_normal.size(); i++ ) {
		vector<CPM::STATE> nodes;
		if ( !shifted )
			nodes = getNodes ( margolus_partitions_normal[i] );
		else
			nodes = getNodes ( margolus_partitions_shifted[i] );

		if ( getRandomBool() ) { // rotate 90 degrees clockwise
			CPM::setNode ( VINT ( nodes[0].pos.x  , nodes[0].pos.y+1, nodes[0].pos.z ), nodes[0].cell_id );
			CPM::setNode ( VINT ( nodes[1].pos.x+1, nodes[1].pos.y  , nodes[1].pos.z ), nodes[1].cell_id );
			CPM::setNode ( VINT ( nodes[2].pos.x  , nodes[2].pos.y-1, nodes[2].pos.z ), nodes[2].cell_id );
			CPM::setNode ( VINT ( nodes[3].pos.x-1, nodes[3].pos.y  , nodes[3].pos.z ), nodes[3].cell_id );
		} else { // rotate 90 degrees counter-clockwise
			CPM::setNode ( VINT ( nodes[0].pos.x+1, nodes[0].pos.y  , nodes[0].pos.z ), nodes[0].cell_id );
			CPM::setNode ( VINT ( nodes[1].pos.x  , nodes[1].pos.y-1, nodes[1].pos.z ), nodes[1].cell_id );
			CPM::setNode ( VINT ( nodes[2].pos.x-1, nodes[2].pos.y  , nodes[2].pos.z ), nodes[2].cell_id );
			CPM::setNode ( VINT ( nodes[3].pos.x  , nodes[3].pos.y+1, nodes[3].pos.z ), nodes[3].cell_id );
		}

	}

// alternate the normal partitioning and the diagonally shifted partitioning
	shifted = ( shifted?false:true );
};

vector<CPM::STATE> MargolusDiffusion::getNodes ( vector<VINT> cells ) {

    vector<CPM::STATE> nodes;
    for ( uint i=0; i<cells.size(); i++ ) {
        CPM::STATE s;
        s.cell_id = CPM::getNode ( cells[i] ).cell_id;
        s.pos 	  = CPM::getNode ( cells[i] ).pos;
        nodes.push_back ( s );
    }
    return nodes;
}

