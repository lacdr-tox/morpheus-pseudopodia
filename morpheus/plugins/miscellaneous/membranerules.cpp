#include "membranerules.h"


REGISTER_PLUGIN ( MembraneRules );

MembraneRules::MembraneRules() : TimeStepListener(ScheduleType::REPORTER) {}


void MembraneRules::computeTimeStep(){
	
	TimeStepListener::computeTimeStep();
	vector<CPM::CELL_ID> cells = celltype -> getCellIDs();

	if(SIM::getTime() == 0.0)
		return;

#pragma omp parallel for
	for(uint c=0; c < cells.size(); c++)
	{
		uint cell_id = cells[c];
	
		switch( reporter.operation ){

			case Reporter::DISTANCE_TRANSFORM: // calculate Euclidean Distance Transform (EDT) from (boolean) input image
			{
				// assumed to contain boolean mask
 				PDE_Layer* input = reporter.input_membranes[0].getMembrane(cell_id);
				// will contain shortest distances to nonzero pixels in input image
 				PDE_Layer* output = reporter.output_membrane.getMembrane(cell_id);

				shared_ptr<MembraneMapper> mapper = shared_ptr <MembraneMapper >( new MembraneMapper(MembraneMapper::MAP_DISTANCE_TRANSFORM, reporter.deformed_sphere) );
				
				mapper->attachToCell(cell_id);
				
				// Calculate distance from ALL NONZERO pixels!
				VINT pos = VINT(0,0,0);
				for (pos.y=0; pos.y < input->size().y; pos.y++){
					for (pos.x=0; pos.x < input->size().x; pos.x++){
						if( input->get(pos) > 0.0 ){
							mapper->set(pos, input->get(pos) );
						}
					}
				}
				
				mapper->fillGaps();
				mapper->copyData( output );
				
				break;
			}
			case Reporter::CELL_SHAPE: // print cell shape information (radii from cell center) to output
			{
				PDE_Layer* output = reporter.output_membrane.getMembrane(cell_id);
				const PDE_Layer* cell_shape = & (CPM::getCell(cell_id).getSphericalApproximation());
				VINT pos;
				for (pos.y=0; pos.y < output->size().y; pos.y++) {
					for (pos.x=0; pos.x < output->size().x; pos.x++){
						output->set(pos, cell_shape->get(pos));
					}
				}
				break;
			}
			case Reporter::LOCAL_NBH_SIZE:
			{
//  				if( cell_id != 39183 )
//  					continue;
				
				cout << "MembraneRules: LOCAL_NBH, cellid " << cell_id << endl;
				shared_ptr<MembraneMapper> mapper = shared_ptr <MembraneMapper >( new MembraneMapper(MembraneMapper::MAP_DISTANCE_TRANSFORM, reporter.deformed_sphere) );
				PDE_Layer* output = reporter.output_membrane.getMembrane(cell_id);

				vector<Neighbor> nbh;
				for (int y=0; y < output->size().y; y++) {
					for (int x=0; x < output->size().x; x++){
						VINT pos(x,y,0);
						nbh = getNeighborhoodSpherical( mapper, pos, reporter.nbh_size, cell_id);
						output->set(pos, nbh.size());
					}
				}
				break;
			}
			case Reporter::IMPUTATION: // fills gaps in discrete data. Useful to compensate for missing lateral contacts (due to holes left by removing BC).
			{
//  				if( cell_id != 39183 )
//  					continue;

				cout << "MembraneRules: IMPUTATION " << cell_id << endl;
				shared_ptr<MembraneMapper> mapper = shared_ptr <MembraneMapper >( new MembraneMapper(MembraneMapper::MAP_DISCRETE, reporter.deformed_sphere) );
				PDE_Layer* input_values = reporter.input_membranes[0].getMembrane(cell_id);
				PDE_Layer* input_mask   = reporter.input_membranes[1].getMembrane(cell_id);
				PDE_Layer* output = reporter.output_membrane.getMembrane(cell_id);
				
				mapper->attachToCell(cell_id);
				
				// set data assuming that value=0 are not set (and should be imputated)
				VINT pos = VINT(0,0,0);
				for (pos.y=0; pos.y < input_values->size().y; pos.y++){
					for (pos.x=0; pos.x < input_values->size().x; pos.x++){
						
						if( !std::isnan( input_values->get(pos) ) && input_values->get(pos) != 0.0 ){
							mapper->set(pos, input_values->get(pos) );
						}
					}
				}

				// perform imputation
				mapper->fillGaps();	
				
				// copy result to output
 				//mapper->copyData(output);
				for (pos.y=0; pos.y < input_values->size().y; pos.y++){
					for (pos.x=0; pos.x < input_values->size().x; pos.x++){
						
						if( input_mask->get(pos) != 1 )
							output->set(pos, mapper->getData().get(pos));
						else
							output->set(pos, 0);
					}
				}
					
				break;
			}
			case Reporter::NEIGHBORS_REPORTER:
			{
//  				if( cell_id != 39183 )
//   					continue;

				cout << "MembraneRules: NEIGHBORS_REPORTER " << cell_id << endl;
 				// cell IDs of neighboring cells
 				PDE_Layer* input = reporter.input_membranes[0].getMembrane(cell_id);
 				PDE_Layer* output = reporter.output_membrane.getMembrane(cell_id);

				// get list of neighbor nodes, sorted by angle
				Cell::Nodes neighbor_nodes; // holds nodes of neighbors of membrane nodes. Used for <concentration>
				const Cell::Nodes& membrane = CPM::getCell ( cell_id ).getSurface();
				for ( Cell::Nodes::const_iterator m = membrane.begin(); m != membrane.end(); ++m ) {
					// check the von neumann neighborhood
					// gain the halo of nodes surrounding the cell
					for ( int i_nei = 0; i_nei < neighbor_sites.size(); ++i_nei ) {
						VINT neighbor_position = ( *m ) + neighbor_sites[i_nei];
						const CPM::STATE& nb_spin = cpm_layer->get ( neighbor_position );

						if ( cell_id != nb_spin.cell_id ) { // if neighbor is different from me
							neighbor_nodes.insert ( neighbor_position ); // add neighbor node to list of unique neighboring points (used for layers below)
						}
					}
				}
				
				MembraneMapper mapper(MembraneMapper::MAP_CONTINUOUS,false);
				mapper.attachToCell(cell_id);
				for ( Cell::Nodes::const_iterator i=neighbor_nodes.begin(); i != neighbor_nodes.end(); ++i ) {
					
					CPM::CELL_ID cell_id_neighbor = int(input->get( mapper.getMembranePosition(*i) ));
					if( CPM::cellExists(cell_id_neighbor) )
						mapper.map(*i, reporter.input_symbol.get(cell_id_neighbor, *i));
					
				}

				mapper.fillGaps();
				mapper.copyData(output);
				
				break;
			}
			case Reporter::GAUSSIAN_FILTER: // apply Gaussian blurring to input 
			{
				//cout << "MembraneRules: GAUSSIAN_FILTER, cellid " << cell_id << endl;	
				
				/// Copy membraneProperty representing contacting points into values
				PDE_Layer* input = reporter.input_membranes[0].getMembrane(cell_id);
				PDE_Layer* output = reporter.output_membrane.getMembrane(cell_id);
				
				// copy input to output (can this be done more efficiently?
				VINT pos = VINT(0,0,0);
				for (pos.y=0; pos.y < input->size().y; pos.y++)
					for (pos.x=0; pos.x < input->size().x; pos.x++)
						output->set(pos, input->get(pos));
						
				output->setDiffusionRate(reporter.blurring);
				output->updateNodeLength(1.0);
				output->doDiffusion(1.0);
				break;
			}
			case Reporter::LOCAL_THRESHOLD: // perform local thresholding on input image
			{
				// skip cells that touch the boundary
				bool boundary_cell = false;
				VINT lattice_size = SIM::getLattice()->size();
				const Cell::Nodes& nodes = CPM::getCell(cell_id).getNodes();
				for (Cell::Nodes::const_iterator pt = nodes.begin(); pt != nodes.end(); pt++){
					if( pt->x >= lattice_size.x-1 || pt->y >= lattice_size.y-1 || pt->z >= lattice_size.z-1 ||
						pt->x <= 1 || pt->y <= 1 || pt->z <= 1 ){
						boundary_cell = true;
						cout << "MembraneRules: Ignoring boundary cell " << cell_id << " (point: " << (*pt) << ")" << endl;
						break;
					}
				}
				if(boundary_cell){
					continue;
				}
				cout << "Cell " << cell_id << " is NOT A boundary cell!" << endl;

				cout << "MembraneRules: LOCAL_THRESHOLD, cellid " << cell_id << endl;
				shared_ptr<MembraneMapper> mapper = shared_ptr <MembraneMapper >( new MembraneMapper(MembraneMapper::MAP_DISTANCE_TRANSFORM, reporter.deformed_sphere) );
				
				/// Copy membraneProperty representing contacting points into values
				PDE_Layer* input = reporter.input_membranes[0].getMembrane(cell_id);				
				PDE_Layer* output = reporter.output_membrane.getMembrane(cell_id);

				// Note: cannot use caching, because spheres are scaled to cell volume
  				bool use_cached_nbh = false;
  				if(false == true && nbh_cache.size() > 0 && !reporter.deformed_sphere)
  					use_cached_nbh = true;
				
				VINT pos;
				for (pos.y=0; pos.y < output->size().y; pos.y++) {
					for (pos.x=0; pos.x < output->size().x; pos.x++){
						double value = input->get(pos);
						double local = 0;
						vector<Neighbor> nbh;
						
						if(!use_cached_nbh)
							nbh = getNeighborhoodSpherical( mapper, pos, reporter.nbh_size, cell_id);
						else
							nbh = nbh_cache[ pos ];
						
						switch(reporter.local_threshold_mode){
							case Reporter::THR_LOCAL_MAX:{
								local = getLocalMax(pos, nbh, cell_id); 
								break;
							}
							case Reporter::THR_LOCAL_MEAN:{
								local = getLocalMean(pos, nbh, cell_id); 
								break;
							}
							case Reporter::THR_LOCAL_MEDIAN:{
								local = getLocalMedian(pos, nbh, cell_id); 
								break;
							}
							case Reporter::THR_LOCAL_MIDPOINT:{
								local = getLocalMidpoint(pos, nbh, cell_id); 
								break;
							}
							case Reporter::THR_LOCAL_STDEV:
							{
								local = getLocalStDev(pos, nbh, cell_id, (reporter.threshold/5.0)); 
								break;
							}
							default:
								local = getLocalMean(pos, nbh, cell_id); break;
						}
						
						if( (value - local) > reporter.threshold ){
							if(reporter.binary)
								output->set(pos, 1.0);
							else
								output->set(pos, value);
						}
						else{
							output->set(pos, 0.0);
						}
						
					}
				}
				output->set(pos, input->get(pos));
				//cout << "End of Local thresholding for cell " << cell_id << endl; 
				break;
			}
			case Reporter::GLOBAL_THRESHOLD:
			{
				/// Copy membraneProperty representing contacting points into values
				PDE_Layer* input = reporter.input_membranes[0].getMembrane(cell_id);
				PDE_Layer* output = reporter.output_membrane.getMembrane(cell_id);
				
				double global_minimum, global_maximum;
				if( reporter.global_threshold_mode == Reporter::THR_RELATIVE_TO_MIN )
					global_minimum = input->min_val();
				if( reporter.global_threshold_mode == Reporter::THR_RELATIVE_TO_MAX )
					global_maximum = input->max_val();
					
				VINT pos;
				for (pos.y=0; pos.y < output->size().y; pos.y++) {
					for (pos.x=0; pos.x < output->size().x; pos.x++){
						double value = input->get(pos);
						
						switch(reporter.global_threshold_mode){

							case( Reporter::THR_ABSOLUTE ):
							{
								if( value > reporter.threshold )
									output->set(pos, value);
								else 
									output->set(pos, 0.0);
								break;
							}
							case( Reporter::THR_RELATIVE_TO_MIN ):
							{
								if( (value - global_minimum) < reporter.threshold )
										output->set(pos, value);
									else 
										output->set(pos, 0.0);
								break;
							}
							case( Reporter::THR_RELATIVE_TO_MAX ):
							{
								if( (global_maximum - value) < reporter.threshold )
									output->set(pos, value);
								else 
									output->set(pos, 0.0);
								break;
							}
							default:
								break;
						}
					}
				}
				break;
			}
			default:
			{
				cout << "Membrane Rule not implemented!" << endl;
				break;
			}
		}

	}
}
	
