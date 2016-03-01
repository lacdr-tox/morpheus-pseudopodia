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

#ifndef SVGPLOTTER_H
#define SVGPLOTTER_H

#include "core/simulation.h"
#include "core/celltype.h"
#include "core/string_functions.h"
#include <sstream>
#include <fstream>
#include <zlib.h>

using namespace SIM;

/** \defgroup svgPlotter
\ingroup analysis_plugins

\section Description
svgPlotter is a plugin which creates svg-files every interval of the Simulation.
These files illustrate the state of simulation and can be easily viewed in a browser.
By default, any cell type is associated with a particular color.
Colors might also be overridden by staining based on certain cell properties.

\section Example
\verbatim
<Analysis>
	<svgPlotter interval="100" compress="true"[ css="yourCssFileName.css" css-internal="true"]>
		<CSS internal="true" file="yourCssFileName.css">
		<Stain cellproperty="orientation" />
	</svgPlotter>
</Analysis>
\endverbatim

\section css-file
It is also possible to color specific cells or cellpopulations by using a css-file.
This file must be placed in the folder from where you call the cpmsim.
\verbatim
line {stroke:black; stroke-width:1px;}			//should not be changed
.cellpop1 {fill:rgb( 238, 0, 0);}			//to color a whole population
.cellpop2 {fill:rgb( 0, 238, 0);}
.cellpop1-cell15  {fill:blue;}				//to color a cell
\endverbatim
Here a link to a table of named colors:
http://de.wikibooks.org/wiki/SVG/_SVG_Farben
*/

class svgPlotter : public Analysis_Listener
{
	public:
		DECLARE_PLUGIN("svgPlotter");
		//string XMLName() const  { return string("svgPlotter"); };
// 		static bool factory_registration;
// 		static Plugin* createInstance();

		virtual void init(double time);
		virtual void notify(double time);
		virtual void finish(double time);

		void loadFromXML(const XMLNode);

		svgPlotter();
		~svgPlotter();

	private:
		uint svgWidth, svgHeight;
		double svgLatticeScale;

		struct boundarySegment{VDOUBLE pos1, pos2;};
		VDOUBLE latticeDim;
		const CPM::LAYER* layer;
		string css_file, css_string;
		bool css_internal;
		bool compress_file;

		stringstream svgText;

		void create_svg(double time);
		void svg_header();
		void svg_body();
		vector<boundarySegment> svg_bounds(const Cell::Nodes& node_list);
		void svg_polygons(vector<boundarySegment>& vec_bound);

		string stain_property_name;
		double stain_property_max;
		string cont_color_scale_1(double);
		string discr_color_scale_1(int);
		VDOUBLE hsl_to_rgb(const VDOUBLE a);
};

#endif
