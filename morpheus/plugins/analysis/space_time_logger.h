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

#ifndef SPACETIMELOGGER_H
#define SPACETIMELOGGER_H

#include <core/interfaces.h>
#include "core/simulation.h"
#include "core/celltype.h"
#include "core/symbol_accessor.h"
#include "core/cell_membrane_accessor.h"
#include "gnuplot_i/gnuplot_i.h"
#include <fstream>
#include <sstream>

/** \defgroup MembranePropertyLogger
\ingroup analysis_plugins

\section Description
MembranePropertyMembranePropertyLogger plugin can report the values of properties of a particular cell to stdout or file.

\section Examples
\verbatim
	<MembranePropertyLogger interval="5">
		<Filename name="delta_notch.log" />
		<Cell type="Celltype1" value="2" />
		<Info string="time D N Vt2 V" delimiter=" "/>
	</MembranePropertyLogger>
*/

class SpaceTimeLogger : public Analysis_Listener
{
	private:
		string filename;
		struct fileData{
			string filename;
			string symbol;
			string symbol_name;
			uint cell_id;
			string celltype;
		} ;
		vector<fileData> files;
		
		vector<ofstream*> fout;
		
		
		string mapping_str;
		enum Mapping
		{
			ALL,
			AVERAGE,
			SINGLE, 
			NONE
		};
		Mapping mapping;
		
		
		struct plot {
			Gnuplot* gnuplot;
			uint plot_cell_id;
			string plot_symbol;
			uint plot_file_index;
			double plot_min;
			double plot_max;
			double plot_max_time;
			bool persist;
			string terminal, every, min_val_x, max_val_x, min_val_y, max_val_y, col_x, cols_y, extension;
			double plot_interval;
			double plot_last_time;
			vector<string> column_tokens;
			bool time_name;
			bool write_commands;
		};
		vector< plot > plots;
		
		uint cell_id;
		string celltype_str;
		shared_ptr<const CellType> celltype;
		string data_format;
		string data_format_delimiter;
		vector<string> symbol_strings; 
		
		
		struct accessor {
			string symbol;
			string fullname;
			
			bool is_membrane_property;
			
			CellMembraneAccessor membrane_accessor;
			
			SymbolAccessor<double> pde_accessor;
			shared_ptr < PDE_Layer > pde_layer;
			VINT pde_size;
			uint dimensions;
			uint slice_y;
			uint slice_z;
		};
		
		vector< accessor > accessors;
		
		//vector< string > symbol_strings;
		//vector< CellMembraneAccessor > membrane_accessors;
		
		bool do_plots;

		void plotData(plot p, double time);
	public:
		SpaceTimeLogger(); // default values
		~SpaceTimeLogger(); // default destructor for cleanup
		DECLARE_PLUGIN("SpaceTimeLogger");
		//virtual void doTimeStep(double current_time, double timespan);
		virtual void notify(double time);
		virtual void loadFromXML(const XMLNode );
		virtual set< string > getDependSymbols();
		virtual void init(double time);
		virtual void finish(double time);
		void log(double time);
};

#endif // SPACETIMELOGGER_H
