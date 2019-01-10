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

#include "core/interfaces.h"
#include "core/focusrange.h"
#include "core/celltype.h"
#include "core/data_mapper.h"
#include "core/plugin_parameter.h"
#include "gnuplot_i/gnuplot_i.h"
#include <fstream>
#include <sstream>

/*
New features (compared to Logger of Morpheus 1.2)
- More consistent interface
- Select symbols instead of Format
- Uses FocusRange restriction and FocusRangeIterators to efficiently get relevant data
- Create multiple plots from same datafile
- More flexible plotting function (also enables e.g. space-time-plots)


TODO list:

Bugs:
- write_csv crashes when trying to get cellIDs in growing population ( line 667: focus.cellID() )
- restrictions on cell ids do not seem to work (line 315), not even for single cell ID. Apparently, these not added to restrictions map.

DONE (moved to restrictions) - remove domain-only
DONE - celltype-selection & at root like Slice (global to all columns)
  - Column symbol-ref (without Global/Cells/Field)
- add symbol if not in columns but in plot
DONE - dropdown list for tab delimited: , \t ; 

Urgent:
- make FocusRange restrictions into multimap such that one can log multiple celltypes, cells, and even slices
- make TimeSymbol, SpaceSymbol and MembraneSpaceSymbol required to be able to select them for Plots
- fix issue with user-specified separator ("\t" is written as string instead of tab)

Questions:
- should we enable mapping within Logger or make Mappings (and Projections) exernal plugins?

Future features:
- add a range of predefined color palettes to override the default gnuplot colorscale: https://github.com/Gnuplotting/gnuplot-palettes
- enable plotting of matrix formatted files
- write matrices to HDF5 files

*/

