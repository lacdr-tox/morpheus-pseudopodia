#include "membraneRules3D.h"
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
	
	// basal_celltype
    getXMLAttribute ( xNode, "basal_celltype", basal_celltype_str);
	
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
	output_symbol_strs.resize(4);
	getXMLAttribute ( xNode, "Output_basal/symbol-ref", output_symbol_strs[0]);
	getXMLAttribute ( xNode, "Output_distance/symbol-ref", output_symbol_strs[1]);
	getXMLAttribute ( xNode, "Output_potential/symbol-ref", output_symbol_strs[2]);
	getXMLAttribute ( xNode, "Output_apical/symbol-ref", output_symbol_strs[3]);
	
}

void MembraneRules3D::init ( CellType* ct ) {
	TimeStepListener::init();
	celltype = ct;
	
	cpm_lattice		= CPM::getLayer();
	lattice3D		= SIM::getLattice();
	latticeSize		= lattice3D->size();
	neighbor_sites	= lattice3D->getNeighborhood( 1 ); // used for getHALO
	
	
	// find celltypes and symbols
	basal_celltype = CPM::findCellType(basal_celltype_str);

	
	// precompute distances of the neighbors within the cubic lattice
	neighbors = lattice3D->getNeighborhoodByOrder(2);
	neighbor_distance.resize( neighbors.size() );
	for (uint i=0; i<neighbors.size(); i++) {
		neighbor_distance[i] = VDOUBLE(neighbors[i]).abs();
	}
	
	output.cellids		= createBoxDouble( latticeSize, 0.0 );
// 	output.cellids->data= 0; 
	output.basal		= createBoxDouble( latticeSize, 0.0 );
// 	output.basal->data	= 0;
	output.posinfo		= createBoxDouble( latticeSize, 0.0 );
// 	output.posinfo->data= 0;
	output.potential	= createBoxDouble( latticeSize, 0.0 );
// 	output.potential->data = 0;
	output.apical 		= createBoxDouble( latticeSize, 0.0 );
// 	output.apical->data	= 0;
	
	
	// output to membraneProperty
	output_membranes.resize( output_symbol_strs.size() );
	for(uint i=0; i<output_symbol_strs.size(); i++){
		cout << "Output symbol: " << output_symbol_strs[i] << "..." << endl;
		output_membranes[i] = ct->findMembrane ( output_symbol_strs[i], true );
	}
		
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
		cd.box  = createBoxDouble( cd.bbox.size, 0.0 );
		cd.box2 = createBoxDouble( cd.bbox.size, 0.0 );
		cd.box3 = createBoxDouble( cd.bbox.size, 0.0 );
		
		// initialize membrane and position data
		const Cell::Nodes& membrane = CPM::getCell ( cells[c] ).getSurface();
		for ( Cell::Nodes::const_iterator m = membrane.begin(); m != membrane.end(); ++m ) {
			MemPosData pd;
			pd.apical 		= false;
			pd.basal 		= false;
			pd.distance 	= no_distance;
			pd.halo 		= getHalo( *m, cells[c] );
			cd.membrane_data.insert( pair< VINT, MemPosData >(*m, pd) ); 
			// membrane mask
			cd.box3->set( *m - cd.bbox.minimum, 1.0 );
			
			VINT m_l = *m;
			if (m_l.x == 72) cout << m_l.x << endl;
			if (m_l.x < cd.bbox.minimum.x || m_l.y < cd.bbox.minimum.y || m_l.z < cd.bbox.minimum.z ||  m_l.x >= cd.bbox.maximum.x||  m_l.y >= cd.bbox.maximum.y||  m_l.z >= cd.bbox.maximum.z) {
				cout << m_l << "\tcd.bbox.minimum: " << cd.bbox.minimum << "\tcd.bbox.maximum: " << cd.bbox.maximum << endl;
				assert(0);
			}
		}
		

		population_data[ c ] = cd;
	}
		
	
