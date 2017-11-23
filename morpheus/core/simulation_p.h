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

#ifndef SIMULATION_CPP
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
#include "celltype.h"
#include "super_celltype.h"
#include "diffusion.h"
#include "time_scheduler.h"
#include "cpm.h"
#include "gnuplot_i/gnuplot_i.h"

using namespace TR1_NAMESPACE;

// make a unique source of randomness available to everyone
vector<mt19937> random_engines;
#if defined USING_CXX0X_TR1
	vector<mt19937> random_engines_alt;
#else
	vector<ranlux_base_01> random_engines_alt;
#endif	

namespace CPM {
	double time=0;
	bool enabled = false;
	
	STATE InitialState,EmptyState; // get overridden during load process;
	uint EmptyCellType;
	Time_Scale time_per_mcs("MCSDuration",1);
	UpdateData global_update_data;
	
	shared_ptr<LAYER> layer;
	shared_ptr<CPMSampler> cpm_sampler;
	Neighborhood boundary_neighborhood;
	Neighborhood update_neighborhood;
	Neighborhood surface_neighborhood;
	bool surface_everywhere=false;
// // 	vector<VINT> interaction_neighborhood;
	shared_ptr<EdgeTrackerBase> edgeTracker;
	
	vector< shared_ptr<CellType> > celltypes;
	map< std::string, uint > celltype_names;
	XMLNode xCellPop,xCellTypes,xCPM;
	
	void loadFromXML(XMLNode node);
	void loadCellTypes(XMLNode node);
	void loadCellPopulations();
	void init();
	XMLNode saveCPM() { return xCPM; };
	XMLNode saveCellTypes() { return xCellTypes; }
	XMLNode saveCellPopulations();
	void createLayer();
}


namespace SIM {
	const string dep_graph_format = "svg";
	bool generate_symbol_graph_and_exit = false;
	
	int numthreads = 1;
	shared_ptr<Lattice> global_lattice;
	Length_Scale node_length("NodeLength",1);
	string lattice_size_symbol;
	XMLNode xGlobals,xSpace;
	
// 	PDE_Sim* pde_sim=NULL;
	std::map<string, shared_ptr<PDE_Layer> > pde_layers;
	std::map<string, shared_ptr<VectorField_Layer> > vector_field_layers;
	vector< shared_ptr<AnalysisPlugin> > analysers;
	vector< shared_ptr<Plugin> > analysis_section_plugins;
	vector< shared_ptr<Plugin> > global_section_plugins;

	unique_ptr<Scope> global_scope;
	Scope* current_scope;

	string morpheus_file_version;
	string prettyFormattingTime( double time_in_sec );
	string prettyFormattingBytes(uint bytes);
	extern "C" size_t getPeakRSS();
	extern "C" size_t getCurrentRSS();
	
	uint random_seed = time(NULL);
	string fileTitle="SnapShot";

	/// Get the base name 
	inline string getSymbolBaseName(string name) { return getGlobalScope()->getSymbolBaseName(name); };
	inline set<string> getSymbolBaseNames(const set<string>& symbols){ set<string> s; for (auto &i : symbols) { s.insert( getGlobalScope()->getSymbolBaseName(i));} return s; };
	
	int main(int argc, char *argv[]);
	void init(int argc, char *argv[]);
	void finalize();
	void createDepGraph();
	void loadFromXML(XMLNode xNode);
	void setRandomSeeds( const XMLNode xNode );
}

#endif

//  Windows
#ifdef _WIN32
#include <Windows.h>
double get_wall_time(){
    LARGE_INTEGER time,freq;
    if (!QueryPerformanceFrequency(&freq)){
        //  Handle error
        return 0;
    }
    if (!QueryPerformanceCounter(&time)){
        //  Handle error
        return 0;
    }
    return (double)time.QuadPart / freq.QuadPart;
}
double get_cpu_time(){
    FILETIME a,b,c,d;
    if (GetProcessTimes(GetCurrentProcess(),&a,&b,&c,&d) != 0){
        //  Returns total user time.
        //  Can be tweaked to include kernel times as well.
        return
            (double)(d.dwLowDateTime |
            ((unsigned long long)d.dwHighDateTime << 32)) * 0.0000001;
    }else{
        //  Handle error
        return 0;
    }
}

//  Posix/Linux
#else
#include <sys/time.h>
double get_wall_time(){
    struct timeval time;
    if (gettimeofday(&time,NULL)){
        //  Handle error
        return 0;
    }
    return (double)time.tv_sec + (double)time.tv_usec * .000001;
}
double get_cpu_time(){
    return (double)clock() / CLOCKS_PER_SEC;
}
#endif

