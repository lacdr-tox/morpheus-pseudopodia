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
#ifndef GNUPLOTTER_H
#define GNUPLOTTER_H

#include "core/simulation.h"
#include "core/interfaces.h"
#include "core/plugin_parameter.h"
#include "core/celltype.h"
#ifdef HAVE_SUPERCELLS
#include "core/super_celltype.h"
#endif
#include "gnuplot_i/gnuplot_i.h"
#include <fstream>
#include <sstream>
/**
\defgroup Gnuplotter Gnuplotter
\ingroup ML_Analysis
\ingroup AnalysisPlugins
\brief Visualisation of spatial simulation states (cells, fields) using GnuPlot.
 
\section Description

Gnuplotter plots the Cell configurations (and optionally Fields) to screen or to file during simulation.
Requires GNUPlot 4.2.5 or higher.


Organized in Plots, several artistic representations can be selected:
  - Cells can be colorized using the value attribute.
  - Spatial Fields can be superimposed under the plot.
  - Additional information can be visualized using CellLabels and CellArrows.
  - VectorFields can be plotted by providing x and y components separately. 

\subsection Attributes
- \b time-step (optional): Frequency of plotting events. If unspecified, adopts to the frequency of input updates. Setting \b time-step=0 will plot only the final state of the simulation.
- \b decorate (optional, true): Enables axis labels and legends.
- \b log-commands (optional, false): Enables logging of data and plotting commands to disc. Allows to manually repeat and manipulate the plots.
- \b file-numbering (optional, time): Set the numbering of the plot images to either be consecutive or based on simulation time.

\subsection Terminal
- \b name: Specifies the output format (e.g. wxt, x11, aqua, png, postscript). Default is Gnuplot default.

\subsection Plot
- \b title: Title of the plot.
- \b slice: Z-slice of 3D lattice to plot.
- \b Cells: Plot the spatial cell pattern restricted to a 2d scenario. Cell coloring is determined by the \b value attribute.
- \b CellLabels: Put labels at the cell center according to the expression provided with the \b value attribute.
- \b CellArrows: Put arrows at the cell center according to the expression provided with the \b value attribute.
- \b CellLinks: Put a line connecting cell centers for each link created by \ref ML_MechanicalLink component.
- \b Field: Plot a scalar field given by the expression in \b value. \b Coarsening will reduce the spatial data resolution.
- \b VectorField: Plot a vector field given by the expression in \b value. \b Coarsening will reduce the spatial data resolution.

\section Examples

Plot CPM state (showing cell types) to screen using WxWidgets terminal
~~~~~~~~~~~~~~~~~~~~~{.xml}
<Analysis>
        <Gnuplotter time-step="100">
            <Terminal name="png"/>
            <Plot>
                <Cells value="cell.id" />
            </Plot>
        </Gnuplotter>
<\Analysis>
~~~~~~~~~~~~~~~~~~~~~

Example: Plot CPM state showing two cell properties to PNG files
~~~~~~~~~~~~~~~~~~~~~{.xml}
<Analysis>
        <Gnuplotter time-step="100">
            <Terminal name="png"/>
             <Plot>
                <Cells value="cell.volume.target" />
             </Plot>
             <Plot>
                <Cells value="cell.divisions">
					<ColorMap>
						<Color value="1"  name="black" />
						<Color value="25" name="red"   />
						<Color value="50" name="yellow"/>
					</ColorMap>
				</Cells>
            </Plot>
        </Gnuplotter>
<\Analysis>
~~~~~~~~~~~~~~~~~~~~~


Example: Plot multiple panels to screen (CPM cell outlines, PDE with surface, PDE with isolines), reducing the plot resolution for speed
~~~~~~~~~~~~~~~~~~~~~{.xml}
<Analysis>
        <Gnuplotter interval="500">
            <Terminal name="wxt"/>
            <Plot>
                <Cells />
				<Field symbol-ref="chemoattractant" surface="false" coarsening="2"/>
            </Plot
			<Plot>
                <Cells />
				<Field symbol-ref="chemoattractant" isolines="true" coarsening="2"/>
             </Plot
        </Gnuplotter>
<\Analysis>
~~~~~~~~~~~~~~~~~~~~~
*/


class LabelPainter  {
public:
	LabelPainter();
	void loadFromXML(const XMLNode node, const Scope * scope);
	void init(const Scope* scope, int slice);
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
	int z_slice;
};

class ArrowPainter  {
public:
	ArrowPainter();
	void loadFromXML(const XMLNode, const Scope * scope);
	void init(const Scope* scope, int slice);
	set<SymbolDependency> getInputSymbols() const;
	void plotData(ostream& );
	int getStyle();
	const string& getDescription() const;
	
private:
	PluginParameter2<VDOUBLE, XMLEvaluator, DefaultValPolicy > arrow;
	PluginParameter2<bool, XMLNamedValueReader, DefaultValPolicy > centering;
	int style;
	int z_slice;
	
};
/** @brief Visualiser for a spatial field in GnuPlot 
 * 
 */