/**
\defgroup Logger
\ingroup ML_Analysis
\ingroup AnalysisPlugins
\brief Writes output to text file. Optionally, plots graphs.

\section Description

Versatile interface to 
1. write simulation data to files in CSV and matrix format and 
2. draw a variety of plots such as time plots, phase plots, space-time plots, surface plots, etc.

\subsection Input

- \b time-step (optional): time between logging events. If unspecified adopts to the frequency of input updates. Setting \b time-step<=0 will log only the final state of the simulation.
- \b name (optional, default=none): shows in GUI, only for user-convenience
- \b force-node-granularity (optional, default=false): force logging per node in a grid-like fashion.
- \b exclude-medium (optional, default=true): when logging cell properties, only include biological cells.

\subsubsection Symbol (required)

Specifies at one or more symbols to write to data file. Symbol can be flexibly combined, but the data format is determined by the symbol with the smallest granularity.

- \b symbol-ref (required): symbol referring to e.g. a global \ref ML_Variable, a cell \ref ML_Property, a \ref ML_MembraneProperty, or a \ref ML_Field.

\subsection Output (required)

Specifies the details of the output file

- \b file-format (optional, default=automatic): output can be written in CSV or matrix format (for surface plots)
- \b header (optional, default=true): whether or not to write symbol names as header
- \b separator (optional, default="tab"): specifies delimiter between values
- \b file-name (optional, default=automatic): filename are created automatically by default, but are overridden if specified
- \b file-numbering (optional, default=time): filenames are named according to simulation time or incremental numbering
- \b file-separation (optional, default=none): writes separate files for time, cells or both (cell+time)

\subsection Restriction (optional)

Restrict the data query to a certain slice, a cell type or certain cell ids.

- \b Slice: restrict query to slice (note: require node granularity)
  - \b axis  (required): x,y,z axis to slice
  - \b value (required): position along axis at which to slice the space (can be expression)

- \b Celltype: restrict query to cell of specific celltype
  - \b celltype (required): name of \ref ML_CellType to query

- \b Cells: write values of \ref ML_Property or \ref ML_MembraneProperty
  - \b cell-ids (required): range of cell IDs to include. Syntax:
    - list of comma-separated IDs: "1,2,5", or
    - range of IDs with dash: "4-8", or
    - combination of list and range: "12-15, 24-28".

- \b domain-only (optional, default=true): query only nodes within domain. if false, also include node outside of domain.
- \b force-node-granularity (optional, default=false): granularity of automatically detected by default, but may be overridden if specified

\subsection Plots (optional)

Specifies one or more plots, generated from the data in the written data file.

\subsubsection Plot (optional)

Symbols can be selected for X and Y dimensions + color and the range of time points can be selected.

- \b X-axis (required): specify a symbol to use as x axis
  - \b logarithmic (optional, default=false): use logarithmic scale
  - \b minimum (optional, default=none): fix minimum of axis
  - \b maximum (optional, default=none): fix maximum of axis
  - \b palette (optional): choose a color scale
  - \b reverse-palette (optional, default="false"): reverse the color scale
  
- \b Y-axis (required): specify one or more symbols to show on y-axis
  - options as for X-axis

- \b Color-bar (optional): specify a symbol to color points or lines
  - options as for X-axis

- \b Range (optional): select time range or filter data to include in plot:
  - \b Data (optional): skip data points using gnuplot's 'every' keyword
    - \b first-line (optional)
    - \b last-line (optional)
    - \b increment (optional)
    This yields: every [increment]::[first-line]::[last-line]
  - \b Time (optional): select time range of data to include
    - all: plot  all time points
    - since last plot: only plot time points since last plot event
    - history: only plot time points between current and (current - history). Requires "history" value to be specified.
    - current: only plot time point
    - \b history (optional): length of history to plot, in units of simulation time (only used when mode="history")

- \b Terminal (required): select an output format
    - \b terminal (required, default=png): select on-screen or file terminal (e.g. wxt, aqua, png, jpg, svg, pdf)
    - \b plot-size (optional): size of the plot, e.g. 600,400,0
    - \b file-numbering (optional, default=time): filenames are named according to simulation time or incremental numbering

- \b Style (optonal): configure the style of the plot
	- \b decorate (optional, default=false): hide axis labels, legend and colorbar
	- \b style (optional, default=points): points, lines, or linespoints 
	- \b line-width (optional, default=1): line width if using lines or linespoints
	- \b point-size (optional, default=1): point size if using points or linespoints
	- \b grid (optional, default=false): draw dotted line in plot at major ticks
	
- \b log-commands (optional, default=false): write gnuplot commands to file
- \b time-step (optional, default=0): interval to create plot.
  - Special values:
    - -1  : end of simulation
    -  0  : same interval as Logger
    - else: should be multiple of Logger/time-step
- \b title (optional, default=none): Title caption for the Plot

\subsubsection SurfacePlot (optional)

Draws a surface plot (e.g. heatmap) from data in matrix format

- \b Color-bar (optional): specify a symbol to color points or lines
  - \b logarithmic (optional, default=false): use logarithmic scale
  - \b minimum (optional, default=none): fix minimum of axis
  - \b maximum (optional, default=none): fix maximum of axis
  - \b palette (optional): choose a color scale
  - \b reverse-palette (optional, default="false"): reverse the color scale

- \b Terminal (required): select an output format
    - \b terminal (required, default=png): select on-screen or file terminal (e.g. wxt, aqua, png, jpg, svg, pdf)
    - \b plot-size (optional): size of the plot, e.g. 600,400,0
    - \b file-numbering (optional, default=time): filenames are named according to simulation time or incremental numbering

	
- \b log-commands (optional, default=false): write gnuplot commands to file
- \b time-step (optional, default=0): interval to create plot.
  - Special values:
    - -1  : end of simulation
    -  0  : same interval as Logger
    - else: should be multiple of Logger/time-step
- \b name (optional, default=none): shows in GUI, only for user-convenience


\section Examples

The example below assume the following symbols to be defined:
\verbatim
TimeSymbol symbol="time"
SpaceSymbol symbol="space"
Field symbol="f"
Property symbol="p"
MembraneProperty symbol="m"
\endverbatim

\subsection Two variables: time and phase plots

Log two global Variables (N and P) n the format below (see PredatorPrey example).
And create a timeplot with multiple variables plotting on Y-axis.
And draw a phase plot with N and P on both axis, indicating time as colors.

\verbatim
<Logger time-step="5">
    <Column>
        <Global symbol-ref="N"/>
    </Column>
    <Column>
        <Global symbol-ref="P"/>
    </Column>
    <Output/>
    <Plot lines-or-points="lines" name="time plot">
        <X-axis>
            <Column symbol-ref="time"/>
        </X-axis>
        <Y-axis>
            <Column symbol-ref="N"/>
            <Column symbol-ref="P"/>
        </Y-axis>
    </Plot>
    <Plot lines-or-points="lines" name="phase plot">
        <X-axis>
            <Column symbol-ref="N"/>
        </X-axis>
        <Y-axis>
            <Column symbol-ref="P"/>
        </Y-axis>
        <Color-bar>
            <Column symbol-ref="time"/>
        </Color-bar>
    </Plot>
</Logger>
\endverbatim

Output format:
\verbatim
time	N	P
0	0.1	0.5
5	0.102334	0.495966
10	0.105018	0.492469
...
\endverbatim

\subsection Cell properties: colored time and phase plots

Log cell properties in a population of cells (see LateralSignaling example).
And create a time plot with points colored according to a cell property.
And draw a phase plot with points colored according to time.

\verbatim
<Logger time-step="0.05">
    <Column>
        <Cells symbol-ref="X"/>
    </Column>
    <Output/>
    <Plot point-size="0.5" name="time plot">
        <X-axis>
            <Column symbol-ref="time"/>
        </X-axis>
        <Y-axis>
            <Column symbol-ref="X"/>
        </Y-axis>
        <Color-bar>
            <Column symbol-ref="Y"/>
        </Color-bar>
    </Plot>
    <Column>
        <Cells symbol-ref="Y"/>
    </Column>
    <Plot point-size="0.5" name="phase plot">
        <X-axis>
            <Column symbol-ref="X"/>
        </X-axis>
        <Y-axis>
            <Column symbol-ref="Y"/>
        </Y-axis>
        <Color-bar>
            <Column symbol-ref="t"/>
        </Color-bar>
    </Plot>
</Logger>
\endverbatim

Output format:
\verbatim
time	cell.id	X	Y
0	1	0	0
0	2	0	0
...
0.05	1	0.0580316	0.0576386
0.05	2	0.0580715	0.0576858
...
\endverbatim

\subsection Slicing: space-time plot and profile plot

Log a slice of two Fields ('a' and 'i') in the following format:
Create a space-time plot with time on X-axis, space on Y-axis and colors indicating concentration of 'a'.
Draw a concentration profile of 'a' at the slice at the current time point.

\verbatim
<Logger time-step="50">
    <Column>
        <Field symbol-ref="a"/>
    </Column>
    <Column>
        <Field symbol-ref="i"/>
    </Column>
    <Slice value="size.y/2" axis="y"/>
    <Output/>
    <Plot time-step="2500" name="space-time plot">
        <X-axis>
            <Column symbol-ref="time"/>
        </X-axis>
        <Y-axis>
            <Column symbol-ref="space.x"/>
        </Y-axis>
        <Color-bar>
            <Column symbol-ref="a"/>
        </Color-bar>
    </Plot>
    <Plot time-range="current" lines-or-points="linespoints" time-step="2500" name="profile plot">
        <X-axis>
            <Column symbol-ref="space.x"/>
        </X-axis>
        <Y-axis>
            <Column symbol-ref="a"/>
        </Y-axis>
    </Plot>
</Logger>
\endverbatim

Output format:
\verbatim
time	space.x	space.y	space.z	a	i
0	0	50	0	0.347683	0.1
0	1	50	0	0.277144	0.1
0	2	50	0	0.622711	0.1
...
25	0	50	0	0.349179	0.0504817
25	1	50	0	0.347195	0.0504857
25	2	50	0	0.390495	0.0504852
...
\endverbatim

*/

