#define SIMULATION_CPP

// #define NO_CORE_CATCH
#ifdef NO_CORE_CATCH
#warning "NO_CORE_CATCH defined. Do not use for productive systems !!"
#endif

#include "simulation_p.h"
#include "cpm_p.h"
#include "rss_stat.h"
#include <boost/program_options.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace SIM {
	
	string dep_graph_format = "dot";
	bool generate_symbol_graph_and_exit = false;
	bool generate_performance_stats = false;
	int numthreads = omp_get_max_threads();
	
	unique_ptr<LatticePlugin> lattice_plugin = nullptr;
	shared_ptr<Lattice> global_lattice;
	
	XMLNode xMorphModel, xDescription, xGlobals, xSpace, xAnalysis;
	
	
	vector< shared_ptr<AnalysisPlugin> > analysers;
	vector< shared_ptr<Plugin> > analysis_section_plugins;
	vector< shared_ptr<Plugin> > global_section_plugins;

	unique_ptr<Scope> global_scope;
	Scope* current_scope;

	string morpheus_file_version;
	string prettyFormattingTime( double time_in_sec );
	string prettyFormattingBytes(uint bytes);

	
	uint random_seed = time(NULL);
	string fileTitle="SnapShot";
	/// directory to read data from
	string input_directory = ".";
	/// directory to write data to
	string output_directory = ".";
	
	
int main(int argc, char *argv[]) {
	bool exception = false;
	
#ifndef NO_CORE_CATCH
	try {
#endif
	
    double init_rt0 = get_wall_time();
	double init_cpu0  = get_cpu_time();
	if (init(argc, argv)) {
	
		double init_rt1 = get_wall_time();
		double init_cpu1  = get_cpu_time();
		
		auto& perf_json = getPerfLogger();
		boost::property_tree::ptree initialisation;
		initialisation.add("name", "initialisation");
		initialisation.add("runtime",  to_str(init_rt1 - init_rt0));
		initialisation.add("cputime",  to_str(init_cpu1 - init_cpu0));
		perf_json.add_child("component",initialisation);
		
		TimeScheduler::compute();
		
		size_t initMem = getCurrentRSS();
		
		finalize();
		
		//  Stop timers
		double wall1 = get_wall_time();
		double cpu1  = get_cpu_time();
		
		cout << "\n=== Simulation finished ===\n";
		string init_time = prettyFormattingTime( init_rt1 - init_rt0 );
		string cpu_time = prettyFormattingTime( cpu1 - init_cpu0 );
		string wall_time = prettyFormattingTime( wall1 - init_rt0 );
		size_t peakMem = getPeakRSS();
		
		cout << "Init Time   = " << init_time << "\n";
		cout << "Wall Time   = " << wall_time << "\n";
		cout << "CPU Time    = " << cpu_time  << " (" << numthreads << " threads)\n\n";
// 		cout << "Memory peak = " << prettyFormattingBytes(peakMem) << "\n";
		
		perf_json.add("runtime", wall1 - init_rt0);
		perf_json.add("cputime", cpu1 - init_cpu0);
// 		perf_json.add("memory", to_str(peakMem));
		perf_json.add("ompthreads", to_str(omp_get_max_threads()));
		
		if (generate_performance_stats) {
			boost::property_tree::write_json("performance.json", perf_json);
		}
	}

#ifndef NO_CORE_CATCH
	}
	catch (MorpheusException e) {
		exception = true;
		cerr << e.what()<< "\n";
		cerr << "\nXMLPath: " << e.where() << endl;
	}
	catch (string e) {
		exception = true;
		cerr << e << endl;
	}
	catch (SymbolError& e) {
// 		switch(e.type()) {
// 			case SymbolError::Type::InvalidDefinition
// 		}
		exception = true;
		cerr << "\n" << e.what()<< "\n";
	}
	catch (std::exception& e) {
		exception = true;
		cerr << e.what() << endl; 
	}
	catch (...) {
		cerr << "Unknown error while creating the simulation";
		exception = true;
	}
#endif
	
	if (exception) {
		cerr.flush();
		if (SIM::generate_symbol_graph_and_exit) {
			try {
				createDepGraph();
			}
			catch(const string& e) {
				cerr << "Unable to generate Model Graph.\n" << e << endl;
			}
			catch(...) {
				cerr << "Unable to generate Model Graph.\n" << endl;
			}

		}
		
		return -1;
	}
	
	return 0;
}

