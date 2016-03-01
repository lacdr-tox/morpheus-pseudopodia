//
// C++ Implementation: displacement_tracker
//
// Description: 
//
//
// Author:  <>, (C) 2009
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "displacement_tracker.h"

REGISTER_PLUGIN(DisplacementTracker);


DisplacementTracker::DisplacementTracker()
{
	celltype.setXMLPath("celltype");
	registerPluginParameter(celltype);
}


void DisplacementTracker::init(const Scope* scope) {
	AnalysisPlugin::init( scope );
	filename = celltype()->getName() + "_displacement.dat";
	fstream storage(filename.c_str(), fstream::out | fstream::trunc);
	storage << "#Time\tPOP_SUM\tMSD\tCELL-TRAJECTORIES..." << endl;
	storage.close();
}

void DisplacementTracker::analyse(double time) {
	assert(celltype());
	double avg_displacement(0.0);
	double msq_displacement(0.0);
	vector<CPM::CELL_ID> cells = celltype()->getCellIDs();
	for (uint i=0; i < cells.size(); i++) {
		map<uint,VDOUBLE>::const_iterator orig = origins.find(cells[i]);
		if ( orig != origins.end() ) {
			avg_displacement += (CPM::getCell(cells[i]).getCenter() - orig->second).abs();
			msq_displacement += (CPM::getCell(cells[i]).getCenter() - orig->second).abs_sqr();
		}
		else origins[cells[i]] = CPM::getCell(cells[i]).getCenter();
	}

	fstream storage(filename.c_str(), fstream::out | fstream::app);
	storage << time << "\t" << avg_displacement / cells.size() << "\t" << msq_displacement / cells.size();
	for (uint i=0; i < cells.size(); i++) {
		storage << "\t" << ( CPM::getCell(cells[i]).getCenter() - origins[cells[i]] ).abs();
	}
	storage << endl;
	storage.close();
	
};


