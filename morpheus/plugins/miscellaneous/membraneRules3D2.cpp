#include "membraneRules3D2.h"
#include <core/membranemapper.h>

const double MembraneRules3D::no_distance = 999999;

REGISTER_PLUGIN ( MembraneRules3D );


// TimeStepListener::ScheduleType::type MembraneRules3D::type() {
// 	return ;
// };

MembraneRules3D::MembraneRules3D(): TimeStepListener(TimeStepListener::ScheduleType::REPORTER, TimeStepFlags::CAN_BE_ADJUSTED)
{}

void MembraneRules3D::loadFromXML ( const XMLNode xNode ) {
	TimeStepListener::loadFromXML ( xNode );
	
	int max_tolerance = 3;
	
// 	<MembraneRules3D>
// 		<Input basal_celltype="[cellType]" apical_symbol="[symbol]" />
// 		<PositionInformation mode="[cytosolic/membrane]" shape="[linear/exponential]" />
// 		<IntercellularCommunication mode="[none/geometric_mean]" />
// 		<Thresholding />
// 			<Global threshold="[double]" />
// 			<Local threshold="[double]" max_distance="[double]" weighted="[boolean]" />
// 		</Thresholding>
// 		<Analysis>
// 			<Cells tolerance="[double]" writeTIFF="[boolean]"  />
// 			<Tissue tolerance="[double]" writeTIFF="[boolean]" border="[double]" />
// 		</Analysis>
// 		<Output basal="[symbol]" apical_predicted="[symbol]" apical_segmented="[symbol]" posinfo="[symbol]" potential="[symbol]" lateral="[symbol]"/>
// 	<MembraneRules3D>
	
	const XMLNode xInput = xNode.getChildNode("Input");
	getXMLAttribute( xInput, "basal_celltype", input.basal_celltype_str);
	getXMLAttribute( xInput, "apical_symbol", input.apical_str);
	
	const XMLNode xPosInfo = xNode.getChildNode("PositionInformation");
	string mode_str, method_str;
	getXMLAttribute( xPosInfo, "mode", mode_str);
	if ( lower_case( mode_str ) == "cytosolic" )
		posinfo.mode = PositionInformation::CYTOSOL;
	else
		posinfo.mode = PositionInformation::MEMBRANE;
	cout << "posinfo.mode : " << posinfo.mode << endl;
	
	getXMLAttribute( xPosInfo, "shape", method_str);
	if ( lower_case( method_str ) == "linear" )
		posinfo.shape =PositionInformation::LINEAR;
	else
		posinfo.shape =PositionInformation::EXPONENTIAL;
	getXMLAttribute( xPosInfo, "exponent", posinfo.exponent);
	
	const XMLNode xComm = xNode.getChildNode("IntercellularCommunication");
	getXMLAttribute( xComm, "mode", mode_str);
	if ( lower_case( mode_str ) == "none" )
		communication.mode = IntercellularCommunication::NONE;
	else
		communication.mode = IntercellularCommunication::GEOMETRIC_MEAN;
	
	const XMLNode xThreshold = xNode.getChildNode("Thresholding");
	if( xThreshold.nChildNode("Global") > 0 ){
		thresholding.mode = Thresholding::GLOBAL;
		getXMLAttribute( xThreshold, "Global/threshold", thresholding.threshold);
		cout << "Global thresholding: threshold: " << thresholding.threshold << " micron\n";
	}
	else if( xThreshold.nChildNode("Local") > 0 ){
		thresholding.mode = Thresholding::LOCAL;
		getXMLAttribute( xThreshold, "Local/threshold", thresholding.threshold);
		getXMLAttribute( xThreshold, "Local/max_distance", thresholding.max_distance);
		getXMLAttribute( xThreshold, "Local/local_distance", thresholding.local_distance);
		cout << "Local thresholding: max_distance: " << thresholding.max_distance << " micron\n";
		getXMLAttribute( xThreshold, "Local/weighted", thresholding.weighted);
	}

	const XMLNode xPruning = xNode.getChildNode("Pruning");
	pruning.unilateral	= false;
	pruning.isolated	= false;
	getXMLAttribute( xPruning, "unilateral_domains", pruning.unilateral);
	getXMLAttribute( xPruning, "isolated_domains", pruning.isolated);
	cout << "Pruning/unilateral_domains ? " << (pruning.unilateral ? "true":"false") << endl;
	cout << "Pruning/isolated_domains ? " << (pruning.isolated ? "true":"false") << endl;
	

	const XMLNode xAnalysis = xNode.getChildNode("Analysis");
	if( xAnalysis.nChildNode("Cells") > 0 ){
		//getXMLAttribute( xAnalysis, "Cells/tolerance", analysisCells.tolerance);
		getXMLAttribute( xAnalysis, "Cells/writeTIFF", analysisCells.writeTIFF);
		getXMLAttribute( xAnalysis, "Cells/writeMembraneMaps", analysisCells.writeMembraneMaps);
		//cout << "Cells: Tolerance: " << analysisCells.tolerance << " micron\n";
		for(int i=0; i<=max_tolerance; i++){
			analysisCells.tolerances.push_back( double(i) );
		}
	}
	if( xAnalysis.nChildNode("Tissue") > 0 ){
		//getXMLAttribute( xAnalysis, "Tissue/tolerance", analysisTissue.tolerance);
		getXMLAttribute( xAnalysis, "Tissue/writeTIFF", analysisTissue.writeTIFF);
		double border_in_microns;
		getXMLAttribute( xAnalysis, "Tissue/border", border_in_microns);
		//cout << "Tissue: Tolerance: " << analysisTissue.tolerance << " micron\n";
		analysisTissue.border = round( border_in_microns / (SIM::getNodeLength() * 1e6) ); // conversion from microns to pixels
		cout << "Tissue: Border: " << border_in_microns << " microns = " << analysisTissue.border << " voxels \n";
		
		for(int i=0; i<=max_tolerance; i++){
			analysisTissue.tolerances.push_back( double(i) );
		}
	}

	
	const XMLNode xOutput = xNode.getChildNode("Output");
	output_symbol_strs.resize(6);
	getXMLAttribute ( xOutput, "basal", output_symbol_strs[0]);
	getXMLAttribute ( xOutput, "lateral", output_symbol_strs[1]);
	getXMLAttribute ( xOutput, "potential", output_symbol_strs[3]);
	getXMLAttribute ( xOutput, "distance", output_symbol_strs[2]);
	getXMLAttribute ( xOutput, "apical_predicted", output_symbol_strs[4]);
	getXMLAttribute ( xOutput, "apical_segmented", output_symbol_strs[5]);
	
	
	/*
	===============================================

	if ( lower_case( mode_str ) == "none" )
		communication.mode = communication.mode::NONE;
	else
		communication.mode = communication.mode::GEOMETRIC_MEAN;
	
	
	// basal_celltype
    getXMLAttribute ( xNode, "basal_celltype", basal_celltype_str);
	
	// apical (stored in PDE)
	getXMLAttribute ( xNode, "apical_pde", apical_str);
	
	// distance method: cytosolic or membrane
	string distance_mode_str = "cytosolic";
    getXMLAttribute ( xNode, "distance_mode", distance_mode_str); // membrane or cytosol
	// Mode: cytosol or membrane
	if ( lower_case( distance_mode_str ) == "membrane" )
		distance_mode = MembraneRules3D::MEMBRANE;
	else if ( lower_case( distance_mode_str ) == "cytosol" )
		distance_mode = MembraneRules3D::CYTOSOL;
	else if ( lower_case( distance_mode_str ) == "none" )
		distance_mode = MembraneRules3D::NONE;
	else
		distance_mode = MembraneRules3D::NONE;
	

	string communication_mode_str = "cytosolic";
    getXMLAttribute ( xNode, "communication_mode", communication_mode_str); // membrane or cytosol
	// Mode: cytosol or membrane
	if ( lower_case( communication_mode_str ) == "cell_autonomous" )
		communication_mode = MembraneRules3D::CELLAUTONOMOUS;
	else
		communication_mode = MembraneRules3D::COMMUNICATION;
	
	// local thresholding
    getXMLAttribute ( xNode, "max_distance", max_distance);
    getXMLAttribute ( xNode, "threshold", threshold);
		
	
	// output to membraneProperty
	output_symbol_strs.resize(6);
	getXMLAttribute ( xNode, "Output_basal/symbol-ref", output_symbol_strs[0]);
	getXMLAttribute ( xNode, "Output_lateral/symbol-ref", output_symbol_strs[1]);
	getXMLAttribute ( xNode, "Output_distance/symbol-ref", output_symbol_strs[2]);
	getXMLAttribute ( xNode, "Output_potential/symbol-ref", output_symbol_strs[3]);
	getXMLAttribute ( xNode, "Output_apical/symbol-ref", output_symbol_strs[4]);
	getXMLAttribute ( xNode, "Output_apical_segmented/symbol-ref", output_symbol_strs[5]);
*/
	
}

void MembraneRules3D::init ( CellType* ct ) {
	TimeStepListener::init();
	celltype = ct;
	//boxnumber = 0;
	cpm_lattice		= CPM::getLayer();
	lattice3D		= SIM::getLattice();
	latticeSize		= lattice3D->size();
	neighbor_sites_halo	= lattice3D->getNeighborhood( 3 ); // used for getHALO
	
	// find celltypes and symbols
	input.basal_celltype = CPM::findCellType(input.basal_celltype_str);

	// find PDE layer containing apical domain
	input.apical_pde = SIM::findPDELayer(input.apical_str);

	// set length, area and volume of a node in physical units (microns)
	node.length 	= SIM::getNodeLength() * 1e6;
	node.area 		= sqr(SIM::getNodeLength() * 1e6);
	node.volume 	= pow(SIM::getNodeLength() * 1e6, 3.0);
	
	// precompute distances of the neighbors within the cubic lattice
	neighbors = lattice3D->getNeighborhoodByOrder( 3 ); // the larger this neighborhood, the more accurate the distance transform
	neighbor_distance.resize( neighbors.size() );
	for (uint i=0; i<neighbors.size(); i++) {
		neighbor_distance[i] = VDOUBLE(neighbors[i]).abs() * node.length; // in micron length units (= 10^-6 meter)
		cout << "Node Length = " <<  SIM::getNodeLength() << ", neighbor_distance[" << i << "] = " << neighbor_distance[i] << "\n";
	}
	
	output.cellids		= createBoxDouble( latticeSize, 0.0 );
	output.basal		= createBoxBoolean( latticeSize, false );
	output.lateral		= createBoxDouble( latticeSize, 0.0 );
	output.posinfo		= createBoxDouble( latticeSize, 0.0 );
	output.potential	= createBoxDouble( latticeSize, 0.0 );
	output.apical 		= createBoxBoolean( latticeSize, false );
	//output.apical 		= createBoxDouble( latticeSize, 0.0 );
	output.apical_segmented	= createBoxBoolean( latticeSize, false );
	
	// output to membraneProperty
	output_membranes.resize( output_symbol_strs.size() );
	for(uint i=0; i<output_symbol_strs.size(); i++){
		cout << "Output symbol: " << output_symbol_strs[i] << "..." << endl;
		output_membranes[i] = ct->findMembrane ( output_symbol_strs[i], true );
	}
	
	// generate kernel for gasussian filter
	//double sigma = 1.0;
	//createFilter(gKernel, sigma);
	
}

void MembraneRules3D::computeTimeStep(){
	TimeStepListener::computeTimeStep();
}

