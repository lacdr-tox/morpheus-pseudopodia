#include "dependency_graph.h"
#include "core/cpm_sampler.h"

REGISTER_PLUGIN(DependencyGraph);

DependencyGraph::DependencyGraph() : AnalysisPlugin()
{
	format.setXMLPath("format");
	map<string, OutFormat> format_names; 
	format_names["svg"] = OutFormat::SVG;
	format_names["png"] = OutFormat::PNG;
	format_names["pdf"] = OutFormat::PDF;
	format_names["dot"] = OutFormat::DOT;
	format.setConversionMap(format_names);
	format.setDefault("png");
	registerPluginParameter(format);
	
	reduced.setXMLPath("reduced");
	reduced.setDefault("false");
	registerPluginParameter(reduced);
	exclude_plugins_string.setXMLPath("exclude-plugins");
	exclude_symbols_string.setXMLPath("exclude-symbols");
	include_tags_option.setXMLPath("include-tags");
	registerPluginParameter(exclude_plugins_string);
	registerPluginParameter(exclude_symbols_string);
	registerPluginParameter(include_tags_option);
}

void DependencyGraph::init(const Scope* scope)
{
	setTimeStep(SIM::getStopTime()); // This means call at the end !!
// 	this->valid_time = -1;
	AnalysisPlugin::init(scope);
}


void DependencyGraph::analyse(double time)
{
	if (!scope_info.empty())
		return;
	for (auto id=0; id<=Scope::max_scope_id; id++) {
		scope_info.push_back(make_unique<ScopeInfo>());
	}
	
	// setting up filters
	exclude_plugins.insert(this->XMLName());
	if (exclude_plugins_string.isDefined()) {
		vector<string> excludes = tokenize(exclude_plugins_string(),"|,");
		for (string e : excludes) {
			e.erase(remove_if(e.begin(), e.end(), [](char i) { return i == ' ';} ), e.end());
			exclude_plugins.insert(e);
		}
	}
	if (exclude_symbols_string.isDefined()) {
		vector<string> excludes = tokenize(exclude_symbols_string(),"|,");
		for (auto& e : excludes) {
			e.erase(remove_if(e.begin(), e.end(), [](char i) { return i == ' ';} ), e.end());
			exclude_symbols.insert(e);
		}
	}
	if (include_tags_option.isDefined()) {
		vector<string> tags = tokenize(include_tags_option(),"|, ");
		include_tags.insert(tags.begin(), tags.end());
	}
	if (reduced())
		skip_symbols_regex = join(skip_symbols,"|") + "|" + join(skip_symbols_reduced,"|");
	else 
		skip_symbols_regex = join(skip_symbols,"|");
	
	stringstream dot;
	
	dot << "digraph {\n";
	dot << "compound=true;\n";
	
	dot << "subgraph cluster{\n";
	dot << "labelloc=\"t\";";
	dot << "label=\"Global\";";
	dot << ""<< graphstyle.at("background") << "\n";
	parse_scope(SIM::getGlobalScope());
	if (scope_info[SIM::getGlobalScope()->getID()]->definitions.str().empty() 
		&& SIM::getGlobalScope()->getSubScopes().empty())
	{
		dot << "empty \n";
	}
	else { 
		dot << "node["<< graphstyle.at("node") <<"]\n";
		write_scope(SIM::getGlobalScope(), dot);
	}
	dot << "}" << endl;
	for (const auto& line : links )  {
		dot << line << "\n";
	}
	dot << "}" << endl;
	
	if (format() ==OutFormat::DOT) {
		//cout << "Rendering Dependency Graph as DOT" << endl;
	
		ofstream out("dependency_graph.dot");
		out << dot.str();
		out.close();
		return;
	}

#ifndef HAVE_GRAPHVIZ
	cout << "No support for graph rendering. Falling back to dot output." << endl;
	ofstream out("dependency_graph.dot");
	out << dot.str();
	out.close();
	return;
#else 

	GVC_t *gvc = gvContext();
	if (!gvc) {
		cout << "DependencyGraph: Unable to create rendering context"<< endl;
		return;
	}

	graph_t *g = agmemread(const_cast<char*>(dot.str().c_str()));
	if (!g) {
		cout << "DependencyGraph: Unable to read Dependency graph"<< endl;
		return;
	}

	gvLayout(gvc, g, "dot");
	
	switch (format()) {
		case OutFormat::PNG :
			// cout << "Rendering Dependency Graph as PNG" << endl;
			gvRenderFilename(gvc, g, "png", "dependency_graph.png");
			break;
		case OutFormat::SVG :
			// cout << "Rendering Dependency Graph as SVG" << endl;
			gvRenderFilename(gvc, g, "svg", "dependency_graph.svg");
			break;
		case OutFormat::PDF :
			//cout << "Rendering Dependency Graph as PDF" << endl;
			gvRenderFilename(gvc, g, "pdf", "dependency_graph.pdf");
			break;
		case OutFormat::DOT :
			// already written
			break;
			
	}
	
	// The windows build of graphviz library is buggy with the freelayout
#if ! defined(WIN32) && ! defined(_WIN32) && ! defined(__WIN32__)
// 	gvFreeLayout(gvc, g);
#endif
	agclose(g);
	gvFreeContext(gvc);
#endif
}