string prettyFormattingTime( double time_in_sec ) {
	char s[100];
	if( time_in_sec >= 0) {
        int msec = time_in_sec*1000;
        int millisec=(msec)%1000;
        int seconds=(msec/1000)%60;
        int minutes=(msec/(1000*60))%60;
        int hours=(msec/(1000*60*60))%24;
		int days=(msec/(1000*60*60*24));
		
		if (days > 0)
			std::sprintf(s, "%dd %02dh %02dm", days, hours, minutes);
        else if(hours > 0)
            std::sprintf(s, "%dh %02dm", hours, minutes);
        else if( minutes > 0 )
            std::sprintf(s, "%dm %02ds", minutes, seconds);
        else
            std::sprintf(s, "%1ds %03dms", seconds, millisec);
    }
    else
        std::sprintf(s, " -- ");
	return string(s);
}

// Prints to the provided buffer a nice number of bytes (KB, MB, GB, etc)
string prettyFormattingBytes(uint bytes)
{
    vector<string> suffixes;
	suffixes.resize(7);
    suffixes[0] = "B";
    suffixes[1] = "Kb";
    suffixes[2] = "Mb";
    suffixes[3] = "Gb";
    suffixes[4] = "Tb";
    suffixes[5] = "Pb";
    suffixes[6] = "Eb";
    uint s = 0; // which suffix to use
    double count = bytes;
    while (count >= 1024 && s < 7)
    {
        s++;
        count /= 1024;
    }
 	stringstream ss;
	if (count - floor(count) == 0.0)
		ss << (int)count << " " << suffixes[s];
        //sprintf(buf, "%d %s", (int)count, suffixes[s]);
    else
		ss << count << " " << suffixes[s];
        //sprintf(buf, "%.1f %s", count, suffixes[s]);
	return ss.str();
}

const string& getTitle() { return fileTitle; }

const string& getInputDirectory() { return input_directory; }

const string& getOutputDirectory() { return output_directory; }

bool dependencyGraphMode() {
	return SIM::generate_symbol_graph_and_exit;
}



double getNodeLength()  {
    return lattice_plugin->getNodeLength()();
};

string getLengthScaleUnit() {
	if (lattice_plugin->getNodeLength().getLengthScaleUnit() == "alu")
		return "alu";
	else
		return "meter";
};

double getLengthScaleValue() {
	return lattice_plugin->getNodeLength().getLengthScaleValue();
};


string getTimeScaleUnit() {
	if (TimeScheduler::getTimeScaleUnit() == "atu")
		return "atu";
	else
		return "sec";
};

// double getTimeScaleValue() {
// 	return sim_stop_time.getTimeScaleValue();
// };

string getTimeName() {
	return getTimeName(getTime());
}

string getTimeName(double time) {
	
	stringstream sstr;
	sstr << fixed << TimeScheduler::getStopTime();
	int length = sstr.str().find_first_of(".") + (AnalysisPlugin::max_time_precision >0 ? AnalysisPlugin::max_time_precision + 1 : 0);
	length = max(5,length);
	sstr.str("");
	
	sstr << setw(length) << setfill('0') << fixed << setprecision(AnalysisPlugin::max_time_precision) << time;
	return sstr.str();
}

double TimeSymbol::get(const SymbolFocus&) const {	return TimeScheduler::getTime(); }

double getTime() {
	return TimeScheduler::getTime();
};

double getStopTime(){
	return TimeScheduler::getStopTime();	
};

double getStartTime(){
	return TimeScheduler::getStartTime();	
};

string centerText(string in) {
	int pos=(int)((80-in.length())/2);
	string out(pos,' ');
	out.append(in);
	return out;
}

