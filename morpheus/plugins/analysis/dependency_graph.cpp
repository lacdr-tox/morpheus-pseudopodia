#include "dependency_graph.h"

REGISTER_PLUGIN(DependencyGraph);

DependencyGraph::DependencyGraph()
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
	
	exclude_plugins.setXMLPath("exclude-plugins");
	exclude_symbols.setXMLPath("exclude-symbols");
	registerPluginParameter(exclude_plugins);
	registerPluginParameter(exclude_symbols);
}


void DependencyGraph::analyse(double time)
{
	stringstream s_tmp;
	Scope::DepGraphConf config;
	config.exclude_plugins.insert(this->XMLName());
	if (exclude_plugins.isDefined()) {
		vector<string> excludes = tokenize(exclude_plugins(),"|");
		for (string e : excludes) {
			e.erase(remove_if(e.begin(), e.end(), [](char i) { return i == ' ';} ), e.end());
			config.exclude_plugins.insert(e);
		}
	}
	if (exclude_symbols.isDefined()) {
		vector<string> excludes = tokenize(exclude_symbols(),"|");
		for (auto& e : excludes) {
			e.erase(remove_if(e.begin(), e.end(), [](char i) { return i == ' ';} ), e.end());
			config.exclude_symbols.insert(e);
		}
	}
	SIM::getGlobalScope()->write_graph(s_tmp,config);

	GVC_t *gvc = gvContext();
	if (!gvc) {
		cout << "DependencyGraph: Unable to create rendering context"<< endl;
		return;
	}

	graph_t *g = agmemread(const_cast<char*>(s_tmp.str().c_str()));
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
			//cout << "Rendering Dependency Graph as DOT" << endl;
			ofstream out("dependency_graph.dot");
			out << s_tmp.str();
			out.close();
// 			gvRenderFilename(gvc, g, "plain", "dependency_graph.dot");
			break;
			
	}
	
	// The windows build of graphviz library is buggy with the freelayout
#if ! defined(WIN32) && ! defined(_WIN32) && ! defined(__WIN32__)
// 	gvFreeLayout(gvc, g);
#endif
	agclose(g);
	gvFreeContext(gvc);
}