////////////////////////////////
//  
//	Registration of basal contacts
//
////////////////////////////////
	
	
#pragma omp parallel for
	for(uint i=0; i<population_data.size(); i++){
		CellData& cd = population_data[i];
		
		cout << "Registering basal contacts for cell " << cd.cell_id << endl;
		CPM::CELL_ID cell_id = cd.cell_id;
		
		for ( map< VINT, MemPosData >::iterator i=cd.membrane_data.begin(); i != cd.membrane_data.end(); ++i ) {
			// count the halo nodes that are of basal celltype
			double basal_contacts = 0;
			for ( Cell::Nodes::const_iterator j= i->second.halo.begin(); j != i->second.halo.end(); ++j ) {
				if( 	CPM::cellExists(  cpm_lattice->get(*j).cell_id) 
					&& (CPM::getCellIndex(cpm_lattice->get(*j).cell_id).celltype == basal_celltype->getID() ) )
					basal_contacts++;
			}
			// set mempos=basal if half or more of the halo nodes are of basal celltype
			bool basal = ( (basal_contacts / i->second.halo.size()) > 0.0 );
			i->second.basal = basal;
				
			cd.box->set( i->first - cd.bbox.minimum, (basal ? 1 : 0));
			output.basal->set( i->first, basal);
		
			VINT m_l = i->first;
			if (m_l.x == 72) cout << m_l.x << endl;
			if (m_l.x < cd.bbox.minimum.x || m_l.y < cd.bbox.minimum.y || m_l.z < cd.bbox.minimum.z ||  m_l.x >= cd.bbox.maximum.x||  m_l.y >= cd.bbox.maximum.y||  m_l.z >= cd.bbox.maximum.z) {
				cout << m_l << "\tcd.bbox.minimum: " << cd.bbox.minimum << "\tcd.bbox.maximum: " << cd.bbox.maximum << endl;
				assert(0);
			}
		}
		
		writeTIFF( "basal", cd.box, cell_id );
	}
	
////////////////////////////////
//  
// //	Compute distance from contacts based on 3D Euclidean Distance Transform
//
////////////////////////////////
	
	
if( distance_mode != MembraneRules3D::NONE){

#pragma omp parallel for
	for(uint i=0; i<population_data.size(); i++){
		CellData& cd = population_data[i];
		CPM::CELL_ID cell_id = cd.cell_id;
		cout << "Calculating distance from basal for cell " << cell_id << endl;
		VINT origin = cd.bbox.minimum;

		shared_ptr< Lattice_Data_Layer<double> > distanceMap;
		shared_ptr< Lattice_Data_Layer<double> >   maskMap;
#pragma omp critical
{		
		// create temporary data containers
		distanceMap	= createBoxDouble( cd.bbox.size,  no_distance);
		maskMap 	= createBoxDouble( cd.bbox.size, 0.0);
}

		switch( distance_mode ){
			case MembraneRules3D::CYTOSOL:
			{
				const Cell::Nodes& cytosol = CPM::getCell ( cell_id ).getNodes();
				for ( Cell::Nodes::const_iterator n = cytosol.begin(); n != cytosol.end(); ++n ) 
					maskMap->set( *n - origin, 1.0 );
				break;
			}
			case MembraneRules3D::MEMBRANE:
			{
				const Cell::Nodes& membrane = CPM::getCell ( cell_id ).getSurface();
				for ( Cell::Nodes::const_iterator n = membrane.begin(); n != membrane.end(); ++n ) 
					maskMap->set( *n - origin, 1.0 );
				break;
			}
			default:{
				cerr << "MembraneRules3D: Unknown mode: select 'cytosol' or 'membrane'." << endl;
				exit(-1);
				break;
			}
		}
		
		// initialize distances: all to no_distance, contacting nodes to 0.
		
		for ( map< VINT, MemPosData >::iterator i=cd.membrane_data.begin(); i != cd.membrane_data.end(); ++i ) {
			if( i->second.basal ){
				distanceMap->set( i->first - origin, 0.0);
// 				maskMap->set( i->first - origin, 1.0 );
			}
		}
// 		distanceMap->reset_boundaries();
		
		//////  COMPUTE DISTANCES  //////
		
		euclideanDistanceTransform( distanceMap, maskMap );
		
		//write map to data containers
		for ( map< VINT, MemPosData >::iterator it=cd.membrane_data.begin(); it != cd.membrane_data.end(); ++it ) {
			it->second.distance = distanceMap->get( it->first - origin );
			output.posinfo->set( it->first, distanceMap->get( it->first - origin ));

			VINT m_l = it->first;
			if (m_l.x == 72) cout << m_l.x << endl;
			if (m_l.x < cd.bbox.minimum.x || m_l.y < cd.bbox.minimum.y || m_l.z < cd.bbox.minimum.z ||  m_l.x >= cd.bbox.maximum.x||  m_l.y >= cd.bbox.maximum.y||  m_l.z >= cd.bbox.maximum.z) {
				cout << m_l << "\tcd.bbox.minimum: " << cd.bbox.minimum << "\tcd.bbox.maximum: " << cd.bbox.maximum << endl;
				assert(0);
			}

		}
		writeTIFF("distance", distanceMap, cell_id );
	}
}

