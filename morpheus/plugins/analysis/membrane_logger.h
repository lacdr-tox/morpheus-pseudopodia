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
/**
\defgroup MembraneLogger
\ingroup ML_Analysis
\ingroup AnalysisPlugins
\brief Logs and plots 2D MembraneProperties

- \b MembraneProperty:
  - \b symbol-ref: Symbol of MembraneProperty to plot. May contain expression.

- \b Plot:
  - \b terminal: gnuplot terminals to plot to screen or file. Note: availability of terminals depends solely on local Gnuplot installation! Plot to screen: x11 (default), wxt (unix), aqua (MacOS). Plot to file  : png, jpeg, gif, postscript, pdf.
  - \b time-step: simulation time between plots
  - \b clean: remove axes, tics and labels
  - \b superimpose: Superimpose multiple plots. Notes: 
  1. each overlay gets uniform color (no gradients), hence only useful for binary data.
  2. plots are overlayed in order specified here.
  - \b pointsize: point size for superimposed plots. Ignored by non-superimposed plots (which use "with image")
  - \b log-commands: write gnuplot commands to file.
  - \b log-commands-sphere: write gnuplot scripts to plot a 3D spherical projection of the 2D lattice.
Usage:
- Animation:
	- Create animation of rotation:  gnuplot membraneLogger_command_rotate.plt
	- Change terminal (wxt, animated gif, png): edit line membraneLogger_command_rotate.plt
- Interactive:
	- View sphere:  gnuplot --persist membraneLogger_command_sphere.plt 
	- Convert data to XYZ format: awk morphConvertMatrixtoXYZ.awk MemLog_[symbol]_[cellid]_[time].log > membraneData_XYZ.txt

By default, it plots the first symbol of the first cell. This can be changed by changing the input to the awk script (either on command line or in rotate script).

  



**/
class MembraneLogger : public AnalysisPlugin
{
private:
	
// 		string celltype_str;

//		bool specified_cells;
	PluginParameterCellType< RequiredPolicy> celltype; 
	// is identical to:
	// PluginParameter2<shared_ptr<const CellType>, XMLNamedValueReader, RequiredPolicy> celltype;

	vector<CPM::CELL_ID> cellids;
 	PluginParameter2< string , XMLValueReader, OptionalPolicy> cell_ids_str;

	vector< PluginParameter_Shared<double, XMLReadableSymbol, RequiredPolicy> > symbols;
	vector< string > symbol_names;

	
	//bool time_name;
	string filename_noextension, filename_log, filename_img;
	fstream fout;
	
	static int instances;
	int instance_id;
	
	string mapping_str;
	enum Mapping
	{
		ALL,
		SELECTION,
	};
	Mapping mapping;

	
	map<int, string> terminalmap_rev;

	Gnuplot* gnuplot;
	enum Terminal{ PNG, PDF, JPG, GIF, SVG, EPS, WXT, X11, AQUA };
	struct Plot{
		PluginParameter2<bool, XMLValueReader, DefaultValPolicy> clean;
		PluginParameter2<Terminal, XMLNamedValueReader, RequiredPolicy> terminal;
		string terminal_str;
		PluginParameter2<bool, XMLValueReader, DefaultValPolicy> logcommands;
		PluginParameter2<bool, XMLValueReader, DefaultValPolicy> logcommandSphere;
		PluginParameter2<bool, XMLValueReader, DefaultValPolicy> superimpose;
		PluginParameter2<double, XMLValueReader, OptionalPolicy> pointsize;
		string extension;
		vector<string> colornames;
	};
	Plot plot;
		
	void plotData(vector<string> symbol_names, double time, uint cellid=-1);
	
	string getFileName(string symbol, string extension, uint cellid=-1);
	void writeMembrane(const PluginParameter_Shared<double, XMLReadableSymbol, RequiredPolicy> symbol, CPM::CELL_ID id, ostream& output=cout);
	void writeCommand3D(vector<string> filenames);
	void log(double time);
	vector<CPM::CELL_ID> parseCellIDs();
	
public:
	MembraneLogger(); // default values
	~MembraneLogger(); // default destructor for cleanup
	DECLARE_PLUGIN("MembraneLogger");
	
	virtual void analyse(double time);
	virtual void loadFromXML(const XMLNode );
	virtual set< string > getDependSymbols();

	virtual void init(const Scope* scope);
	virtual void finish(double time);
};

#endif // LOGGER_H
