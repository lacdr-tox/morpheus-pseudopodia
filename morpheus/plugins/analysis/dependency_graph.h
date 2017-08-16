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

/** 
 *  \defgroup ML_DependencyGraph DependencyGraph
 *  \ingroup ML_Analysis
 *  \ingroup AnalysisPlugins
 *  \brief Visualisation of the symbol dependency graph and the scheduling
 * 
\section Description
DependencyGraph extractes the information used by morpheus for ordering and scheduling of 
numerical schemes and represents them in a graphical manner.

- \b format : (Image) format of the output
- \b exclude-plugins (optional): List of plugin names to be excluded from the graph, separated by '|' or ','
- \b exclude-symbols (optional): List of symbol names to be excluded from the graph, separated by '|' or ','

**/

class DependencyGraph: public AnalysisPlugin {
	enum class OutFormat {SVG, PNG, PDF, DOT};
	PluginParameter2<OutFormat,XMLNamedValueReader, DefaultValPolicy > format;
	PluginParameter2<string,XMLValueReader,OptionalPolicy> exclude_symbols;
	PluginParameter2<string,XMLValueReader,OptionalPolicy> exclude_plugins;
	
public:
	DECLARE_PLUGIN("DependencyGraph");
	
    DependencyGraph();
	void init(const Scope* scope) override;
    void analyse(double time) override;
};