void MembraneRules3D::executeTimeStep(){
	TimeStepListener::executeTimeStep();
	vector<CPM::CELL_ID> cells = celltype -> getCellIDs();
	
	//population_data.resize( cells.size() );

////////////////////////////////
//  
//	Initialization
//
////////////////////////////////
	
	population_data.resize( cells.size() );

//#pragma omp parallel for // createBox generates problems when multithreading
	for(uint c=0; c < cells.size(); c++) {
		CellData cd;

		cd.cell_id = cells[c];
		// create bounding box for each cell
		cd.bbox = getBoundingBoxCell( cells[c] );
		//cout << "cell: " << cd.cell_id << "\tbox: " <<  boxnumber << endl;
		cd.box  = createBoxDouble( cd.bbox.size, 0.0 );
		//cd.box2 = createBoxDouble( cd.bbox.size, 0.0 );
		cd.box3 = createBoxBoolean( cd.bbox.size, false );
		cd.boundarycell = 0;
		
		// initialize membrane and position data
		const Cell::Nodes& membrane = CPM::getCell ( cells[c] ).getSurface();
		for ( Cell::Nodes::const_iterator m = membrane.begin(); m != membrane.end(); ++m ) {
			MemPosData pd;
			pd.apical_predicted 		= false;
			pd.apical_segmented = false;
			pd.basal 		= false;
			pd.distance 	= no_distance;
			pd.isolated_lateral = false;
			pd.lateral		= 0;
			//pd.halo 		= getHalo( *m, cells[c] );
			cd.membrane_data.insert( pair< VINT, MemPosData >(*m, pd) ); 
			// membrane mask
			cd.box3->set( *m - cd.bbox.minimum, true );
			
			VINT m_l = *m;
			//if (m_l.x == 72) cout << m_l.x << endl;
			if (m_l.x < cd.bbox.minimum.x || m_l.y < cd.bbox.minimum.y || m_l.z < cd.bbox.minimum.z ||  m_l.x >= cd.bbox.maximum.x||  m_l.y >= cd.bbox.maximum.y||  m_l.z >= cd.bbox.maximum.z) {
				cout << m_l << "\tcd.bbox.minimum: " << cd.bbox.minimum << "\tcd.bbox.maximum: " << cd.bbox.maximum << endl;
				assert(0);
			}
			
			if (m_l.x == 1 || m_l.y == 1 || m_l.z == 1 || 
				m_l.x == latticeSize.x ||  m_l.y == latticeSize.y ||  m_l.z == latticeSize.z ) {
				cd.boundarycell++;
			}
			
		}
		
		const Cell::Nodes& cell_nodes = CPM::getCell ( cells[c] ).getNodes();
		for ( Cell::Nodes::const_iterator n = cell_nodes.begin(); n != cell_nodes.end(); ++n ) {
			if( !CPM::isBoundary( *n ) ) // exclude membrane nodes
				cd.box->set( *n - cd.bbox.minimum, cd.cell_id);
		}
		// channel 1: cell ID (and cell volume)
		if( analysisCells.writeTIFF && cd.boundarycell == 0 )
			writeTIFF( "multichannel", cd.box, cd.cell_id );
		
		if( analysisCells.writeTIFF && cd.boundarycell == 0  )
			writeTIFF("0_cellID", cd.box, cd.cell_id );

		cd.box->data=0;
		
		population_data[ c ] = cd;
	}
	
////////////////////////////////
//  
//	Registration of apical contacts (stored in PDE layer!)
//
////////////////////////////////
	
	
	cout << "Registering apical contacts" << endl;
#pragma omp parallel for
	for(uint c=0; c<population_data.size(); c++){
		CellData& cd = population_data[c];
		
		for ( map< VINT, MemPosData >::iterator i=cd.membrane_data.begin(); i != cd.membrane_data.end(); ++i ) {
			// count the halo nodes that are of apical celltype
			Cell::Nodes halo = getHalo( i->first, cd.cell_id);
			double apical_contacts = 0;
			for ( Cell::Nodes::const_iterator j= halo.begin(); j != halo.end(); ++j ) {
				if( input.apical_pde->get( *j ) ) 
					apical_contacts++;
			}
			// set mempos=apical if half or more of the halo nodes are of basal celltype
			bool apical = (apical_contacts > 0 ? true : false); //( (apical_contacts / i->second.halo.size()) > 0.0 );
			i->second.apical_segmented = apical;
				
			cd.box->set( i->first - cd.bbox.minimum, (apical ? 1 : 0));
			output.apical_segmented->set( i->first, apical);
		
			VINT m_l = i->first;
			//if (m_l.x == 72) cout << m_l.x << endl;
			if (m_l.x < cd.bbox.minimum.x || m_l.y < cd.bbox.minimum.y || m_l.z < cd.bbox.minimum.z ||  m_l.x >= cd.bbox.maximum.x||  m_l.y >= cd.bbox.maximum.y||  m_l.z >= cd.bbox.maximum.z) {
				cout << m_l << "\tcd.bbox.minimum: " << cd.bbox.minimum << "\tcd.bbox.maximum: " << cd.bbox.maximum << endl;
				assert(0);
			}
		}
		// channel 2: apical segmented
		if( analysisCells.writeTIFF && cd.boundarycell == 0 )
			writeTIFF( "multichannel", cd.box, cd.cell_id );
		cd.box->data=0;
	}
	
////////////////////////////////
//  
//	Registration of basal contacts
//
////////////////////////////////
	
	
	cout << "Registering basal contacts" << endl;
#pragma omp parallel for
	for(uint c=0; c<population_data.size(); c++){
		CellData& cd = population_data[c];
		
		CPM::CELL_ID cell_id = cd.cell_id;
		
		for ( map< VINT, MemPosData >::iterator i=cd.membrane_data.begin(); i != cd.membrane_data.end(); ++i ) {
			// count the halo nodes that are of basal celltype
			Cell::Nodes halo = getHalo( i->first, cell_id);
			double basal_contacts = 0;
			for ( Cell::Nodes::const_iterator j= halo.begin(); j != halo.end(); ++j ) {
				if( 	CPM::cellExists(  cpm_lattice->get(*j).cell_id) 
					&& (CPM::getCellIndex(cpm_lattice->get(*j).cell_id).celltype == input.basal_celltype->getID() ) )
					basal_contacts++;
			}
			// set mempos=basal if half or more of the halo nodes are of basal celltype
			bool basal = ( (basal_contacts / halo.size()) > 0.0 );
			i->second.basal = basal;
				
			cd.box->set( i->first - cd.bbox.minimum, (basal ? 1 : 0));
			output.basal->set( i->first, basal);
		
			VINT m_l = i->first;
			//if (m_l.x == 72) cout << m_l.x << endl;
			if (m_l.x < cd.bbox.minimum.x || m_l.y < cd.bbox.minimum.y || m_l.z < cd.bbox.minimum.z ||  m_l.x >= cd.bbox.maximum.x||  m_l.y >= cd.bbox.maximum.y||  m_l.z >= cd.bbox.maximum.z) {
				cout << m_l << "\tcd.bbox.minimum: " << cd.bbox.minimum << "\tcd.bbox.maximum: " << cd.bbox.maximum << endl;
				assert(0);
			}
		}
		// channel 3: basal contacts
		if( analysisCells.writeTIFF && cd.boundarycell == 0 )
			writeTIFF( "multichannel", cd.box, cell_id );
		
		if( analysisCells.writeTIFF && cd.boundarycell == 0  )
			writeTIFF("1_basal", cd.box, cd.cell_id );

		cd.box->data=0;
	}
	
////////////////////////////////
//  
//	Registration of lateral contacts (for visualization and pruning of isolated domains!)
//
////////////////////////////////
	
	cout << "Registering lateral contacts (for visualization and pruning) "<< endl;
	
	/// matrix of lateral cell contacts
	int num_of_col = population_data.size();
	int num_of_row = population_data.size();
	vector< vector<int> > contact_matrix; // records contact area in pixels
	contact_matrix.resize( num_of_col , vector<int>( num_of_row , 0 ) );
	map< CPM::CELL_ID, int > id_to_index;
	map< int, CPM::CELL_ID > index_to_id;
	for(uint c=0; c<population_data.size(); c++){
		id_to_index[ population_data[c].cell_id ] = c;
		index_to_id[ c ] = population_data[c].cell_id;
	}
	
#pragma omp parallel for
	for(uint c=0; c<population_data.size(); c++){
		CellData& cd = population_data[c];
		CPM::CELL_ID cell_id = cd.cell_id;
		
		// for each point on the membrane
		for ( map< VINT, MemPosData >::iterator i=cd.membrane_data.begin(); i != cd.membrane_data.end(); ++i ) {
			Cell::Nodes halo = getHalo( i->first, cell_id);
			int lateral_id = 0;
			double min_dist = no_distance;
			// for each point in the halo
			for ( Cell::Nodes::const_iterator j= halo.begin(); j != halo.end(); ++j ) {
				
				CPM::CELL_ID node_cell_id = cpm_lattice->get(*j).cell_id;
				double node_dist = lattice3D->orth_distance( VDOUBLE(i->first), VDOUBLE(*j) ).abs();
				
				if( node_cell_id != cell_id 			// not self
					&& CPM::cellExists( node_cell_id ) 	// cell exists
					&& (CPM::getCellIndex( node_cell_id ).celltype == celltype->getID() ) // is of same cell type 
					&& node_dist < min_dist ) {	// closer as the previous neighbor

					// set lateral to ID of closest cell discovered in halo (0 otherwise)
					lateral_id = node_cell_id;
					min_dist = node_dist;
				}	
			}
			
			// if not registered as basal, make it lateral (mutual exclusive)
			if( !i->second.basal )
				i->second.lateral = lateral_id;
			
			if( lateral_id != 0 )
				contact_matrix[ c ][ id_to_index[ lateral_id ]]++;
			
			i->second.lateral = lateral_id;
			cd.neighbors.insert( lateral_id );

				
			cd.box->set( i->first - cd.bbox.minimum, lateral_id);
			output.lateral->set( i->first, lateral_id);
		
			VINT m_l = i->first;
			//if (m_l.x == 72) cout << m_l.x << endl;
			if (m_l.x < cd.bbox.minimum.x || m_l.y < cd.bbox.minimum.y || m_l.z < cd.bbox.minimum.z ||  m_l.x >= cd.bbox.maximum.x||  m_l.y >= cd.bbox.maximum.y||  m_l.z >= cd.bbox.maximum.z) {
				cout << m_l << "\tcd.bbox.minimum: " << cd.bbox.minimum << "\tcd.bbox.maximum: " << cd.bbox.maximum << endl;
				assert(0);
			}
		}
		// channel 4: lateral contacts (and cell ids of neighboring cells)
		if( analysisCells.writeTIFF && cd.boundarycell == 0 )
			writeTIFF( "multichannel", cd.box, cell_id );

		if( analysisCells.writeTIFF && cd.boundarycell == 0 )
			writeTIFF( "2_lateral", cd.box, cell_id );
		cd.box->data=0;
	}

	// write contact matrix to file
	ofstream foutmat1;
	foutmat1.open("contact_matrix_id_idx.log", ios::out);
	for(uint c=0; c<population_data.size(); c++){
		foutmat1 << population_data[c].cell_id<< "\t" << c << "\n";
	}
	foutmat1.close();
	ofstream foutmat;
	foutmat.open("contact_matrix.log", ios::out);
	for(uint i=0; i < contact_matrix.size(); i++){
		for(uint j=0; j < contact_matrix.size(); j++){
			foutmat << contact_matrix[i][j] * node.area << "\t";
		}
		foutmat << "\n";
	}
	foutmat.close();
	
////////////////////////////////
//  
// Distance transform from basal contacts (3D Euclidean Distance Transform)
//
////////////////////////////////
	
	cout << "Computing distance from basal (positional information)" << endl;
#pragma omp parallel for
	for(uint i=0; i<population_data.size(); i++){
		CellData& cd = population_data[i];
		VINT origin = cd.bbox.minimum;

		shared_ptr< Lattice_Data_Layer<double> > distanceMap;
		shared_ptr< Lattice_Data_Layer<bool> > maskMap;
		
#pragma omp critical
{		
		// create temporary data containers
		distanceMap	= createBoxDouble( cd.bbox.size, no_distance);
		maskMap 	= createBoxBoolean( cd.bbox.size, false);
}

		switch( posinfo.mode ){
			case PositionInformation::CYTOSOL:
			{
				//cout << "PositionInformation::CYTOSOL \n";
				const Cell::Nodes& cytosol = CPM::getCell ( cd.cell_id ).getNodes();
				for ( Cell::Nodes::const_iterator n = cytosol.begin(); n != cytosol.end(); ++n )
					maskMap->set( *n - origin, true );
				break;
			}
			case PositionInformation::MEMBRANE:
			{
				//cout << "PositionInformation::MEMBRANE \n";
				const Cell::Nodes& membrane = CPM::getCell ( cd.cell_id ).getSurface();
				for ( Cell::Nodes::const_iterator n = membrane.begin(); n != membrane.end(); ++n ) 
					maskMap->set( *n - origin, true );
				break;
			}
			default:{
				cerr << "MembraneRules3D: Unknown mode: select 'cytosolic' or 'membrane'." << endl;
				exit(-1);
				break;
			}
		}
		
		// initialize distances: all to no_distance, contacting nodes to 0.
		for ( map< VINT, MemPosData >::iterator i=cd.membrane_data.begin(); i != cd.membrane_data.end(); ++i ) {
			if( i->second.basal ){
				distanceMap->set( i->first - origin, 0.0);
			}
		}
 		//distanceMap->reset_boundaries();
		
		//////  COMPUTE DISTANCES  //////
		
		euclideanDistanceTransform( distanceMap, maskMap );
		
		double distance_maximum=0;
		for ( map< VINT, MemPosData >::iterator it=cd.membrane_data.begin(); it != cd.membrane_data.end(); ++it ){
			if( distanceMap->get( it->first - origin ) > distance_maximum && distanceMap->get( it->first - origin ) != no_distance )
				distance_maximum = distanceMap->get( it->first - origin );
		}
		
		//cout << "CELL: " << cd.cell_id << ", max dist = " << distance_maximum << "\n";
		
		//write map to data containers
		for ( map< VINT, MemPosData >::iterator it=cd.membrane_data.begin(); it != cd.membrane_data.end(); ++it ) {
			if( posinfo.shape == PositionInformation::LINEAR ){
				it->second.distance = distanceMap->get( it->first - origin ); 
				//it->second.distance_norm = pow(distanceMap->get( it->first - origin ),2.0) / pow(distance_maximum,1.0);
				cd.box->set( it->first - cd.bbox.minimum, it->second.distance );
				output.posinfo->set( it->first, distanceMap->get( it->first - origin ) );
			}
			if( posinfo.shape == PositionInformation::EXPONENTIAL ){
				double distance_exp = pow( distanceMap->get( it->first - origin ), posinfo.exponent );
				it->second.distance = distance_exp;
				cd.box->set( it->first - cd.bbox.minimum, it->second.distance );
				output.posinfo->set( it->first, distance_exp );
			}
			
			VINT m_l = it->first;
			if (m_l.x < cd.bbox.minimum.x || m_l.y < cd.bbox.minimum.y || m_l.z < cd.bbox.minimum.z ||  m_l.x >= cd.bbox.maximum.x||  m_l.y >= cd.bbox.maximum.y||  m_l.z >= cd.bbox.maximum.z) {
				cout << m_l << "\tcd.bbox.minimum: " << cd.bbox.minimum << "\tcd.bbox.maximum: " << cd.bbox.maximum << endl;
				assert(0);
			}
		}
			
		if( analysisCells.writeTIFF && cd.boundarycell == 0  )
			writeTIFF("3_distance", cd.box, cd.cell_id );
	}

////////////////////////////////
//  
// 		Intercellular communication (geometric mean)
//
////////////////////////////////

	if( communication.mode != IntercellularCommunication::NONE ){
		cout << "Calculating intercellular potential "<< endl;
		
#pragma omp parallel for
		for(uint i=0; i<population_data.size(); i++){
			CellData& cd = population_data[i];
			CPM::CELL_ID cell_id = cd.cell_id;
			
			
			// for every membrane node, get average of distance values in adjacent cell
			for ( map< VINT, MemPosData >::iterator it=cd.membrane_data.begin(); it != cd.membrane_data.end(); ++it ) {
				
				double nb_distance = 0.0;
				uint nb_count=0;
				Cell::Nodes halo = getHalo( it->first, cell_id);
				double mindist = no_distance;
				for ( Cell::Nodes::const_iterator n = halo.begin(); n != halo.end(); ++n ){
					
					// get cell id of adjacent cell 
					CPM::CELL_ID nb_cell_id = cpm_lattice->get( *n ).cell_id;
					
					// check if adjacent cell is of my own cell type 
					uint nb_celltype_id = CPM::getCell( nb_cell_id ).getCellType()->getID();
					if ( nb_celltype_id  ==  celltype->getID() ) {
						if (getCellData( nb_cell_id ).membrane_data.count(*n)) {
							
							double node_dist = lattice3D->orth_distance( VDOUBLE(it->first), VDOUBLE(*n) ).abs();
							if( node_dist < mindist ){
								nb_distance = getCellData( nb_cell_id ).membrane_data[*n].distance;
								mindist = node_dist;
							}
						}
					}
				}

				// NOTE: no averaging over halo node anymore, since we only the nb_distance is the distance at the closest node only!
				// This makes the comparison between with/without intercellular communication easier as we cancel out effects of local averaging
				
// 				// take geometric mean over own distance and neighbors distance
				double geom_mean = sqrt( it->second.distance * nb_distance );
				it->second.potential = geom_mean;
				if( std::isnan( it->second.potential ) )
					cout << "POTENTIAL is NaN! " << it->first << "\n";
				
// 				double max_potential = max( it->second.distance,  nb_distance );
// 				it->second.potential = max_potential;

				VINT m_l = it->first;
				//if (m_l.x == 72) cout << m_l.x << endl;
				if (m_l.x < cd.bbox.minimum.x || m_l.y < cd.bbox.minimum.y || m_l.z < cd.bbox.minimum.z ||  m_l.x >= cd.bbox.maximum.x||  m_l.y >= cd.bbox.maximum.y||  m_l.z >= cd.bbox.maximum.z) {
					cout << m_l << "\tcd.bbox.minimum: " << cd.bbox.minimum << "\tcd.bbox.maximum: " << cd.bbox.maximum << endl;
					assert(0);
				}
			}
			
			// copy data to maps
			cd.box->data = 0.0;
			for ( map< VINT, MemPosData >::iterator it=cd.membrane_data.begin(); it != cd.membrane_data.end(); ++it ) {
				//cout << it->first << "\t" << it->second.potential << endl;
				cd.box->set( it->first - cd.bbox.minimum, it->second.potential );
				output.potential->set( it->first, it->second.potential);
			}
			
			if( analysisCells.writeTIFF && cd.boundarycell == 0 )
				writeTIFF("4_potential", cd.box, cell_id );
			cd.box->data=0;
		}
	}	

////////////////////////////////
//  
// 		Global thresholding
//
////////////////////////////////

if( thresholding.mode == Thresholding::GLOBAL ){
	cout << "Global thresholding" << endl;

#pragma omp parallel for
	for(uint i=0; i<population_data.size(); i++){
		CellData& cd = population_data[i];
		CPM::CELL_ID cell_id = cd.cell_id;
		for ( map< VINT, MemPosData >::iterator it=cd.membrane_data.begin(); it != cd.membrane_data.end(); it++ ) {
			
			switch( communication.mode ){
				case IntercellularCommunication::NONE: // cell autonomous
				{
					if( it->second.distance > thresholding.threshold )
						it->second.apical_predicted = it->second.distance;
					break;
				}
				case IntercellularCommunication::GEOMETRIC_MEAN: // with communication, take potential
				{
					if( it->second.potential > thresholding.threshold )
						it->second.apical_predicted = it->second.potential;
					break;
				}
				default:
					cerr << "Unknown communication mode" << endl;
					break;
			}
			output.apical->set( it->first, it->second.apical_predicted);
		}
		
		if( analysisCells.writeTIFF && cd.boundarycell == 0 )
			writeTIFF( "apical_beforepruning", cd.box, cell_id );
		cd.box->data=0;
	} 
}

////////////////////////////////
//  
// 		Local thresholding
//
////////////////////////////////

else if( thresholding.mode == Thresholding::LOCAL ){
	
		// initialize a number of computeLattices, equal to the number of threads
		int numthreads = 1;
		
	uint count_cell=0;
#pragma omp parallel for schedule(dynamic)
		for(uint i=0; i<population_data.size(); i++){
			CellData& cd = population_data[i];
			CPM::CELL_ID cell_id = cd.cell_id;
			
			cout << "Local thresholding for cell " << cell_id << endl;
			VINT origin = cd.bbox.minimum;

			int thread = omp_get_thread_num();
			struct timeval t1, t2; double elapsedTime;
			gettimeofday( &t1, NULL);
			
			// set mask to membrane nodes
			cd.box3->data=false;
			for ( map< VINT, MemPosData >::iterator it=cd.membrane_data.begin(); it != cd.membrane_data.end(); it++ ) {
				cd.box3->set(it->first - origin , true);
			}

			// for every membrane node
			bool apical = false;
			//double apical = 0.0;
			
			uint total = cd.membrane_data.size();
			for ( map< VINT, MemPosData >::iterator it=cd.membrane_data.begin(); it != cd.membrane_data.end(); it++ ) {
				
				VINT m_l =  it->first;
				if (m_l.x < cd.bbox.minimum.x || m_l.y < cd.bbox.minimum.y || m_l.z < cd.bbox.minimum.z ||  m_l.x >= cd.bbox.maximum.x||  m_l.y >= cd.bbox.maximum.y||  m_l.z >= cd.bbox.maximum.z) {
					cout << m_l << "\tcd.bbox.minimum: " << cd.bbox.minimum << "\tcd.bbox.maximum: " << cd.bbox.maximum << endl;
					assert(0);
				}
				apical = localThresholding( it->first, cd);
				it->second.apical_predicted = apical;
				output.apical->set( it->first, apical);

			}
			gettimeofday( &t2, NULL);
			elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
			elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms
			
			
			
			// copy data to box
			cd.box->data = 0.0;
			for ( map< VINT, MemPosData >::iterator it=cd.membrane_data.begin(); it != cd.membrane_data.end(); ++it ) {
				cd.box->set( it->first - cd.bbox.minimum, it->second.apical_predicted );
			}

			if( analysisCells.writeTIFF && cd.boundarycell == 0 )
				writeTIFF( "5_local_threshold", cd.box, cell_id );
			cd.box->data=0;
			 
			cout <<"Cell no. " << ++count_cell << " / " << population_data.size() << ", " << 100.0 * double(count_cell)/double(population_data.size()) << "%\tcell id: " << cell_id << "\tmemsize: "<< total << "\texectime: " << elapsedTime << " ms." << endl;
		}
}		

////////////////////////////////
//  
// 		Pruning (remove apical if adjacent cell does not have apical at same position)
//
////////////////////////////////

	if( pruning.unilateral )
	{
		cout << "Pruning unilateral apical domains" << endl;
		
#pragma omp parallel for
		for(uint i=0; i<population_data.size(); i++){
			CellData& cd = population_data[i];
			CPM::CELL_ID cell_id = cd.cell_id;
			
			// for every membrane node, get average of distance values in adjacent cell
			for ( map< VINT, MemPosData >::iterator it=cd.membrane_data.begin(); it != cd.membrane_data.end(); ++it ) {
				
				if( it->second.apical_predicted ){ 
					uint nb_apical = 0;
					Cell::Nodes halo = getHalo( it->first, cell_id);
					for ( Cell::Nodes::const_iterator n = halo.begin(); n != halo.end(); ++n ){
						
						// get cell id of adjacent cell 
						CPM::CELL_ID nb_cell_id = cpm_lattice->get( *n ).cell_id;
						
						// check if adjacent cell is of my own cell type 
						uint nb_celltype_id = CPM::getCell( nb_cell_id ).getCellType()->getID();
						if ( nb_celltype_id  ==  celltype->getID() ) {
							if (getCellData( nb_cell_id ).membrane_data.count(*n)) {
								nb_apical += (getCellData( nb_cell_id ).membrane_data[*n].apical_predicted ? 1 : 0);
							}
						}
					}
					
					// only keep apical if there adjacent membrane has at least one apical voxel
					it->second.apical_predicted = (nb_apical > 0);
					
				}
			}
			
			// copy data to maps
			cd.box->data = 0.0;
			for ( map< VINT, MemPosData >::iterator it=cd.membrane_data.begin(); it != cd.membrane_data.end(); ++it ) {
				//cout << it->first << "\t" << it->second.potential << endl;
				cd.box->set( it->first - cd.bbox.minimum, it->second.apical_predicted );
				output.apical->set( it->first, it->second.apical_predicted);
			}
			
			if( analysisCells.writeTIFF && cd.boundarycell == 0 )
				writeTIFF("6_pruned_uni", cd.box, cell_id );
			cd.box->data=0;
		}
	}

////////////////////////////////
//  
// 		Prune apical from isolated lateral domains
//		- lateral contacts that only one other cell and basal around it cannot be connected to the BC, therefore, we remove them
//
////////////////////////////////

	if( pruning.isolated ){

		cout << "Pruning apical from isolated lateral domains" << endl;

	// parallelization seems to cause race-conditions
		for(uint i=0; i<population_data.size(); i++){
			CellData& cd = population_data[i];
			CPM::CELL_ID cell_id = cd.cell_id;
			VINT origin = cd.bbox.minimum;
			
			shared_ptr< Lattice_Data_Layer<double> > distanceMap;
			shared_ptr< Lattice_Data_Layer<bool> > maskMap;
			distanceMap	= createBoxDouble( cd.bbox.size, no_distance);
			maskMap 	= createBoxBoolean( cd.bbox.size, false);
			
			for ( map< VINT, MemPosData >::iterator it=cd.membrane_data.begin(); it != cd.membrane_data.end(); ++it ) {
				// for each lateral contact, identify whether it is a boundary between lateral domains
				if( it->second.lateral > 0 ){
					maskMap->set( it->first - origin, true);
					Cell::Nodes halo = getHaloOfSelf( it->first, cell_id);
					for ( Cell::Nodes::const_iterator n = halo.begin(); n != halo.end(); ++n ){
						if( getCellData( cd.cell_id ).membrane_data.count(*n)){
							int lateral_cellid = getCellData( cd.cell_id ).membrane_data[*n].lateral;
							if( it->second.lateral != lateral_cellid 
								&& lateral_cellid > 0 ){
								distanceMap->set( it->first - origin, 0 );
								//cout << it->first << " is BOUNDARY between " << it->second.lateral << " and " << getCellData( cd.cell_id ).membrane_data[*n].lateral << endl;
								continue;
							}
						
						}
					}
				}
			}
			euclideanDistanceTransform( distanceMap, maskMap );
			
			vector< int > isolated_domains_identified;
			for ( map< VINT, MemPosData >::iterator it=cd.membrane_data.begin(); it != cd.membrane_data.end(); ++it ) {
				if( it->second.lateral > 0 ){
					it->second.isolated_lateral = (distanceMap->get( it->first - origin ) == no_distance );
					
					if( it->second.isolated_lateral && std::find(isolated_domains_identified.begin(), isolated_domains_identified.end(), it->second.lateral) == isolated_domains_identified.end() ){
						isolated_domains_identified.push_back( it->second.lateral );
					}
					
					//cout << "it->second.isolated_lateral ? " << (it->second.isolated_lateral?"true":"false") << endl;
				}
				if( it->second.apical_predicted && it->second.isolated_lateral ){
					//cout << "APICAL PREDICTED on ISOLATED LATERAL DOMAIN!" << endl;
					it->second.apical_predicted = false;
				}
			}
			cd.isolated_domains = isolated_domains_identified.size();
			cout << cd.cell_id << ": Apical domain on " << isolated_domains_identified.size() << " isolated lateral domains pruned.\n";
			for(uint i=0; i < isolated_domains_identified.size(); i++){
				cout << isolated_domains_identified[i] << "\t";
			}
			if( isolated_domains_identified.size() > 0)
				cout << endl;
	// 
	// 		// copy data to maps
			cd.box->data = 0.0;
			for ( map< VINT, MemPosData >::iterator it=cd.membrane_data.begin(); it != cd.membrane_data.end(); ++it ) {
				//cout << it->first << "\t" << it->second.potential << endl;
				cd.box->set( it->first - origin, it->second.apical_predicted );
				output.apical->set( it->first, it->second.apical_predicted);
			}
			
			if( analysisCells.writeTIFF && cd.boundarycell == 0 )
				writeTIFF("7_pruned_iso", cd.box, cell_id );
			cd.box->data=0;
		}
	}
	
	///////////////////////////////
//  
// Counting basal/apical domains
//
////////////////////////////////

	cout << "Counting basal/apical domains" << endl;

	for(uint i=0; i<population_data.size(); i++){
		CellData& cd = population_data[i];
		CPM::CELL_ID cell_id = cd.cell_id;
		cd.num_basal=0;
		cd.num_apical_pre=0;
		cd.num_apical_seg=0;
		cd.num_lateral=0;
		
		VINT origin = cd.bbox.minimum;
		
		shared_ptr< Lattice_Data_Layer<double> > distanceMap_basal;
		shared_ptr< Lattice_Data_Layer<bool> > maskMap_basal;
		distanceMap_basal		= createBoxDouble( cd.bbox.size, no_distance);
		maskMap_basal 			= createBoxBoolean( cd.bbox.size, false);

		shared_ptr< Lattice_Data_Layer<double> > distanceMap_apical_seg;
		shared_ptr< Lattice_Data_Layer<bool> > maskMap_apical_seg;
		distanceMap_apical_seg	= createBoxDouble( cd.bbox.size, no_distance);
		maskMap_apical_seg 		= createBoxBoolean( cd.bbox.size, false);

		shared_ptr< Lattice_Data_Layer<double> > distanceMap_apical_pre;
		shared_ptr< Lattice_Data_Layer<bool> > maskMap_apical_pre;
		distanceMap_apical_pre	= createBoxDouble( cd.bbox.size, no_distance);
		maskMap_apical_pre 		= createBoxBoolean( cd.bbox.size, false);

		shared_ptr< Lattice_Data_Layer<double> > distanceMap_lateral;
		shared_ptr< Lattice_Data_Layer<bool> > maskMap_lateral;
		distanceMap_lateral		= createBoxDouble( cd.bbox.size, no_distance);
		maskMap_lateral 		= createBoxBoolean( cd.bbox.size, false);

		// first, set mask
		for ( map< VINT, MemPosData >::iterator it=cd.membrane_data.begin(); it != cd.membrane_data.end(); ++it ) {
			// for each lateral contact, identify whether it is a boundary between lateral domains
			if( it->second.basal > 0 ){
				maskMap_basal->set( it->first - origin, true);
			}
			if( it->second.apical_segmented > 0 ){
				maskMap_apical_seg->set( it->first - origin, true);
			}
			if( it->second.apical_predicted > 0 ){
				maskMap_apical_pre->set( it->first - origin, true);
			}
			if( it->second.lateral > 0 ){
				maskMap_lateral->set( it->first - origin, true);
			}
		}
		
		// second, count patches (above minimal size).
		/// algorithm checks whether distanceTransform was already run for this patch (if not, run distTransform and count new patch)
		double min_size = 50.0 * node.area; // in micron^2;
		for ( map< VINT, MemPosData >::iterator it=cd.membrane_data.begin(); it != cd.membrane_data.end(); ++it ) {
			
			 if( it->second.basal > 0 ){
				if( distanceMap_basal->get( it->first - origin ) == no_distance ){
					distanceMap_basal->set( it->first - origin, 0);
					double approxsize = euclideanDistanceTransform( distanceMap_basal, maskMap_basal );
					if( approxsize > min_size )
						cd.num_basal++;
				}
				else{
					// patch in which point it->first lies is already counted
				}
			 }
			 if( it->second.apical_segmented > 0 ){
				if( distanceMap_apical_seg->get( it->first - origin ) == no_distance ){
					distanceMap_apical_seg->set( it->first - origin, 0);
					double approxsize = euclideanDistanceTransform( distanceMap_apical_seg, maskMap_apical_seg );
					if( approxsize > min_size )
						cd.num_apical_seg++;
				}
				else{
					// patch in which point it->first lies is already counted
				}
			 }
			 if( it->second.apical_predicted > 0 ){
				if( distanceMap_apical_pre->get( it->first - origin ) == no_distance ){
					distanceMap_apical_pre->set( it->first - origin, 0);
					double approxsize = euclideanDistanceTransform( distanceMap_apical_pre, maskMap_apical_pre );
					if( approxsize > min_size )
						cd.num_apical_pre++;
				}
				else{
					// patch in which point it->first lies is already counted
				}
			 }
			 if( it->second.lateral > 0 ){
				if( distanceMap_lateral->get( it->first - origin ) == no_distance ){
					distanceMap_lateral->set( it->first - origin, 0);
					double approxsize = euclideanDistanceTransform( distanceMap_lateral, maskMap_lateral );
					if( approxsize > min_size )
						cd.num_lateral++;
				}
				else{
					// patch in which point it->first lies is already counted
				}
			 }
		}
		cout << cell_id << " has " << cd.num_basal << " basal, " << cd.num_apical_pre << " apical (predicted), " << cd.num_apical_seg <<" apical (segmented), and " << cd.num_lateral << " lateral domains.\n"; 
	}


////////////////////////////////
//  
// Cell orientation, based on tensor
//
////////////////////////////////

	ofstream fout_tensor_basal, fout_tensor_apical, fout_tensor_apical_segmented, fout_tensor_lateral;
	fout_tensor_basal.open("tensors_basal.log", ios::out);
	fout_tensor_apical.open("tensors_apical.log", ios::out);
	fout_tensor_apical_segmented.open("tensors_apical_segmented.log", ios::out);
	fout_tensor_lateral.open("tensors_lateral.log", ios::out);
	
	for(uint i=0; i<population_data.size(); i++){
		
		CellData& cd = population_data[i];
// 		if( cd.boundarycell > 0 )
// 			continue;
		
		VDOUBLE center = CPM::getCell( cd.cell_id ).getCenter();
		
		Cell::Nodes nodes_basal;
		Cell::Nodes nodes_apical_predicted;
		Cell::Nodes nodes_apical_segmented;
		Cell::Nodes nodes_lateral;
		for ( map< VINT, MemPosData >::iterator i=cd.membrane_data.begin(); i != cd.membrane_data.end(); ++i ) {
			if( i->second.apical_segmented	) 
				nodes_apical_segmented.insert( i->first );
			if( i->second.apical_predicted	) 
				nodes_apical_predicted.insert( i->first );
			if( i->second.basal ){
				//cout << "basal: " << i->first << "\n";
				nodes_basal.insert( i->first );
			}
			if( i->second.lateral > 0 ) 
				nodes_lateral.insert( i->first );
		}
		
		cd.tensor_apical_segmented = getTensor( nodes_apical_segmented,	center );
		cd.tensor_apical_predicted	= getTensor( nodes_apical_predicted,	center);
		cd.tensor_basal 			= getTensor( nodes_basal,	center);
		cd.tensor_lateral 			= getTensor( nodes_lateral,	center );
		
		Tensor t_b = cd.tensor_basal;
		Tensor t_a = cd.tensor_apical_predicted;
		Tensor t_as = cd.tensor_apical_segmented;
		Tensor t_l = cd.tensor_lateral;
		
		//cout << "tensor: " << t_b.center << "\t" <<  t_b.axes[0] << "\t length: " << t_b.lengths[0] <<"\t|\t" << t_b.axes[1] << "\t length: " << t_b.lengths[1] << "\n";
		
		fout_tensor_basal << cd.cell_id << "\t" << t_b.center << "\t" 
			<< t_b.axes[0] << "\t" << t_b.lengths[0] << "\t" 
			<< t_b.axes[1] << "\t" << t_b.lengths[1] << "\t" 
			<< t_b.axes[2] << "\t" << t_b.lengths[2] << "\t" 
			<< t_b.ave_vector << "\t" << t_b.ave_vector.abs() << "\n";
			
		fout_tensor_apical_segmented << cd.cell_id << "\t" << t_as.center << "\t" 
			<< t_as.axes[0] << "\t" << t_as.lengths[0] << "\t" 
			<< t_as.axes[1] << "\t" << t_as.lengths[1] << "\t" 
			<< t_as.axes[2] << "\t" << t_as.lengths[2] << "\t" 
			<< t_as.ave_vector << "\t" << t_as.ave_vector.abs() << "\n";

		fout_tensor_apical << cd.cell_id << "\t" << t_a.center << "\t" 
			<< t_a.axes[0] << "\t" << t_a.lengths[0] << "\t" 
			<< t_a.axes[1] << "\t" << t_a.lengths[1] << "\t" 
			<< t_a.axes[2] << "\t" << t_a.lengths[2] << "\t" 
			<< t_a.ave_vector << "\t" << t_a.ave_vector.abs() << "\n";
			
		fout_tensor_lateral << cd.cell_id << "\t" << t_l.center << "\t" 
			<< t_l.axes[0] << "\t" << t_l.lengths[0] << "\t" 
			<< t_l.axes[1] << "\t" << t_l.lengths[1] << "\t" 
			<< t_l.axes[2] << "\t" << t_l.lengths[2] << "\t" 
			<< t_l.ave_vector << "\t" << t_l.ave_vector.abs() << "\n";

	}
	fout_tensor_basal.close();
	fout_tensor_apical.close();
	fout_tensor_lateral.close();
	fout_tensor_apical_segmented.close();
	
////////////////////////////////
//  
// 		Cell-based Quantification
//
////////////////////////////////
	
	cout << "Quantification cells "<< endl;
	vector< string > output_ss(population_data.size(), "");
	
//#pragma omp parallel for 
	for(uint i=0; i<population_data.size(); i++){
		
		ostringstream ss;
		CellData cd = population_data[i];

		// cell ID
		ss << cd.cell_id << "\t";

		// cell position (centroid in global lattice coordinates)
		ss << CPM::getCell( cd.cell_id ).getCenter() << "\t";

		// is boundary cell (in micron^2)
		double border_area = cd.boundarycell * node.area;
		ss << border_area << "\t";
		
		if( cd.boundarycell > 0 ){
			cout << "Skipping cell with border area = " << border_area << " micron^2." << endl;
			continue;
		}

// 		// cell volume (in micron^3)
 		double cell_volume = node.volume * CPM::getCell( cd.cell_id ).nNodes();
 		ss << cell_volume << "\t";
		
// 		if( cell_volume < 100 ){ 
// 			cout << "Skipping cell with volume = " << cell_volume << " micron^3." << endl;
// 			continue;
// 		}

		////////////////////
		// 1. basal surface area
		// 2. apical surface area SEGMENTED
		// 3. apical surface area PREDICTED
		int basal_sa=0, apical_sa=0, lateral_sa=0, apical_segmented_sa=0;
		for ( map< VINT, MemPosData >::iterator it=cd.membrane_data.begin(); it != cd.membrane_data.end(); ++it ) {

			// counting non-zero per voxels in membrane data as a proxy for surface area
			if( it->second.basal )
				basal_sa++;
			if( it->second.apical_predicted )
				apical_sa++;
			if( it->second.apical_segmented )
				apical_segmented_sa++;
			if( it->second.lateral )
				lateral_sa++;
		}
		ss << node.area*basal_sa << "\t" << node.area*apical_sa << "\t" << node.area*apical_segmented_sa<< "\t" << node.area*lateral_sa << "\t";
		
		//////////////////// 
		
		// Number of neighboring hepatocytes
		ss << cd.neighbors.size() << "\t";
		// Number of isolated basal domains
		ss << cd.num_basal << "\t";
		// Number of isolated apical domains (predicted)
		ss << cd.num_apical_pre << "\t";
		// Number of isolated apical domains (segmented)
		ss << cd.num_apical_seg << "\t";
		// Number of isolated lateral domains
		ss << cd.num_lateral << "\t";
		
		
		////////////////////
		/// COMPUTE SCORES
		//////////////////// 
		bool compute_scores = false;
		vector< Fscore > scores( analysisCells.tolerances.size() );
		double d_score = 0;
		if( compute_scores ){
		
			// measure distances between predicted and segmented APICAL voxels
			shared_ptr< Lattice_Data_Layer<double> > distanceMap_from_prediction = createBoxDouble( cd.bbox.size, no_distance);
			shared_ptr< Lattice_Data_Layer<double> > distanceMap_from_segmentation = createBoxDouble( cd.bbox.size, no_distance);
			shared_ptr< Lattice_Data_Layer<bool> > maskMap = createBoxBoolean( cd.bbox.size, false); // EDT on membrane only

			VINT origin = cd.bbox.minimum;
			for ( map< VINT, MemPosData >::iterator it=cd.membrane_data.begin(); it != cd.membrane_data.end(); ++it ) {
				if( it->second.apical_segmented )
					distanceMap_from_segmentation->set( it->first - origin, 0.0);
				if( it->second.apical_predicted )
					distanceMap_from_prediction->set( it->first - origin, 0.0);
				maskMap->set( it->first - origin, true); // EDT on membrane only such that the distance threshold is the max shift over membrane
			}
			euclideanDistanceTransform( distanceMap_from_segmentation, maskMap );
			euclideanDistanceTransform( distanceMap_from_prediction, maskMap );
					
			// determine TRUE POSITIVE etc based on presence and distance between apical voxels
			//double distance_threshold = analysisCells.tolerance;  //distance threshold is the max shift over membrane
			//int tp=0, fp=0, fn=0; 

			shared_ptr< Lattice_Data_Layer<bool> > FN = createBoxBoolean( cd.bbox.size, false );
			shared_ptr< Lattice_Data_Layer<bool> > TP = createBoxBoolean( cd.bbox.size, false );
			shared_ptr< Lattice_Data_Layer<bool> > FP = createBoxBoolean( cd.bbox.size, false );
			shared_ptr< Lattice_Data_Layer<bool> > cell = createBoxBoolean( cd.bbox.size, false );
			

			double d_score1=0, d_score2=0; uint total_predicted=0, total_segmented=0;
			for(uint i=0; i < analysisCells.tolerances.size(); i++){
				scores[i].tolerance = analysisCells.tolerances[i];

				FN->data = false;
				TP->data = false;
				FP->data = false;
				cell->data = false;

				for ( map< VINT, MemPosData >::iterator it=cd.membrane_data.begin(); it != cd.membrane_data.end(); ++it ) {

					// FN
					if( it->second.apical_segmented && distanceMap_from_prediction->get( it->first - origin) > analysisCells.tolerances[i] ){
						scores[i].fn++;
						FN->set( it->first - origin, true );
					}else{ cd.box3->set( it->first - origin, false); }
					// TP
					if( it->second.apical_predicted && distanceMap_from_segmentation->get( it->first - origin) <= analysisCells.tolerances[i] ){
						scores[i].tp++;
						TP->set( it->first - origin, true );
					}
					//
					if( it->second.apical_predicted && distanceMap_from_segmentation->get( it->first - origin) >  analysisCells.tolerances[i] ){
						scores[i].fp++;
						FP->set( it->first - origin, true );
					}

					if( i == 0 ){
						if( it->second.apical_predicted  ){
							total_predicted++;
							double d = distanceMap_from_segmentation->get( it->first - origin);
							d_score1 += (d<no_distance ? d:0);
						}
						if( it->second.apical_segmented ){
							total_segmented++;
							double d = distanceMap_from_prediction->get( it->first - origin);
							d_score2 += (d<no_distance ? d:0);
						}
					}
				}
				
				const Cell::Nodes& cell_nodes = CPM::getCell ( cd.cell_id ).getNodes();
				for ( Cell::Nodes::const_iterator n = cell_nodes.begin(); n != cell_nodes.end(); ++n ) {
					if( !CPM::isBoundary( *n ) ) // exclude membrane nodes
						cell->set( *n - cd.bbox.minimum, cd.cell_id);
				}


				ostringstream fn; fn << "FNTPFP_" << scores[i].tolerance;
				if( analysisCells.writeTIFF && cd.boundarycell == 0 ){
					writeFNTPFPcell( fn.str(), FN, TP, FP, cell, cd );
				}
			}
			
			for(uint i=0; i < analysisCells.tolerances.size(); i++){
				computeFscore( scores[i] );
			}
			
			d_score1 /= total_predicted;
			d_score2 /= total_segmented;
			d_score = (d_score1 + d_score2) / 2.0;
		}
		population_data[i].dscore = d_score;
		
		
		////////////////////
		/// END OF COMPUTE SCORES
		//////////////////// 

		cout << "ID: " << cd.cell_id << "\tD: " << d_score;
		for(uint i=0; i < analysisCells.tolerances.size(); i++){
			cout << "\tTol: " << scores[i].tolerance << "\tF: " << scores[i].fscore << "\tP: " << scores[i].precision << "\tS: " << scores[i].sensitivity << "\t";
		}
		cout << "\n";
		
		ss << d_score << "\t";
		for(uint i=0; i < analysisCells.tolerances.size(); i++){
			ss << "\t" << scores[i].tolerance << "\t" << scores[i].fscore << "\t" << scores[i].precision << "\t" << scores[i].sensitivity << "\t";
		}
		
		output_ss[i] = ss.str();
		
		ostringstream fn;	fn << "fscores_" << cd.cell_id << ".log";
		ofstream out(fn.str().c_str(),std::ios_base::binary);
		out << "#T\tF\tP\tS\tTP\tFP\tFN\n";
		for(uint i=0; i < analysisCells.tolerances.size(); i++){
			out << scores[i].tolerance << "\t" << scores[i].fscore << "\t" << scores[i].precision << "\t" << scores[i].sensitivity << "\t" 
											   << scores[i].tp << "\t" << scores[i].fp << "\t" << scores[i].fn << "\n";
		}
		out.close();
		
	}
	cout << "\n";
	
	//// write output to file ////
	fout.open("hepatocytes.log", ios::out);
	ostream& outputfile = fout.is_open() ? fout : cout;
	// header
	outputfile << "#ID\tX Y X\tBound\tVOL\tBAS\tAPI_PRE\tAPI_SEG\tLAT\tNEIGH\tNBAS\tNAPI_SEG\tNAPI_PRE\tNLAT\tD";
	for(uint i=0; i < analysisCells.tolerances.size(); i++){
		outputfile << "\tT\tF\tP\tS";
	}
	outputfile << "\n";
	
	for(uint i=0; i<population_data.size(); i++){
		outputfile << output_ss[i];
		if( ! output_ss[i].empty() )
			outputfile << "\n"; 
	}
	fout.close();

	
	
////////////////////////////////
//  
// 		Tissue-level quantification
//
////////////////////////////////

	
	shared_ptr< Lattice_Data_Layer<double> > distanceMap_from_prediction;
	shared_ptr< Lattice_Data_Layer<bool> > prediction;
	shared_ptr< Lattice_Data_Layer<double> > distanceMap_from_segmentation;
	shared_ptr< Lattice_Data_Layer<bool> > maskMap;
	distanceMap_from_prediction		= createBoxDouble( latticeSize, no_distance);
	distanceMap_from_segmentation	= createBoxDouble( latticeSize, no_distance);
	maskMap = createBoxBoolean( latticeSize, true);
	output.TP = createBoxBoolean( latticeSize, false );
	output.FP = createBoxBoolean( latticeSize, false );
	output.FN = createBoxBoolean( latticeSize, false );
	
	// get predicted BC from membranes of individual cells
	prediction	= createBoxBoolean( latticeSize, false );
	for(uint p=0; p<population_data.size(); p++){
		CellData cd = population_data[p];
		VINT origin = cd.bbox.minimum;
		for ( map< VINT, MemPosData >::iterator i=cd.membrane_data.begin(); i != cd.membrane_data.end(); ++i ) {
			if( i->second.apical_predicted > 0 ){
				prediction->set(i->first, true);
			}
		}
	}
	
	// set distances maps
	VINT pos(0,0,0);
	for(pos.z=0; pos.z < latticeSize.z; pos.z++){
		for(pos.y=0; pos.y < latticeSize.y; pos.y++){
			for(pos.x=0; pos.x < latticeSize.x; pos.x++){
				if( input.apical_pde->get( pos ) > 0.0)
					distanceMap_from_segmentation->set( pos, 0.0 );
				if( prediction->get( pos ) == true )
					distanceMap_from_prediction->set( pos, 0.0 );
			}
		}
	}
	euclideanDistanceTransform( distanceMap_from_segmentation, maskMap );
	euclideanDistanceTransform( distanceMap_from_prediction, maskMap );
		
	// determine TRUE POSITIVE etc based on presence and distance between apical voxels
	//double distance_threshold = analysisTissue.tolerance;  //distance threshold is the max shift over membrane
	int border = analysisTissue.border; // discard voxels near lattice border
	//int tp=0, fp=0, fn=0; 
	vector< Fscore > scores( analysisCells.tolerances.size() );
	double d_score1=0., d_score2=0.; int total_predicted=0, total_segmented=0;
	
	for(uint i=0; i < analysisCells.tolerances.size(); i++){
		scores[i].tolerance = analysisCells.tolerances[i];
	
		output.TP->data = false;
		output.FP->data = false;
		output.FN->data = false;
		
		for(pos.z=border; pos.z < latticeSize.z-border; pos.z++){
			for(pos.y=border; pos.y < latticeSize.y-border; pos.y++){
				for(pos.x=border; pos.x < latticeSize.x-border; pos.x++){
					
					// RED
					///// input.apical_pde->get( pos ) ===> output.apical_segmented->get(pos)
					if( input.apical_pde->get( pos ) > 0  && distanceMap_from_prediction->get( pos ) > analysisTissue.tolerances[i] ){
						scores[i].fn++;
						output.FN->set(pos, true);
					}
					// GREEN
					if( prediction->get( pos ) == true  && distanceMap_from_segmentation->get( pos ) <= analysisTissue.tolerances[i] ){
						scores[i].tp++;
						output.TP->set(pos, true);
					}
					// BLUE
					if( prediction->get( pos ) == true  && distanceMap_from_segmentation->get( pos ) > analysisTissue.tolerances[i] ){
						scores[i].fp++;
						output.FP->set(pos, true);
					}
					
					if( i == 0 ){ 
						if( prediction->get(pos) == true ){
							total_predicted++;
							d_score1 += distanceMap_from_segmentation->get( pos );
						}
						if( input.apical_pde->get( pos ) > 0  ){
							total_segmented++;
							d_score2 += distanceMap_from_prediction->get( pos );
						}
					}
				}
			}
		}
		
		if( analysisTissue.writeTIFF ){ 
			ostringstream filename;
			filename << "FN_TP_FP_" << scores[i].tolerance << ".tif";
			writeFNTPFPTIFF( filename.str() );
		}
	}

	
	cout << " ====================================================\n";
	for(uint i=0; i < analysisTissue.tolerances.size(); i++){
		computeFscore( scores[i] );
	}
	
	d_score1 /= total_predicted;
	d_score2 /= total_segmented;
	double d_score = (d_score1 + d_score2) / 2.0;
	
	cout << "D: " << d_score;
	for(uint i=0; i < analysisTissue.tolerances.size(); i++){
		cout << "\tT: " << scores[i].tolerance <<  "\tF: " << scores[i].fscore << "\tP: " << scores[i].precision << "\tS: " << scores[i].sensitivity << "\n";
	}

	//// write output to file ////
	fout.open("tissue.log", ios::out);
	ostream& output_tissue = fout.is_open() ? fout : cout;
	// header
	output_tissue << "T\tD\tF\tP\tS\tTP\tFP\tFN\n";
	// data
	for(uint i=0; i < analysisTissue.tolerances.size(); i++){
		output_tissue 	<< scores[i].tolerance << "\t" << d_score << "\t"
						<< scores[i].fscore << "\t" << scores[i].precision << "\t" << scores[i].sensitivity << "\t"
						<< scores[i].tp << "\t" << scores[i].fp << "\t" << scores[i].fn << "\n";
	}
	
	fout.close();
	
////////////////////////////////
//  
// 		Write Image OUTPUT 
//
////////////////////////////////

	shared_ptr<MembraneMapper> mapper_basal 	= shared_ptr <MembraneMapper >( new MembraneMapper(MembraneMapper::MAP_BOOLEAN, false) );
	shared_ptr<MembraneMapper> mapper_lateral 	= shared_ptr <MembraneMapper >( new MembraneMapper(MembraneMapper::MAP_DISCRETE, false) );
	shared_ptr<MembraneMapper> mapper_distance 	= shared_ptr <MembraneMapper >( new MembraneMapper(MembraneMapper::MAP_CONTINUOUS, false) );
	shared_ptr<MembraneMapper> mapper_potential	= shared_ptr <MembraneMapper >( new MembraneMapper(MembraneMapper::MAP_CONTINUOUS, false) );
	shared_ptr<MembraneMapper> mapper_apical	= shared_ptr <MembraneMapper >( new MembraneMapper(MembraneMapper::MAP_BOOLEAN, false) );
	shared_ptr<MembraneMapper> mapper_apical_segmented	= shared_ptr <MembraneMapper >( new MembraneMapper(MembraneMapper::MAP_BOOLEAN, false) );
	
	
	cout << "Writing 2D plots" << endl;

	if( analysisCells.writeMembraneMaps )
		for(uint i=0; i<population_data.size(); i++){
			CellData cd = population_data[i];

			// skip cell that touch the lattice boudary
			//if( cd.boundarycell > 0 )
			//	continue;
			
			//cout << "Tensor "<< ":\t" << cd.tensor_basal.axes[0]  << "\t length = " << cd.tensor_basal.axes[0].abs() << "\n";
			Eigen::Matrix3f rotation_matrix = getRotationMatrix( cd.tensor_basal.axes[0], VDOUBLE(0,0,1) );
			//Eigen::Matrix3f rotation_matrix2 = getRotationMatrix( cd.tensor_apical_predicted.axes[0], VDOUBLE(0,0,1) );
			//Eigen::Matrix3f rotation_matrix = rotation_matrix1 * rotation_matrix2;

			if( rotation_matrix.sum() > 0 ){
				mapper_basal->setRotationMatrix( rotation_matrix );
				mapper_lateral->setRotationMatrix( rotation_matrix );
				mapper_distance->setRotationMatrix( rotation_matrix );
				mapper_potential->setRotationMatrix( rotation_matrix );
				mapper_apical->setRotationMatrix( rotation_matrix );
				mapper_apical_segmented->setRotationMatrix( rotation_matrix );
			}
			
			mapper_basal->attachToCell(cd.cell_id);
			mapper_lateral->attachToCell(cd.cell_id);
			mapper_distance->attachToCell(cd.cell_id);
			mapper_potential->attachToCell(cd.cell_id);
			mapper_apical->attachToCell(cd.cell_id);
			mapper_apical_segmented->attachToCell(cd.cell_id);
			
			for ( map< VINT, MemPosData >::iterator it=cd.membrane_data.begin(); it != cd.membrane_data.end(); ++it ) {
				//cout << it->first << "\t= " << (it->second.basal?"basal":"nonbasal") << endl;
				
				mapper_basal->map		(it->first, it->second.basal); 
				mapper_lateral->map		(it->first, it->second.lateral);
				mapper_distance->map	(it->first, (it->second.distance == no_distance ? 0 : it->second.distance ));
				mapper_potential->map	(it->first, (it->second.potential > 200 ? 0 : it->second.potential ));
				mapper_apical->map		(it->first, it->second.apical_predicted);
				mapper_apical_segmented->map(it->first, it->second.apical_segmented);
			}
			mapper_basal->fillGaps();
			mapper_lateral->fillGaps();
			mapper_distance->fillGaps();
			mapper_potential->fillGaps();
			mapper_apical->fillGaps();
			mapper_apical_segmented->fillGaps();
			mapper_basal->copyData( output_membranes[0].getMembrane( cd.cell_id ) );
			mapper_lateral->copyData( output_membranes[1].getMembrane( cd.cell_id ) );
			mapper_distance->copyData( output_membranes[2].getMembrane( cd.cell_id ) );
			mapper_potential->copyData( output_membranes[3].getMembrane( cd.cell_id ) );
			mapper_apical->copyData( output_membranes[4].getMembrane( cd.cell_id ) );
			mapper_apical_segmented->copyData( output_membranes[5].getMembrane( cd.cell_id ) );

		}

	if( analysisTissue.writeTIFF ){ 
		writeMultichannelTIFF(); 
	}
}

