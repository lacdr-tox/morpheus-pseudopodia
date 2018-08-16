#ifndef CPM_P_H
#define CPM_P_H

#include "cpm.h"
#include "scales.h"
#include "cpm_sampler.h"

#if (not defined SIMULATION_CPP) && (not defined CPM_CPP)
	#error You may not include cpm_p.h from any source but simulation.cpp and cpm.cpp!
#endif

namespace CPM {
// 	double time=0;
	void loadFromXML(XMLNode node, Scope* scope);
	void loadCellTypes(XMLNode node);
	void loadCellPopulations();
	void init();
	XMLNode saveCPM();
	XMLNode saveCellTypes();
	XMLNode saveCellPopulations();
	void createLayer();
}

#endif
