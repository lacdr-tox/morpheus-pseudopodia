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

#ifndef LOGGER_H
#define LOGGER_H

#include <core/interfaces.h>
#include "core/simulation.h"
#include "core/celltype.h"
#include "core/symbol_accessor.h"
#include "gnuplot_i/gnuplot_i.h"
#include <fstream>
#include <sstream>
/** \ingroup AnalysisPlugins
 *  \defgroup Logger

Writes output to text file. Optionally, plots data to image.
*/
class Logger : public AnalysisPlugin
{
	private:
		bool time_name;
		string filename_noextension, filename_log, filename_img;
		fstream fout;
		
		static int instances;
		int instance_id;
		
		string mapping_str;
		enum Mapping
		{
			ALL,
			AVERAGE,
			SUM,
			MINMAX,
			SINGLE,
			MEMBRANE_ALL,
			MEMBRANE_SINGLE,
			MEMBRANE_AVERAGE,
			PDE_ALL,
			PDE_SUM,
			PDE_AVE,
			PDE_VAR,
			PDE_MINMAX
		};
		Mapping mapping;
		
		uint cell_id;
		string celltype_str;
		string pdesymbol_str;
		bool log_global,log_cell,log_pde;
		weak_ptr<const CellType> celltype;
		vector< shared_ptr< PDE_Layer > > pdelayers;
		
		string data_format;
		
		enum SymAccType{
			DOUBLE,
			VECTOR
		};
		vector<string> headers;
		bool log_header;
		vector< SymAccType > SATypes;
		vector< string > symbol_strings;
		vector< SymbolAccessor<double> > sym_d;
		vector< SymbolAccessor<VDOUBLE> > sym_v;
		uint numsym_d,numsym_v;
		
		int pde_slice;
		
		bool plot;
		bool persist;
		bool clean;
		bool plot_endstate; // only execute plotData at end of simulation
		Gnuplot* gnuplot;

		string terminal, every, min_val_x, max_val_x, min_val_y, max_val_y, min_val_cb, max_val_cb, col_x, cols_y, col_cb, extension;
		bool logscale_x, logscale_y, logscale_cb, write_commands;
		double plot_interval;
		double last_plot_time;
		vector<string> column_tokens;
		void plotData();
// 		XMLNode stored_node;
	public:
		Logger(); // default values
		~Logger(); // default destructor for cleanup
		DECLARE_PLUGIN("Logger");
		//virtual void doTimeStep(double current_time, double timespan);
		void analyse(double time);
		void loadFromXML(const XMLNode );
		set< string > getDependSymbols();
// 		virtual XMLNode saveFromXML();
		void init(const Scope* scope);
		void finish(double time);
		void log();
};

#endif // LOGGER_H
