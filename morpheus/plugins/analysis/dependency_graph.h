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
#ifdef HAVE_GRAPHVIZ
#include <graphviz/gvc.h>
#warning Building with graphviz support
#else
#warning Building without graphviz support
#endif
#include <regex>

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
	PluginParameter2<bool, XMLValueReader, DefaultValPolicy> reduced;
	PluginParameter2<string,XMLValueReader,OptionalPolicy> exclude_symbols_string;
	PluginParameter2<string,XMLValueReader,OptionalPolicy> exclude_plugins_string;
	PluginParameter2<string,XMLValueReader,OptionalPolicy> include_tags_option;
	set<string> exclude_symbols;
	set<string> exclude_plugins;
	set<string> include_tags;
	vector<string> skip_symbols = {"_.*"};
	vector<string> skip_symbols_reduced = {".*\\.x", ".*\\.y",".*\\.z",".*\\.phi",".*\\.theta",".*\\.abs"};
	regex skip_symbols_regex;
	
	struct ScopeInfo {
		map<string,Symbol> symbols;
		stringstream definitions;
		vector<int> tsl;
	};
	vector< unique_ptr<ScopeInfo> > scope_info;
	set<string> links;
	
	const map<string, string> graphstyle = {
		{string("type_")+ TypeInfo<double>::name(),"fillcolor=\"#d3d247\""},
		{string("type_")+ TypeInfo<VDOUBLE>::name(),"fillcolor=\"#b5b426\""},
		{"type_all","fillcolor=\"#8f8eod\""},
		{"node","style=filled,fillcolor=\"#fffea3\""},
		{"background","bgcolor=\"#2341782f\""},  // blue-ish (with alpha value 47 (0-255))
		{"arrow_connect","dir=none, style=\"dashed\", penwidth=1, color=\"#38568c\""},
		{"arrow_write","penwidth=3, color=\"#8f100d\""},
		{"arrow_read","penwidth=2, color=\"#112c5f\""}
	};
	
	void parse_scope(const Scope* scope);
	vector<Symbol> parse_symbol(Symbol symbol);
	void write_scope(const Scope* scope, ostream& out);
	
	string pluginDotName(Plugin* p);
	string tslDotName(TimeStepListener* tsl);
	string dotName(const string& a );

public:
	DECLARE_PLUGIN("ModelGraph");
	
	DependencyGraph();
	void init(const Scope* scope) override;
	void analyse(double time) override;
};