/** Provides the indices of the neighboring nodes for a given position on the cell surface
 *  Calculation is based on distance transform from the given position (and should therefore also work for non-spherical cell shapes in the future)
 */
vector<MembraneRules::Neighbor> MembraneRules::getNeighborhoodSpherical(shared_ptr<MembraneMapper> mapper, VINT position, double distance, CPM::CELL_ID cell_id){
	
	vector<Neighbor> neighbors;
	
	mapper->attachToCell(cell_id);
	mapper->set(position, 1);
	mapper->ComputeDistance();
	const PDE_Layer& distances_from_node = mapper->getDistance();
	
	
	VINT pos(0.0,0.0,0.0);
	for (pos.y=0; pos.y < distances_from_node.size().y; pos.y++) {
		for (pos.x=0; pos.x < distances_from_node.size().x; pos.x++){
			double d = distances_from_node.get(pos);
			
// 			if(cell_id == 39183 && position.x==floor(distances_from_node.size().x / 2) && position.y== floor(distances_from_node.size().y / 2))
//  				cout << std::fixed << std::setprecision(4) << setfill('0') << setw(3) << d << " ";
			
			if( d <= distance ){
				MembraneRules::Neighbor nb;
				
				VDOUBLE node_size(1.0,1.0,0);
				double theta_y = (double(pos.y)+0.5)  * (M_PI/lattice->size().y);
				node_size.x = sin(theta_y);
				nb.area = node_size.x*node_size.y;

				nb.pos = pos;
				nb.distance = d;
				
				neighbors.push_back( nb );
			}
		}
// 		if(cell_id == 39183 && position.x==floor(distances_from_node.size().x / 2) && position.y== floor(distances_from_node.size().y / 2))
// 			cout << "\n";
	}

	if(!reporter.deformed_sphere && false == true) // cannot use caching, because spheres are scaled to cell volume
		nbh_cache.insert(pair<VINT, vector<Neighbor> >(position, neighbors) );
	
	return neighbors;
}

