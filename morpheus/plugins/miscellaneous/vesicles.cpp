#include "vesicles.h"

REGISTER_PLUGIN(Vesicles);

Vesicles::Vesicles(){
//empty constructor
};


void Vesicles::loadFromXML(const XMLNode xNode)
{
	Plugin::loadFromXML(xNode);
	
	J_in	= 0.;
	s0		= 0.;
	k_min	= 0.;
	r_fuse	= 0.;
	r_fission=0.;
	delta_t = 1.0;
	
	getXMLAttribute(xNode,"Internalization/influx", J_in);
	getXMLAttribute(xNode,"Internalization/typicalSize", s0);

	getXMLAttribute(xNode,"Fusion/rate", r_fuse);
	getXMLAttribute(xNode,"Fission/rate", r_fission);
	
	getXMLAttribute(xNode,"Conversion/rate", k_min);

	getXMLAttribute(xNode,"Timestep/value", delta_t);

	cout << "loadXML: FUSION RATE " << r_fuse << endl;

// 	getXMLAttribute(xNode,"MembraneProperty/symbol-ref", cellmembrane_symbol_str);	
// 	if(!cellmembrane_symbol_str.size()){
// 		cerr << "Vesicles::loadFromXML: No Property/symbol-ref given!" << endl;
// 		exit(1);
// 	}
}

void Vesicles::init(CellType* ct)
{	
	Plugin::init();
	celltype = ct;
	
// 	J_in	= 2000.;
// 	s0	= 3000.;
// 	k_min	= 1e-3;
// 	r_fuse	= 1e-4;
// 	r_fission=1e-5;
// 	delta_t = 1.0;
	
// 	cellmembrane = celltype->findMembrane(cellmembrane_symbol_str);
// 	vector <shared_ptr< const CellType > > celltypes = CPM::getCellTypes();
// 	
	
	fout.open("vesicles.log", ios::out);
	
	cout << "init: FUSION RATE " << r_fuse << endl;

}



void Vesicles::mcs_notify(uint time) {
  
	
  cout << time << "\t FUSION RATE " << r_fuse << endl;
  if(time == 1){
    vector<CPM::CELL_ID> cells = celltype->getCellIDs();
    endosomes_all_cells.resize( cells.size() );
  }
  //cout << time <<  "\tendosomes_all_cells.size() =  " << endosomes_all_cells.size() << " (num cells = " <<  celltype->getCellIDs().size() << ")" << endl;
  
	vector<CPM::CELL_ID> cells = celltype->getCellIDs();

	
	// for every cell
//#pragma omp parallel for schedule(dynamic)
	for (int c=0; c < cells.size(); c++ ) { 
	  const Cell& cell = celltype->getCell( cells[c] );
	  
	  //vector< Endo > endosomes_all_cells[c] = endosomes_all_cells[c];
	  random_shuffle( endosomes_all_cells[c].begin(), endosomes_all_cells[c].end() );
	  
	  // internalization
	  double num_to_create = (J_in * delta_t) / s0 ;
	  for(uint i=0; i < num_to_create; i++){
	    Endo endo;
	    endo.cargo = getRandomGamma(s0,2.0);
	    endo.rab5  = getRandomGamma(s0,2.0);
	    endosomes_all_cells[c].push_back( endo );
	  }
	  if( num_to_create < 1.0 ){
	    if( getRandom01() < num_to_create ){
	      Endo endo;
	      endo.cargo = getRandomGamma(s0,2.0);
	      endo.rab5  = getRandomGamma(s0,2.0);
	      endosomes_all_cells[c].push_back( endo );
	    }
	  }
	  
	  // loop over all existing endosomes
	  // fusion
	  for(uint e=0; e<endosomes_all_cells[c].size(); e++){
	  int N = endosomes_all_cells[c].size();
	    double p_fuse = r_fuse*delta_t * (( endosomes_all_cells[c].size() - 1.0 ) / 2.0 );
	    //cout << "fusion " << p_fuse  << endl;
	    if( getRandom01() < p_fuse ){
	      if( endosomes_all_cells[c].size() > 1 ){
		
// 		Endo random_endo = endosomes_all_cells[c][ e ];
		uint random_endo_position = e;
		while( e == random_endo_position ){
		  random_endo_position = getRandomUint( endosomes_all_cells[c].size() ); 
// 		  cout << "random  = " << random_endo_position << endl;
// 		  endosomes_all_cells[c][ random_endo_position ];
		}
		Endo random_endo = endosomes_all_cells[c][ random_endo_position ];
		endosomes_all_cells[c][e].cargo += random_endo.cargo;
		endosomes_all_cells[c][e].rab5  = pow(pow(endosomes_all_cells[c][e].rab5,3.0/2.0) + pow(random_endo.rab5,3.0/2.0),2.0/3.0);
		
		
// 		endosomes_all_cells[c].erase( endosomes_all_cells[c].begin() + int(random_endo_position)-1 );
		endosomes_all_cells[c][random_endo_position] = endosomes_all_cells[c].back();
		endosomes_all_cells[c].pop_back();
		if( e >= random_endo_position )
		  e--;
		
	      }
	    }
	  }

	  // loop over all existing endosomes
	  // fission
	  int endosome_pop_size = endosomes_all_cells[c].size(); // Note: newly created endosome cannot do fission in this iteration
	  for(uint e=0; e<endosome_pop_size; e++){
	  
	    double p_fission = r_fission*delta_t;
	    
	    if( getRandom01() < p_fission ){
	      
	      double fraction = min(0.0, getRandomGamma(s0,2.0));
	      Endo endo;
	      endo.cargo = fraction * endosomes_all_cells[c][e].cargo;
	      endo.rab5  = fraction * endosomes_all_cells[c][e].rab5;
	      endosomes_all_cells[c].push_back( endo );
	      
	    }
	  }

	  // loop over all existing endosomes
	  // degradation
	  for(uint e=0; e<endosomes_all_cells[c].size(); e++){
	  
	    if( getRandom01() < k_min ){
	        endosomes_all_cells[c][e] = endosomes_all_cells[c].back();
		endosomes_all_cells[c].pop_back();
		e--;
	    }
	  }
	  
	  // end of loops (put back new endosomes vector to endosomes_all_cells!)
	  
	  //endosomes_all_cells[ c ] = endosomes_all_cells[c];

	  
	   // for output (test)
	  double total_cargo=0.;
	  double total_rab=0.;
	  for(uint e=0; e<endosomes_all_cells[c].size(); e++){
	    total_cargo += endosomes_all_cells[c][e].cargo;
	    total_rab   += endosomes_all_cells[c][e].rab5;
	  }
	  fout << SIM::getTime() << "\tN: " << endosomes_all_cells[c].size() << "\tCargo: " << total_cargo << "\tRab5: " << total_rab << endl;
	}
  
		  		  
};



