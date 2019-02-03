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

#if (not defined SIMULATION_CPP) && (not defined CPM_CPP)
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
#include "parse_arg.h"
#include "interfaces.h"
#include "random_functions.h"
#include "celltype.h"
#include "super_celltype.h"
#include "diffusion.h"
#include "time_scheduler.h"
#include "gnuplot_i/gnuplot_i.h"



namespace SIM {
	const string dep_graph_format = "svg";
	bool generate_symbol_graph_and_exit = false;
	
	int numthreads = 1;
	shared_ptr<Lattice> global_lattice;
	Length_Scale node_length("NodeLength",1);
	string lattice_size_symbol;
	XMLNode xDescription,xGlobals,xSpace;
	
	
// 	PDE_Sim* pde_sim=NULL;
// 	std::map<string, shared_ptr<PDE_Layer> > pde_layers;
// 	std::map<string, shared_ptr<VectorField_Layer> > vector_field_layers;
	vector< shared_ptr<AnalysisPlugin> > analysers;
	vector< shared_ptr<Plugin> > analysis_section_plugins;
	vector< shared_ptr<Plugin> > global_section_plugins;

	unique_ptr<Scope> global_scope;
	Scope* current_scope;

	string morpheus_file_version;
	string prettyFormattingTime( double time_in_sec );
	string prettyFormattingBytes(uint bytes);

	
	uint random_seed = time(NULL);
	string fileTitle="SnapShot";
	/// directory to read data from
	string input_directory = ".";
	/// directory to write data to
	string output_directory = ".";

	/// Get the base name 
// 	inline string getSymbolBaseName(string name) { return getGlobalScope()->getSymbolBaseName(name); };
// 	inline set<string> getSymbolBaseNames(const set<string>& symbols){ set<string> s; for (auto &i : symbols) { s.insert( getGlobalScope()->getSymbolBaseName(i));} return s; };
	
	int main(int argc, char *argv[]);
	bool init(int argc, char *argv[]);
	void finalize();
	void createDepGraph();
	void loadFromXML(XMLNode xNode);
	void setRandomSeeds( const XMLNode xNode );
	
}

#endif