double MembraneRules::getLocalMean(VINT pos, vector<Neighbor>& nbh, uint cell_id){
	PDE_Layer* data = reporter.input_membranes[0].getMembrane(cell_id);
	vector<double> vec;
	double sum=0;
	double size=0;
	for ( int i_nei = 0; i_nei < nbh.size(); i_nei++ ) {
		VINT nbpos = nbh[i_nei].pos;
		sum  += data->get(nbpos) * nbh[i_nei].area;
		size += nbh[i_nei].area;
	}
	double mean = sum / size;
	return mean;
}

double MembraneRules::getLocalMidpoint(VINT pos, vector<Neighbor>& nbh, uint cell_id){
	PDE_Layer* data = reporter.input_membranes[0].getMembrane(cell_id);
	vector<double> vec;
	for ( int i_nei = 0; i_nei < nbh.size(); i_nei++ ) {
		VINT nbpos = nbh[i_nei].pos;
		vec.push_back(data->get(nbpos) * nbh[i_nei].area );
	}
	double maximum=-99999999.9, minimum=999999999.9;
	for( uint i=0; i < vec.size(); i++ ){
		maximum = max(maximum, vec[i]);
		minimum = min(minimum, vec[i]);
	}
	return (maximum-minimum)/ 2.0;
}