////////////////////////////////
//  
// 		Intercellular communication (geometric mean)
//
////////////////////////////////

	if( communication_mode == MembraneRules3D::COMMUNICATION ){
		
#pragma omp parallel for
		for(uint i=0; i<population_data.size(); i++){
			CellData& cd = population_data[i];
			CPM::CELL_ID cell_id = cd.cell_id;
			cout << "Calculating intercellular potential for cell " << cell_id << endl;
			
			// for every membrane node, get average of distance values in adjacent cell
			
			for ( map< VINT, MemPosData >::iterator it=cd.membrane_data.begin(); it != cd.membrane_data.end(); ++it ) {
				
				double nb_distance = 0.0;
				uint nb_count=0;
				Cell::Nodes& halo = it->second.halo;
				for ( Cell::Nodes::const_iterator n = halo.begin(); n != halo.end(); ++n ){
					
					// get cell id of adjacent cell 
					CPM::CELL_ID nb_cell_id = cpm_lattice->get( *n ).cell_id;
					
					// check if adjacent cell is of my own cell type 
					uint nb_celltype_id = CPM::getCell( nb_cell_id ).getCellType()->getID();
					if ( nb_celltype_id  ==  celltype->getID() ) {
						if (getCellData( nb_cell_id ).membrane_data.count(*n)) {
							nb_distance += getCellData( nb_cell_id ).membrane_data[*n].distance;
							nb_count++;
						}
					}
				}
				
				// average over the sites in halo for this membrane node
				nb_distance /= double(nb_count);
				
				// take geometric mean over own distance and neighbors distance
				double geom_mean = sqrt( it->second.distance * nb_distance );
				it->second.potential = geom_mean;

				VINT m_l = it->first;
				if (m_l.x == 72) cout << m_l.x << endl;
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
			writeTIFF("potential", cd.box, cell_id );
		}
	}	
////////////////////////////////
//  
// 		Local thresholding
//
////////////////////////////////

if( max_distance > 0 ){
	
		// initialize a number of computeLattices, equal to the number of threads
		int numthreads = 1;
// #pragma omp parallel
// {
// 			numthreads = omp_get_num_threads();
// }
// 		distanceMaps.resize( numthreads );
// 		maskMaps.resize( numthreads );
// 
// 		for(uint i=0; i<numthreads; i++){
// 			distanceMaps[i] = createBoxDouble( latticeSize );
// 			maskMaps[i] = createBoxDouble( latticeSize );
// 		}
	
	uint count_cell=0;
	#pragma omp parallel for shared(count_cell)
		for(uint i=0; i<population_data.size(); i++){
			CellData& cd = population_data[i];
			CPM::CELL_ID cell_id = cd.cell_id;
			cout << "Local thresholding for cell " << cell_id << endl;
			VINT origin = cd.bbox.minimum;

			int thread = omp_get_thread_num();
			struct timeval t1, t2; double elapsedTime;
			gettimeofday( &t1, NULL);
			
			// for every membrane node
			//bool apical = false;
			double apical = 0.0;
			uint count=0; uint total = cd.membrane_data.size();
			for ( map< VINT, MemPosData >::iterator it=cd.membrane_data.begin(); it != cd.membrane_data.end(); it++ ) {
				
// 				if(count % 100 == 0)
// 					cout << count << " / " << total << " : " << 100.0 * double(count)/double(total) << " %" << endl;
// 				count++;
				//bool apical = localThresholding1( i->first, cell_id );
				//bool apical = localThresholding2( i->first, cell_id );
				//bool apical = localThresholding3( it->first, cell_id, cd, thread);
				VINT m_l =  it->first;
				if (m_l.x < cd.bbox.minimum.x || m_l.y < cd.bbox.minimum.y || m_l.z < cd.bbox.minimum.z ||  m_l.x >= cd.bbox.maximum.x||  m_l.y >= cd.bbox.maximum.y||  m_l.z >= cd.bbox.maximum.z) {
					cout << m_l << "\tcd.bbox.minimum: " << cd.bbox.minimum << "\tcd.bbox.maximum: " << cd.bbox.maximum << endl;
					assert(0);
				}
				apical = localThresholding4( it->first, cell_id, cd);
				it->second.apical = apical;
				output.apical->set( it->first, apical);

			}
			gettimeofday( &t2, NULL);
			elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
			elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms
			
			// copy data to maps
			cd.box->data = 0.0;
			for ( map< VINT, MemPosData >::iterator it=cd.membrane_data.begin(); it != cd.membrane_data.end(); ++it ) {
				cd.box->set( it->first - cd.bbox.minimum, it->second.apical );
			}

			writeTIFF( "apical", cd.box, cell_id );
			 
			cout <<"Cell no. " << ++count_cell << " / " << population_data.size() << ", " << 100.0 * double(count_cell)/double(population_data.size()) << "%\tcell id: " << cell_id << "\tmemsize: "<< total << "\texectime: " << elapsedTime << " ms." << endl;
		}
}		
////////////////////////////////
//  
// 		Write OUTPUT 
//
////////////////////////////////

	shared_ptr<MembraneMapper> mapper_basal 	= shared_ptr <MembraneMapper >( new MembraneMapper(MembraneMapper::MAP_BOOLEAN, false) );
	shared_ptr<MembraneMapper> mapper_distance 	= shared_ptr <MembraneMapper >( new MembraneMapper(MembraneMapper::MAP_CONTINUOUS, false) );
	shared_ptr<MembraneMapper> mapper_potential	= shared_ptr <MembraneMapper >( new MembraneMapper(MembraneMapper::MAP_CONTINUOUS, false) );
	shared_ptr<MembraneMapper> mapper_apical	= shared_ptr <MembraneMapper >( new MembraneMapper(MembraneMapper::MAP_CONTINUOUS, false) );
	
	
	for(uint i=0; i<population_data.size(); i++){
		CellData cd = population_data[i];
		mapper_basal->attachToCell(cd.cell_id);
		mapper_distance->attachToCell(cd.cell_id);
		mapper_potential->attachToCell(cd.cell_id);
		mapper_apical->attachToCell(cd.cell_id);
		for ( map< VINT, MemPosData >::iterator it=cd.membrane_data.begin(); it != cd.membrane_data.end(); ++it ) {
			mapper_basal->map		(it->first, it->second.basal);
			mapper_distance->map	(it->first, (it->second.distance == no_distance ? 0 : it->second.distance ));
			mapper_potential->map	(it->first, it->second.potential );
			mapper_apical->map		(it->first, it->second.apical);
		}
		mapper_basal->fillGaps();
		mapper_distance->fillGaps();
		mapper_potential->fillGaps();
		mapper_apical->fillGaps();
		mapper_basal->copyData( output_membranes[0].getMembrane( cd.cell_id ) );
		mapper_distance->copyData( output_membranes[1].getMembrane( cd.cell_id ) );
		mapper_potential->copyData( output_membranes[2].getMembrane( cd.cell_id ) );
		mapper_apical->copyData( output_membranes[3].getMembrane( cd.cell_id ) );
	}

	writeMultichannelTIFF(); 

}

