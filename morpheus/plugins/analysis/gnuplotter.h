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

#include "core/simulation.h"
#include "core/interfaces.h"
#include "core/plugin_parameter.h"
#include "core/celltype.h"
#include "core/symbol_accessor.h"
#include "gnuplot_i/gnuplot_i.h"
#include <fstream>
#include <sstream>
/** 
 *  \defgroup Gnuplotter Gnuplotter
 *  \ingroup ML_Analysis
 *  \ingroup AnalysisPlugins
 *  \brief Visualisation of spatial simulation states (cells, fields) using GnuPlot.
 * 
\section Description

Gnuplotter plots the Cell configurations (and optionally Fields) to screen or to file during simulation.
Requires GNUPlot 4.2.5 or higher.


Organised in Plots, several artistic representations can be selected :
  - Cells can be colorized using the value attribute.
  - Spatial Fields can be superimposed under the plot.
  - Additional information can be visualized using CellLabels and CellArrows.
  - VectorFields can be plotted by providing x and y components seperately. 

\subsection Attributes
- \b time-step (optional): Frequency of plotting events. If unspecified adopts to the frequency of input updates. Setting \b time-step<=0 will plot only the final state of the simulation.
- \b decorate (optional, true): Enables axis labels and legends.
- \b log-commands (optional, false): Enables logging of data and plot commands to disc. Allows to manually repeat and manipulate the plots.
- \b file-numbering (optional, time): Set the numbering of the plot images to either be consecutive or based on simulation time.

\subsection Terminal
- \b name: Specifies the output format (e.g. wxt, x11, aqua, png, postscript). Default is Gnuplot default.

\subsection Plot
- \b Cells: Plot the spatial cell pattern restricted to a 2d scenario. Cell coloring is determined by the \b value attribute.
- \b CellLabels: Put labels at the cell center according to the expression provided with the \b value attribute.
- \b CellArrows: Put arrows at the cell center according to the expression provided with the \b value attribute.
- \b Field: Plot a scalare field given by the expression in \b value. \b Coarsening will reduce the spatial data resolution.
- \b VectorField: Plot a vector field given by the expression in \b value. \b Coarsening will reduce the spatial data resolution.

\section Examples

Plot CPM state (showing cell types) to screen using WxWidgets terminal
\verbatim
<Analysis>
        <Gnuplotter interval="100">
            <Terminal name="wxt"/>
        </Gnuplotter>
<\Analysis>
\endverbatim

Example: Plot CPM state showing two cell properties to PNG files
\verbatim
<Analysis>
        <Gnuplotter interval="100">
            <Terminal name="png"/>
            <CellProperty name="target volume" type="integer"/>
            <CellProperty name="divisions" type="integer"/>
        </Gnuplotter>
<\Analysis>
\endverbatim


Example: Plot CPM state showing a cell properties (no cell membrane), and with custom color map
\verbatim
    <Analysis>
        <Gnuplotter interval="500">
            <Terminal name="wxt"/>
	    <Membrane value="false"/>
            <CellProperty name="target volume" type="integer">
		<ColorMap>
			<Color value="1"  name="black" />
			<Color value="25" name="red"   />
			<Color value="50" name="yellow"/>
		</ColorMap>decorate
	    </CellProperty>
	</Gnuplotter>
    </Analysis>
\endverbatim

Example: Plot multiple panels to screen (CPM, PDE with surface, PDE with isolines)
\verbatim
<Analysis>
        <Gnuplotter interval="500">
            <Terminal name="wxt"/>
            <Layer name="chemoattractant" surface="false" />
            <Layer name="chemoattractant" isolines="true"/>
        </Gnuplotter>
<\Analysis>
\endverbatim

Example: Plot multiple superimposed panels to postscript files (CPM, PDE surface and PDE isolines)
\verbatim
<Analysis>
        <Gnuplotter interval="500">
            <Terminal name="postscript"/>
            <Layer name="chemoattractant" superimpose="true" isolines="true" surface="false" />
        </Gnuplotter>
<\Analysis>
\endverbatim

Example: To gain plotting speed, plot PDE layer using binary files, with a resolution smaller than the world size (gradients will be interpolated)
\verbatim
<Analysis>
        <Gnuplotter interval="25">
            <Terminal name="wxt"/>
            <Layer name="chemoattractant" surface="true" binary="true" resolution="50" />
        </Gnuplotter>
<\Analysis>
\endverbatim

*/