void splash(bool show_usage) {

    time_t t = time(0); // get time now
    struct tm * now = localtime( & t );
    int current_year = now->tm_year + 1900;

	cout << endl;
	cout << centerText("<<  M O R P H E U S  >>") << endl;
	cout << centerText("Modeling environment for multi-scale and multicellular systems biology") << endl;
    stringstream copyright;
    copyright << "Copyright 2009-"<< current_year <<", Technische Universität Dresden, Germany";
    cout << centerText( copyright.str() ) << endl;

	stringstream version;
	version << "Version " << MORPHEUS_VERSION_STRING;
	version << ", Revision " << MORPHEUS_REVISION_STRING;
    cout << centerText( version.str() ) << endl;

    if( show_usage ){
    cout << endl << endl;
    cout << " Usage: "<< endl;
    cout << "  morpheus [OPTIONS] " << endl << endl;
    cout << " Options:  " << endl;
    cout << " -file [XML-FILE]      Run simulator with XML configuration file" << endl;
	cout << " -outdir [PATH]        Set the output directory" << endl;
	cout << " -P [Symbol]=[VALUE]   Override the initial values of symbols from the \"Global\" section" << endl;
    cout << " -version              Show release version" << endl;
    cout << " -revision             Show SVN revision number" << endl;
	cout << " -gnuplot-version      Show version of GnuPlot used" << endl;
	cout << " -gnuplot-path [FILE]  Set the path to the GnuPlot executable" << endl;
    cout << endl << endl;
    }

	cout << " External applications" << endl;
	try {
		cout << "  GnuPlot executable:   " <<  Gnuplot::get_GNUPlotPath() << endl;
		cout << "  GnuPlot version:      " <<  Gnuplot::version() << endl;
	}
	catch (...) {
		cout << "Morpheus cannot find/run GnuPlot executable" << endl;
	}
	cout << endl << endl;
}