double MembraneRules::getLocalMedian(VINT pos, vector<Neighbor>& nbh, uint cell_id){
	PDE_Layer* data = reporter.input_membranes[0].getMembrane(cell_id);
	vector<double> vec;
	for ( int i_nei = 0; i_nei < nbh.size(); i_nei++ ) {
		VINT nbpos = nbh[i_nei].pos;
		double val = data->get(nbpos); // * nbh[i_nei].area;
		if( val > 0.0 )
			vec.push_back( val );
	}
	sort(vec.begin(), vec.end());
	typedef vector<double>::size_type vec_sz; 
	int v_size = vec.size();
	vec_sz mid = v_size/2;
	double median = 0.0;
	if(v_size>0)
		median = (v_size % 2 == 0 ? ((vec[mid] + vec[mid-1]) / 2.0) : vec[mid]); 
	return median;
}

double MembraneRules::getLocalStDev(VINT pos, vector<Neighbor>& nbh, uint cell_id, double var){
	PDE_Layer* data = reporter.input_membranes[0].getMembrane(cell_id);
	vector<double> vec;
	for ( int i_nei = 0; i_nei < nbh.size(); i_nei++ ) {
		VINT nbpos = nbh[i_nei].pos;
		double val = data->get(nbpos) * nbh[i_nei].area;
		if( val > 0.0 )
			vec.push_back( val );
	}
	
	double sum = std::accumulate(vec.begin(), vec.end(), 0.0);
	double mean = sum / vec.size();
	
	double sq_sum = std::inner_product(vec.begin(), vec.end(), vec.begin(), 0.0);
	double stdev = std::sqrt(sq_sum / vec.size() - mean * mean);
	
	return mean - var*stdev;
}


