#include "flipcell_motion.h"

REGISTER_PLUGIN ( FlipCellMotion );

FlipCellMotion::FlipCellMotion() : InstantaneousProcessPlugin( TimeStepListener::XMLSpec::XML_REQUIRED  ) {
	condition.setXMLPath("Condition/text");
	neighborhood.setXMLPath("neighborhood");
	neighborhood.setDefault("2");
	registerPluginParameter(condition);
	registerPluginParameter(neighborhood);
}

void FlipCellMotion::init(const Scope* scope) {
	InstantaneousProcessPlugin::init(scope);
	this->scope = scope;
	condition.init();
    lattice = SIM::getLattice();
	cpm_lattice = CPM::getLayer();
	nbh = lattice->getNeighborhoodByDistance( neighborhood() ).neighbors();
	// Add "cell.center" as output(!) symbol  (not depend symbol)
	registerCellPositionOutput(); 
};


void FlipCellMotion::executeTimeStep()
{
	if(!scope->getCellType())
		return;
		
	vector<CPM::CELL_ID> cells = scope->getCellType()->getCellIDs();
	if( !cells.size() ){
		cerr << "FlipCells: Error: No cells in population." <<  endl;
		exit(-1);
	}
	
	// TODO: use range-based iterator??
	for (uint c=0; c<cells.size(); c++) {
		
		// get random cell, but make sure it is not medium
		bool found = false;
		SymbolFocus focus1;
		while( !found ){
			CPM::CELL_ID cellID1 = cells[ getRandomUint(cells.size()-1) ];
			focus1.setCell( cellID1 );
			if( focus1.celltype() != CPM::getEmptyCelltypeID() ){
				found=true;
// 					cout << "CellID: " << cellID1 << ", celltype ID: " << focus1.cell_index().celltype << ", numnodes: " << focus1.cell().getNodes().size() << "\n";
			}

		}
		
		// if condition is valid for cell 1
		bool cond = (condition(focus1) > 0.0);
		if( cond ){
			
			const Cell::Nodes nodes1 = focus1.cell().getNodes();
			if( nodes1.size() != 1 ){
				cerr << "FlipCells: Cannot flip cell (1) " << focus1.cellID() << ", numnodes = " << nodes1.size() << "\n";
				cerr << "FlipCells: Cannot flip cells with more than one lattice site. FlipCells is only applicable to point-like cell models." <<  endl;
				exit(-1);
			}
			VINT pos_cell1 = (*nodes1.begin());
			
			bool flipped = false;
			while( !flipped ){
				
				// pick random neighboring cell
				VINT pos_cell2 = pos_cell1 + nbh[ getRandomUint( nbh.size()-1) ];
				if( !lattice->inside( pos_cell2 ) ){
					// if neighbor is outside of lattice
					if( !lattice->resolve( pos_cell2 ) ){
						// if not beyond a periodic boundary, then pick another cell to flip with
						continue;
					}
				}
					
				SymbolFocus focus2( pos_cell2 );

				const Cell::Nodes nodes1 = focus1.cell().getNodes();
				const Cell::Nodes nodes2 = focus2.cell().getNodes();
				CPM::CELL_ID id1 = focus1.cellID();
				CPM::CELL_ID id2 = focus2.cellID();
				assert( nodes1 != nodes2 );
				assert( id1 != id2 );

// 					cout << "ID1: " << id1 << " Pos 1: " << pos_cell1 << "\n" <<
//  							"ID2: " << id2 << " Pos 2: " << pos_cell2 << "\n";

				if( nodes2.size() > 1 ){
					cerr << "FlipCells: Cannot flip cell (2) " << focus2.cellID() << ", numnodes = " << nodes2.size() << "\n";
					cerr << "FlipCells: Cannot flip cells with more than one lattice site. FlipCells is only applicable to point-like cell models." <<  endl;
					exit(-1);
				}

				///////////////// FLIP NODES ////////////////
				if (! CPM::setNode(pos_cell2, id1)){
					cerr << "FlipCells: unable to set ID " << id1 << " to node " << pos_cell2 << endl;
					exit(-1);
				}
				if (! CPM::setNode(pos_cell1, id2)){
					cerr << "FlipCells: unable to set ID " << id2 << " to node " << pos_cell1 << endl;
					exit(-1);
				}
// 					cout << "FLIPPED: Pos 1: " <<  pos_cell1 << " -> CellID " <<  cpm_lattice->get(pos_cell1).cell_id << endl;
// 					cout << "FLIPPED: Pos 2: " <<  pos_cell2 << " -> CellID " <<  cpm_lattice->get(pos_cell2).cell_id << endl;
					
				flipped = true;
			}
		}
	}
};