Eigen::Matrix3f MembraneRules3D::getRotationMatrix(VDOUBLE a, VDOUBLE b){

	Eigen::Matrix3f matrix;
	if( a.abs() == 0 || b.abs() == 0 || a == b ){
		cout << "Warning: getRotationMatrix, vectors are zero length or equal!\n";
		matrix << 0,0,0,
				0,0,0,
				0,0,0;
		return matrix;
	}
	
	// 1. Calculate rotation between two vectors, in axis-angle rotation row vector
	// See the MatLab function vrrotvec.m 
	VDOUBLE an = a / a.abs();
	VDOUBLE bn = b / b.abs();
	// axb gives the axis
	VDOUBLE axb = cross(an, bn); 
	axb = axb / axb.abs();
	// ac gives the angle
	double ac = acos( dot(an, bn) );
	
	// 2. Convert rotation from axis-angle to matrix representation.
	// See the MatLab function vrrotvec2mat.m 
	double s = sin(ac);
	double c = cos(ac);
	double t = 1-c;
	VDOUBLE n = axb;

	matrix << 
		t*n.x*n.x + c,		t*n.x*n.y - s*n.z, 	t*n.x*n.z + s*n.y,
		t*n.x*n.y + s*n.z, 	t*n.y*n.y + c,		t*n.y*n.z - s*n.x,
		t*n.x*n.z - s*n.y, 	t*n.y*n.z + s*n.x, 	t*n.z*n.z + c;

	return matrix;
}