string DependencyGraph::tslDotName(TimeStepListener* tsl)
{
	string ts;
	if (tsl->timeStep()>0)
		ts =  to_str(tsl->timeStep());
	else 
		ts = "0";
	
	std::replace( ts.begin(), ts.end(), '.', '_');
	std::replace( ts.begin(), ts.end(), '-', '_');
	
	return tsl->XMLName() + "_" +  (reduced() || tsl->getFullName().empty() ? to_str(tsl->scope()->getID()) + "_" + ts :  to_str(hash<string>()(tsl->getFullName())) );
}

string DependencyGraph::pluginDotName(Plugin* p) {
	if (dynamic_cast<TimeStepListener*>(p)) {
		return tslDotName(dynamic_cast<TimeStepListener*>(p));
	}
	else {
		return p->XMLName() + "_" + (reduced() || p->getFullName().empty() ? to_str(p->scope()->getID()) : to_str(hash<string>()(p->getFullName())));
	}
}

string DependencyGraph::dotName(const string& a ) {
	string b(a);
	static const map<string,string> replacements =
		{ {".","_"}, {"α","alpha"}, {"β","beta"}, {"γ","gamma"}, {"δ","delta"}, {"ε","epsilon"}, {"ζ","zeta"}, {"η","eta"}, {"θ","theta"}, {"ι","iota"}, {"κ","kappa"}, {"λ","lambda"}, {"μ","mu"}, {"ν","nu"}, {"ξ","xi"}, {"ο","omicron"}, {"π","pi"}, {"ρ","rho"}, {"σ","sigma"}, {"ς","sigma"}, {"τ","tau"}, {"υ","upsilon"}, {"φ","phi"}, {"χ","chi"}, {"ψ","psi"}, {"ω","omega"}, {"Α","Alpha"}, {"Β","Beta"}, {"Γ","Gamma"}, {"Δ","Delta"}, {"Ε","Epsilon"}, {"Ζ","Zeta"}, {"Η","Eta"}, {"Θ","Theta"}, {"Ι","Iota"}, {"Κ","Kappa"}, {"Λ","Lambda"}, {"Μ","Mu"}, {"Ν","Nu"}, {"Ξ","Xi"}, {"Ο","Omicron"}, {"Π","Pi"}, {"Ρ","Rho"}, {"Σ","Sigma"}, {"Τ","Tau"}, {"Υ","upsilon"}, {"Φ","Phi"}, {"Χ","Chi"}, {"Ψ","Psi"}, {"Ω","Omega"} };
	string name;
	
	for (const auto& repl : replacements)  {
		auto pos = b.find(repl.first,0);
		while (pos!=string::npos) {
			b.replace(pos,repl.first.size(),repl.second);
			pos = b.find(repl.first,pos+1);
		}
	}
	return b;
}