bool init(int argc, char *argv[]) {
	const bool allow_single_dash_long_options = true;
	const bool allow_symbol_override_options = true;
	namespace po = boost::program_options;

	po::options_description desc(
		"                         <<  M O R P H E U S  >>\n"
		"   Modeling environment for multi-scale and multicellular systems biology\n"
		"        Copyright 2009-2020, Technische Universität Dresden, Germany\n"
		"                 Version " 
		MORPHEUS_VERSION_STRING
		", Revision "
		MORPHEUS_REVISION_STRING
		"\n\nOptions:                   (single dash long option names available for backward compatibility)"
	);
	
	desc.add_options()
		("version,v", "Print morpheus version.")
		("revision,r", "Print morpheus revision.")
		("gnuplot-path", po::value<std::string>(),"Set path to gnuplot executable.")
		("gnuplot-version","Print gnuplot version.")
		("file,f", po::value<std::string>(),"MorpheuML model to simulate.")
		("set,set-symbol,s", po::value<std::vector<std::string>>(), "Override initial value of global symbol. Use assignment syntax [symbol=value].")
		("perf-stats", "Generate performance stats in json format.")
		("outdir", po::value<std::string>(), "override output directory.")
		("symbol-graph,model-graph", po::value<std::string>()->implicit_value("dot"), "Generate the model graph in the given format [dot,svg,pdf,png].")
		("help,h", "show this help page.");
	
	// positional option is treated as a model
	po::positional_options_description pos_desc;
	pos_desc.add("file",1);
	
	int style = po::command_line_style::unix_style;
	if (allow_single_dash_long_options) style |=  po::command_line_style::allow_long_disguise;
	auto cmd_line_parser = po::command_line_parser(argc, argv).options(desc).style(style).positional(pos_desc);
	if (allow_symbol_override_options) cmd_line_parser.allow_unregistered();

	po::variables_map cmd_line;
	auto parsed_cmd_line = cmd_line_parser.run();
	po::store(parsed_cmd_line, cmd_line);
	po::notify(cmd_line);

	if (cmd_line.count("help") ) {
		desc.print(cout);
		return false;
	}
	
	if (cmd_line.count("revision") ) {
		cout << "Revision: " <<  MORPHEUS_REVISION_STRING  << endl;
		return false;
	}

	if (cmd_line.count("version")) {
		cout << "Version: " << MORPHEUS_VERSION_STRING << endl;
		return false;
	}

	if (cmd_line.count("gnuplot-path")) {
		Gnuplot::set_GNUPlotPath(cmd_line["gnuplot-path"].as<string>());
	}
	
	if (cmd_line.count("gnuplot-version")) {
		string version;
		try {
			version = Gnuplot::version();
		}
		catch (GnuplotException &e) {
			throw string(e.what());
		}
		cout << version << endl;
		return false;
	}

	if (cmd_line.count("symbol-graph")) {
		generate_symbol_graph_and_exit = true;
		dep_graph_format = cmd_line["symbol-graph"].as<string>();
	}
	else {
		generate_symbol_graph_and_exit = false;
	}

	struct stat filestatus;
	if (cmd_line.count("outdir")) {
		output_directory = cmd_line["outdir"].as<string>();
		if ( access( output_directory.c_str(), R_OK | W_OK) != 0) {
			throw  string("Error: output directory '") + output_directory + "' does not exist or is not writable.";
		}
		cout << "Setting output directory " << output_directory << endl;
	}

	if ( cmd_line.count("file")  != 1 ) {
        cout << "Error: Single morpheus model must be specified." << endl;
		desc.print(cout);
        return false;
    }

	string filename = cmd_line["file"].as<string>();
	int filenotexists = stat( filename.c_str(), &filestatus );
	if ( filenotexists > 0 || filename.empty() ) {
		throw  string("Error: file '") + filename + "' does not exist.";
	}
	else if ( filestatus.st_size == 0 ) {
		throw  string("Error: file '") + filename + "' is empty.";
	}
	
	generate_performance_stats = cmd_line.count("stats");

	
	// Attach global overrides to the global scope
	map<string,string> overrides;
	if (allow_symbol_override_options) {
		vector<string> unrecognized = collect_unrecognized(parsed_cmd_line.options, po::exclude_positional);
		for (auto& param : unrecognized) {
			boost::replace_all(param,"\""," ");
			int s_idx = param.find_first_not_of("-");
			int idx = param.find_first_of("=", s_idx);
			if (idx == string::npos || idx <= s_idx || idx >= param.size()-1) {
				throw string ("Invalid parameter override ") +  param;
			}
			overrides[boost::trim_copy(param.substr(s_idx,idx-s_idx))] = boost::replace_all_copy(param.substr(idx+1, param.size()-idx-1),"\""," ");
		}
	}
	if (cmd_line.count("set")) {
		vector<string> params = cmd_line["parameter"].as<vector<string>>();
		for (auto& param : params) {
			boost::replace_all(param,"\""," ");
			int idx=param.find_first_of("=");
			if (idx == string::npos || idx == 0 || idx >= param.size()-1) {
				throw string ("Invalid parameter override ") +  param;
			}
			overrides[boost::trim_copy(param.substr(0,idx))] = param.substr(idx+1, param.size()-idx-1);
		}
	}
	
// 	cout << "registered overrides: ";
// 	for ( const auto& o : overrides ) {
// 		cout << o.first << " = " << o.second << "; ";
// 	}
// 	cout << endl;
 
	try {
		init(readFile(filename), overrides);
	}
	catch (const string &e) {
		cout << "Could not initialize model " << filename << endl;
		cout << e << endl;
		return false;
	}
	catch (const MorpheusException &e) {
		cout << "Could not initialize model " << filename << endl;
		cout << e.what() << endl;
		cout << e.where() << endl;
		return false;
	}
	
	if (SIM::generate_symbol_graph_and_exit) {
		createDepGraph();
		cout << "Generated symbol dependency graph. Exiting." << endl;
		return false;
	}

	cout.flush();
	return true;
};

bool init(string model, map<string,string> overrides) {
	XMLResults results;
	xMorphModel = XMLNode::parseString(model.c_str(),"MorpheusModel", &results);
	if (results.error!=eXMLErrorNone) {
		stringstream s;
		s << "Unable to read model" << std::endl; 
		s << XMLNode::getError(results.error) <<  " at line " << results.nLine << " col " << results.nColumn << "!" << std::endl; 
		throw s.str();
	}
	
	global_scope = unique_ptr<Scope>(new Scope());
	// Attach global overrides to the global scope
	for (const auto& param : overrides ) {
// 		if (param.first == "file") continue;
		getGlobalScope()->value_overrides()[param.first] = param.second;
	}
	current_scope = global_scope.get();
	
	loadFromXML(xMorphModel);
	
	// try to match cmd line options with symbol names and adjust values accordingly
	// check that global overrides have been used
	for ( const auto& override: global_scope->value_overrides() ) {
		cout << "Error: Command line override " << override.first << "=" << override.second << " not used!" << endl;
	}
	if (! global_scope->value_overrides().empty()) {
		return false;
	}

	return true;
}