bool MembraneRules3D::localThresholding( VINT m_l, CellData& cd ){
	
	if (m_l.x < cd.bbox.minimum.x || m_l.y < cd.bbox.minimum.y || m_l.z < cd.bbox.minimum.z ||  m_l.x >= cd.bbox.maximum.x||  m_l.y >= cd.bbox.maximum.y||  m_l.z >= cd.bbox.maximum.z) {
		cout << m_l << "\tcd.bbox.minimum: " << cd.bbox.minimum << "\tcd.bbox.maximum: " << cd.bbox.maximum << endl;
		assert(0);
	}

	// global Lattice coordinates
	uint dist = uint(round(thresholding.max_distance / (SIM::getNodeLength() * 1e6))); // micron to pixels conversion
	VINT bbmin(m_l.x-dist, m_l.y-dist, m_l.z-dist);
	VINT bbmax(m_l.x+dist, m_l.y+dist, m_l.z+dist);
	bbmin = maxVINT(bbmin, VINT(0,0,0));
	bbmax = minVINT(bbmax, latticeSize);
	bbmin = maxVINT(bbmin, cd.bbox.minimum);
	bbmax = minVINT(bbmax, cd.bbox.maximum);
	VINT bbsize = bbmax - bbmin;
	VINT origin = bbmin;

	cd.box->data = no_distance;

	// initialize distance
	cd.box->set(m_l - cd.bbox.minimum, 0);
	euclideanDistanceTransform(cd.box, cd.box3, bbmin-cd.bbox.minimum, bbmax-cd.bbox.minimum);
	
	double local_mean = 0.0; 
	double local_median = 0.0;
	vector<double> values;
	double local_median_small = 0.0;
	vector<double> values_small;
	double local_count = 0.0;
	double local_mean_small = 0.0; 
	double local_count_small = 0.0;
	double local_minimum = 99999.0; 
	double local_maximum = -99999.0;
	VINT pos; // in global coordinates
	
#pragma parallel for
	for(pos.z = bbmin.z; pos.z < bbmax.z; pos.z++){
		for(pos.y = bbmin.y; pos.y < bbmax.y; pos.y++){
			for(pos.x = bbmin.x; pos.x < bbmax.x; pos.x++){
				
				
				// skip voxels outside of masked area
				if( !cd.box3->get(pos - cd.bbox.minimum) ){
					continue;
				}

				// skip non-lateral voxels
				if( !cd.membrane_data[ pos ].lateral && cd.membrane_data[ pos ].basal ){
					continue;
				}

//  				if( cd.membrane_data.count( pos ) == 0 ){
//  					cout << "Warning! On membrane but not in MemData ?? : VINT = " << pos << endl;
//  					continue;
//  				}

				double dist = cd.box->get(pos - cd.bbox.minimum);
				
				// get local average of small local neighborhood (instead of Gaussian blurring)
				if( dist >= 0 && dist <= thresholding.local_distance ){
					//cout << "< maxdist: " << pos << " pos+origin: " << pos+origin << endl;
					double value = 0.0;
					switch( communication.mode ){
						case IntercellularCommunication::NONE:{
							value = sqr(cd.membrane_data[ pos ].distance);
							break;
						}
						case IntercellularCommunication::GEOMETRIC_MEAN:{
							value = sqr(cd.membrane_data[ pos ].potential);
							break;
						}
					}
					
					if(value > 0)
						values_small.push_back( value );


					if(thresholding.weighted){
						double weight = exp(- dist / (thresholding.local_distance/3.0));
						local_mean_small  += value * weight;
						local_count_small += weight;
					}
					else{
						local_mean_small  += value;
						local_count_small += 1.0;
					}
					//cout << " local_mean_small: " << local_mean_small << "\n";
				}
					
				if( dist > 0 && dist <= thresholding.max_distance ){

					double value = 0.0;
					switch( communication.mode ){
						case IntercellularCommunication::NONE:{
							value = sqr(cd.membrane_data[ pos ].distance);							//cout << "Dist " << dist << " <= max_dist " << thresholding.max_distance << " || distance from basal = " << value << "\n";
							break;
						}
						case IntercellularCommunication::GEOMETRIC_MEAN:{
							value = sqr(cd.membrane_data[ pos ].potential);
							break;
						}
					}
					
					if(value > 0)
						values.push_back( value );
					
					if( value < local_minimum )
						local_minimum = value;
					if( value > local_maximum )
						local_maximum = value;
					
					if(thresholding.weighted){
						double weight = exp(- dist / (thresholding.max_distance/3.0));
						local_mean  += value * weight;
						local_count += weight;
					}
					else{
						local_mean  += value;
						local_count += 1.0;
					}
					//cout << " local_mean: " << local_mean << "\n";
				}
				
			}
		}
	}
	if(local_count > 0)
		local_mean /= local_count;
	if(local_count_small > 0)
		local_mean_small /= local_count_small;
	
	local_median = getMedian( values );
	local_median_small = getMedian( values_small );
	
	// ==== 6. do thresholding based on local mean
	// Note: now replaced by local_mean_small
// 	double my_value;
// 	switch( communication.mode ){
// 		case IntercellularCommunication::NONE:{
// 			my_value = cd.membrane_data[ m_l ].distance;
// 			break;
// 		}
// 		case IntercellularCommunication::GEOMETRIC_MEAN:{
// 			my_value = cd.membrane_data[ m_l ].potential;
// 			break;
// 		}
// 		default: {
// 			cerr << "Unknown method for intercellular communication." << endl;
// 			break;
// 		}
//	}

	///*** MEAN ***///
	//bool result = ( (local_mean_small - thresholding.threshold) > local_mean );
	///*** MEDIAN ***///
	bool result = ((local_mean_small - thresholding.threshold) > local_median);
	///*** CONTAST: if value is closer to local maximum -> ON, else OFF ***///
	//bool result = (abs(local_mean_small - local_maximum) < abs(local_mean_small - local_minimum));
	
	//cout << "local_mean_small: " << local_mean_small << "\tlocal_mean: " << local_mean << "\tresult: " << (result?"yes!":"no..") << "\n";

	
// 	cout <<	"cell: " << cd.cell_id << ", pos: " << m_l <<  ", local mean: " << local_mean 
// 													   << ", local mean small: "<< local_mean_small 
// 													   << " => " << (result?"yes!":"no..") << "\n";
	return result;
	//return ((my_value - thresholding.threshold) > local_mean);
	//return local_mean;
}