double MembraneRules::getLocalMax(VINT pos, vector<Neighbor>& nbh, uint cell_id){
	PDE_Layer* data = reporter.input_membranes[0].getMembrane(cell_id);
	double maximum=-99999.9;
	for ( int i_nei = 0; i_nei < nbh.size(); i_nei++ ) {
		double value = data->get( nbh[i_nei].pos);
		if( value > maximum)
			maximum = value;
	}
	return maximum;
}



void MembraneRules::print_valarray( Lattice_Data_Layer<double> l, uint precision, string filename){

    ofstream fout;
    fout.open(filename.c_str());
    
    for (uint y=0; y < l.size().y; y++){
     fout <<"\n";
		for (uint x=0; x < l.size().x; x++){
         fout << setiosflags(ios::fixed) << setprecision(precision) << setw(1) << l.get(VINT(x,y,0)) << " ";
		}
	}
	fout <<"\n";
    fout.close();
} 

void MembraneRules::init ( CellType* ct ) {
	TimeStepListener::init();
	celltype = ct;
	buffer.resize ( MembraneProperty::resolution );

	cpm_layer 	= CPM::getLayer();
	lattice 	= SIM::getLattice();
	neighbor_sites  = lattice->getNeighborhood ( 1 );
	
	cout << "num input_symbols: " <<   reporter.input_symbol_strs.size() << endl;
	if ( reporter.input == Reporter::IN_MEMBRANE) {
		bool stored_picky_mode = SIM::PickyMode;
		SIM::PickyMode = false;
		// Create a Symbol that defaults to zero if there is no container in the back ... (i.e. no membrane defined in a medium celltype)
		for(uint i=0; i<reporter.input_symbol_strs.size(); i++)
			reporter.input_membranes.push_back( ct->findMembrane ( reporter.input_symbol_strs[i] ) );
		SIM::PickyMode = stored_picky_mode;
	}
	
	if( reporter.operation == Reporter::LOCAL_THRESHOLD ){
		// cannot use global variable mapper due to multithreading
		//mapper = shared_ptr <MembraneMapper >( new MembraneMapper(MembraneMapper::MAP_DISTANCE_TRANSFORM, true) ); 
	}
	if( reporter.operation == Reporter::NEIGHBORS_REPORTER ){
		reporter.input_symbol = findSymbol<double>( reporter.input_symbol_str);

	}
	cout << "output_symbol_str: " <<   reporter.output_symbol_str << endl;
	if ( reporter.output == Reporter::OUT_MEMBRANEPROPERTY ) {
		reporter.output_membrane = ct->findMembrane ( reporter.output_symbol_str, true );
	}

}