void finalize() {
	TimeScheduler::finish();
	wipe();
}

void setRandomSeeds( const XMLNode xNode ){
	//  seed
	random_seed = time(NULL);
    if ( ! xNode.isEmpty() ){
			getXMLAttribute(xNode,"value",random_seed);
        }
    else{
        cout << "Time/RandomSeed not specified, using arbitray seed (based on time)." << endl;
    }
	setRandomSeed(random_seed);
}

void createDepGraph() {
	shared_ptr<AnalysisPlugin> dep_graph_writer;
	
	for (uint i=0;i<analysers.size();i++) {
		if (analysers[i]->XMLName() == "DependencyGraph") {
			dep_graph_writer = analysers.at(i);
			dep_graph_writer->setParameter("format", dep_graph_format);
			
			break;
		}
	}
	if (!dep_graph_writer) {
		dep_graph_writer = dynamic_pointer_cast<AnalysisPlugin>(PluginFactory::CreateInstance("DependencyGraph"));
		if (!dep_graph_writer) {
			cerr << "Unable to create instance for DependencyGraph Plugin." << endl;
			return;
		}
		dep_graph_writer->setParameter("format",dep_graph_format);
		dep_graph_writer->init(getGlobalScope());
	}
	dep_graph_writer->analyse(0);
}