double MembraneRules3D::getMedian(vector<double>& vec) 
{ 
	if( vec.size() == 0 )
		return 0;
	assert( vec.size() > 0 );
// 	sort(vec.begin(), vec.end());
// 	for(uint i=0; i< vec.size(); i++)
// 		cout << vec[i] << ", ";
// 	cout << "\n";
	
	std::nth_element( vec.begin(), vec.begin() + vec.size() / 2, vec.end() ); 
// 	cout << "median = " << *( vec.begin() + vec.size() / 2 );
// 	cout << "vec size= " << vec.size() << "\n";
	return *( vec.begin() + vec.size() / 2 );
	
}


set< string > MembraneRules3D::getDependSymbols() {
	set< string > s;
// 	for(uint i=0; i<reporter.input_symbol_strs.size(); i++)
// 		s.insert ( reporter.input_symbol_strs[i] );
	return s;
}

set< string > MembraneRules3D::getOutputSymbols() {
	set< string > s;
// 	s.insert ( reporter.output_symbol_str );
// 	if ( reporter.output == Reporter::OUT_CELLPROPERTY ) 
// 		s.insert( reporter.output_cellproperty_str );
	return s;
}

MembraneRules3D::BoundingBox MembraneRules3D::getBoundingBoxNode(VINT m, CPM::CELL_ID cell_id) {
	
	uint dist = uint(ceil(thresholding.max_distance));
	VINT bbmin(m.x-dist, m.y-dist, m.z-dist);
	VINT bbmax(m.x+dist, m.y+dist, m.z+dist);

	VINT minimum(latticeSize.x, latticeSize.y, latticeSize.z);
	VINT maximum(0,0,0);
	const Cell::Nodes& nodes = CPM::getCell(cell_id).getSurface();
	for (Cell::Nodes::const_iterator pt = nodes.begin(); pt != nodes.end(); pt++){
		if ( pt->x < bbmin.x || pt->y < bbmin.y || pt->z < bbmin.z ||
			 pt->x > bbmax.x || pt->y > bbmax.y || pt->z > bbmax.z )
			continue;
		
		minimum.x = std::min(pt->x, minimum.x);
		minimum.y = std::min(pt->y, minimum.y);
		minimum.z = std::min(pt->z, minimum.z);
		maximum.x = std::max(pt->x, maximum.x);
		maximum.y = std::max(pt->y, maximum.y);
		maximum.z = std::max(pt->z, maximum.z);

	}
	
	BoundingBox bb;
	bb.minimum = minimum;
	bb.maximum = maximum;
	bb.size = bb.maximum - bb.minimum;
	return bb;
}