void DependencyGraph::parse_scope(const Scope* scope)
{
	auto& info = *scope_info[scope->getID()];
	// Parse the TimeStepListeners, register their dependencies and collect their links
	auto tsls = scope->getTSLs();
	
			
	for (auto tsl : tsls) {
		// Apply filters
		if (reduced() && dynamic_cast<AnalysisPlugin*>(tsl))
			continue;
		if ( exclude_plugins.count( tsl->XMLName() ) )
			continue;
		if (/*include_tags_option.isDefined() &&*/ !tsl->isTagged(include_tags) )
			continue;
			
		if ( dynamic_cast<CPMSampler*>(tsl)) {
			string cpm_name = tslDotName(tsl);
			set<SymbolDependency> inter_dep = dynamic_cast<CPMSampler*>(tsl)->getInteractionDependencies();
			
			// Global CPM TSL Box
			info.definitions
				<< cpm_name 
				<< " ["
				<< "shape=plaintext"
				<< ", label=<"
				<< "<font face=\"times\" point-size=\"15\">"
				<< "<table cellborder=\"0\" border=\"1\" cellspacing=\"0\" cellpadding=\"3\">"
				<< "  <tr><td>" << tsl->XMLName() << "</td></tr>";
			if (!tsl->getFullName().empty() && !reduced()) { info.definitions
				<< "<tr><td><i>\"" + tsl->getFullName() << "\"</i></td></tr>";
			}
			info.definitions
				<< "<hr/><tr><td>" <<  tsl->timeStep() << "</td></tr>"
				<< "</table>"
				<< "</font>"
				<< ">"
				<< ", URL=\"morph://MorpheusModel/CPM\""
				<< " ]\n" ;
			
			for (auto ct_scope : scope->getComponentSubScopes()) {
				assert(ct_scope->getCellType());
				auto dependencies = ct_scope->getCellType()->cpmDependSymbols();
				// check existing dependecies
				bool have_dep = false;
				for (const auto& dep : dependencies) {
					if (exclude_plugins.count(dep.first->XMLName()))
						 continue;
					if (include_tags_option.isDefined() && !dep.first->isTagged(include_tags) )
						continue;
					// draw a line to the first CellType CPM plugin to linke it's CPM box
					if (!have_dep) {
						string first_plugin = pluginDotName( dep.first );
						stringstream link;
						link << cpm_name << " -> " << first_plugin << " [" << graphstyle.at("arrow_connect") << ",lhead=" << string("cluster_cpm") << ct_scope->getID() << "] \n";
						links.emplace(link.str());
					}
					have_dep = true;
					auto link_symbols = parse_symbol(dep.second);
					
				}
			}
			
			// Interaction Readers (other inputs stem from the celltype plugins, thus discarded here)
			for (auto dep : inter_dep) {
				auto link_symbols = parse_symbol(dep);
				for (auto symbol : link_symbols) {
					// create a link
					stringstream link;
					link << dotName(symbol->name()) << "_" << symbol->scope()->getID() << " -> " << cpm_name << " [" << graphstyle.at("arrow_read") << "] \n";
					links.emplace(link.str());
				}
			}
			
			// Writers
			auto dependencies = reduced() ? tsl->getLeafOutputSymbols() : tsl->getOutputSymbols();
			
			for (auto dep : dependencies) {
				if (dep->name() == SymbolBase::CellSurface_symbol || dep->name() == SymbolBase::CellLength_symbol || dep->name() == SymbolBase::CellVolume_symbol) continue;
				auto link_symbols = parse_symbol(dep);
				for (auto symbol : link_symbols) {
					// create a link
					stringstream link;
					link << tslDotName(tsl)  << " -> " << dotName(symbol->name()) << "_" << symbol->scope()->getID() << " [" << graphstyle.at("arrow_write") << "] \n";
					links.emplace(link.str());
				}
			}
		}
		else {
			// TSL Box
			info.definitions
				<< tslDotName(tsl)
				<< "[ "
				<< "shape=plaintext,  margin=\"0\""
				<< ", label=<"
				<< "<font face=\"times\" point-size=\"15\">"
				<< "<table cellborder=\"0\" border=\"1\" cellspacing=\"0\" cellpadding=\"3\" align=\"center\">\n"
				<< "<tr><td>" << tsl->XMLName() << "</td></tr>\n";
			if (!tsl->getFullName().empty() && !reduced()) { info.definitions
				<< "<tr><td><I>\"" << tsl->getFullName() << "\"</I></td></tr>\n"; 
			}
			info.definitions
			    << "<hr/>\n"
				<< "<tr><td>" <<  tsl->timeStep() << "</td></tr>"
				<< "</table>"
				<< "</font>"
				<< ">";
			if (!tsl->getXMLNode().isEmpty()) { info.definitions 
				<< ", URL=\"morph://" << getXMLPath(tsl->getXMLNode()) << "\""  /*<< ", tooltip=\"" << getXMLPath(tsl->getXMLNode()) << "\"" */;
			}
			info.definitions
				<< " ]\n" ;
			
			// Readers
			auto dependencies = reduced() ? tsl->getLeafDependSymbols() : tsl->getDependSymbols();
			for (auto dep : dependencies) {
				auto link_symbols = parse_symbol(dep);
				for (auto symbol : link_symbols) {
					// create a link
					stringstream link;
					link << dotName(symbol->name()) << "_" << symbol->scope()->getID() << " -> " << tslDotName(tsl) << " [" << graphstyle.at("arrow_read") << "] \n";
					links.emplace(link.str());
				}
			}
			
			// Writers
			dependencies = reduced() ? tsl->getLeafOutputSymbols() : tsl->getOutputSymbols();
			for (auto dep : dependencies) {
				auto link_symbols = parse_symbol(dep);
				for (auto symbol : link_symbols) {
					// create a link
					stringstream link;
					link << tslDotName(tsl)  << " -> " << dotName(symbol->name()) << "_" << symbol->scope()->getID() << " [" << graphstyle.at("arrow_write") << "] \n";
					links.emplace(link.str());
				}
			}
		}
	}
	
	if (scope->ct_component) {
		auto cpm_dep = scope->ct_component->cpmDependSymbols();
		bool found_valid_cpm_plugin = false;
		for (auto dep : cpm_dep) {
			if ( exclude_plugins.count( dep.first->XMLName() ) > 0)
				continue;
			if (include_tags_option.isDefined() && !dep.first->isTagged(include_tags) )
				continue;
			found_valid_cpm_plugin = true;
			break;
		}
		if ( found_valid_cpm_plugin ) {
			string cpm_blob_name = string("cluster_cpm") + to_str(scope->getID());
			info.definitions << "subgraph " << cpm_blob_name << " {\n";
			info.definitions << "label=\"CPM plugins\";\n";
			string current_plugin = "";
			string plugin_node_name;
			for (auto dep : cpm_dep) {
				if ( exclude_plugins.count( dep.first->XMLName() ))
					continue;
				if (include_tags_option.isDefined() && !dep.first->isTagged(include_tags) )
					continue;
				
				plugin_node_name = pluginDotName(dep.first);
				if (plugin_node_name != current_plugin) {
					current_plugin = plugin_node_name;
					info.definitions
						<< plugin_node_name
						<< "[ "
						<< "shape=plaintext,  margin=\"0\""
						<< ", label=<"
						<< "<font face=\"times\" point-size=\"15\"><table cellborder=\"0\" border=\"1\" cellspacing=\"0\" cellpadding=\"3\" align=\"center\" >\n"
						<< "  <tr><td>" << dep.first->XMLName() << "</td></tr>\n";
					if (!dep.first->getFullName().empty() && !reduced()) {
						info.definitions
						<< "  <tr><td><i>\"" << dep.first->getFullName() << "\"</i></td></tr>\n";
					}
					info.definitions
						<< "</table></font>"
						<< ">";
					if (!dep.first->getXMLNode().isEmpty()) {
						info.definitions
						<< ", URL=\"morph://" << getXMLPath(dep.first->getXMLNode()) << "\"";
					}
					info.definitions
						<< "];\n";
				}
				auto symbols = parse_symbol(dep.second);
				for (auto symbol : symbols) {
					// create a link
					stringstream link;
					link << dotName(symbol->name()) << "_" << symbol->scope()->getID() << " -> " << plugin_node_name << " [" << graphstyle.at("arrow_read") << "] \n";
					links.emplace(link.str());
				}
			}
			info.definitions << "}\n";
		}
	}
	
	
	for (auto subscope : scope->getSubScopes()) {
		parse_scope(subscope);
	}
	
	// Generate symbol definitions
	for (auto sym : info.symbols) {
		string type_style("type_"); type_style+= sym.second->type();
		string label_style;
		if (graphstyle.count(type_style)) label_style = graphstyle.at(type_style);
		else label_style = graphstyle.at("type_all");
		string link_style = graphstyle.at("arrow_read");
		
		// Change style for composite symbols
		if (dynamic_pointer_cast<const CompositeSymbol_I>(sym.second) ) {
			label_style += ",peripheries=2";
			link_style = graphstyle.at("arrow_connect");
		}
		if ( dynamic_pointer_cast<const VectorComponentAccessor>(sym.second)) {
			link_style = graphstyle.at("arrow_connect");
		}
		
		info.definitions << dotName(sym.first) << "_" << scope->getID() 
			<< "["
			<< "label=" << "\""<< sym.first<<"\"" 
			<< ", " << label_style;
		if (!sym.second->XMLPath().empty())
			info. definitions << ", URL=\"morph://" << sym.second->XMLPath() << "\"";
		info. definitions << "]\n";

		auto dependencies = sym.second->dependencies();
		for (auto dep : dependencies ) {
			auto symbols = parse_symbol(dep);
			for (auto symbol : symbols) {
				// create a link
				stringstream link;
				link << dotName(symbol->name()) << "_" << symbol->scope()->getID() << " -> " << dotName(sym.first) << "_" << scope->getID() << " [" << link_style << "] \n";
				links.emplace(link.str());
			}
		}
	}
}