class LoggerWriterBase;
class LoggerPlotBase;

class Logger : public AnalysisPlugin
{
	static int instances;
	int instance_id;
private:
	XMLNode stored_node;
	vector< PluginParameter_Shared<double, XMLReadableSymbol> > inputs;
	vector<shared_ptr<LoggerWriterBase> > writers;
	vector<shared_ptr<LoggerPlotBase> > plots;

	Granularity logger_granularity = Granularity::Global;
	bool permit_incomplete_symbols = true;
	FocusRange range;

	// Restrictions
	//multimap<FocusRangeAxis, int> restrictions;
	multimap<FocusRangeAxis, int> restrictions;
	// Celltype
	PluginParameterCellType< OptionalPolicy > celltype;
	// Cell IDs
	PluginParameter2<string, XMLValueReader, OptionalPolicy> cellids_str;
	vector<CPM::CELL_ID> parseCellIDs(string cell_ids_string);
	vector<CPM::CELL_ID> cellids;
	// Slice
	bool slice;
	PluginParameter2<double, XMLEvaluator, OptionalPolicy> slice_value;
	PluginParameter2<FocusRangeAxis, XMLNamedValueReader, OptionalPolicy> slice_axis;
	// Domain only
	PluginParameter2<bool, XMLValueReader, DefaultValPolicy> domain_only;
	// Node granularity
	PluginParameter2<bool, XMLValueReader, DefaultValPolicy> force_node_granularity;
	// Medium exclusion
	PluginParameter2<bool, XMLValueReader, OptionalPolicy> exclude_medium;

public:
	Logger(); // default values
	DECLARE_PLUGIN("Logger");
	~Logger(); // default destructor for cleanup
	void loadFromXML(const XMLNode, Scope* scope ) override;
	void init(const Scope* scope) override;
	void analyse(double time) override;
	void finish() override;
	