MembraneRules3D::BoundingBox MembraneRules3D::getBoundingBoxCell(CPM::CELL_ID cell_id) {
	//assert( CPM::getCell(cell_id).getNodes() > 0 ); 
	// get bounding box of cell
	VINT bbmax(0,0,0), bbmin(latticeSize.x, latticeSize.y, latticeSize.z);
	const Cell::Nodes& nodes = CPM::getCell(cell_id).getNodes();
	for (Cell::Nodes::const_iterator pt = nodes.begin(); pt != nodes.end(); pt++){
		bbmin.x = std::min(pt->x, bbmin.x);
		bbmin.y = std::min(pt->y, bbmin.y);
		bbmin.z = std::min(pt->z, bbmin.z);
		bbmax.x = std::max(pt->x, bbmax.x);
		bbmax.y = std::max(pt->y, bbmax.y);
		bbmax.z = std::max(pt->z, bbmax.z);
	}
	bbmax += VINT(1,1,1);
	
	BoundingBox bb;
	bb.minimum = bbmin;
	bb.maximum = bbmax;
	bb.size = bbmax - bbmin;
	
	//cout << "Cell ID: " << cell_id << ", BB min: " << bb.minimum << ", max " << bb.maximum << ", size: " << bb.size << endl;
	
	return bb;
}
	
shared_ptr< Lattice_Data_Layer< double > > MembraneRules3D::createBoxDouble( VINT boxsize, double default_value ){
	
	// create temporary lattice to hold distance values
	XMLNode xLattice = XMLNode::createXMLTopNode("Lattice");
	xLattice.addChild("Size").addAttribute("value",to_cstr(boxsize));
	XMLNode xLatticeBC = xLattice.addChild("BoundaryConditions");
	XMLNode xLatticeBCC1 = xLatticeBC.addChild("Condition");
	xLatticeBCC1.addAttribute("boundary",to_cstr("x"));
	xLatticeBCC1.addAttribute("type","constant");
	XMLNode xLatticeBCC2 = xLatticeBC.addChild("Condition");
	xLatticeBCC2.addAttribute("boundary",to_cstr("-x"));
	xLatticeBCC2.addAttribute("type","constant");
	XMLNode xLatticeBCC3 = xLatticeBC.addChild("Condition");
	xLatticeBCC3.addAttribute("boundary",to_cstr("y"));
	xLatticeBCC3.addAttribute("type","constant");
	XMLNode xLatticeBCC4 = xLatticeBC.addChild("Condition");
	xLatticeBCC4.addAttribute("boundary",to_cstr("-y"));
	xLatticeBCC4.addAttribute("type","constant");
	XMLNode xLatticeBCC5 = xLatticeBC.addChild("Condition");
	xLatticeBCC5.addAttribute("boundary",to_cstr("z"));
	xLatticeBCC5.addAttribute("type","constant");
	XMLNode xLatticeBCC6 = xLatticeBC.addChild("Condition");
	xLatticeBCC6.addAttribute("boundary",to_cstr("-z"));
	xLatticeBCC6.addAttribute("type","constant");
	
	//boxnumber++;
	//cout << xLattice.writeToFile(string( string("box_") + to_str(boxnumber) + string(".xml") ).c_str() ) << endl;
	shared_ptr<const Lattice> latticeBox = shared_ptr<const Lattice>(new Cubic_Lattice(xLattice));
	return shared_ptr< Lattice_Data_Layer<double> >(new Lattice_Data_Layer<double>( latticeBox, 2, default_value));
}
	
shared_ptr< Lattice_Data_Layer< bool > > MembraneRules3D::createBoxBoolean( VINT boxsize, bool default_value ){
	
	// create temporary lattice to hold distance values
	XMLNode xLattice = XMLNode::createXMLTopNode("Lattice");
	xLattice.addChild("Size").addAttribute("value",to_cstr(boxsize));
	XMLNode xLatticeBC = xLattice.addChild("BoundaryConditions");
	XMLNode xLatticeBCC1 = xLatticeBC.addChild("Condition");
	xLatticeBCC1.addAttribute("boundary",to_cstr("x"));
	xLatticeBCC1.addAttribute("type","constant");
	XMLNode xLatticeBCC2 = xLatticeBC.addChild("Condition");
	xLatticeBCC2.addAttribute("boundary",to_cstr("-x"));
	xLatticeBCC2.addAttribute("type","constant");
	XMLNode xLatticeBCC3 = xLatticeBC.addChild("Condition");
	xLatticeBCC3.addAttribute("boundary",to_cstr("y"));
	xLatticeBCC3.addAttribute("type","constant");
	XMLNode xLatticeBCC4 = xLatticeBC.addChild("Condition");
	xLatticeBCC4.addAttribute("boundary",to_cstr("-y"));
	xLatticeBCC4.addAttribute("type","constant");
	XMLNode xLatticeBCC5 = xLatticeBC.addChild("Condition");
	xLatticeBCC5.addAttribute("boundary",to_cstr("z"));
	xLatticeBCC5.addAttribute("type","constant");
	XMLNode xLatticeBCC6 = xLatticeBC.addChild("Condition");
	xLatticeBCC6.addAttribute("boundary",to_cstr("-z"));
	xLatticeBCC6.addAttribute("type","constant");
	
	//boxnumber++;
	//cout << xLattice.writeToFile(string( string("box_") + to_str(boxnumber) + string(".xml") ).c_str() ) << endl;
	shared_ptr<const Lattice> latticeBox = shared_ptr<const Lattice>(new Cubic_Lattice(xLattice));
	return shared_ptr< Lattice_Data_Layer<bool> >(new Lattice_Data_Layer<bool>(latticeBox, 2, default_value));
}
	
Cell::Nodes MembraneRules3D::getHalo( VINT mempos, CPM::CELL_ID id){
	Cell::Nodes halo;
	for ( int i = 0; i < neighbor_sites_halo.size(); ++i ) {
		VINT nbpos = mempos + neighbor_sites_halo[i];
		const CPM::STATE& nb_spin = cpm_lattice->get ( nbpos );

		if ( id != nb_spin.cell_id ) { // if neighbor is different from me
			halo.insert ( nbpos ); // add neighbor node to list of unique neighboring points (used for layers below)
		}
	}
	return halo;
}

Cell::Nodes MembraneRules3D::getHaloOfSelf( VINT mempos, CPM::CELL_ID id){
	Cell::Nodes halo;
	for ( int i = 0; i < neighbor_sites_halo.size(); ++i ) {
		VINT nbpos = mempos + neighbor_sites_halo[i];
		const CPM::STATE& nb_spin = cpm_lattice->get ( nbpos );

		if ( id == nb_spin.cell_id ) { // if neighbor node has same cell ID
			halo.insert ( nbpos ); // add neighbor node to list of unique neighboring points (used for layers below)
		}
	}
	return halo;
}


template <class T>
void MembraneRules3D::writeTIFF(string prepend, shared_ptr< Lattice_Data_Layer< T > >& box, CPM::CELL_ID id){
	
	TIFF *output;
	uint32 width, height, slices;
	bool append=false;
	
	append=false;
	stringstream filename;
	filename << prepend << "_cell_" << id << ".tif";
	
	ifstream ifile(filename.str().c_str());
	// open file for writing, except if file already exists, than open it for appending
	string mode = "w";
	if( ifile ){ 
		mode = "a";
	}
	if((output = TIFFOpen(filename.str().c_str(), mode.c_str())) == NULL){
		cerr << "Could not open image " << filename << " to write/appending." << endl;
		exit(-1);
	}
	
		// Allocate buffer according to file format
	width 	=	box->size().x;
	height	=	box->size().y;
	slices	=	box->size().z;

	
	uint32 buffer32[width * height];
	uint16 bpp, spp;
	uint format = 32;
	bpp = 32; // bits per pixel
	spp = 1; // samples per pixel (1 = greyscale, 3 = RGB)
	
	VINT pmin(0,0,0);
	VINT pmax = box->size();
	
	VINT pos(0,0,0);
	for (pos.z=pmin.z; pos.z<pmax.z; pos.z++)
	{
		for (pos.y=pmin.y; pos.y<pmax.y; pos.y++)
		{
			for (pos.x=pmin.x; pos.x<pmax.x; pos.x++)
			{
				double value = box->get(pos);
				if(value == no_distance)
					value = 0;
				uint bufindex = (pos.x-pmin.x) + (pos.y-pmin.y)*width;
				(float&)buffer32[bufindex] = (float)value;
			}
		}
			
		// Set TIFF info fields
		TIFFSetField(output, TIFFTAG_IMAGEWIDTH,  width); //x
		TIFFSetField(output, TIFFTAG_IMAGELENGTH, height); //y
		TIFFSetField(output, TIFFTAG_BITSPERSAMPLE, bpp);
		TIFFSetField(output, TIFFTAG_ROWSPERSTRIP, height); //bpp); 
		TIFFSetField(output, TIFFTAG_SAMPLESPERPIXEL, spp);
		uint sampleformat;
		switch(format){
			case 8: { sampleformat = 1; break; } // unsigned integer
			case 16:{ sampleformat = 2; break; } // signed integer
			case 32:{ sampleformat = 3; break; } // IEEE floating point
		}
		TIFFSetField(output, TIFFTAG_SAMPLEFORMAT, sampleformat);
		TIFFSetField(output, TIFFTAG_SOFTWARE,  "morpheus");
		TIFFSetField(output, TIFFTAG_PLANARCONFIG,  PLANARCONFIG_CONTIG);
		TIFFSetField(output, TIFFTAG_PHOTOMETRIC,   PHOTOMETRIC_MINISBLACK);
		//TIFFSetField(output, TIFFTAG_XRESOLUTION, 1);
		//TIFFSetField(output, TIFFTAG_YRESOLUTION, 1);	
		TIFFSetField(output, TIFFTAG_COMPRESSION,   COMPRESSION_LZW);
		
		// Multipage TIFF
		unsigned short page, numpages_before=0, numpages_after=0;
		if( slices>1 ){
			TIFFSetField(output, TIFFTAG_SUBFILETYPE, 0);
		}
		
		// Now, write the buffer to TIFF
		for (uint i=0; i<height; i++){
			if(TIFFWriteScanline(output, &buffer32[i * width], i, 0) < 0){
				cerr << "Could not write TIFF image " << endl;
				exit(-1);
			}
		}
		
		if( slices>1 ){
			TIFFWriteDirectory(output);
		}
	}
	TIFFClose(output);
	
	
};

