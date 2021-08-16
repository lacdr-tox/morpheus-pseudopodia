//////
//
// This file is part of the modelling and simulation framework 'Morpheus',
// and is made available under the terms of the BSD 3-clause license (see LICENSE
// file that comes with the distribution or https://opensource.org/licenses/BSD-3-Clause).
//
// Authors:  Joern Starruss and Walter de Back
// Copyright 2009-2016, Technische Universität Dresden, Germany
//
//////

#ifndef SIMULATION_P_H
#define SIMULATION_P_H

#if (not defined SIMULATION_CPP) && (not defined CPM_CPP) && (not defined MODEL_TEST_CPP)
	#error You may not include simulation_p.h from any source but simulation.cpp!
#endif

#include "simulation.h"

#include <map>
#include <algorithm>

#include <sys/types.h>
#include <sys/stat.h>
#include <thread>

#ifdef HAVE_GNU_SYSLIB_H
	#include <sys/unistd.h>
	
	#ifdef WIN32
		#include <errno.h>
	#else
		#include <sys/errno.h>
	#endif
#endif

#include "version.h"
#include "interfaces.h"
#include "random_functions.h"
#include "lattice_plugin.h"
#include "celltype.h"
#include "super_celltype.h"
#include "diffusion.h"
#include "time_scheduler.h"
#include "gnuplot_i/gnuplot_i.h"



namespace SIM {

	int main(int argc, char *argv[]);
	bool init(int argc, char *argv[]);
	bool init(string model, map<string,string> overrides);
	void finalize();
	void createDepGraph();
	void loadFromXML(XMLNode xNode);
	void setRandomSeeds( const XMLNode xNode );
	
}

#endif