double MembraneRules3D::localThresholding4( VINT m_l, CPM::CELL_ID cell_id, CellData& cd){
	
	if (m_l.x < cd.bbox.minimum.x || m_l.y < cd.bbox.minimum.y || m_l.z < cd.bbox.minimum.z ||  m_l.x >= cd.bbox.maximum.x||  m_l.y >= cd.bbox.maximum.y||  m_l.z >= cd.bbox.maximum.z) {
		cout << m_l << "\tcd.bbox.minimum: " << cd.bbox.minimum << "\tcd.bbox.maximum: " << cd.bbox.maximum << endl;
		assert(0);
	}

	// global Lattice coordinates
	uint dist = uint(ceil(max_distance));
	VINT bbmin(m_l.x-dist, m_l.y-dist, m_l.z-dist);
	VINT bbmax(m_l.x+dist, m_l.y+dist, m_l.z+dist);
	bbmin = maxVINT(bbmin, VINT(0,0,0));
	bbmax = minVINT(bbmax, latticeSize);
	bbmin = maxVINT(bbmin, cd.bbox.minimum);
	bbmax = minVINT(bbmax, cd.bbox.maximum);
	VINT bbsize = bbmax - bbmin;
	
	VINT origin = bbmin;

	cd.box->data = no_distance;
	cd.box2->data = 0;

	// initialize distance
	cd.box->set(m_l - cd.bbox.minimum, 0);
	
/*	// initialize mask
	VINT pos; // in full 3D space
	for(pos.z = bbmin.z; pos.z < bbmax.z; pos.z++){
		for(pos.y = bbmin.y; pos.y < bbmax.y; pos.y++){
			for(pos.x = bbmin.x; pos.x < bbmax.x; pos.x++){
				//cout << "init: " << pos << " pos+origin: " << pos+origin << endl;
				if( cpm_lattice->get(pos).cell_id == cell_id ){
					if( CPM::isBoundary(pos) ){
						cd.box2->set(pos-cd.bbox.minimum, 1);
						//cout << "mask: " << pos << " pos+origin: " << pos+origin << endl;
					}
				}
			}
		}
	}
*/
	
// 	cout << "box origin: "<< cd.bbox.minimum << "\t bbmin: " << bbmin << endl;
	euclideanDistanceTransform(cd.box, cd.box3, bbmin-cd.bbox.minimum, bbmax-cd.bbox.minimum);
	
// 	if(cell_id == 576){
// 		writeTIFF( "distance_node", cd.box, cell_id );
// 		exit(-1);
// 	}
	double local_mean  = 0.0; 
	double local_count = 0.0;
	VINT pos;
	for(pos.z = bbmin.z; pos.z < bbmax.z; pos.z++){
		for(pos.y = bbmin.y; pos.y < bbmax.y; pos.y++){
			for(pos.x = bbmin.x; pos.x < bbmax.x; pos.x++){
				
				if( cd.box3->get(pos - cd.bbox.minimum) == false )
					continue;
									
				double dist = cd.box->get(pos - cd.bbox.minimum);
				if( dist <= max_distance ){
					//cout << "< maxdist: " << pos << " pos+origin: " << pos+origin << endl;
					double value = 0.0;
					switch( communication_mode ){
						case MembraneRules3D::CELLAUTONOMOUS:{
							value = cd.membrane_data[ pos ].distance;
							break;
						}
						case MembraneRules3D::COMMUNICATION:{
							value = cd.membrane_data[ pos ].potential;
							break;
						}
					}
					
					bool weighted=false;
					if(weighted){
						double weight = exp(- dist / (max_distance/3.0));
						local_mean  += value * weight;
						local_count += weight;
					}
					else{
						local_mean  += value;
						local_count += 1.0;
					}

				}
			}
		}
	}
	if(local_count > 0)
		local_mean /= local_count;
	
	// ==== 6. do thresholding based on local mean
	double my_value;
	switch( communication_mode ){
		case MembraneRules3D::CELLAUTONOMOUS:{
			my_value = cd.membrane_data[ m_l ].distance;
			break;
		}
		case MembraneRules3D::COMMUNICATION:{
			my_value = cd.membrane_data[ m_l ].potential;
			break;
		}
		default: {
			cerr << "Unknown method for intercellular communication." << endl;
			break;
		}
	}
	//cout << "id: " << cd.cell_id << ", VINT: " << m_l << ", local_mean: " << local_mean << "\n";
	return ((my_value - threshold) > local_mean);
}