class LabelPainter  {
public:
	LabelPainter();
	void loadFromXML(const XMLNode node);
	void init();
	set<SymbolDependency> getInputSymbols() const;
	const string& getDescription() const;
	void plotData(ostream& );
	uint fontsize() {return _fontsize;}
	string fontcolor() {return _fontcolor;}

private:
	PluginParameter2<string,XMLStringifyExpression,RequiredPolicy> value;
	vector<shared_ptr< const CellType> > celltypes;
	string _fontcolor;
	uint _fontsize;
};

class ArrowPainter  {
public:
	ArrowPainter();
    void loadFromXML(const XMLNode );
	void init();
	set<SymbolDependency> getInputSymbols() const;
	void plotData(ostream& );
	int getStyle();
	const string& getDescription() const;
	
private:
	PluginParameter2<VDOUBLE, XMLEvaluator, DefaultValPolicy > arrow;
	int style;
	
};
/** @brief Visualiser for a spatial field in GnuPlot 
 * 
 */
class FieldPainter {
public:
    void loadFromXML(const XMLNode node );
	void init();
	set<SymbolDependency> getInputSymbols() const;
	void plotData(ostream& out );
	bool getSurface() { if( surface.isDefined() ) return surface.get(); else return true;}
	int getIsolines() { if( isolines.isDefined() ) return isolines.get(); else return 0;}
	const string& getDescription() const;
	string getValueRange() const;
	
	string getColorMap() const;
	
private:
// 	vector <shared_ptr <const CellType > > celltypes;
	PluginParameter2<double,XMLEvaluator> field_value;
	PluginParameter2<int,XMLValueReader,DefaultValPolicy> coarsening;
	PluginParameter2<float,XMLEvaluator,OptionalPolicy> min_value, max_value;
	PluginParameter2<int,XMLValueReader,OptionalPolicy> isolines;
	PluginParameter2<bool,XMLValueReader,OptionalPolicy> surface;
	PluginParameter2<int,XMLValueReader,DefaultValPolicy> z_slice;
	map<double,string> color_map;
	

// 	int style;
// 	string color;
// 	int coarsening;
// 	int slice;
// 	uint isolines;
// 	int max_resolution;
// 	bool data_cropping;
};

class VectorFieldPainter  {
public:
    void loadFromXML(const XMLNode node_ );
	void init();
	set<SymbolDependency> getInputSymbols() const;
	void plotData(ostream& out_ );
	int getStyle();
	string getColor();
	string getDescription();
private:
// 	vector <shared_ptr <const CellType > > celltypes;
	PluginParameter2<VDOUBLE,XMLEvaluator> value;
// 	SymbolReader x_symbol, y_symbol;
// 	double scaling;
	int style;
	string color;
	int coarsening;
	int slice;
};

class CellPainter  {
	public:
		enum  DataLayout { binary_matrix, ascii_matrix, point_wise, boundary_cell_wise };
		struct CellBoundarySpec { vector< vector<VDOUBLE> > polygons; float value; };
	private:
		
// 		SymbolReader symbol;
		PluginParameter2<double, XMLReadableSymbol, DefaultValPolicy> symbol;
		
		struct boundarySegment{VDOUBLE pos1, pos2;};
		shared_ptr<const CPM::LAYER> cpm_layer;
		
		vector<boundarySegment> getBoundarySnippets(const Cell::Nodes& surface, bool (*comp)(const CPM::STATE& a, const CPM::STATE& b));
		float getCellValue(CPM::CELL_ID cell_id);
		vector< vector<VDOUBLE> > polygons(vector<boundarySegment> vec_bound);
		

		
		uint z_level; // denotes the z level to slice a 3d simulation
		float min_val, max_val;
		static const float transparency_value;
		static bool same_cell ( const CPM::STATE& a, const CPM::STATE& b) { return ( a.cell_id == b.cell_id && a.super_cell_id == b.super_cell_id); };
		static bool same_super_cell (const CPM::STATE& a, const CPM::STATE& b) { return (a.super_cell_id == b.super_cell_id); };
		