class FieldPainter {
public:
    void loadFromXML(const XMLNode node, const Scope * scope);
	void init(const Scope* scope, int slice);
	set<SymbolDependency> getInputSymbols() const;
	void plotData(ostream& out );
	bool getSurface() { if( surface.isDefined() ) return surface.get(); else return true;}
	int getIsolines() { if( isolines.isDefined() ) return isolines.get(); else return 0;}
	const string& getDescription() const;
	string getValueRange() const;
	int getCoarsening() const;
	
	string getColorMap() const;
	
private:
// 	vector <shared_ptr <const CellType > > celltypes;
	PluginParameter2<double,XMLEvaluator> field_value;
	PluginParameter2<int,XMLValueReader,DefaultValPolicy> coarsening;
	PluginParameter2<float,XMLEvaluator,OptionalPolicy> min_value, max_value;
	PluginParameter2<int,XMLValueReader,OptionalPolicy> isolines;
	PluginParameter2<bool,XMLValueReader,OptionalPolicy> surface;
// 	PluginParameter2<int,XMLValueReader,DefaultValPolicy> z_slice;
	map<double,string> color_map;
	int z_slice;
	

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
    void loadFromXML(const XMLNode node, const Scope * scope );
	void init(const Scope* scope, int slice);
	set<SymbolDependency> getInputSymbols() const;
	void plotData(ostream& out_ );
	int getStyle();
	string getColor();
	string getDescription();
private:
// 	vector <shared_ptr <const CellType > > celltypes;
	PluginParameter2<VDOUBLE,XMLEvaluator> value;
	PluginParameter2<bool, XMLNamedValueReader, DefaultValPolicy > centering;
// 	SymbolReader x_symbol, y_symbol;
// 	double scaling;
	int style;
	string color;
	int coarsening;
	int z_slice;
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
		

		
		int z_level; // denotes the z level to slice a 3d simulation
		float min_val, max_val;
		static const float transparency_value;
		static bool same_cell ( const CPM::STATE& a, const CPM::STATE& b) { return a.cell_id==b.cell_id; };
		static bool same_super_cell (const CPM::STATE& a, const CPM::STATE& b) { return (true); };
		
		bool flooding;
		bool reset_range_per_frame;
		DataLayout data_layout;
		bool is_hexagonal;
		vector<vector<float>> view;
		map<double,string> color_map;
		bool external_palette;
		bool external_range, external_range_min, external_range_max;
		double range_min, range_max;
		double fill_opacity;
		void updateDataLayout();
		void loadPalette(const XMLNode node);
		XMLNode savePalette() const;
		void setDefaultPalette();
		
	public:
		
		CellPainter();
		~CellPainter();
		void loadFromXML(const XMLNode, const Scope* scope );
		void init(const Scope* scope, int slice);
		set<SymbolDependency> getInputSymbols() const;
		float getMaxVal() { return max_val;}
		float getMinVal() { return min_val;}
		double getOpacity() { return fill_opacity; }
		const string& getDescription() const;
		void writeCellLayer(ostream& out);
		DataLayout getDataLayout();
		void setDataLayout(CellPainter::DataLayout layout);
		vector<CellPainter::CellBoundarySpec> getCellBoundaries();
		string getPaletteCmd();
		static float getTransparentValue() { return transparency_value; };
};

class CellLinkPainter {
	typedef vector<CPM::CELL_ID> LinkType;
	SymbolAccessor<LinkType> bonds;
	string color;
	int z_slice;
	
public: 
	CellLinkPainter();
	void loadFromXML(const XMLNode node, const Scope* scope);
	void init(const Scope* scope, int slice);
	void plotData(ostream& );
	string getColor() { return color; }
	string getDescription() const;
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
			bool field, cells, labels, arrows, vectors, links;
			int  z_slice;
			shared_ptr<CellPainter> cell_painter;
			shared_ptr<LabelPainter> label_painter;
			shared_ptr<FieldPainter> field_painter;
			shared_ptr<ArrowPainter> arrow_painter;
			shared_ptr<VectorFieldPainter> vector_field_painter;
			shared_ptr<CellLinkPainter> cell_link_painter;
			string field_data_file;
			string cells_data_file;
			string membranes_data_file;
			string labels_data_file;
			string arrow_data_file;
			string vector_field_file;
			string link_data_file;
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
// 		PluginParameter2<double,XMLValueReader,OptionalPolicy> cell_opacity;
// 		PluginParameter2<double,XMLValueReader,DefaultValPolicy> pointsize;
		
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

		virtual void loadFromXML (const XMLNode xNode, Scope* scope) override;

		virtual void init(const Scope* scope) override;
		virtual void analyse(double time) override;
		virtual void finish() override {};

};

#endif