bool MembraneRules3D::localThresholding3( VINT m, CPM::CELL_ID cell_id, CellData& cd, int thread){
	
	uint dist = uint(ceil(max_distance));
	VINT bbbmin(m.x-dist, m.y-dist, m.z-dist);
	VINT bbbmax(m.x+dist, m.y+dist, m.z+dist);
	VINT bbmin = maxVINT(bbbmin, VINT(0,0,0));
	VINT bbmax = minVINT(bbbmax, latticeSize);
	
// 	if( bbbmin != bbmin )
// 		cout << "bbbmin: " << bbbmin << ", bbmin: " << bbmin << endl;
// 	if( bbbmax != bbmax )
// 		cout << "bbbmax: " << bbbmax << ", bbmax: " << bbmax << endl;
	
	distanceMaps[thread]->data = no_distance;
	maskMaps[thread]->data = 0;

	// initialize distance
	distanceMaps[thread]->set(m, 0);
	
	// initialize mask
	VINT pos = bbmin;
	for(pos.z = bbmin.z; pos.z < bbmax.z; pos.z++){
		for(pos.y = bbmin.y; pos.y < bbmax.y; pos.y++){
			for(pos.x = bbmin.x; pos.x < bbmax.x; pos.x++){
				
				if( cpm_lattice->get(pos).cell_id == cell_id ){
					if( CPM::isBoundary(pos) ){
						maskMaps[thread]->set(pos, 1);
					}
				}
			}
		}
	}
	
	euclideanDistanceTransform(distanceMaps[thread], maskMaps[thread], bbmin, bbmax);
	
	//writeTIFF( "distance_node", distanceMaps[thread], cell_id );
	
	double local_mean  = 0.0; 
	double local_count = 0.0;
	pos = bbmin;
	for(pos.z = bbmin.z; pos.z < bbmax.z; pos.z++){
		for(pos.y = bbmin.y; pos.y < bbmax.y; pos.y++){
			for(pos.x = bbmin.x; pos.x < bbmax.x; pos.x++){
				double dist = distanceMaps[thread]->get(pos);
				if( dist < max_distance ){
					
					double value = 0.0;
					switch( communication_mode ){
						case MembraneRules3D::CELLAUTONOMOUS:{
							value = cd.membrane_data[ pos ].distance;
							break;
						}
						case MembraneRules3D::COMMUNICATION:{
							value = cd.membrane_data[ pos ].potential;
							break;
						}
					}
					
					bool weighted=false;
					if(weighted){
						double weight = exp(- dist / (max_distance/3.0));
						local_mean += value * weight;
						local_count += weight;
					}
					else{
						local_mean += value;
						local_count++;
					}

				}
			}
		}
	}
	local_mean /= local_count;
	
	// ==== 6. do thresholding based on local mean
	double my_value;
	switch( communication_mode ){
		case MembraneRules3D::CELLAUTONOMOUS:{
			my_value = cd.membrane_data[m].distance;
			break;
		}
		case MembraneRules3D::COMMUNICATION:{
			my_value = cd.membrane_data[m].potential;
			break;
		}
		default: {
			cerr << "Unknown method for intercellular communication." << endl;
			break;
		}
	}
	return ((my_value - threshold) > local_mean);
}