		bool flooding;
		bool reset_range_per_frame;
		DataLayout data_layout;
		bool is_hexagonal;
		vector<vector<float>> view;
		map<double,string> color_map;
		bool external_palette;
		bool external_range, external_range_min, external_range_max;
		double range_min, range_max;
		void updateDataLayout();
		void loadPalette(const XMLNode node);
		XMLNode savePalette() const;
		void setDefaultPalette();
		
	public:
		
		CellPainter();
		~CellPainter();
		virtual void loadFromXML(const XMLNode );
		void init();
		set<SymbolDependency> getInputSymbols() const;
		float getMaxVal() { return max_val;}
		float getMinVal() { return min_val;}
		uint getSlice() { return z_level;}
		const string& getDescription() const;
		void writeCellLayer(ostream& out);
		DataLayout getDataLayout();
		vector<CellPainter::CellBoundarySpec> getCellBoundaries();
		string getPaletteCmd();
		static float getTransparentValue() { return transparency_value; };
};

class Gnuplotter : public AnalysisPlugin
{
	public:
		/**
		 * @brief Data set describing a single plot
		 *
		 * Single plots must include at least one and at most all of the following painters
		 * * CellPainter  -- Draws CPM cell configurations
		 * * FieldPainter -- Draws a spatial field, ie. Fields
		 * * ArrowPainter -- Drawing arrows at the center of CPM cells
		 * * LabelPainter -- Putting labels based on a double or vector value at the center of a CPM cells
		 * * VectorFieldPainter -- Draws a spatial field of arrows, i.e. vector field
		 *
		 * In addition, all filenames are stored in here
		 */
		struct PlotSpec {
			PlotSpec();
			static VDOUBLE size();
			static VDOUBLE view_oversize();
			bool field, cells, labels, arrows, vectors;
			shared_ptr<CellPainter> cell_painter;
			shared_ptr<LabelPainter> label_painter;
			shared_ptr<FieldPainter> field_painter;
			shared_ptr<ArrowPainter> arrow_painter;
			shared_ptr<VectorFieldPainter> vector_field_painter;
			string field_data_file;
			string cells_data_file;
			string membranes_data_file;
			string labels_data_file;
			string arrow_data_file;
			string vector_field_file;
			string title;
		};
		
	private:
		
		struct plotPos {
			double left, top, right, bottom;
		};
		struct plotLayout {
			int rows; /// Number of rows of the plot layout
			int cols; /// Number of columns of the plot layout
			double plot_aspect_ratio; /// Height to width ratio of a single plot
			double layout_aspect_ratio; /// Height to width ratio of the layout
			vector<plotPos> plots;
		};
		enum class Terminal{ PNG, PDF, JPG, GIF, SVG, EPS, SCREEN };
		PluginParameter2<Terminal,XMLNamedValueReader,DefaultValPolicy> terminal;
		PluginParameter2<VINT,XMLValueReader,OptionalPolicy> terminal_size;
		PluginParameter2<double,XMLValueReader,OptionalPolicy> cell_opacity;
		PluginParameter2<double,XMLValueReader,DefaultValPolicy> pointsize;
		
		struct TerminalSpec {
			string name;
			VDOUBLE size;
			bool vectorized;
			bool visual;
			double font_size;
			double line_width;
			string font;
			string extension;
		};
		map<Terminal, TerminalSpec> terminal_defaults;
		
		
		static int instances;
		int instance_id;
		Gnuplot* gnuplot;
		enum class FileNumbering { SEQUENTIAL, TIME };
		PluginParameter2<FileNumbering, XMLNamedValueReader, DefaultValPolicy > file_numbering;
		bool log_plotfiles;
		bool decorate;
// 		bool interpolation_pm3d;

		vector<PlotSpec> plots;				// vector storing all plots
		plotLayout getPlotLayout( uint plot_count, bool border = true );
		
		bool pipe_data; 			// do not put data into files but directly pipe them to gnuplot
		
	public:
		Gnuplotter(); // default values
		~Gnuplotter(); // default destructor for cleanup

		DECLARE_PLUGIN("Gnuplotter");

		virtual void loadFromXML (const XMLNode xNode) override;

		virtual void init(const Scope* scope) override;
		virtual void analyse(double time) override;
		virtual void finish() override {};

};