	const vector< PluginParameter_Shared<double, XMLReadableSymbol> >& getInputs() const { return inputs;};
	string getInputsDescription(const string& s) const;
	int addWriter(shared_ptr<LoggerWriterBase> writer);
	const vector<shared_ptr<LoggerWriterBase> >& getWriters() const { return writers; };
	int getInstanceID() const { return instance_id; };
	int getInstanceNum() const { return instances; };
// 	bool getForceNodeGranularity();

	Granularity getGranularity() { return logger_granularity; }
	const multimap<FocusRangeAxis, int>& getRestrictions() { return restrictions; }
	bool permitIncompleteSymbols() { return permit_incomplete_symbols; }
	bool getDomainOnly() { return domain_only(); };
};

class LoggerWriterBase {
public:
	LoggerWriterBase(Logger& logger) : logger(logger) {};
	virtual ~LoggerWriterBase() {};
	virtual void init() =0;
	virtual void write() =0;
	virtual void finish() {};

protected:
	Logger& logger;
};

/** This Writer shall write data in text mode that is accessible to GnuPlot **/
class LoggerTextWriter : public LoggerWriterBase {
public:
	enum class FileSeparation { TIME, CELL, TIME_CELL, NONE };
	enum class OutputFormat{ CSV, MATRIX };
	
	LoggerTextWriter(Logger& logger, LoggerTextWriter::OutputFormat format);
	LoggerTextWriter(Logger& logger, string xml_base_path = "");
	void init() override;
	void write() override;
	
	OutputFormat getOutputFormat() const { return file_format; };
	FileSeparation getFileSeparation() const { return file_separation; } ;
	string getSeparator() { return separator(); };
	string getDataFile(double time, string symbol = "") const;
	string getDataFile(const SymbolFocus& f, double time, string symbol = "") const;
	string getDataFileBaseName() const { return file_basename; };
	vector<string> getDataFiles();
	const vector <SymbolAccessor<double> >& getSymbols() { return output_symbols;};
	int addSymbol(string name);
	bool hasHeader(){ return header(); }; 
	
private:
	const Scope* output_scope;
	OutputFormat file_format;
	bool forced_format;
	
	int file_write_count;
	FileSeparation file_separation;
	string file_basename;
	string file_extension;
	set<string> files_opened;
	
	PluginParameter2<bool, XMLValueReader, DefaultValPolicy> header;
	PluginParameter2<string, XMLNamedValueReader, DefaultValPolicy> separator;
	PluginParameter2<string, XMLValueReader, DefaultValPolicy> filename;
	
	enum class FileNumbering { SEQUENTIAL, TIME };
	PluginParameter2<FileNumbering, XMLNamedValueReader, DefaultValPolicy > file_numbering;
	PluginParameter2<FileSeparation, XMLNamedValueReader, DefaultValPolicy> xml_file_separation;
	PluginParameter2<OutputFormat, XMLNamedValueReader, OptionalPolicy> xml_file_format;
		
	unique_ptr<ofstream> getOutFile(const SymbolFocus& f, string symbol = "");
	void writeCSV();
	void writeMatrix();
	void writeMatrixColHeader(FocusRange range, ofstream& fs);
	void writeMatrixRowHeader(SymbolFocus focus, FocusRange range, ofstream& fs);	
	vector <SymbolAccessor<double> > output_symbols;
	vector <string> csv_header;
	
	
	
};