bool MembraneRules3D::localThresholding2( VINT m, CPM::CELL_ID cell_id ){
	
	// ==== 1. create distance and mask latticeSize
	BoundingBox bb = getBoundingBoxNode(m, cell_id);
	shared_ptr<Lattice_Data_Layer<double>> distanceMap	= createBoxDouble( bb.size, no_distance );
	shared_ptr<Lattice_Data_Layer<double>> maskMap 		= createBoxDouble( bb.size, false );
	
	// ==== 2. initialize distances and mask 
// 	distanceMap->data = no_distance;
// 	distanceMap->set_boundary_value(Boundary::mx,no_distance);
// 	distanceMap->set_boundary_value(Boundary::px,no_distance);
// 	distanceMap->set_boundary_value(Boundary::my,no_distance);
// 	distanceMap->set_boundary_value(Boundary::py,no_distance);
// 	distanceMap->set_boundary_value(Boundary::mz,no_distance);
// 	distanceMap->set_boundary_value(Boundary::pz,no_distance);
	distanceMap->set( m-bb.minimum, 0 );
	
	
	maskMap->data = false;
	const Cell::Nodes& membrane = CPM::getCell ( cell_id ).getSurface();
	for ( Cell::Nodes::const_iterator n = membrane.begin(); n != membrane.end(); ++n ){
		if( n->x < bb.maximum.x && n->y < bb.maximum.y && n->z < bb.maximum.z 
			&&  n->x > bb.minimum.x && n->y > bb.minimum.y && n->z > bb.minimum.z )
			maskMap->set( *n - bb.minimum, true );
	}

	// ==== 3. compute 3D EDT
	euclideanDistanceTransform(distanceMap, maskMap);
	
	// ==== 4. get positions of all neighboring nodes with distance < max_distance
	// ==== 5. compute local mean

	double local_mean=0.0; 
	uint local_count=0;
	VINT pos(0,0,0);
	for(pos.z = 0; pos.z < bb.size.z; pos.z++){
		for(pos.y = 0; pos.y < bb.size.y; pos.y++){
			for(pos.x = 0; pos.x < bb.size.x; pos.x++){
				if( distanceMap->get(pos) < max_distance ){
					local_mean += population_data[cell_id].membrane_data[ pos ].distance;
					local_count++;
				}
			}
		}
	}
	local_mean /= local_count;
	
	// ==== 6. do thresholding based on local mean
	CellData& cd = getCellData( cell_id );
	double my_value;
	switch( communication_mode ){
		case MembraneRules3D::CELLAUTONOMOUS:{
			my_value = cd.membrane_data[m].distance;
			break;
		}
		case MembraneRules3D::COMMUNICATION:{
			my_value = cd.membrane_data[m].potential;
			break;
		}
		default: {
			cerr << "Unknown method for intercellular communication." << endl;
			break;
		}
	}
	return (my_value - threshold > local_mean);
}