void loadFromXML(const XMLNode xNode) {

// Loading simulation parameters
	string nnn;
	/*********************************************/
	/** LOADING XML AND REGISTRATION OF SYMBOLS **/
	/*********************************************/
	
	
	getXMLAttribute(xNode,"version",morpheus_file_version);
	xDescription = xNode.getChildNode("Description");
	getXMLAttribute(xDescription,"Title/text",fileTitle);
	XMLNode xTime = xNode.getChildNode("Time");
	TimeScheduler::loadFromXML(xTime, global_scope.get());
	
	
	getXMLAttribute(xTime,"TimeSymbol/symbol",SymbolBase::Time_symbol);
	auto time_symbol = make_shared<TimeSymbol>(SymbolBase::Time_symbol);
	time_symbol->setXMLPath(getXMLPath(xTime)+"/TimeSymbol");
	global_scope->registerSymbol(time_symbol);
	
	setRandomSeeds(xTime.getChildNode("RandomSeed"));

	xSpace = xNode.getChildNode("Space");
	getXMLAttribute(xSpace,"SpaceSymbol/symbol",SymbolBase::Space_symbol);
	auto space_symbol = make_shared<LocationSymbol>(SymbolBase::Space_symbol);
	space_symbol->setXMLPath(getXMLPath(xSpace)+"/SpaceSymbol");
	global_scope->registerSymbol(space_symbol);
	
	// Loading and creating the underlying lattice
	cout << "Creating lattice"<< endl;
	XMLNode xLattice = xSpace.getChildNode("Lattice");
	
	lattice_plugin = make_unique<LatticePlugin>();
	lattice_plugin->loadFromXML(xLattice, global_scope.get());
	
// 	if (xLattice.nChildNode("NodeLength"))
// 		node_length.loadFromXML(xLattice.getChildNode("NodeLength"), global_scope.get());
// 	try {
// 		string lattice_code="cubic";
// 		getXMLAttribute(xLattice, "class", lattice_code);
// 		if (lattice_code=="cubic") {
// 			global_lattice =  shared_ptr<Lattice>(new Cubic_Lattice(xLattice));
// 		} else if (lattice_code=="square") {
// 			global_lattice =  shared_ptr<Lattice>(new Square_Lattice(xLattice));
// 		} else if (lattice_code=="hexagonal") {
// 			global_lattice =  shared_ptr<Lattice>(new Hex_Lattice(xLattice));
// 		} else if (lattice_code=="linear") {
// 			global_lattice =  shared_ptr<Lattice>(new Linear_Lattice(xLattice));
// 		}
// 		else throw string("unknown Lattice type " + lattice_code);
// 		if (! global_lattice)
// 				throw string("Error creating Lattice type " + lattice_code);
// 	}
// 	catch (string e) {
// 		throw MorpheusException(e,xLattice);
// 	}
// 	
// 	lattice_size_symbol="";
// 	if (getXMLAttribute(xLattice,"Size/symbol",lattice_size_symbol)) {
// 		auto lattice_size = SymbolAccessorBase<VDOUBLE>::createConstant(lattice_size_symbol,"Lattice Size", global_lattice->size());
// 		global_scope->registerSymbol( lattice_size );
// 	}
	
	MembranePropertyPlugin::loadMembraneLattice(xSpace, global_scope.get());
	
	// Loading global definitions
	if (xNode.nChildNode("Global")) {
		xGlobals = xNode.getChildNode("Global");
		cout << "Loading [" << xGlobals.nChildNode() << "] Global Plugins" <<endl;
		for (int i=0; i<xGlobals.nChildNode(); i++) {
			XMLNode xGlobalChild = xGlobals.getChildNode(i);
			string xml_tag_name(xGlobalChild.getName());
			shared_ptr<Plugin> p = PluginFactory::CreateInstance(xml_tag_name);
			
			if (! p.get()) {
				Plugin::getFactory().printKeys();
				throw MorpheusException(string("Unknown Global plugin ") + xml_tag_name, xGlobalChild);
			}
			
			p->loadFromXML(xGlobalChild, global_scope.get());
			global_section_plugins.push_back(p);
		}
	}
	
	// Loading cell types, CPM and CellPopulations
	CPM::loadFromXML(xNode, global_scope.get());

	/*****************************************************/
	/** CREATION AND INTERLINKING of the DATA STRUCTURE **/
	/*****************************************************/
	
	lattice_plugin->init(global_scope.get());
	
	global_lattice = lattice_plugin->getLattice();
	
	// all model constituents are loaded. let's initialize them (i.e. interlink)
	global_scope->init();
	for (auto glob : global_section_plugins) {
#ifdef NO_CORE_CATCH
		glob->init(SIM::getGlobalScope());
#else
		try {
			glob->init(SIM::getGlobalScope());
		}
		catch (string e) {
			string s("Error in Plugin ");
			s+= glob->XMLName() + "\n" + e;
			throw MorpheusException(s,glob->getXMLNode());
		}
		catch (const SymbolError& e) {
			string s("Error in Plugin ");
			s+= glob->XMLName() + "\n" + e.what();
			throw MorpheusException(s,glob->getXMLNode());
		}

#endif
	}

	CPM::init();

	xAnalysis = xNode.getChildNode("Analysis");
	if ( ! xAnalysis.isEmpty() ) {
		cout << "Loading Analysis tools [" << xAnalysis.nChildNode() << "]" <<endl;
		for (int i=0; i<xAnalysis.nChildNode(); i++) {
			XMLNode xNode = xAnalysis.getChildNode(i);
			try {
				string xml_tag_name(xNode.getName());
				shared_ptr<Plugin> p = PluginFactory::CreateInstance(xml_tag_name);
				
				if (! p.get()) 
					throw(string("Unknown analysis plugin " + xml_tag_name));
				
				p->loadFromXML(xNode, SIM::global_scope.get());
				
				if (dynamic_pointer_cast<AnalysisPlugin>(p) ) {
					analysers.push_back( dynamic_pointer_cast<AnalysisPlugin>(p) );
				}
				else {
					analysis_section_plugins.push_back(p);
				}
			}
			catch (string er) {
				cout << er << endl;
			}
		}
	}

	// before loading all the Analysis tools that might create some files we should switch the cwd
	if (chdir(output_directory.c_str()) != 0) 
		throw(string("Could not change to output directory \"") + output_directory + "\"");
	
	for (uint i=0;i<analysis_section_plugins.size();i++) {
		analysis_section_plugins[i]->init(global_scope.get());
	}
	for (uint i=0;i<analysers.size();i++) {
		analysers[i]->init(global_scope.get());
	}
	
	TimeScheduler::init(global_scope.get());
	cout << "model is up" <<endl;
};