/** This Writer shall provide data to GnuPlot in piping mode .. 
    Also might provide a matrix data backend for non-fixed-size cell populations
 **/
class LoggerGnuplotWriter : public LoggerWriterBase {
};


#ifdef HAVE_HDF5

/** This Writer shall write data in hdf5 format, for easy input into python via pandas  .. **/
class LoggerHDF5Writer : public LoggerWriterBase {
	
};

#endif

class LoggerPlotBase {
public:
	enum class Palette { DEFAULT, HOT, AFMHOT, GRAY, OCEAN, RAINBOW, GRV };
	LoggerPlotBase(Logger& logger, string xml_base_path);
	virtual void init() =0;
	void checkedPlot();
	
protected:
	virtual void plot() =0;
	
	enum class Terminal{ PNG, PDF, JPG, GIF, SVG, EPS, SCREEN };
	map<Terminal, string> terminal_file_extension;
	map<Terminal, string> terminal_name;
	PluginParameter2<Terminal, XMLNamedValueReader, DefaultValPolicy> terminal;
	PluginParameter2<bool, XMLValueReader, DefaultValPolicy> logcommands;
	PluginParameter2<double, XMLValueReader, DefaultValPolicy> time_step;
	PluginParameter2<string, XMLValueReader, OptionalPolicy> title;
	double last_plot_time;
	uint plot_num;
	
	enum class FileNumbering { SEQUENTIAL, TIME };
	PluginParameter2<FileNumbering, XMLNamedValueReader, DefaultValPolicy> file_numbering;
	PluginParameter2<VINT, XMLValueReader, OptionalPolicy> plotsize;
	
	struct Axis{
		bool defined;
		PluginParameter2<string, XMLValueReader, OptionalPolicy> symbol;
		int column_num;
		vector< PluginParameter_Shared<string, XMLValueReader, OptionalPolicy> > symbols;
		vector<int> column_nums;
		string label;
		
		PluginParameter2<double, XMLEvaluator, OptionalPolicy> min;
		PluginParameter2<double, XMLEvaluator, OptionalPolicy> max;
		PluginParameter2<bool, XMLValueReader, DefaultValPolicy> logarithmic;
		PluginParameter2<Palette, XMLNamedValueReader, OptionalPolicy> palette;
		PluginParameter2<bool, XMLValueReader, DefaultValPolicy> palette_reverse;
	};
};

class LoggerLinePlot : public LoggerPlotBase {
public:
	LoggerLinePlot(Logger& logger, string xml_base_path);
	void init() override;
	void plot() override;
	
private:
	Logger& logger;
	shared_ptr<LoggerTextWriter> writer;
	shared_ptr<Gnuplot> gnuplot;
		// Range
	enum class TimeRange { ALL, SINCELAST, HISTORY, CURRENT};
	PluginParameter2<double, XMLEvaluator, OptionalPolicy> history;
	PluginParameter2<TimeRange, XMLNamedValueReader, DefaultValPolicy> timerange;
	PluginParameter2<double, XMLEvaluator, OptionalPolicy> first_line;
	PluginParameter2<double, XMLEvaluator, OptionalPolicy> last_line;
	PluginParameter2<double, XMLEvaluator, OptionalPolicy> increment;
	
		// LineStyle
	enum class Style { POINTS, LINES, LINESPOINTS};
	PluginParameter2<Style, XMLNamedValueReader, DefaultValPolicy> style;
	PluginParameter2<bool, XMLValueReader, DefaultValPolicy> decorate;
	PluginParameter2<double, XMLValueReader, DefaultValPolicy> pointsize;
	PluginParameter2<double, XMLValueReader, DefaultValPolicy> linewidth;
	PluginParameter2<VINT, XMLValueReader, OptionalPolicy> plotsize;
	PluginParameter2<bool, XMLValueReader, DefaultValPolicy> grid;

	struct { Axis x; Axis y; Axis cb; } axes;
	int num_defined_symbols;
};

class LoggerMatrixPlot : public LoggerPlotBase {
public:
	LoggerMatrixPlot(Logger& logger, string xml_base_path);
	void init() override;
	void plot() override;
	
private:
	Logger& logger;
	shared_ptr<LoggerTextWriter> writer;
	shared_ptr<Gnuplot> gnuplot;
	Axis cb_axis;
	vector<string> getLabels();
};

#endif // LOGGER_H
