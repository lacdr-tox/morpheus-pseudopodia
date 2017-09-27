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

#ifndef HISTOGRAMLOGGER_H
#define HISTOGRAMLOGGER_H

#include <core/interfaces.h>
#include "core/simulation.h"
#include "core/focusrange.h" // FocusRange
#include "gnuplot_i/gnuplot_i.h"
#include "core/plugin_parameter.h"

/** 
 *  \defgroup HistogramLogger HistogramLogger
 * \ingroup ML_Analysis
 * \ingroup AnalysisPlugins
 *  \brief Computation of frequency distributions and visualisation of histograms.
 * 
\section Description
HistogramLogger copmputes a frequency distribution from user-specified cell properties. Optionally, plots them in a histogram.

\section Input
- interval(default = 0.0)
- number-of-bins (default = 20): Number of bins.
- minimum (default = min value): Minimum value. Is used to calculate widths of bins. If not specified, taken from data.
- maximum (default = max value): Maximum value. Is used to calculate widths of bins. If not specified, taken from data.
- normalized (default = false): Calculate the absolute frequency distribution (false) or normalize by the number of data point (true).
- logarithmic_bins (default = false): Use logarithmic bin sizes (base 10)
- logarithmic_freq (default = false): Use logarithm frequencies (base 10).

Input symbol(s)
---------------
- Column:
  - symbol-ref (required): Symbol to compute frequency distriubution from, written as column in histogram file
  - celltype (optional): To report symbol from single cell type (in case of symbol-ref revers to cell Property). If not defined, all celltypes are assumed.
  - label (optional): Custom label to add to plot. If not defined, symbol name is used. 

Visualization
-------------
- Plot:
  - terminal (required): Gnuplot output terminal (e.g. x11, wxt, png, pdf).
  - minimum (optional): Fixed minimum frequency (default: 0) used in the plot. If not specified, taken from distribution.
  - maximum (optional): Fixed maximum frequency (default: 0) used in the plot. If not specified, taken from distribution.
  - log-commands (optional): Write Gnuplot command to file.

\section Examples


\verbatim
 <HistogramLogger interval="1">
	<Binning minimum="-0.1" normalized="true" maximum="1.1" number_bins="20"/>
	<Column symbol-ref="X" celltype="cells"/>
	<Column symbol-ref="Y" celltype="cells"/>
	<Plot minimum="0" maximum="1.0" terminal="png" persist="true"/>
</HistogramLogger>
\endverbatim

*/

class HistogramLogger : public AnalysisPlugin
{
private:

    // histogram details
	PluginParameter2<uint, XMLValueReader, DefaultValPolicy> numbins;
	PluginParameter2<bool, XMLValueReader, DefaultValPolicy> normalized;
	PluginParameter2<double, XMLValueReader, OptionalPolicy> minimum_fixed;
	PluginParameter2<double, XMLValueReader, OptionalPolicy> maximum_fixed;
	PluginParameter2<double, XMLValueReader, DefaultValPolicy> y_minimum;
	PluginParameter2<double, XMLValueReader, OptionalPolicy> y_maximum;
	PluginParameter2<bool, XMLValueReader, DefaultValPolicy> logarithmic_x;
	PluginParameter2<bool, XMLValueReader, DefaultValPolicy> logarithmic_y;
	double minimum, maximum;

	string filename;
    fstream fout;

    struct Bin
    {
        double minimum;
        double maximum;
        double frequency;
    };

    struct Column {
		PluginParameter2<double, XMLReadableSymbol, RequiredPolicy> symbol;
		PluginParameter2<string, XMLValueReader, OptionalPolicy> label;
		PluginParameterCellType< OptionalPolicy> celltype;
        vector< Bin > bins;
    };


    vector< shared_ptr<Column> > columns;
    vector< string > headers;

	enum Terminal{ PNG, PDF, JPG, GIF, SVG, EPS, WXT, X11, AQUA };
	struct Plot {
        bool enabled;
		PluginParameter2<Terminal, XMLNamedValueReader, OptionalPolicy> terminal;
		PluginParameter2<bool, XMLValueReader, DefaultValPolicy> logcommands;
        bool persist;
        string extension;
    };

    Gnuplot* gnuplot;
    Plot plot;
    void plotData(double time);
	
public:
    HistogramLogger(); // default values
    ~HistogramLogger(); // default destructor for cleanup
    DECLARE_PLUGIN("HistogramLogger");

    
    virtual void analyse(double time) override;
    virtual void loadFromXML(const XMLNode ) override;

    virtual void init(const Scope* scope) override;
    virtual void finish() override;
    void writelog(double time);
};
/// @endcond
#endif // HISTROGRAMLOGGER_H