void MembraneRules3D::writeFNTPFPcell(string prepend, 	shared_ptr< Lattice_Data_Layer< bool > >& FN,
														shared_ptr< Lattice_Data_Layer< bool > >& TP,
														shared_ptr< Lattice_Data_Layer< bool > >& FP,
														shared_ptr< Lattice_Data_Layer< bool > >& cell,
														CellData cd){
	
	TIFF *output;
	uint32 width, height, slices;
	bool append=false;
	
	append=false;
	stringstream filename;
	filename << prepend << "_cell_" << cd.cell_id << ".tif";
	if((output = TIFFOpen(filename.str().c_str(), "w")) == NULL){
		cerr << "Could not open image " << filename << " to write." << endl;
		exit(-1);
	}
	
	// Allocate buffer according to file format
	width 	=	FN->size().x;
	height	=	FN->size().y;
	slices	=	FN->size().z;

	
	uint8 buffer8[width * height];
	uint16 bpp, spp;
	uint format = 8;
	bpp = 8; // bits per pixel
	spp = 1; // samples per pixel (1 = greyscale, 3 = RGB)
	int numchannel = 4;
	
	VINT pmin(0,0,0);
	VINT pmax = FN->size();
	VINT origin = cd.bbox.minimum;

	
	VINT pos(0,0,0);
	for (pos.z=pmin.z; pos.z<pmax.z; pos.z++)
	{
		for(uint channel=0; channel<numchannel; channel++)
		{	
			for (pos.y=pmin.y; pos.y<pmax.y; pos.y++)
			{
				for (pos.x=pmin.x; pos.x<pmax.x; pos.x++)
				{
					uint8 value = 0.0;
					switch(channel){
						// RED = False negative
						case 0: value=(FN->get(pos) ? 255:0); 	break;
						// GREEN = True positive
						case 1: value=(TP->get(pos) ? 255:0); 	break;
						// BLUE = False positive
						case 2: value=(FP->get(pos) ? 255:0); 	break;
						// WHITE = Cell ID
						case 3: value=(cell->get(pos) ? 255:0); break;
						default: cout << "unknown channel"; 	break;
					}
					if(value == no_distance)
						value = 0;
					uint bufindex = (pos.x-pmin.x) + (pos.y-pmin.y)*width;
					(uint8&)buffer8[bufindex] = (uint8)value;
				}
			}
				
			// Set TIFF info fields
			TIFFSetField(output, TIFFTAG_IMAGEWIDTH,  width); //x
			TIFFSetField(output, TIFFTAG_IMAGELENGTH, height); //y
			TIFFSetField(output, TIFFTAG_BITSPERSAMPLE, bpp);
			TIFFSetField(output, TIFFTAG_ROWSPERSTRIP, height); //bpp); 
			TIFFSetField(output, TIFFTAG_SAMPLESPERPIXEL, spp);
			uint sampleformat;
			switch(format){
				case 8: { sampleformat = 1; break; } // unsigned integer
				case 16:{ sampleformat = 2; break; } // signed integer
				case 32:{ sampleformat = 3; break; } // IEEE floating point
			}
			TIFFSetField(output, TIFFTAG_SAMPLEFORMAT, sampleformat);
			TIFFSetField(output, TIFFTAG_SOFTWARE,  "morpheus");
			TIFFSetField(output, TIFFTAG_PLANARCONFIG,  PLANARCONFIG_CONTIG);
			TIFFSetField(output, TIFFTAG_PHOTOMETRIC,   PHOTOMETRIC_MINISBLACK);
			//TIFFSetField(image, TIFFTAG_XRESOLUTION, 1);
			//TIFFSetField(image, TIFFTAG_YRESOLUTION, 1);	
			TIFFSetField(output, TIFFTAG_COMPRESSION,   COMPRESSION_LZW);
			
			// Multipage TIFF
			unsigned short page, numpages_before=0, numpages_after=0;
			TIFFSetField(output, TIFFTAG_SUBFILETYPE, 0);
			
			// Now, write the buffer to TIFF
			for (uint i=0; i<height; i++){
				if(TIFFWriteScanline(output, &buffer8[i * width], i, 0) < 0){
					cerr << "Could not write TIFF output " << endl;
					exit(-1);
				}
			}
			
			TIFFWriteDirectory(output);
		}
	}
	TIFFClose(output);

};

void MembraneRules3D::writeFNTPFPTIFF( string filename ){
	TIFF *image;
	uint32 width, height, slices;
	bool append=false;
	
	append=false;
	if((image = TIFFOpen(filename.c_str(), "w")) == NULL){
		cerr << "Could not open image " << filename << " to write." << endl;
		exit(-1);
	}
	
	// Allocate buffer according to file format
	width 	=	latticeSize.x;
	height	=	latticeSize.y;
	slices	=	latticeSize.z;

	uint8 buffer8[width * height];
	uint16 bpp, spp;
	uint format = 8;
	bpp = 8; // bits per pixel
	spp = 1; // samples per pixel (1 = greyscale, 3 = RGB)
	
	VINT pmin(0,0,0);
	VINT pmax = latticeSize;
	
	VINT pos(0,0,0);
	for (pos.z=pmin.z; pos.z<pmax.z; pos.z++)
	{
		for(uint channel=0; channel<3; channel++)
		{	
			for (pos.y=pmin.y; pos.y<pmax.y; pos.y++)
			{
				for (pos.x=pmin.x; pos.x<pmax.x; pos.x++)
				{
					uint8 value = 0.0;
					switch(channel){
						// RED = False negative
						case 0: value=(output.FN->get(pos) ? 255:0); 	break;
						// GREEN = True positive
						case 1: value=(output.TP->get(pos) ? 255:0); 	break;
						// BLUE = False positive
						case 2: value=(output.FP->get(pos) ? 255:0); 	break;
						default: cout << "unknown channel"; 		break;
					}
					if(value == no_distance)
						value = 0;
					uint bufindex = (pos.x-pmin.x) + (pos.y-pmin.y)*width;
					(uint8&)buffer8[bufindex] = (uint8)value;
				}
			}
				
			// Set TIFF info fields
			TIFFSetField(image, TIFFTAG_IMAGEWIDTH,  width); //x
			TIFFSetField(image, TIFFTAG_IMAGELENGTH, height); //y
			TIFFSetField(image, TIFFTAG_BITSPERSAMPLE, bpp);
			TIFFSetField(image, TIFFTAG_ROWSPERSTRIP, height); //bpp); 
			TIFFSetField(image, TIFFTAG_SAMPLESPERPIXEL, spp);
			uint sampleformat;
			switch(format){
				case 8: { sampleformat = 1; break; } // unsigned integer
				case 16:{ sampleformat = 2; break; } // signed integer
				case 32:{ sampleformat = 3; break; } // IEEE floating point
			}
			TIFFSetField(image, TIFFTAG_SAMPLEFORMAT, sampleformat);
			TIFFSetField(image, TIFFTAG_SOFTWARE,  "morpheus");
			TIFFSetField(image, TIFFTAG_PLANARCONFIG,  PLANARCONFIG_CONTIG);
			TIFFSetField(image, TIFFTAG_PHOTOMETRIC,   PHOTOMETRIC_MINISBLACK);
			//TIFFSetField(image, TIFFTAG_XRESOLUTION, 1);
			//TIFFSetField(image, TIFFTAG_YRESOLUTION, 1);	
			TIFFSetField(image, TIFFTAG_COMPRESSION,   COMPRESSION_LZW);
			
			// Multipage TIFF
			unsigned short page, numpages_before=0, numpages_after=0;
			TIFFSetField(image, TIFFTAG_SUBFILETYPE, 0);
			
			// Now, write the buffer to TIFF
			for (uint i=0; i<height; i++){
				if(TIFFWriteScanline(image, &buffer8[i * width], i, 0) < 0){
					cerr << "Could not write TIFF image " << endl;
					exit(-1);
				}
			}
			
			TIFFWriteDirectory(image);
		}
	}
	TIFFClose(image);

}

void MembraneRules3D::writeMultichannelTIFF(){
	TIFF *image;
	uint32 width, height, slices;
	bool append=false;
	
	append=false;
	stringstream filename;
	filename << "output.tif";
	if((image = TIFFOpen(filename.str().c_str(), "w")) == NULL){
		cerr << "Could not open image " << filename << " to write." << endl;
		exit(-1);
	}
	
	// Allocate buffer according to file format
	width 	=	latticeSize.x;
	height	=	latticeSize.y;
	slices	=	latticeSize.z;
	
	// put fscores of each cell in map
	map< CPM::CELL_ID, double > fscores;
	for(uint i=0; i<population_data.size(); i++){
		fscores.insert( pair< CPM::CELL_ID, double >( population_data[i].cell_id, population_data[i].dscore ) );
	}

	uint32 buffer32[width * height];
	uint16 bpp, spp;
	uint format = 32;
	bpp = 32; // bits per pixel
	spp = 1; // samples per pixel (1 = greyscale, 3 = RGB)
	
	VINT pmin(0,0,0);
	VINT pmax = latticeSize;
	
	VINT pos(0,0,0);
	for (pos.z=pmin.z; pos.z<pmax.z; pos.z++)
	{
		for(uint channel=0; channel<7; channel++)
		{	
			for (pos.y=pmin.y; pos.y<pmax.y; pos.y++)
			{
				for (pos.x=pmin.x; pos.x<pmax.x; pos.x++)
				{
					double value = 0.0;
					switch(channel){
						case 0: value=double(output.basal->get(pos));		break;
						case 1: value=double(output.apical->get(pos)); 		break;
						case 2: value=double(output.apical_segmented->get(pos)); 	break;
						//case 3: value=double( CPM::isBoundary(pos) ? cpm_lattice->get(pos).cell_id : 0 );break; // put cell ID only on membrane
						case 3: value=double( cpm_lattice->get(pos).cell_id );break;
						case 4: value=double( CPM::isBoundary(pos) ? output.posinfo->get(pos) : 0 ); 	break;
						case 5: value=double(output.potential->get(pos)); 	break;
						case 6:{
							value=fscores[cpm_lattice->get(pos).cell_id]; 
							break;
						}
						default: cout << "unknown channel"; 				break;
					}
					if(value == no_distance)
						value = 0;
					uint bufindex = (pos.x-pmin.x) + (pos.y-pmin.y)*width;
					(float&)buffer32[bufindex] = (float)value;
				}
			}
				
			// Set TIFF info fields
			TIFFSetField(image, TIFFTAG_IMAGEWIDTH,  width); //x
			TIFFSetField(image, TIFFTAG_IMAGELENGTH, height); //y
			TIFFSetField(image, TIFFTAG_BITSPERSAMPLE, bpp);
			TIFFSetField(image, TIFFTAG_ROWSPERSTRIP, height); //bpp); 
			TIFFSetField(image, TIFFTAG_SAMPLESPERPIXEL, spp);
			uint sampleformat;
			switch(format){
				case 8: { sampleformat = 1; break; } // unsigned integer
				case 16:{ sampleformat = 2; break; } // signed integer
				case 32:{ sampleformat = 3; break; } // IEEE floating point
			}
			TIFFSetField(image, TIFFTAG_SAMPLEFORMAT, sampleformat);
			TIFFSetField(image, TIFFTAG_SOFTWARE,  "morpheus");
			TIFFSetField(image, TIFFTAG_PLANARCONFIG,  PLANARCONFIG_CONTIG);
			TIFFSetField(image, TIFFTAG_PHOTOMETRIC,   PHOTOMETRIC_MINISBLACK);
			//TIFFSetField(image, TIFFTAG_XRESOLUTION, 1);
			//TIFFSetField(image, TIFFTAG_YRESOLUTION, 1);	
			TIFFSetField(image, TIFFTAG_COMPRESSION,   COMPRESSION_LZW);
			
			// Multipage TIFF
			unsigned short page, numpages_before=0, numpages_after=0;
			TIFFSetField(image, TIFFTAG_SUBFILETYPE, 0);
			
			// Now, write the buffer to TIFF
			for (uint i=0; i<height; i++){
				if(TIFFWriteScanline(image, &buffer32[i * width], i, 0) < 0){
					cerr << "Could not write TIFF image " << endl;
					exit(-1);
				}
			}
			
			TIFFWriteDirectory(image);
		}
	}
	TIFFClose(image);

}

int MembraneRules3D::euclideanDistanceTransform( shared_ptr<Lattice_Data_Layer<double> >& distanceMap, shared_ptr<Lattice_Data_Layer<bool> >&maskMap){
	return euclideanDistanceTransform( distanceMap, maskMap, VINT(0,0,0), distanceMap->size());
}