void saveToXML() {
	ostringstream filename("");
	filename << fileTitle << setfill('0') << setw(6) << getTimeName() << ".xml.gz";
	cout << "Saving " << filename.str()<< endl;

	xMorphModel = XMLNode::createXMLTopNode("MorpheusModel");
	if (!morpheus_file_version.empty())
		xMorphModel.addAttribute("version",morpheus_file_version.c_str());

	XMLNode xTimeNode = xMorphModel.addChild( TimeScheduler::saveToXML() );

	xMorphModel.addChild(xDescription);

	xMorphModel.addChild(xSpace);
	
	// saving Field data
	// TODO:: global_scope::saveToXML -> Field / VectorField
	for (auto plugin : global_section_plugins) {
		xGlobals.addChild(plugin->saveToXML());
	}
	
	// saving global_scope
	xMorphModel.addChild(xGlobals);

	// saving cell types
	xMorphModel.addChild(CPM::saveCellTypes());
	
	// save CPM details (interaction energy and metropolis kinetics)
	xMorphModel.addChild(CPM::saveCPM());
	
	if ( ! (analysers.empty() && analysis_section_plugins.empty() )) {
		XMLNode xAnalysis = xMorphModel.addChild("Analysis" );
		for (uint i=0; i<analysis_section_plugins.size(); i++ ) {
			xAnalysis.addChild(analysis_section_plugins[i]->saveToXML());
		}
		for (uint i=0; i<analysers.size(); i++ ) {
			xAnalysis.addChild(analysers[i]->saveToXML());
		}
	}

	/****************************/
	/** SAVING SIMULATION DATA **/
	/****************************/

	// cell populations
	xMorphModel.addChild(CPM::saveCellPopulations());

	int xml_size;
	XMLSTR xml_data=xMorphModel.createXMLString(1,&xml_size);

	gzFile zfile = gzopen(filename.str().c_str(), "w9");
	if (Z_NULL == zfile) {
		cerr<<"Cannot open file " << filename.str()  << endl;
		exit(-1);
	}
	int written = gzwrite(zfile, xml_data, xml_size);
	if ( written != xml_size) {
		cerr<<"Error writing to file " << filename.str()  << " wrote "<< written << " of " << xml_size << endl;
		exit(-1);
	}
	gzclose(zfile);
	free(xml_data);
}

void wipe()
{ 
	TimeScheduler::wipe();
	
	analysers.clear();
	analysis_section_plugins.clear();
	global_section_plugins.clear();
	CPM::wipe();
	
	lattice_plugin.reset();
	global_lattice.reset();
	global_scope.reset();
	
	global_scope = unique_ptr<Scope>(new Scope());
	current_scope = global_scope.get();
}

boost::property_tree::ptree& getPerfLogger() {
	static boost::property_tree::ptree perf_logger;
	return perf_logger;
// 	static auto perf_logger =
// 		shared_ptr<boost::property_tree::ptree>(new boost::property_tree::ptree(), [](boost::property_tree::ptree *that) {
// 			boost::property_tree::write_json("performance.json", *that);
// 			delete that;
// 		});
// 	return *perf_logger;
}

Lattice::Structure getLatticeStructure() {
	return lattice_plugin->getStructure();
}

shared_ptr <const Lattice> getLattice() {
	if (!global_lattice) {
		cerr << "Trying to access global lattice, while it's not defined yet!" << endl; assert(0); exit(-1);
	}
	return global_lattice;
};

const Lattice& lattice() {
	if (!global_lattice) {
		cerr << "Trying to access global lattice, while it's not defined yet!" << endl; assert(0); exit(-1);
	}
	return *global_lattice;
}

const Scope* getScope() { return current_scope; }

Scope* getGlobalScope() { return global_scope.get(); }

Scope* createSubScope(string name, CellType* ct) { if (! current_scope) throw string("Cannot create subscope from empty scope"); return current_scope->createSubScope(name,ct); }

deque<Scope*> scope_stash;
void enterScope(const Scope* scope) { if (!scope) throw(string("Invalid scope in enterScope")); cout << "Entering scope " << scope->getName() << endl; scope_stash.push_back(current_scope); current_scope = const_cast<Scope*>(scope);}

void leaveScope() { if (scope_stash.empty()) throw (string("Invalid scope in leaveScope on empty Stack")); cout << "Leaving scope " << current_scope->getName(); current_scope = scope_stash.back(); scope_stash.pop_back();  cout << ", back to scope " << current_scope->getName() << endl;  }


}