//bool MembraneRules::isReporter() {
//    return true;
//}

void MembraneRules::executeTimeStep() {
    TimeStepListener::executeTimeStep();
}

set< string > MembraneRules::getDependSymbols() {
	set< string > s;
	for(uint i=0; i<reporter.input_symbol_strs.size(); i++)
		s.insert ( reporter.input_symbol_strs[i] );
	return s;
}

set< string > MembraneRules::getOutputSymbols() {
	set< string > s;
	s.insert ( reporter.output_symbol_str );
	return s;
}



void MembraneRules::loadFromXML ( const XMLNode xNode ) {

	TimeStepListener::loadFromXML ( xNode );

	
	XMLNode xRule;
	if( xNode.nChildNode("CellShapeReporter") ){
		cout << "MembraneRules: CellShapeReporter" << endl;
		xRule = xNode.getChildNode("CellShapeReporter");
		reporter.operation = Reporter::CELL_SHAPE;
	}
	else if( xNode.nChildNode("NeighborhoodSizeReporter") ){
		cout << "MembraneRules: NeighborhoodSizeReporter" << endl;
		xRule = xNode.getChildNode("NeighborhoodSizeReporter");
		getXMLAttribute(xRule, "cell_shape", reporter.deformed_sphere);
		getXMLAttribute(xRule, "distance", reporter.nbh_size);
		reporter.operation = Reporter::LOCAL_NBH_SIZE;
	}
	else if( xNode.nChildNode("Imputation") ){
		xRule = xNode.getChildNode("Imputation");
		reporter.operation = Reporter::IMPUTATION;
	}
	else if( xNode.nChildNode("NeighborsReporter") ){
		xRule = xNode.getChildNode("NeighborsReporter");
		getXMLAttribute(xRule, "symbol", reporter.input_symbol_str);
		reporter.operation = Reporter::NEIGHBORS_REPORTER;
	}
	else if( xNode.nChildNode("DistanceTransform") ){
		xRule = xNode.getChildNode("DistanceTransform");
		getXMLAttribute(xRule, "cell_shape", reporter.deformed_sphere);
		reporter.operation = Reporter::DISTANCE_TRANSFORM;
	}
	else if( xNode.nChildNode("GaussianFilter") ){
		xRule = xNode.getChildNode("GaussianFilter");
		getXMLAttribute(xRule, "amount", reporter.blurring);
		reporter.operation = Reporter::GAUSSIAN_FILTER;
	}
	else if( xNode.nChildNode("LocalThreshold") ){
		xRule = xNode.getChildNode("LocalThreshold");
		reporter.operation = Reporter::LOCAL_THRESHOLD;
		reporter.nbh_size = 5;
		reporter.binary = false;
		reporter.deformed_sphere=false;
		getXMLAttribute(xRule, "threshold", reporter.threshold);
		getXMLAttribute(xRule, "distance", reporter.nbh_size);
		getXMLAttribute(xRule, "binary", reporter.binary);
		getXMLAttribute(xRule, "cell_shape", reporter.deformed_sphere);
		
		string method;
		getXMLAttribute(xRule, "method", method);
		if( method == "average") 
			reporter.local_threshold_mode = Reporter::THR_LOCAL_MEAN;
		else if( method == "maximum") 
			reporter.local_threshold_mode = Reporter::THR_LOCAL_MAX;
		else if( method == "midpoint") 
			reporter.local_threshold_mode = Reporter::THR_LOCAL_MIDPOINT;
		else if( method == "median") 
			reporter.local_threshold_mode = Reporter::THR_LOCAL_MEDIAN;
		else if( method == "standard_deviation") 
			reporter.local_threshold_mode = Reporter::THR_LOCAL_STDEV;
		else{
			cerr << "MembraneRules: Unknown method for local thresholding." << endl;
			exit(-1);
		}
		
		
	}
	else if( xNode.nChildNode("Threshold") ){
		xRule = xNode.getChildNode("Threshold");
		getXMLAttribute(xRule, "threshold", reporter.threshold);
		string mode;
		getXMLAttribute(xRule, "mode", mode);
		reporter.global_threshold_mode = Reporter::THR_ABSOLUTE;
		if( mode == "absolute") 
			reporter.global_threshold_mode = Reporter::THR_ABSOLUTE;
		else if( mode == "relative to maximum") 
			reporter.global_threshold_mode = Reporter::THR_RELATIVE_TO_MAX;
		else if( mode == "relative to minimum") 
			reporter.global_threshold_mode = Reporter::THR_RELATIVE_TO_MIN;
		
	}

	// Input
	XMLNode xInput  = xRule.getChildNode ( "Input" );
	for(uint i=0; i < xInput.nChildNode ( "MembraneProperty" ); i++) {
		reporter.input = Reporter::IN_MEMBRANE;
		string input_symbol_str;
		if( !getXMLAttribute ( xInput, "MembraneProperty/symbol-ref", input_symbol_str ) ) {
			cerr << "MembraneRules::loadFromXML: Input MembraneProperty/symbol not defined!" << endl;
			exit( -1 );
		}
		else
			reporter.input_symbol_strs.push_back( input_symbol_str );
	}
	if(reporter.input_symbol_strs.empty() && reporter.operation != Reporter::CELL_SHAPE && reporter.operation != Reporter::LOCAL_NBH_SIZE){
		cerr << "MembraneRules::loadFromXML: Unknown Input ! (num children: " << xInput.nChildNode() << ")" << endl;
		exit( -1 );
	}
	// Mask (for Imputation)
	XMLNode xMask = xRule.getChildNode ( "Mask" );
	for(uint i=0; i < xMask.nChildNode ( "MembraneProperty" ); i++) {
		reporter.input = Reporter::IN_MEMBRANE;
		string input_symbol_str;
		if( !getXMLAttribute ( xMask, "MembraneProperty/symbol-ref", input_symbol_str ) ) {
			cerr << "MembraneRules::loadFromXML: Mask MembraneProperty/symbol not defined!" << endl;
			exit( -1 );
		}
		else
			reporter.input_symbol_strs.push_back( input_symbol_str );
	}


	// Output
	XMLNode xOutput = xRule.getChildNode ( "Output" );
	if ( xOutput.nChildNode ( "MembraneProperty" ) ) {
		reporter.output = Reporter::OUT_MEMBRANEPROPERTY;
		getXMLAttribute ( xOutput, "MembraneProperty/symbol-ref", reporter.output_symbol_str );
	}
	else {
		cerr << "MembraneRules::loadFromXML: Unknown Output !" << endl;
		exit ( -1 );
	}

}