bool MembraneRules3D::localThresholding1( VINT m, CPM::CELL_ID cell_id ){

	CellData& cd = getCellData( cell_id );
	double local_mean = 0.0;
	uint local_size=0;
	
	const Cell::Nodes& membrane = CPM::getCell ( cell_id ).getSurface();
	for ( Cell::Nodes::const_iterator n = membrane.begin(); n != membrane.end(); ++n ){
		if( sqrt( sqr( n->x - m.x ) + sqr( n->y - m.y ) + sqr( n->z - m.z ) ) < max_distance ){
			double value = 0.0;
			
			switch( communication_mode ){
				case MembraneRules3D::CELLAUTONOMOUS:{
					value = cd.membrane_data[ *n ].distance;
					break;
				}
				case MembraneRules3D::COMMUNICATION:{
					value = cd.membrane_data[ *n ].potential;
					break;
				}
				
			}
			local_mean += value;
			local_size ++;
		}
	}
	
	local_mean /= local_size;
	
	double my_value;
	switch( communication_mode ){
		case MembraneRules3D::CELLAUTONOMOUS:{
			my_value = cd.membrane_data[m].distance;
			break;
		}
		case MembraneRules3D::COMMUNICATION:{
			my_value = cd.membrane_data[m].potential;
			break;
		}
		default: {
			cerr << "Unknown method for intercellular communication." << endl;
			break;
		}
	}
	
	
	bool apical = false;
	if( my_value - threshold > local_mean )
		apical = true;
	else
		apical = false;
	
	//cout << my_value << "\t > local mean: " << local_mean << " apical? " << (apical ? "yes":"no") << "\n";
	return apical;
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
	
	uint dist = uint(ceil(max_distance));
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
	return bb;
}
/*
shared_ptr< Lattice_Data_Layer< bool > > MembraneRules3D::createBoxBool( VINT boxsize ){
	
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
	
	shared_ptr<const Lattice> latticeBox = shared_ptr<const Lattice>(new Cubic_Lattice(xLattice));
	return shared_ptr< Lattice_Data_Layer<bool> >(new Lattice_Data_Layer<bool>(latticeBox, 0, false));
	//shared_ptr<PDE_Layer>(new PDE_Layer(latticeBox,false));
}

shared_ptr< Lattice_Data_Layer< uint > > MembraneRules3D::createBoxInteger( VINT boxsize ){
	
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
	
	shared_ptr<const Lattice> latticeBox = shared_ptr<const Lattice>(new Cubic_Lattice(xLattice));
	return shared_ptr< Lattice_Data_Layer<uint> >(new Lattice_Data_Layer<uint>(latticeBox, 0, 0));
}
	*/
	
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
	
	shared_ptr<const Lattice> latticeBox = shared_ptr<const Lattice>(new Cubic_Lattice(xLattice));
	return shared_ptr< Lattice_Data_Layer<double> >(new Lattice_Data_Layer<double>(latticeBox, 2, default_value));
}
	

	
Cell::Nodes MembraneRules3D::getHalo( VINT mempos, CPM::CELL_ID id){
	Cell::Nodes halo;
	for ( int i = 0; i < neighbor_sites.size(); ++i ) {
		VINT nbpos = mempos + neighbor_sites[i];
		const CPM::STATE& nb_spin = cpm_lattice->get ( nbpos );

		if ( id != nb_spin.cell_id ) { // if neighbor is different from me
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
	if((output = TIFFOpen(filename.str().c_str(), "w")) == NULL){
		cerr << "Could not open image " << filename << " to write." << endl;
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
		for(uint channel=0; channel<5; channel++)
		{	
			for (pos.y=pmin.y; pos.y<pmax.y; pos.y++)
			{
				for (pos.x=pmin.x; pos.x<pmax.x; pos.x++)
				{
					double value = 0.0;
					switch(channel){
						case 0: value=double(output.basal->get(pos));		break;
						case 1: value=double(output.apical->get(pos)); 		break;
						case 2: value=double(cpm_lattice->get(pos).cell_id);break;
						case 3: value=double(output.posinfo->get(pos)); 	break;
						case 4: value=double(output.potential->get(pos)); 	break;
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


void MembraneRules3D::euclideanDistanceTransform( shared_ptr<Lattice_Data_Layer<double> >& distanceMap, shared_ptr<Lattice_Data_Layer<double> >&maskMap){
	euclideanDistanceTransform( distanceMap, maskMap, VINT(0,0,0), distanceMap->size());
}

void MembraneRules3D::euclideanDistanceTransform( shared_ptr<Lattice_Data_Layer<double>>& distanceMap, shared_ptr<Lattice_Data_Layer<double> >&maskMap, VINT bottomleft, VINT topright){
	
	bool done = false;
	int iterations=0;
	int dir = 0;
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

// 	std::ofstream d;
// 	d.open(string( string("mask_") + to_str(iterations) + string(".log") ).c_str(), ios_base::trunc);
// 	//distanceMap->write_ascii(d);
// 	maskMap->write_ascii(d);
// 	d.close();

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
