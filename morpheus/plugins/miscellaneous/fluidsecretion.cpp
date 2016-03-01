#include "fluidsecretion.h"

REGISTER_PLUGIN(FluidSecretion);

FluidSecretion::FluidSecretion() : TimeStepListener(ScheduleType::MCSListener) {
//empty constructor
};


void FluidSecretion::loadFromXML(const XMLNode xNode)
{
	TimeStepListener::loadFromXML(xNode);
	getXMLAttribute(xNode,"TargetVolume/symbol-ref", targetvolume_str);	
	if(!targetvolume_str.size()){
		cerr << "FluidSecretion::loadFromXML: No TargetVolume/symbol-ref given!" << endl;
		exit(1);
	}
	getXMLAttribute(xNode,"MembraneProperty/symbol-ref", cellmembrane_symbol_str);	
	if(!cellmembrane_symbol_str.size()){
		cerr << "FluidSecretion::loadFromXML: No Property/symbol-ref given!" << endl;
		exit(1);
	}
	getXMLAttribute(xNode,"FluidCelltype/name", fluid_celltype_name);	
	if(!fluid_celltype_name.size()){
		cerr << "FluidSecretion::loadFromXML: No FluidCelltype/name given!" << endl;
		exit(1);
	}	
	// symbol_condition / expression_condition
	string symbol_name;
	symbolic_rate = getXMLAttribute(xNode,"Rate/symbol-ref", symbol_name);
	if (symbolic_rate) {
		sym_rate = SIM::findSymbol<double>(symbol_name);
	} 
	else {
		string expression;
		getXMLAttribute(xNode,"Rate/text",expression);
		fct_rate = shared_ptr<Function> (new Function() );
		fct_rate->setExpr(expression);
	}
}

void FluidSecretion::init()
{	
	TimeStepListener::init();
	
	setTimeStep(timeStep()*10);
	
	celltype = SIM::getScope()->getCellType();
	cellmembrane = celltype->findMembrane(cellmembrane_symbol_str);
	vector <shared_ptr< const CellType > > celltypes = CPM::getCellTypes();
	
	for(uint i=0; i< celltypes.size(); i++){
		if( celltypes[i]->getName() == fluid_celltype_name )
			fluid_ct = const_pointer_cast< CellType >(celltypes[i]);
	}
	if (!fluid_ct) {
		cerr << "unable to find '" << fluid_celltype_name << "' in FluidSecretion::init" << endl;
		exit(-1);
	}
	targetvolume = fluid_ct->findCellProperty<double>(targetvolume_str, true);
	
	
	if (! symbolic_rate) fct_rate->init();

}



