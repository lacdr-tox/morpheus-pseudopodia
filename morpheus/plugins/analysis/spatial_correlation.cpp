#include "spatial_correlation.h"

/*
 Spatial Autocorrelation: Geary's C

 C = (N-1)  SUM_i SUM_j w_ij*(X_i - X_j)^2
    		------------------------------
			2 * W * SUM_i  ( X_i - X_ave)^2
			
Modes:
- Global: w_ij equals eucledian distance between cells
- Classes: w_ij equals 1 if eucledian distance between cells falls within classes (e.g. 0 - 10), 0 otherwise
*/

REGISTER_PLUGIN(SpatialCorrelation);

void SpatialCorrelation::loadFromXML (const XMLNode xNode) {
	Analysis_Listener::loadFromXML(xNode);
	getXMLAttribute(xNode, "celltype", celltype_name);
	getXMLAttribute(xNode, "symbol-ref", symbolref_str);

	getXMLAttribute(xNode, "mode", mode_str);
	
	num_classes = 0;
	getXMLAttribute(xNode, "classes", num_classes);
	
}

set< string > SpatialCorrelation::getDependSymbols()
{
	set< string > symbols;
	symbols.insert(symbolref_str);
	return symbols;
}


void SpatialCorrelation::init(double time) {
	Analysis_Listener::init(time);
	
	celltype = CPM::findCellType(celltype_name);
	if ( ! celltype.get() ) throw string("unknown CellType in Displacement Tracker");

	// get symbol
	property = SIM::findSymbol<double>( symbolref_str );
	if( !property.valid() ){
		cerr << "SpatialCorrelation: symbolref is not valid!" << endl;
		exit(-1);
	}
	
	
	// determine bin sizes, based on lattice size and specified number of classes
	lattice = SIM::getLattice();
	VINT lat_size = lattice->size();	
	
	lattice_max_size = max(lat_size.x, max(lat_size.y, lat_size.z));
	
/*	if( num_classes > lattice_max_size ){
		cout << "SpatialCorrelation: WARNING: Number of classes (" <<  num_classes << ") is larger than maximum lattice size (" << lattice_max_size << ")." << endl;
		num_classes = lattice_max_size; 
		cout << " Adjusted number of classes to " << num_classes << endl;
	} */
	binsize = double(lattice_max_size) / double(num_classes) ; 

	// initialize output file
	
	string filename = celltype_name + "_spatialcorrelation.log";
	storage.open(filename.c_str(), fstream::out | fstream::trunc);
	storage << "MCS\tD_min\tD_max\tC" << endl;
	
	mode_classes = true;
	if( mode_str == "global")
		mode_classes = false;
	else if( mode_str == "classes")
		mode_classes = true;
	if(mode_classes && num_classes == 0){
		cerr << "SpatialCorrelation: Number of classes is zero or not specified! This is required for mode \"Classes\"" << endl;
		exit(-1);
	}
	

	
}

void SpatialCorrelation::notify(double time) {

	Analysis_Listener::notify(time);
	assert(celltype);
	
	double X_ave(0.0);
	
	// population-average value of X (cell property) 	
	vector<CPM::CELL_ID> cells = celltype->getCellIDs();
	for (uint i=0; i < cells.size(); i++) {
		X_ave += property.get(cells[i]);
	}
	X_ave /= cells.size();
	

	if( mode_classes ){
		map<double, double> C;

		// calculate Geary's C for various a number of distance classes
#pragma omp parallel for
		for(int i=0; i < num_classes; i++ ) {
			double bin = double(i)*binsize;
			double C_bin = compute_C( true, X_ave, bin );
			cout << bin << "-" << binsize+bin << " : " << C_bin << endl;
			C.insert( pair<double,double>(bin, C_bin) );
		}
	
		for ( map<double, double>::iterator it=C.begin() ; it != C.end(); it++ ){
			//if (!isnan( it->second ))
			storage << time << "\t" << double(it->first) <<  "\t" << (binsize + double(it->first) ) <<  "\t" << it->second << endl; 
		}
		
	}
	else{ // global metric
		double C = compute_C( false, X_ave );
		storage << time << "\t" << C << endl; 
	}
};


double SpatialCorrelation::compute_C( bool classes, double X_ave, double max_distance){
	
	double cumulative_num(0.0);
	double cumulative_denom(0.0);
	double W_sum(0.0);
        double V(0.0);

        vector<CPM::CELL_ID> cells = celltype->getCellIDs();
        map<CPM::CELL_ID, double> properties;
        for (uint i=0; i < cells.size(); i++) {
             properties.insert(pair<CPM::CELL_ID, double>( cells[i], property.get(cells[i]) ) );
        }


	for (uint i=0; i < cells.size(); i++) {

                double X_i =  properties.find(cells[i])->second;
		VDOUBLE cell_center_i = CPM::getCell(cells[i]).getCenter();
		
		cumulative_denom += sqr(X_i - X_ave);
                V+=1.0;

                for (uint j=i+1; j < cells.size(); j++) {
			
			VDOUBLE cell_center_j = CPM::getCell(cells[j]).getCenter();			
			double distance_ij = lattice->orth_distance( cell_center_i, cell_center_j ).abs();
			
			double w_ij = 0.;
                        if( true ){
                                if( distance_ij <= max_distance )
					w_ij = 1.0;
				else
					continue; //w_ij = 0.0;
			}
			else // global metric
				w_ij = distance_ij;
			
			W_sum += w_ij;
		
                       double X_j = properties.find(cells[j])->second;
			double diff_X = sqr(X_i - X_j);
			cumulative_num += w_ij * diff_X;
		}
	}

	double N = double(cells.size());
        double C = ((N - 1.0) * (cumulative_num)) / (2.0*W_sum*cumulative_denom);

        cout << "Class: "<< max_distance << ", N = " << N << " numer: " << (cumulative_num) << " / denom: " << (W_sum*cumulative_denom) << " ||  W: "<< W_sum << " V: " << V << endl;
	return C;
	
}

void SpatialCorrelation::finish(double time) {
	storage.close();
}