int MembraneRules3D::euclideanDistanceTransform( shared_ptr<Lattice_Data_Layer<double>>& distanceMap, shared_ptr<Lattice_Data_Layer<bool> >&maskMap, VINT bottomleft, VINT topright){
	
	bool done = false;
	int iterations=0;
	int dir = 0;
	set<int> indices;
	VINT lsize = distanceMap->size();
// 	cout << "dist size " << lsize << " mask size "<< maskMap->size() << endl;
	
		/////// ITERATIVE DISTANCE TRANSFORM //////
	// scan (bounding box) lattice is various directions
	// while, for each point, check the shortest distance in the neighborhood, and copy this + distance to the neighbor
	// terminate when no changes occur.
	while (! done ) {

// 		std::ofstream d;
// 		d.open(string( string("distance_") + to_str(iterations) + string(".log") ).c_str(), ios_base::trunc);
// 		distanceMap->write_ascii(d);
// 		d.close();

		done = true;
		iterations++;
		uint changes = 0;
		// 2^3 = 8 Scanning directions
// // 			F=forward, B=backward
// // 			0: Fx Fy Fz
// // 			1: Bx By Fz
// // 			2: Bx Fy Fz
// // 			3: Fx By Fz
// // 			4: Fx Fy Bz
// // 			5: Bx By Bz
// // 			6: Bx Fy Bz
// // 			7: Fx By Bz
// 			bool x_fwd = (dir == 0 || dir == 3 || dir == 4 || dir == 7);
// 			bool y_fwd = (dir == 0 || dir == 2 || dir == 4 || dir == 6);
// 			bool z_fwd = (dir == 0 || dir == 1 || dir == 2 || dir == 3);

		
		// 0: Fx Fy Fz
		// 1: Bx Fy Fz
		// 2: Fx By Fz
		// 3: Fx Fy Bz
		// 4: Bx By Fz
		// 5: Fx By Bz
		// 6: Bx Fy Bz
		// 7: Bx By Bz
		bool x_fwd = (dir == 0 || dir == 2 || dir == 3 || dir == 5);
		bool y_fwd = (dir == 0 || dir == 1 || dir == 3 || dir == 6);
		bool z_fwd = (dir == 0 || dir == 1 || dir == 2 || dir == 4);
		
		VINT start(	x_fwd ? bottomleft.x : topright.x-1,
					y_fwd ? bottomleft.y : topright.y-1,
					z_fwd ? bottomleft.z : topright.z-1 );
		VINT iter(	x_fwd ? +1 : -1,
					y_fwd ? +1 : -1,
					z_fwd ? +1 : -1 );
		VINT stop(	x_fwd ? topright.x : bottomleft.x-1,
					y_fwd ? topright.y : bottomleft.y-1,
					z_fwd ? topright.z : bottomleft.z-1 );
		VINT pos(0,0,0);
		//cout << "Direction = " << dir << ", initpos = " << init << ", iter = " << iter << endl;
			
		for (pos.z = start.z; pos.z != stop.z; pos.z += iter.z) {
			for (pos.y = start.y; pos.y != stop.y; pos.y += iter.y) {
				pos.x = start.x;
				int idx = maskMap->get_data_index(pos);
				for (; pos.x != stop.x; pos.x += iter.x, idx +=iter.x) {
					if (idx <0 || idx>=maskMap->shadow_size_size_xyz) {
						cout << " Invalid index " << idx << " at " << pos << " max is " << maskMap->shadow_size_size_xyz<< endl;
						cout << " Lattice size " << lsize << " bottom " << bottomleft << " top " << topright << endl;
						assert(0);
					}
					// Skip all points outside of cell in order to compute the geodesic or constrained distance transform
					if( !maskMap->data[idx])
						continue;
					
// 					double min_dist = distanceMap->get( pos );
					double orig_dist = distanceMap->data[idx];
					double min_dist = orig_dist;
					for (uint i=0; i<neighbors.size(); i++) {
						
						double dist = distanceMap->data[distanceMap->get_data_index( pos + neighbors[i] )];
						
						if( dist != no_distance ){
							dist += neighbor_distance[i];
							if( dist < min_dist ){
								min_dist = dist;
							}
						}
					}
					
					if ( min_dist - orig_dist < -10e-6) {
						distanceMap->data[idx] = min_dist;
						indices.insert(idx);
						done = false;
						changes++;
					}
					
				} // end of x loop
			} // end of y loop
		} // end of z loop
		dir = (dir + 1) % 8;
// 		distanceMap->reset_boundaries();

		//cout << "Iterations: " << iterations << ", changes: " << changes << endl;

	}

 	//std::ofstream d;
 	//d.open(string( string("mask_") + to_str(iterations) + string(".log") ).c_str(), ios_base::trunc);
 	//distanceMap->write_ascii(d);
 	//maskMap->write_ascii(d);
 	//d.close();
	return indices.size();
};

MembraneRules3D::CellData& MembraneRules3D::getCellData( CPM::CELL_ID id ){
	// get distance value of membrane node at position in halo (*n) belonging to adjacent cell
	for(uint ii=0; ii < population_data.size(); ii++){
		if(population_data[ii].cell_id == id){
			return population_data[ii];
		}
	}
	return population_data[ population_data.size()-1 ];
};

VINT MembraneRules3D::minVINT(VINT a, VINT b){
	return VINT( (a.x<=b.x ? a.x:b.x), (a.y<=b.y ? a.y:b.y), (a.z<=b.z ? a.z:b.z) );
}

VINT MembraneRules3D::maxVINT(VINT a, VINT b){
	return VINT( (a.x>=b.x ? a.x:b.x), (a.y>=b.y ? a.y:b.y), (a.z>=b.z ? a.z:b.z) );
}

void MembraneRules3D::computeFscore( Fscore& scores ){
	scores.precision 	= double(scores.tp) / double(scores.tp + scores.fp);
	scores.sensitivity	= double(scores.tp) / double(scores.tp + scores.fn);
	scores.fscore		= 2.0*( scores.precision * scores.sensitivity ) / ( scores.precision + scores.sensitivity );
}

// taken from elliptic approximation in cell.cpp
const MembraneRules3D::Tensor MembraneRules3D::getTensor(Cell::Nodes nodes, VDOUBLE cell_center, bool inertiatensor) {	
	
	// inertia tensor
	vector<double> I(6, 0.0);
	vector<double> Q(6, 0.0);
	// 
	VDOUBLE P;
	
	const double numnodes = double( nodes.size() );
	double w = 1.0/numnodes;
	for (Cell::Nodes::const_iterator pt = nodes.begin(); pt != nodes.end(); pt++)
	{
		VDOUBLE delta = lattice3D->to_orth(*pt) - cell_center;
		// NORMALIZE VECTOR (discard cell shape)
		if( !inertiatensor )
			delta = delta.norm();
		I[LC_XX] += delta.y * delta.y + delta.z * delta.z;
		I[LC_YY] += delta.x * delta.x + delta.z * delta.z;
		I[LC_ZZ] += delta.x * delta.x + delta.y * delta.y;
		I[LC_XY] += -delta.x * delta.y;
		I[LC_XZ] += -delta.x * delta.z;
		I[LC_YZ] += -delta.y * delta.z;

// 		I[LC_XX]+=sqr(delta.y)+sqr(delta.z);
// 		I[LC_YY]+=sqr(delta.x)+sqr(delta.z);
// 		I[LC_ZZ]+=sqr(delta.x)+sqr(delta.y);
// 		I[LC_XY]+=-delta.x*delta.y;
// 		I[LC_XZ]+=-delta.x*delta.z;
// 		I[LC_YZ]+=-delta.y*delta.z;

// 		Q[LC_XX] += (delta.x*delta.x) - 1.0/3.0;
// 		Q[LC_YY] += (delta.y*delta.y) - 1.0/3.0;
// 		Q[LC_ZZ] += (delta.z*delta.z) - 1.0/3.0;
// 		Q[LC_XY] += (delta.x * delta.y);
// 		Q[LC_XZ] += (delta.x * delta.z);
// 		Q[LC_YZ] += (delta.y * delta.z);
		
		P.x += delta.x;
		P.y += delta.y;
		P.z += delta.z;
	}
	// calculate average direction
	P.x = P.x / numnodes;
	P.y = P.y / numnodes;
	P.z = P.z / numnodes;
	//P = P.norm();
	//P = P / P.abs();

	Tensor tensor = calcLengthHelper3D(I,numnodes);
	tensor.ave_vector = P; // ave_vector takes average direction from center to point cloud, can be used to orient the axes
	
	// make a best guess to sort the directions
	//  based on the side where most points are (think of simple epithelium)
	for(uint i=0; i<tensor.axes.size(); i++){
		double sign_dotprod = sign( dot(tensor.ave_vector, tensor.axes[i]) );
		tensor.axes[i] = tensor.axes[i] * sign_dotprod;
	}
	tensor.center = cell_center;
	
// 	cout << "getTensor: cell center = " << tensor.center << endl;
// 	cout << "getTensor: " << "\ttensor 1: " << tensor.axes[0].norm() << "\tlength: " << tensor.lengths[0] << "\n";
// 	cout << "getTensor: " << "\ttensor 2: " << tensor.axes[1].norm() << "\tlength: " << tensor.lengths[1] << "\n";
// 	cout << "getTensor: " << "\ttensor 3: " << tensor.axes[2].norm() << "\tlength: " << tensor.lengths[2] << "\n";
// 	cout << "getTensor: " << "\tavedir  : " << tensor.ave_vector << "\tlength: " << tensor.ave_vector.abs() << "\n";
	
	return tensor;
}

inline double MembraneRules3D::sign(double x){
	return (x > 0) - (x < 0);
}

// taken from elliptic approximation in cell.cpp
MembraneRules3D::Tensor MembraneRules3D::calcLengthHelper3D(const std::vector<double> I, int N) const
{
	Tensor t;
	if(N<=1) {
		for (uint i=0; i<3; i++) {
			t.lengths.push_back(N);
			t.axes.push_back(VDOUBLE(0,0,0));
 		}
		return t;
	} // gives nan otherwise
	
	// From of the inertia tensor (principal moments of inertia) we compute the eigenvalues and
	// obtain the cell length by assuming the cell was an ellipsoid
	Eigen::Matrix3f eigen_m;
	eigen_m << I[LC_XX], I[LC_XY], I[LC_XZ],
	           I[LC_XY], I[LC_YY], I[LC_YZ],
	           I[LC_XZ], I[LC_YZ], I[LC_ZZ];
			   
	Eigen::SelfAdjointEigenSolver<Eigen::Matrix3f> eigensolver(eigen_m);
	if (eigensolver.info() != Eigen::Success) {
		cerr << "Cell::calcLengthHelper3D: Computing eigenvalues was not successfull!" << endl;
	}

	Eigen::Vector3f eigen_values = eigensolver.eigenvalues();
	Eigen::Matrix3f EV = eigensolver.eigenvectors();
	Eigen::Matrix3f Am;
	Am << -1,  1,  1,
	       1, -1,  1,
	       1,  1, -1;
	Eigen::Array3f axis_lengths = ((Am * eigen_values).array() * (2.5/double(N))).sqrt();
	//Eigen::Vector3i sorted_indices;
	for (uint i=0; i<3; i++) {
		t.lengths.push_back(axis_lengths(i));
		t.axes.push_back(VDOUBLE(EV(0,i),EV(1,i),EV(2,i)).norm());
	}
	// sorting axes by length
	bool done=false;
	while (!done) {
		for (uint i=0;1;i++) {
			if (t.lengths[i] < t.lengths[i+1]) {
				swap(t.lengths[i],t.lengths[i+1]);
				swap(t.axes[i],t.axes[i+1]);
			}
			if (i==2) {
				done=true;
				break;
			}
		}
	} 
	return t;
}

/*
void MembraneRules3D::gaussianBlur3D(CellData& cd){
	
	VINT bbmin = cd.bbox.minimum;
	VINT bbmax = cd.bbox.maximum;
	shared_ptr<Lattice_Data_Layer<double>> tmp1	= createBoxDouble( cd.bbox.size, no_distance );
	shared_ptr<Lattice_Data_Layer<double>> tmp2	= createBoxDouble( cd.bbox.size, no_distance );

	
	VINT pos;
	double sum;
	
	// Z -direction
	for(pos.z = bbmin.z; pos.z < bbmax.z; pos.z++){
		for(pos.y = bbmin.y; pos.y < bbmax.y; pos.y++){
			for(pos.x = bbmin.x; pos.x < bbmax.x; pos.x++){
				sum = 0.0;
				for(int i=-2; i<=2; i++){
					VINT position = pos; 
					position.z = pos.z + i;
					position.z = (position.z < 0 ? -position.z-1 : (position.z >= bbmax.z ? 2*bbmax.z - position.z - 1 : position.z));
					if( cd.membrane_data.count( position ) ) 
						sum += gKernel[i+2]*cd.membrane_data[ position ].potential;
				}
				tmp1->set(pos-bbmin, sum);
			}
		}
	}

	// Y -direction
	for(pos.z = bbmin.z; pos.z < bbmax.z; pos.z++){
		for(pos.y = bbmin.y; pos.y < bbmax.y; pos.y++){
			for(pos.x = bbmin.x; pos.x < bbmax.x; pos.x++){
				sum = 0.0;
				for(int i=-2; i<=2; i++){
					VINT position = pos; 
					position.y = pos.y + i;
					position.y = (position.y < 0 ? -position.y-1 : (position.y >= bbmax.y ? 2*bbmax.y - position.y - 1 : position.y));
					sum += gKernel[i+2]*tmp1->get(position-bbmin);
				}
				tmp2->set(pos-bbmin, sum);
			}
		}
	}
	
	// X -direction
	for(pos.z = bbmin.z; pos.z < bbmax.z; pos.z++){
		for(pos.y = bbmin.y; pos.y < bbmax.y; pos.y++){
			for(pos.x = bbmin.x; pos.x < bbmax.x; pos.x++){
				sum = 0.0;
				for(int i=-2; i<=2; i++){
					VINT position = pos; 
					position.x = pos.x + i;
					position.x = (position.x < 0 ? -position.x-1 : (position.x >= bbmax.x ? 2*bbmax.x - position.x - 1 : position.x));
					sum += gKernel[i+2]*tmp2->get(position-bbmin);
				}
				// only set blurred value on membrane
				if(cd.box3->get(pos-bbmin) == true){
					cd.membrane_data[ pos ].potential = sum;
				}
			}
		}
	}
	
	
	
}
void MembraneRules3D::createFilter(double gKernel[5], double sigma){
    // set standard deviation to 1.0
    //double sigma = 1.0;
    double r, s = 2.0 * sigma * sigma;
 
    // sum is for normalization
    double sum = 0.0;
 
    // generate 1x5 kernel
    for (int x = -2; x <= 2; x++)
    {
		r = sqrt(x*x);
		gKernel[x + 2] = (exp(-(r*r)/s))/(M_PI * s);
		sum += gKernel[x + 2];
    }
 
    // normalize the Kernel
    for(int i = 0; i < 5; ++i)
		gKernel[i] /= sum;
 
} 
*/
	
