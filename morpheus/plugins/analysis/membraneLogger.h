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

#ifndef MEMBRANELOGGER_H
#define MEMBRANELOGGER_H

#include <core/interfaces.h>
#include <core/plugin_parameter.h>
#include "gnuplot_i/gnuplot_i.h"
// #include "core/simulation.h"
// #include "core/celltype.h"
// #include "core/symbol_accessor.h"
// #include <fstream>
// #include <sstream>

class MembraneLogger : public AnalysisPlugin
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
			SINGLE,
		};
		Mapping mapping;
		
		bool specified_cells;
		vector<CPM::CELL_ID> cellids;
		string celltype_str;
		vector< string > symbol_strings;
		shared_ptr<const CellType> celltype;
		
		
		bool plot;
		bool persist;
		bool clean;
		Gnuplot* gnuplot;
		string terminal, every, plot_extension;
		double plot_interval;
		double plot_pointsize;
		bool plot_superimpose;
		bool plot_logcommand;
		bool plot_logcommandSphere;
		vector<string> colornames;
		double last_plot_time;
		void plotData(vector<string> symbols, double time, uint cellid=-1);
		
		string getFileName(string symbol, string extension, uint cellid=-1);
		void writeMembrane(CellMembraneAccessor membrane, CPM::CELL_ID id, ostream& output=cout);
		void writeCommand3D(vector<string> filenames);
	public:
		MembraneLogger(); // default values
		~MembraneLogger(); // default destructor for cleanup
		DECLARE_PLUGIN("MembraneLogger");
		//virtual void doTimeStep(double current_time, double timespan);
		virtual void notify(double time);
		virtual void loadFromXML(const XMLNode );
		virtual set< string > getDependSymbols();

		virtual void init(double time);
		virtual void finish(double time);
		void log(double time);
};

#endif // LOGGER_H