void FluidSecretion::executeTimeStep() {
	TimeStepListener::executeTimeStep();
	
// 	if (currentTime() < 200)
// 		return;
		
	cout << "FluidSecretion: time = " << currentTime() << endl;
	
	// Randomize updating of cells
	// This is necessary because changes in the lattice can cause a first-updated cell to 'push' a later-updated cell
	vector<CPM::CELL_ID> cells = celltype->getCellIDs();
	random_shuffle( cells .begin(), cells .end() );
	
	for (int c=0; c < cells.size(); c++ ) {
		
		const Cell& cell = CPM::getCell( cells[c] );
		
		// we'd like to secrete there, where the concentration on the membrane is highest.
		
		vector<double> secretion_probabilities;
                uint cellmembranesize = cellmembrane.size( cells[c] ).x;
// 		cout << "Size of cell membrane (number of lattice sizes) = " << cellmembranesize << endl;
		secretion_probabilities.resize(cellmembranesize, 0.0);
		double total=0.;
		//cout << "secretion : " << endl;
		for(uint i=0; i<cellmembranesize; i++){			
			double concentration = cellmembrane.get(cells[c], i);
			secretion_probabilities[i] = concentration;
			total += concentration;
		}
		if( total == 0 ) // if cellmembrane has no concentration, skip and proceed to other cell
			continue;
		
		// make a cumulative probability distribution (between 0 and 1)
		for(uint i=0; i<cellmembranesize; i++){
			secretion_probabilities[i] /= total;
			// make cumulative
			secretion_probabilities[i] += secretion_probabilities[i-1];			
// 			cout << i << ": " << setw(4) << secretion_probabilities[i] << " " << endl;
		}
		
		// choose a random site in the cell membrane, proportionally to concentration 
		double random = getRandom01();
		uint secretion_pos = 0;
		for(uint i=0; i<cellmembranesize; i++){
			if( secretion_probabilities[i] <= random ){
				secretion_pos = i;
			}
		}
		uint secretion_angle = 90.0 + uint((double)secretion_pos * (360.0 / (double)cellmembranesize )) ;
// // 		cout << random  << "\tsecretion angle = " << secretion_angle << " (pos = " << secretion_pos << ") with value " << secretion_probabilities[secretion_pos] - (secretion_pos>0?secretion_probabilities[secretion_pos-1]:0) << endl;

		
		// choose a membrane point that is closest to this angle
		const Cell::Nodes membrane = cell.getSurface();
		VDOUBLE cell_center = cell.getCenter();
		vector<double> angles_to_membrane_points;
		vector<VINT  > membrane_points;
		
		const Cell::Nodes& membrane_nodes = cell.getSurface();		
// 		cout << "membrane_nodes.size(): " << membrane_nodes.size() << endl;
		
		if( membrane_nodes.size() < 2)
			continue;
			
		
		for (Cell::Nodes::const_iterator m=membrane_nodes.begin(); m != membrane_nodes.end(); ++m ){
			VINT mem_p = (*m);
			VDOUBLE vector_to_membrane_point = (VDOUBLE) mem_p - cell_center;
			double angle = angle2d( vector_to_membrane_point, VDOUBLE(0.0, 1.0, 0.0) );
			//cout << angle << endl;
			angles_to_membrane_points.push_back( (angle + M_PI)* (180.0/M_PI) );
			membrane_points.push_back( mem_p );			
		}
		
		double min_diff = (double)cellmembranesize;
		uint min_diff_pos = 0;
		for (int i=0; i<angles_to_membrane_points.size(); i++){
			double diff = abs(secretion_angle - angles_to_membrane_points[i]);
			if( diff < min_diff ){
				min_diff 		= diff;
				min_diff_pos 	= i;
			}
		}

		VINT secretion_point = membrane_points[ min_diff_pos ];
		
		// determine the rate 
		//double concentration_at_secretion_site = cellmembrane.get(cells[c], secretion_pos);
		double secr_amount = 0.0; 
	
		bool divide = false;
		if ( symbolic_rate ) {
			secr_amount = sym_rate.get( cells[c] );
		}
		else {
			CPM::CELL_ID mycell_id = cells[c];
			secr_amount = fct_rate->get( secretion_point );
		}
		
		if( secr_amount <= 0 )
			continue;
		
		// Get the neighborhood of the chosen secretion point
		uint my_celltype = CPM::getCellIndex(cells[c]).celltype;
		shared_ptr <const CPM::LAYER > cpm_layer=CPM::getLayer();
		const vector< VINT> neighbor_sites = SIM::getLattice()->getNeighborhood(1);
		vector <shared_ptr <const CellType > > celltypes = CPM::getCellTypes();
		
		// check the von neumann neighborhood of the secretion point
		for ( int i_nei = 0; i_nei < neighbor_sites.size(); ++i_nei )
		{
			VINT nb_pos = secretion_point + neighbor_sites[i_nei];
			const CPM::STATE& nb_state = cpm_layer->get ( nb_pos );
			
			// if neighbor is not part of the same cell
			if( nb_state.cell_id != cells[c] ){ 
				
				uint nb_celltype_id = CPM::getCellIndex( nb_state.cell_id ).celltype;
				shared_ptr <const CellType > nb_celltype = celltypes[ nb_celltype_id ];
				cout << "Neighbor of secretion point is " << nb_state.pos << " = " << nb_celltype->getName() << " (status = " << CPM::getCellIndex( nb_state.cell_id ).status  << ")" << endl;
				
				
				if( nb_celltype_id == fluid_ct->getID() ){ // if neighbor is FLUID
// 					cout << "Neighbor is FLUID!"<< endl;
// 					cout << "FluidSecretion:: Adding fluid to exiting fluid cell (increase targetvolume)" << endl;
					// Increase target volume of fluid cell
					double old_targetvolume = targetvolume.get(nb_state.cell_id);
					targetvolume.set(nb_state.cell_id, old_targetvolume  +  secr_amount);
// 					cout << "FluidSecretion:: Increased target voume of fluid cell from " << old_targetvolume  << " to " << targetvolume.get(nb_state.cell_id) << endl;
					break;
				}
				else if( CPM::getCellIndex( nb_state.cell_id ).status == CPM::REGULAR_CELL){
					//if( nb_state == CPM::getEmptyState() /*SIM::VIRTUAL_CELL*/ ){ // if neighbor is MEDIUM
// 					cout << "FluidSecretion:: Creating new fluid cell " << endl;

					CPM::STATE new_state;
					CPM::CELL_ID new_id = fluid_ct->createCell();
					new_state.cell_id = new_id;
					new_state.pos = nb_pos; // NOTE: this will cause pressure towards other cells (using secretion_point won't )???
					CPM::setNode( new_state.pos , new_id );					
					//Cell& fluid_ct->getCell( new_id );
					targetvolume.set(new_id, secr_amount);

					//new_state.cell = fluid_celltype->createCell();
					//new_state.celltype =  fluid_celltype->getID();					
					//new_state.pos = secretion_point;
					//fluid_cell = fluid_celltype->getCell(new_state.cell);
					//cpa(fluid_cell) += rate;
					
// 					cout << "FluidSecretion:: Created new fluid cell with targetvolume =  " << targetvolume.get(new_id) << endl;
					break;
				}
				// 	2c. Else, if no fluid of medium neighbor, do not secrete
				else{
					//cout << "FluidSecretion:: Neighbor of secretion is not regular cell." << endl;
				}
			}
		}		
		
	}
	
};



double FluidSecretion::angle2d(VDOUBLE v1, VDOUBLE v2){
	//return MOD(atan2(v1.x*v2.y-v2.x*v1.y,v1.x*v2.x+v1.y*v2.y),2*M_PI); // [0, 2PI]
	return atan2(v1.x*v2.y-v2.x*v1.y,v1.x*v2.x+v1.y*v2.y); // [-PI, PI] 
}

set< string > FluidSecretion::getDependSymbols()
{
	return set<string>();
}

set< string > FluidSecretion::getOutputSymbols()
{
	set<string> symbols;
	symbols.insert("cell.center");
	return symbols;
}

