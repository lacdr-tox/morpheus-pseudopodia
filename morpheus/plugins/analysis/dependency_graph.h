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

#include "core/interfaces.h"
#include "core/plugin_parameter.h"
#include <graphviz/gvc.h>

/** \ingroup AnalysisPlugins
 *  \defgroup DependencyGraph
 *  \brief Visualisation representation of the symbol dependency graph and the scheduling
 * 
\section Description
DependencyGraph extractes the information used by morpheus for ordering and scheduling of 
numerical schemes and represent them in a graphical manner.
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
\endverbatim **/

class DependencyGraph: public AnalysisPlugin {
	enum class OutFormat {SVG, PNG, PDF, DOT};
	PluginParameter2<OutFormat,XMLNamedValueReader, DefaultValPolicy > format;
	PluginParameter2<string,XMLValueReader,OptionalPolicy> exclude_symbols;
	PluginParameter2<string,XMLValueReader,OptionalPolicy> exclude_plugins;
	
public:
	DECLARE_PLUGIN("DependencyGraph");
	
    DependencyGraph();
    void analyse(double time) override;
};