vector<Symbol> DependencyGraph::parse_symbol(Symbol symbol) {
// 	if (exclude_symbols.count(symbol->name()))
// 		return {};
	for (const auto& excl :exclude_symbols ) {
		if (symbol->name().size() >= excl.size()) {
			if (symbol->name().substr(0,excl.size()) == excl)
				return {};
		}
	}
	if (!symbol->scope())
		return {};
	if (symbol->scope()->ct_component && symbol->scope()->ct_component->isMedium()) {
		if (symbol->name() == SymbolBase::CellPosition_symbol || symbol->name() == SymbolBase::CellID_symbol ||  symbol->name() == SymbolBase::CellType_symbol) {
			return {};
		}
	}
	
	if (regex_match(symbol->name(), skip_symbols_regex)) {
		auto dependencies = symbol->dependencies();
		vector<Symbol> symbols;
		for (auto dep: dependencies) {
			auto syms = parse_symbol(dep);
			for (auto s: syms) symbols.push_back(s);
		}
		return symbols;
	}
	else {
		auto & registry = scope_info[symbol->scope()->getID()]->symbols;
		if (!registry.count(symbol->name())) {
			scope_info[symbol->scope()->getID()]->symbols[symbol->name()] = symbol;
			auto dependencies = symbol->dependencies();
			for (auto dep: dependencies) {
				auto syms = parse_symbol(dep);
			}
		}
		return { symbol };
	}
}


void DependencyGraph::write_scope(const Scope* scope, ostream& dot) {
	if (scope->ct_component && scope->ct_component->isMedium()) {
		if (scope_info[scope->getID()]->definitions.str().empty())
			return;
	}
	dot << scope_info[scope->getID()]->definitions.str();
	
	for (auto sub_scope : scope->getSubScopes()) {
		dot << "subgraph cluster_" << sub_scope->getID() <<" {\n";
		dot << "label=\"" << sub_scope->getName() << "\";\n";
		write_scope(sub_scope, dot);
		dot << "}\n";
	}
}
