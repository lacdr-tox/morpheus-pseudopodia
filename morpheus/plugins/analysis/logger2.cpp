#include "logger2.h"
int Logger::instances=0;

REGISTER_PLUGIN(Logger);
	
Logger::Logger(){
	Logger::instances++; 
    instance_id = Logger::instances;
};
Logger::~Logger(){
	Logger::instances--;
};

void Logger::loadFromXML(const XMLNode xNode){
    stored_node = xNode;
    slice = false;
    
	// Load symbols
	XMLNode xInput = xNode.getChildNode("Input");
	for (uint i=0; i<xInput.nChildNode("Symbol"); i++) {
		auto input = make_shared<PluginParameter2 <double, XMLReadableSymbol> >();
		input->setXMLPath("Input/Symbol["+to_str(i)+"]/symbol-ref");
		inputs.push_back(input);
		registerPluginParameter(*input.get());
	}
	
	// Restriction
	// Slice
	map<string, FocusRangeAxis> slice_axis_map;
	slice_axis_map["x"] = FocusRangeAxis::X;
	slice_axis_map["y"] = FocusRangeAxis::Y;
	slice_axis_map["z"] = FocusRangeAxis::Z;
	slice_value.setXMLPath("Restriction/Slice/value");
	slice_axis.setXMLPath("Restriction/Slice/axis");
	slice_axis.setConversionMap(slice_axis_map);
	registerPluginParameter(slice_value);
	registerPluginParameter(slice_axis);
	
	// Celltype
	celltype.setXMLPath("Restriction/Celltype/celltype");
	registerPluginParameter(celltype);
	
	// Cell IDs
	cellids_str.setXMLPath("Restriction/Cells/cell-ids");
	registerPluginParameter(cellids_str);
	
	// Domain
	domain_only.setXMLPath("Restriction/domain-only");
	domain_only.setDefault("false");
	registerPluginParameter(domain_only);
	
	// Force node granularity
	//  By default, the granularity is determined automatically by checking the symbol with the smallest granularity
	//  This can be overridden by specifying force-node-granularity="true"
	force_node_granularity.setXMLPath("Input/force-node-granularity");
	force_node_granularity.setDefault("false");
	registerPluginParameter(force_node_granularity);

	// output
	XMLNode xOutput = xNode.getChildNode("Output");
	if (xOutput.nChildNode("TextOutput")) {
		cout << "Output/TextOutput" << endl;
		auto output = make_shared<LoggerTextWriter>(*this,"Output/TextOutput");
		writers.push_back(output);
	}
	if (xOutput.nChildNode("HDF5Output")) {
		// not implemented yet
	}

	// plots
	XMLNode xPlots = xNode.getChildNode("Plots");
	for (uint i=0; i<xPlots.nChildNode("Plot"); i++) {
// 		cout << "Logger::loadFromXML: Line " << i << endl;
		cout << "Number of Y-axis Symbols = " << xPlots.getChildNode("Plot",0).getChildNode("Y-axis",0).nChildNode("Symbol") << endl;
		shared_ptr<LoggerPlotBase> p( new LoggerLinePlot(*this, string("Plots/Plot[") + to_str(i) + "]") );
		plots.push_back(p);
	}
	for (uint i=0; i<xPlots.nChildNode("SurfacePlot"); i++) {
// 		cout << "Logger::loadFromXML: SurfacePlot " << i << endl;
		shared_ptr<LoggerPlotBase> p( new LoggerMatrixPlot(*this, string("Plots/SurfacePlot[") + to_str(i) + "]") );
		plots.push_back(p);
	}
	
	AnalysisPlugin::loadFromXML(xNode);
}


void Logger::init(const Scope* scope){
// 	cout << "Logger::init" << endl;
	
	// Set the PluginParameters of the symbols to be resolved in celltype scope, if it is defined as a restriction.
	const Scope* logging_scope = scope;
	celltype.init();
	if ( celltype.isDefined() && celltype() &&  ! cellids_str.isDefined()) {
		logging_scope = celltype()->getScope();
		for (auto &c : inputs) {
			c->setScope(logging_scope);
		}
	}
		
    AnalysisPlugin::init(scope);

	try{
		// Determine GRANULARITY
		
		// first, set global granularity by default
		logger_granularity = Granularity::Global;
		try {
			for(auto &c : inputs){
				logger_granularity+= c->granularity();
			}
		}
		catch (string s) {
			throw MorpheusException("Incompatible data granularity in Logger: " + s +".", stored_node);
		}

		// if node granularity is forced, set to node granularity
		if( force_node_granularity() == true ){
			logger_granularity = Granularity::Node;
			cout << "Logger: Forced node granularity" <<  endl;
		}
        
        if (slice_axis.isDefined() && slice_value.isDefined()) {
			slice = true;
		}
// 		cout << "Slice...?" << (slice?"true":"false") << endl;
		
		// Collect RESTRICTIONS
		
		if( cellids_str.isDefined() ){
			cellids = parseCellIDs( cellids_str() );
			for(auto &i : cellids){
				if( CPM::cellExists(i) ){
					restrictions.insert( pair<FocusRangeAxis, int>( FocusRangeAxis::CELL, i ) );
				}
			}
		}
		else if( celltype.isDefined() ){
			if(celltype()) {
				restrictions.insert( pair<FocusRangeAxis, int>( FocusRangeAxis::CellType, celltype()->getID()) );
			}
			else
				throw MorpheusException(string("Cannot restrict Logger to celltype "+celltype()->getName()+" because it is not defined."), stored_node);
		}
		
		if( slice ){
			if( logger_granularity != Granularity::Node )
                throw MorpheusException("Restriction to slice is only available for node granularity (e.g. Fields).\nYou may want to use the force-node-granularity option.", stored_node);
			restrictions.insert( pair<FocusRangeAxis, int>( slice_axis(),int( slice_value.get(SymbolFocus()))) );
		}

		// Now, we can set the FocusRange of the Logger
		range = FocusRange(logger_granularity, restrictions, domain_only());

		cout << "Logger INIT:  " << endl;
		cout << "Granularity:  " << logger_granularity <<  endl;
// 		cout << "Restrictions: " <<  endl;
// 		for(auto &r : restrictions)
// 			cout << int(r.first) << " = " << int(r.second) << endl;
// 		cout << "Range:        " << range.size() << endl;

		// initialize output
		for (auto o : writers){
			o->init();
		}
		
		// initialize plots
		for (auto p : plots){
			p->init();
		}
	}
	catch (string e) {
		throw MorpheusException(e,this->stored_node);
	}
	catch (GnuplotException e) {
		throw MorpheusException(e.what(), this->stored_node);
    }
};

int Logger::addWriter(shared_ptr<LoggerWriterBase> writer)
{
// 	cout << "Adding another Writer to the Logger" << endl;
	writers.push_back(writer);
	return writers.size()-1;
}

void Logger::analyse(double time){
	//cout << "Logger::analyse: " << time << "\t" << SIM::getTime() << endl;
	// write output files
	for (auto out : writers) {
		out->write();
	}
	//  and draw plots
	for(auto p : plots) {
		p->checkedPlot();
	}
}

void Logger::finish(){
//    cout << "Logger::finish..." << endl;
// 	for(auto p:plots)
// 		p->finish();
}

vector<CPM::CELL_ID> Logger::parseCellIDs(string cell_ids_string){
	cout << "Logger::parseCellIDs" << endl;
	vector<CPM::CELL_ID> ids;
	vector<string> tokens;
	tokens = tokenize(cell_ids_string, ",");
	for(uint i=0; i < tokens.size(); i++){
		cout << tokens[i] << "\n";
		vector<string> cell_ids = tokenize(tokens[i], "-");
		if(cell_ids.size() == 2){
			uint s = atoi(cell_ids[0].c_str());
			uint e = atoi(cell_ids[1].c_str());
			for(uint j = s; j <= e; j++){
				cout << j << ", ";
				ids.push_back( j );
			}
			cout << "\n";
		}
		else
			ids.push_back( atoi(tokens[i].c_str()) );
	}
	if ( ids.empty() ){
		throw string("No cell IDs were extracted from provided string: " + cell_ids_string);
	}

	return ids;
}

LoggerTextWriter::LoggerTextWriter(Logger& logger, string xml_base_path) : LoggerWriterBase(logger) { 
	
	header.setXMLPath(xml_base_path+"/header");
	header.setDefault("true");
	logger.registerPluginParameter(header);
	
	map<string, string> separator_convmap;
	separator_convmap["tab"]="\t";
	separator_convmap["comma"]=",";
	separator_convmap["semicolon"]=";";
	separator.setConversionMap(separator_convmap);
	separator.setXMLPath(xml_base_path+"/separator");
	separator.setDefault("tab");
	logger.registerPluginParameter(separator);
	
	filename.setXMLPath(xml_base_path + "/file-name");
	filename.setDefault("logger");
	logger.registerPluginParameter(filename);
	
	map<string, FileNumbering> numbering_map;
	numbering_map["time"] = FileNumbering::TIME;
	numbering_map["sequential"] = FileNumbering::SEQUENTIAL;
	file_numbering.setConversionMap(numbering_map);
	file_numbering.setDefault("time");
	file_numbering.setXMLPath(xml_base_path+"/file-numbering");
	logger.registerPluginParameter(file_numbering);
	
	map<string,FileSeparation> separation_map;
	separation_map["none"] = FileSeparation::NONE ;
	separation_map["time"] = FileSeparation::TIME ;
	separation_map["cell"] = FileSeparation::CELL ;
	separation_map["time+cell"] = FileSeparation::TIME_CELL;
	xml_file_separation.setXMLPath(xml_base_path+"/file-separation");
	xml_file_separation.setConversionMap(separation_map);
	xml_file_separation.setDefault("none");
	logger.registerPluginParameter(xml_file_separation);
	
	
	xml_file_format.setXMLPath(xml_base_path + "/file-format");
	map<string,OutputFormat> format_map;
	format_map["csv"] = OutputFormat::CSV;
	format_map["matrix"] = OutputFormat::MATRIX;
	xml_file_format.setConversionMap(format_map);
	logger.registerPluginParameter(xml_file_format);
	
	forced_format = false;
};

LoggerTextWriter::LoggerTextWriter(Logger& logger, LoggerTextWriter::OutputFormat format): LoggerWriterBase(logger)
{
	header.read("true");
	
	map<string, string> separator_convmap;
	separator_convmap["tab"]="\t";
	separator_convmap["comma"]=",";
	separator_convmap["semicolon"]=";";
	separator.setConversionMap(separator_convmap);
	separator.setDefault("tab");
	separator.read("tab");
	
	filename.setDefault("logger");
	filename.read("logger");
	
	map<string, FileNumbering> numbering_map;
	numbering_map["time"] = FileNumbering::TIME;
	numbering_map["sequential"] = FileNumbering::SEQUENTIAL;
	file_numbering.setConversionMap(numbering_map);
	file_numbering.setDefault("time");
	file_numbering.read("time");
	
	map<string,FileSeparation> separation_map;
	separation_map["none"] = FileSeparation::NONE ;
	separation_map["time"] = FileSeparation::TIME ;
	separation_map["cell"] = FileSeparation::CELL ;
	separation_map["time+cell"] = FileSeparation::TIME_CELL;
	xml_file_separation.setConversionMap(separation_map);
	xml_file_separation.setDefault("time");
	xml_file_separation.read("none");
	
	forced_format = true;
	file_format = format;
}


void LoggerTextWriter::init() {

// 	cout << "LoggerTextWriter::init" << endl;

	if ( (filename() == "logger" || filename() == "automatic") ) {
		file_basename = "logger";
		if (logger.getInstanceNum()>1) {
			file_basename+="_"+to_str(logger.getInstanceID());
		}
	}
	else 
		file_basename = filename();
	
	file_extension = ".txt";
	
	file_write_count = 0;
	
	Granularity granularity = logger.getGranularity();
	FocusRange range(granularity, logger.getRestrictions(), logger.getDomainOnly());
	
	if (forced_format) {
		if (file_format == OutputFormat::MATRIX && !range.isRegular() ) {
			throw string("Logger TextWriter: Cannot produce matrix data from given data (range is irregular).");
		}
	}
	else if(xml_file_format.isDefined()) {
		file_format = xml_file_format();
		if (file_format == OutputFormat::MATRIX && !range.isRegular() ) {
			throw string("Logger TextWriter: Cannot produce matrix data from given data (range is irregular).");
		}
	}
	else {
		if ( !range.isRegular() || range.dataAxis().size()<1 || (range.dataAxis().size()==1 && range.dataAxis()[0] == FocusRangeAxis::CELL) )
			file_format = OutputFormat::CSV;
		else
			file_format = OutputFormat::MATRIX;
	}
	
	cout << "Logger range:" << endl;
	cout << "-> size " << range.size() << endl;
	cout << "-> dataAxes " << range.dataAxis().size() << endl;
	cout << "-> data.sizes: "; for (auto &s :range.dataSizes() ) { cout << s << ", "; }; cout << endl;
	
	// Check that the file_separation value makes sense
	file_separation = xml_file_separation();
	if (range.dataAxis().empty() || range.dataAxis()[0] != FocusRangeAxis::CELL) {
		if ( file_separation == FileSeparation::CELL)
			file_separation = FileSeparation::NONE;
		if (file_separation == FileSeparation::TIME_CELL)
			file_separation = FileSeparation::TIME;
	}
	
	if (file_format == OutputFormat::CSV) {
// 		if (file_separation != FileSeparation::TIME && file_separation != FileSeparation::TIME_CELL) {
		output_symbols.push_back( SIM::findGlobalSymbol<double>(SymbolData::Time_symbol) );
		csv_header.push_back(SymbolData::Time_symbol);
// 		}
		
		for (auto& axis :range.dataAxis() ) {
			switch(axis) {
				case FocusRangeAxis::CELL :
					output_symbols.push_back( SIM::findGlobalSymbol<double>(SymbolData::CellID_symbol) );
					csv_header.push_back(SymbolData::CellID_symbol);
					break;
				case FocusRangeAxis::X :
					csv_header.push_back(SymbolData::Space_symbol+".x");
					output_symbols.push_back( SIM::findGlobalSymbol<double>(SymbolData::Space_symbol+".x") );
					break;
				case FocusRangeAxis::Y :
					csv_header.push_back(SymbolData::Space_symbol+".y");
					output_symbols.push_back( SIM::findGlobalSymbol<double>(SymbolData::Space_symbol+".y") );
					break;
				case FocusRangeAxis::Z :
					csv_header.push_back(SymbolData::Space_symbol+".z");
					output_symbols.push_back( SIM::findGlobalSymbol<double>(SymbolData::Space_symbol+".z") );
					break;
				case FocusRangeAxis::MEM_X :
					csv_header.push_back(SymbolData::MembraneSpace_symbol+".phi");
					output_symbols.push_back( SIM::findGlobalSymbol<double>(SymbolData::MembraneSpace_symbol+".phi") );
					break;
				case FocusRangeAxis::MEM_Y :
					csv_header.push_back(SymbolData::MembraneSpace_symbol+".theta");
					output_symbols.push_back( SIM::findGlobalSymbol<double>(SymbolData::MembraneSpace_symbol+".theta") );
					break;
				case FocusRangeAxis::NODE :
					csv_header.push_back(SymbolData::Space_symbol+".x");
					output_symbols.push_back( SIM::findGlobalSymbol<double>(SymbolData::Space_symbol+".x") );
					if (SIM::lattice().getDimensions()>1) {
						csv_header.push_back(SymbolData::Space_symbol+".y");
						output_symbols.push_back( SIM::findGlobalSymbol<double>(SymbolData::Space_symbol+".y") );
					}
					if (SIM::lattice().getDimensions()>2) {
						csv_header.push_back(SymbolData::Space_symbol+".z");
						output_symbols.push_back( SIM::findGlobalSymbol<double>(SymbolData::Space_symbol+".z") );
					}
					break;
				default:
					assert(0);
					break;
			}
		}
		auto& symbols = logger.getInputs();
		for (auto& s : symbols) {
			csv_header.push_back( s->name() );
			output_symbols.push_back( s->accessor() );
		}
	}
	else if (file_format == OutputFormat::MATRIX) {
		auto& symbols = logger.getInputs();
		for (auto& s : symbols) {
			csv_header.push_back( s->name() );
			output_symbols.push_back( s->accessor() );
		}
	}
	
}

int LoggerTextWriter::addSymbol(string name) {
	const Scope* s = SIM::getGlobalScope();
	if (!output_symbols.empty())
		s = output_symbols.back().getScope();
	SymbolAccessor<double> d = s->findSymbol<double>(name);
	output_symbols.push_back(d);
	if (OutputFormat::CSV == file_format) {
		csv_header.push_back(d.getName());
	}
	logger.registerInputSymbol(d.getName(), d.getScope());
	return output_symbols.size()-1;
}

string LoggerTextWriter::getDataFile(double time, string symbol) const {
	return getDataFile(SymbolFocus::global, time, symbol);
}

string LoggerTextWriter::getDataFile(const SymbolFocus& f, double time, string symbol) const {
	stringstream fn; 
	fn << file_basename;
	
	if (!symbol.empty()) {
		fn << "_"<< symbol;
	}
	
	switch (file_separation) {
		case FileSeparation::NONE :
			break;
		case FileSeparation::CELL :
			fn << "_" << f.cellID();
			break;
		case FileSeparation::TIME_CELL :
			fn <<  "_" << f.cellID();
		case FileSeparation::TIME :
			if (file_numbering() == FileNumbering::TIME)
				fn << "_" << SIM::getTimeName(time);
			else 
				fn << "_" << setfill('0') << setw(4) << file_write_count;
			break;
	}
	
	fn << file_extension;
	return fn.str();
}

vector<string> LoggerTextWriter::getDataFiles() {
	vector<string> ret;
	for (auto file : files_opened)  {
		ret.push_back(file.first);
	}
	return ret;
}

ofstream& LoggerTextWriter::getOutFile(const SymbolFocus& focus, string symbol) {
	
	string filename = getDataFile(focus, SIM::getTime(), symbol);
	
	auto cached_fs = files_opened.find(filename);
	if (cached_fs != files_opened.end()) {
		if (!cached_fs->second->is_open()) {
			cached_fs->second->open(cached_fs->first,  ofstream::out | ofstream::app );
		}
		return *cached_fs->second;
		
	}
	cout << "opening file " << filename << endl;
	
	files_opened[filename] = make_shared<ofstream>(filename, ofstream::out | ofstream::trunc);
	// files_opened[filename] = ofstream(filename,  ofstream::out | ofstream::trunc);
	// files_opened.insert( std::make_pair(filename, ofstream(filename,  ofstream::out | ofstream::trunc) ) );
	ofstream& out = *files_opened[filename];
	// out.open(filename,  ofstream::out | ofstream::trunc);
	
	if (file_format == OutputFormat::CSV && header() /* && f.tellp()==0 */ ) {
		//out << "#";
		bool first = true;
		for (auto& h : csv_header) {
			out << (first ? "" : separator())<< "\"" << h << "\"";
			first = false;
		}
		out << "\n";
	}
	if (file_format == OutputFormat::MATRIX && header() /* && f.tellp()==0 */) {
		// write header
		// TODO Matrix header missing ...
	}
	
	return out;
}


void LoggerTextWriter::write()
{
	file_write_count++;
	
	if (file_format == OutputFormat::CSV)
		writeCSV();
	else
		writeMatrix();
	
	for ( auto &file : files_opened) {
		file.second->close();
	}
}


void LoggerTextWriter::writeCSV() {
	
	Granularity granularity = logger.getGranularity();
	FocusRange range(granularity, logger.getRestrictions(), logger.getDomainOnly());
// 	auto& symbols = logger.getInputs();
	
	stringstream time;
	time << SIM::getTime();
	
	bool separate_cells = (file_separation == FileSeparation::TIME_CELL) || ((file_separation == FileSeparation::CELL) && (range.dataAxis()[0] == FocusRangeAxis::CELL));
	if (separate_cells) {
		auto cells = range.cells();
		multimap<FocusRangeAxis,int> plain_restrictions = logger.getRestrictions();
		auto cell_restr = plain_restrictions.equal_range(FocusRangeAxis::CELL);
		plain_restrictions.erase(cell_restr.first, cell_restr.second);
		auto celltype_restr = plain_restrictions.equal_range(FocusRangeAxis::CellType);
		plain_restrictions.erase(celltype_restr.first, celltype_restr.second);
		
		for (auto cell : cells) {
			multimap<FocusRangeAxis,int> restrictions = plain_restrictions;
			restrictions.insert(make_pair(FocusRangeAxis::CELL, cell) );
			FocusRange cell_range(granularity, restrictions, logger.getDomainOnly());
			ofstream& out = getOutFile( *(cell_range.begin()) );
			for (const SymbolFocus& focus : cell_range) {
				// write point of data row
				for (uint i=0; i<output_symbols.size(); i++ ) {
					if (i!=0) out << separator();
					out << output_symbols[i]( focus );
				}
				out << "\n";
			}
		}
		
	}
	else {
		ofstream& out = getOutFile( *(range.begin()) );
		for (const SymbolFocus& focus : range) {
			// write point of data row header
			for (uint i=0; i<output_symbols.size(); i++ ) {
				if (i!=0) out << separator();
				out << output_symbols[i]( focus );
			}
			out << "\n";
		}
		// treat single cell as 0-dimensional data
		int dim = range.dimensions();
		if( dim == 1 && range.size() == 1)
			dim = 0;
		// only separate multi-dimensional data
		if( dim > 0 )
			out << "\n";

	}
}



void LoggerTextWriter::writeMatrix() {
	Granularity granularity = logger.getGranularity();
	FocusRange range(granularity, logger.getRestrictions(), false);
	assert(range.isRegular());
	
	string sep = separator();
	
	bool separate_cells = file_separation == FileSeparation::TIME_CELL || (file_separation == FileSeparation::CELL && range.dataAxis()[0] == FocusRangeAxis::CELL);
	int row_length = range.dataSizes().back();
	int row_count = 1;
	if (range.dataSizes().size()>1)
		row_count = range.dataSizes()[range.dataSizes().size()-2];
	//cout << "row count = " << row_count << endl; 

	for (auto& symbol : output_symbols) {
		if (separate_cells) {
			// parallel output processing
			if (range.dataAxis()[0] == FocusRangeAxis::CELL) {
				vector<CPM::CELL_ID> cells = range.cells();
				multimap<FocusRangeAxis,int> plain_restrictions = logger.getRestrictions();
				auto cell_restr = plain_restrictions.equal_range(FocusRangeAxis::CELL);
				plain_restrictions.erase(cell_restr.first, cell_restr.second);
				auto celltype_restr = plain_restrictions.equal_range(FocusRangeAxis::CellType);
				plain_restrictions.erase(celltype_restr.first, celltype_restr.second);
				
#pragma omp parallel for
				for (uint c=0; c<cells.size(); c++) {
					multimap<FocusRangeAxis,int> restrictions = plain_restrictions;
					restrictions.insert( make_pair(FocusRangeAxis::CELL,cells[c]) );
					FocusRange range(granularity,restrictions);
// 					ofstream out(getOutFile( *(range.begin()), symbol->name()), ofstream::out | ofstream::app);
					ostream& out = getOutFile( *(range.begin()), symbol.getName());
					
					if( header() ){
						// not implemented yet
					}
					
					uint col=0;
					uint row=0;
					for (auto f : range) {
						out << symbol(f);
						col++;
						if (col==row_length) {
							out << "\n";
							col=0; row++;
							if (row == row_count) {
								out << "\n";
								row=0;
							}
						}
						else 
							out << sep;
					}
					// if 2D surface (e.g. membraneprop), separate the data sets by double blank line (to use gnuplot's index keyword)
					if( range.dimensions() > 1 )
						out << "\n\n";
				}
			}
			else {
				// That case should not happen
				cerr << "Missing implemtation" << endl;
				assert(0);
				exit(-1);
			}
		}
		else {
			if (range.size() == 0) return;
			ofstream& out = getOutFile( *(range.begin()), symbol.getName());
// 			FocusRange range(granularity,logger.getRestrictions());
			bool write_header_block=header();
			uint col=0;
			uint row=0;
			for (auto f : range) {
				if (write_header_block ){
					// write header above columns
					// for multiD, write header at the start of each data block
					if ( range.dimensions() > 1 ){
						writeMatrixColHeader(range, out);
						cout << "writeMatrixColHeader" << endl;
					}
					// for a 1D range, only write header at start (single data block)
					else if ( range.dimensions() == 1 && SIM::getTime() == SIM::getStartTime()){
						writeMatrixColHeader(range, out);
					}
					write_header_block=false;
				}
				if (col==0 && header() )
					// write header for row
					writeMatrixRowHeader(f, range, out);
				
				out << symbol(f);
				col++;
				if (col==row_length) {
					out << "\n";
					col=0; row++;
					if (row == row_count && row_count > 1) {
						out << "\n";
						row=0;
						write_header_block=header();
					}
				}
				else 
					out << sep;
			}
			// if 2D surface, separate the data sets by double blank line (to use gnuplot's index keyword)
			if( range.dimensions() > 1 )
				out << "\n\n";
		}
	}
}

void LoggerTextWriter::writeMatrixColHeader(FocusRange range, ofstream& fs)
{
	// write a header line containing
	// - first: number of columns
	// - then: x coordinates of matrix
	vector<int> sizes = range.dataSizes();
	string sep = separator();
	int dimensions = range.dimensions();
	Granularity granularity = logger.getGranularity();
	bool write_coordinate = true;
	int numcolumns = sizes[ sizes.size() - 1 ];
	fs << numcolumns << sep;
	int colnum = 1;
	double x_value = 0.0;
	double x_min = numeric_limits<double>::max();
	double y_min = numeric_limits<double>::max();
	double x_max = numeric_limits<double>::min();
	double y_max = numeric_limits<double>::min();
	for(SymbolFocus focus : range){
		switch( granularity ){
			case Granularity::Global:{
				// senseless to write Global to Matrix
				break;
			}
			case Granularity::Cell:{
				x_value = focus.cellID();
				//axes.x.label = SymbolData::CellID_symbol;
				break;
			}
			case Granularity::MembraneNode:{
				x_value = focus.membrane_pos().x;
				//axes.x.label = SymbolData::MembraneSpace_symbol;
				break;
			}
			case Granularity::Node:{
				
				auto restrictions = logger.getRestrictions();
				// check whether there is a slice-restriction
				bool is_slice = false;
				FocusRangeAxis slice;
				for(auto restriction : restrictions){
					if( restriction.first == FocusRangeAxis::X ||
						restriction.first == FocusRangeAxis::Y ||
						restriction.first == FocusRangeAxis::Z ){
						is_slice = true;
						slice = restriction.first;
					}
				} 
				
				if( is_slice ){
					switch( slice ){
						case FocusRangeAxis::X:{
							// y is cols, z is rows
							x_value = focus.pos().y;
							//axes.x.label = SymbolData::Space_symbol+".y";
							break;
						}
						case FocusRangeAxis::Y:{
							// x is cols, z is rows
							x_value = focus.pos().x;
							//axes.x.label = SymbolData::Space_symbol+".x";
							break;
						}
						case FocusRangeAxis::Z:{
							// x is cols, y is rows
							x_value = focus.pos().x;
							//axes.x.label = SymbolData::Space_symbol+".x";
							break;
						}
					}
				}
				else{
					x_value = focus.pos().x;
					//axes.x.label = SymbolData::Space_symbol+".x";
				}
				break;
			}
		}
		if( x_value < x_min )
			x_min = x_value;
		if( x_value > x_max )
			x_max = x_value;
		fs << x_value << sep;
		colnum++;
		if( colnum > numcolumns )
			break;
	}
	fs << "\n";
	fs.flush();	
}

void LoggerTextWriter::writeMatrixRowHeader(SymbolFocus focus, FocusRange range, ofstream& fs){
	Granularity granularity = logger.getGranularity();
	double y_value = 0.0;
	double x_min = numeric_limits<double>::max();
	double y_min = numeric_limits<double>::max();
	double x_max = numeric_limits<double>::min();
	double y_max = numeric_limits<double>::min();

	switch( granularity ){
		case Granularity::Global:
			// senseless to write Global to Matrix
			break;
		case Granularity::Cell:{
			y_value = SIM::getTime();
			break;
		}
		case Granularity::MembraneNode:{
			if( range.dimensions() > 1 ){
				y_value = focus.membrane_pos().y;
			}
			else{
				y_value = SIM::getTime();
			}
				
			break;
		}
		case Granularity::Node:{
			if( range.dimensions() == 1 ){
				y_value = SIM::getTime();
			}
			else{ // dimension > 1
				auto restrictions = logger.getRestrictions();
				// check whether there is a slice-restriction
				bool is_slice = false;
				FocusRangeAxis slice;
				for(auto restriction : restrictions){
					if( restriction.first == FocusRangeAxis::X ||
						restriction.first == FocusRangeAxis::Y ||
						restriction.first == FocusRangeAxis::Z ){
						is_slice = true;
						slice = restriction.first;
					}
				} 
				
				if( is_slice ){
					switch( slice ){
						case FocusRangeAxis::X:{
							// y is cols, z is rows
							y_value = focus.pos().z;
							break;
						}
						case FocusRangeAxis::Y:{
							// x is cols, z is rows
							y_value = focus.pos().z;
							break;
						}
						case FocusRangeAxis::Z:{
							// x is cols, y is rows
							y_value = focus.pos().y;
							break;
						}
					}
				}
				else {
					y_value = focus.pos().y;
				}
			}
			break;
		}
	}
	if( y_value < y_min )
		y_min = y_value;
	if( y_value > y_max )
		y_max = y_value;
	fs << y_value << separator();
}

//// -------------------------------------------------------------------------------------------


LoggerPlotBase::LoggerPlotBase(Logger& logger, string xml_base_path)
{
	map<string, Terminal> terminalmap;
	terminalmap["png"] = Terminal::PNG;
	terminalmap["pdf"] = Terminal::PDF;
	terminalmap["jpeg"] = Terminal::JPG;
	terminalmap["gif"] = Terminal::GIF;
	terminalmap["svg"] = Terminal::SVG;
	terminalmap["postscript"] = Terminal::EPS;
	terminalmap["screen"] = Terminal::SCREEN;
	
	time_step.setXMLPath(xml_base_path+"/time-step");
	logger.registerPluginParameter(time_step);
	
	logcommands.setXMLPath(xml_base_path+"/log-commands");
	logcommands.setDefault( "false" );
	logger.registerPluginParameter(logcommands);
	
	terminal.setXMLPath(xml_base_path+"/Terminal/terminal");
	terminal.setConversionMap(terminalmap);
	terminal.setDefault( "png" );
	logger.registerPluginParameter(terminal);
	
	terminal_file_extension[Terminal::PNG] = "png";
	terminal_file_extension[Terminal::PDF] = "pdf";
	terminal_file_extension[Terminal::JPG] = "jpeg";
	terminal_file_extension[Terminal::SVG] = "svg";
	terminal_file_extension[Terminal::EPS] = "eps";
	terminal_file_extension[Terminal::GIF] = "gif";
	terminal_file_extension[Terminal::SCREEN] = "";
	
	terminal_name[Terminal::PNG] = "pngcairo";
	terminal_name[Terminal::PDF] = "pdfcairo";
	terminal_name[Terminal::JPG] = "jpeg";
	terminal_name[Terminal::SVG] = "svg";
	terminal_name[Terminal::EPS] = "epscairo";
	terminal_name[Terminal::GIF] = "gif";
	
	plotsize.setXMLPath(xml_base_path+"/Terminal/plot-size");
    logger.registerPluginParameter(plotsize);

	
	map<string, FileNumbering> file_numbering_map;
	file_numbering_map["time"] = FileNumbering::TIME;
	file_numbering_map["sequential"] = FileNumbering::SEQUENTIAL;
	
    file_numbering.setXMLPath(xml_base_path+"/file-numbering");
	file_numbering.setConversionMap(file_numbering_map);
    file_numbering.setDefault( "time" );
    logger.registerPluginParameter(file_numbering);
	
	last_plot_time = 0.0;
	plot_num = 0; // for sequential file numbering
}

void LoggerPlotBase::checkedPlot()
{
// 	cout << SIM::getTime() << "\t" << (last_plot_time + time_step()) << endl;
	if( !time_step.isDefined() ) { // not defined, always plot if Logger is executed (as joern suggested)
		this->plot(); last_plot_time = SIM::getTime();
	}
	else if (time_step() <= 0.0) { // if time=step=0.0 or -1, only plot at the end of simulation
		if( SIM::getTime() == SIM::getStopTime() ){
			this->plot(); last_plot_time = SIM::getTime();
		}
	}
	else if( (last_plot_time + time_step()) - SIM::getTime() < 10e-6   // t > t_-1 + dt 
		|| SIM::getTime() < 10e-6 ) { // if t=0.0
		this->plot(); last_plot_time = SIM::getTime();
	}
	else{
		//cout << "NO PLOT" << endl;
		// do not plot
	}
}



LoggerLinePlot::LoggerLinePlot(Logger& logger, string xml_base_path) : LoggerPlotBase(logger, xml_base_path), logger(logger)
{
	   // --- Range ---
	map<string, TimeRange> timerangemap;
	timerangemap["all"] = TimeRange::ALL;
	timerangemap["since last plot"] = TimeRange::SINCELAST;
	timerangemap["history"] = TimeRange::HISTORY;
	timerangemap["current"] = TimeRange::CURRENT;

	timerange.setXMLPath(xml_base_path+"/Range/Time/mode");
	timerange.setConversionMap(timerangemap);
	timerange.setDefault( "all" );
	logger.registerPluginParameter(timerange);

	history.setXMLPath(xml_base_path+"/Range/Time/history");
	logger.registerPluginParameter(history);

	first_line.setXMLPath(xml_base_path+"/Range/Data/first-line");
	logger.registerPluginParameter(first_line);

	last_line.setXMLPath(xml_base_path+"/Range/Data/last-line");
	logger.registerPluginParameter(last_line);

	increment.setXMLPath(xml_base_path+"/Range/Data/increment");
	logger.registerPluginParameter(increment);
	
	    // --- Style ---

	map<string, Style> stylemap;
	stylemap["points"]         = Style::POINTS;
	stylemap["lines"]          = Style::LINES;
	stylemap["linespoints"]    = Style::LINESPOINTS;

	style.setXMLPath(xml_base_path+"/Style/style");
	style.setConversionMap(stylemap);
	style.setDefault( "points" );
	logger.registerPluginParameter(style);

	decorate.setXMLPath(xml_base_path+"/Style/decorate");
	decorate.setDefault( "true" );
	logger.registerPluginParameter(decorate);

	pointsize.setXMLPath(xml_base_path+"/Style/point-size");
	pointsize.setDefault( "1.0" );
	logger.registerPluginParameter(pointsize);

	linewidth.setXMLPath(xml_base_path+"/Style/line-width");
	linewidth.setDefault( "1.0" );
	logger.registerPluginParameter(linewidth);

	grid.setXMLPath(xml_base_path+"/Style/grid");
	grid.setDefault( "false" );
	logger.registerPluginParameter(grid);
	
	
	string tag = "X-axis";
// 	axes.x.present = true;
	axes.x.symbol.setXMLPath(xml_base_path+"/"+tag+"/Symbol/symbol-ref");
	axes.x.min.setXMLPath(xml_base_path+"/"+tag+"/"+"minimum");
	axes.x.max.setXMLPath(xml_base_path+"/"+tag+"/"+"maximum");
	axes.x.logarithmic.setXMLPath(xml_base_path+"/"+tag+"/"+"logarithmic");
	axes.x.logarithmic.setDefault("false");
	logger.registerPluginParameter(axes.x.symbol);
	logger.registerPluginParameter(axes.x.min);
	logger.registerPluginParameter(axes.x.max);
	logger.registerPluginParameter(axes.x.logarithmic);

	// prepare for up to 10 y-axis elements
	tag = "Y-axis";
	int num_ycols = 10;
	axes.y.symbols.resize(num_ycols);
	for(uint i=0; i<num_ycols;i++){
		axes.y.symbols[i]->setXMLPath(xml_base_path+"/"+tag+"/Symbol["+to_str(i)+"]/symbol-ref");
		logger.registerPluginParameter(axes.y.symbols[i]);
	}
	axes.y.min.setXMLPath(xml_base_path+"/"+tag+"/"+"minimum");
	axes.y.max.setXMLPath(xml_base_path+"/"+tag+"/"+"maximum");
	axes.y.logarithmic.setXMLPath(xml_base_path+"/"+tag+"/"+"logarithmic");
	axes.y.logarithmic.setDefault("false");
	logger.registerPluginParameter(axes.y.min);
	logger.registerPluginParameter(axes.y.max);
	logger.registerPluginParameter(axes.y.logarithmic);
	
	tag = "Color-bar";
	axes.cb.symbol.setXMLPath(xml_base_path+"/"+tag+"/Symbol/symbol-ref");
	logger.registerPluginParameter(axes.cb.symbol);

	axes.cb.min.setXMLPath(xml_base_path+"/"+tag+"/"+"minimum");
	logger.registerPluginParameter(axes.cb.min);
	axes.cb.max.setXMLPath(xml_base_path+"/"+tag+"/"+"maximum");
	logger.registerPluginParameter(axes.cb.max);

	axes.cb.logarithmic.setXMLPath(xml_base_path+"/"+tag+"/"+"logarithmic");
	axes.cb.logarithmic.setDefault("false");
	logger.registerPluginParameter(axes.cb.logarithmic);

	map<string, LoggerPlotBase::Palette> paletteMap;
	paletteMap["hot"]       = Palette::HOT;
	paletteMap["afmhot"]    = Palette::AFMHOT;
	paletteMap["rainbow"]   = Palette::RAINBOW;
	paletteMap["gray"]      = Palette::GRAY;
	paletteMap["ocean"]     = Palette::OCEAN;
	paletteMap["grv"]       = Palette::GRV;
	paletteMap["default"]   = Palette::DEFAULT;
	axes.cb.palette.setXMLPath(xml_base_path+"/"+tag+"/"+"palette");
	axes.cb.palette.setConversionMap( paletteMap );
	logger.registerPluginParameter(axes.cb.palette);

	axes.cb.palette_reverse.setXMLPath(xml_base_path+"/"+tag+"/"+"reverse-palette");
	axes.cb.palette_reverse.setDefault("false");
	logger.registerPluginParameter(axes.cb.palette_reverse);
	
	
	
	
};


void LoggerLinePlot::init() {
// 	cout << "LoggerLinePlot::init" << endl;

	// Check a suitable output is present
	const auto& writers = logger.getWriters();
	for (auto out : writers) {
		if (dynamic_pointer_cast<LoggerTextWriter>(out)) {
			writer = dynamic_pointer_cast<LoggerTextWriter>(out);
			if (writer->getOutputFormat() != LoggerTextWriter::OutputFormat::CSV || 
			(writer->getFileSeparation() != LoggerTextWriter::FileSeparation::NONE && writer->getFileSeparation() != LoggerTextWriter::FileSeparation::CELL) )
				writer = nullptr;
			else 
				break;
		}
	}
	
	if (!writer) {
		int ret = logger.addWriter(make_shared<LoggerTextWriter>(logger, LoggerTextWriter::OutputFormat::CSV));
		if (ret<0)
			throw string("Unable to create writer for LoggerLinePlot");
		writer =  dynamic_pointer_cast<LoggerTextWriter>(logger.getWriters()[ret]);
		if (!writer)
			throw string("Unable to create writer for LoggerLinePlot");
		writer->init();
	}
	

	// Check whether all required input is defined and read the column number
	auto row_symbols = writer->getSymbols();
	
	struct SymCol { string symbol; int* col; };
	vector<SymCol> all_sym_cols;
	
	if (!axes.x.symbol.isDefined())
		throw string( "LoggerLinePlot: X-axis symbol not defined" );
	all_sym_cols.push_back( { axes.x.symbol(), &axes.x.column_num } );

	axes.cb.defined = axes.cb.symbol.isDefined();
	if (axes.cb.defined)
		all_sym_cols.push_back( { axes.cb.symbol(), &axes.cb.column_num } );
	
	// count the number of defined symbols
	num_defined_symbols = 0;
	for (int i=0; i< axes.y.symbols.size(); i++) {
		if (axes.y.symbols[i]->isDefined())
			num_defined_symbols++;
		else
			break;
	}
	cout << "Found " << num_defined_symbols << " symbols to plot" << endl; 
	axes.y.column_nums.resize(num_defined_symbols);
	
	for (int i=0; i<num_defined_symbols; i++) {
		all_sym_cols.push_back( { axes.y.symbols[i](), &axes.y.column_nums[i] } );
	}
	
	for (auto& sym : all_sym_cols) {
		bool found = false;
		for (int i=0; i<row_symbols.size(); i++) {
			// TODO Be tolerant to white space error
			if (row_symbols[i].getName() == sym.symbol) {
				found = true;
				*sym.col = i+1;
				break;
			}
		}
		if (!found) {
			int col = writer->addSymbol(sym.symbol);
			*sym.col = col+1;
		}
	}
	
	gnuplot = make_shared<Gnuplot>();
	 
	if( timerange() == TimeRange::HISTORY && !history.isDefined() )
	throw string("LoggerLinePlot: TimeRange::History mode requires the specification of a history length (in units of simulation time).");
	
	// set the axis labels
	// x label
	axes.x.label = "\""+axes.x.symbol()+"\"";
	// y label
	axes.y.label = "\"";
	for (auto s : axes.y.symbols){
		if (s->isDefined()) {
			if ( axes.y.label.size() > 1 ) // cannot check for empty() because it already contains a '"' character
				axes.y.label += ", ";
			axes.y.label += s();
		}
	}
	axes.y.label += "\"";
	
	axes.cb.defined = axes.cb.symbol.isDefined();
	if (axes.cb.defined) {
		axes.cb.label = "\""+axes.cb.symbol()+"\"";
	}
	
}

void LoggerLinePlot::plot()
{
	// generate filename for plot output file
	string plotfilename;
	
	if (terminal() != Terminal::SCREEN) {
		string outputfilename_ext = terminal_file_extension[ terminal() ];
		ostringstream fn;
		string datafilename_base = writer->getDataFileBaseName();
		fn << datafilename_base << "_plot_";
		fn << axes.x.symbol() << "_";
		fn << axes.y.symbols[0]() << "_";
		if( axes.cb.symbol.isDefined() )
			fn << axes.cb.symbol() << "_";
		
		if ( file_numbering() == FileNumbering::SEQUENTIAL ){
			fn << setfill('0') << setw(5) << plot_num++; 
			//fn << setfill('0') << setw(5) << int(rint( SIM::getTime() / logger.timeStep()));
		}
		else
			fn << SIM::getTimeName();
		fn << "." << outputfilename_ext;
		
		plotfilename = fn.str();
	}
//	cout << "LoggerPlot: plotfilename " << plotfilename << endl;

	ostringstream ss;

	
	string terminal_str  = terminal_name[terminal()];
	if (!terminal_str.empty()) {
		ss << "set terminal " << terminal_str << " ";

		if( plotsize.isDefined() ){
			VINT size = plotsize();
			ss << "size " << size.x << ", " << size.y << ";\n";
		}
		
		ss << ";\n";
	}
	if (terminal() != Terminal::SCREEN) {
		ss << "set output '" << plotfilename << "';\n";
	}

	// Separator
	if( writer->getSeparator() != "\t" ){
		ss << "set datafile separator \""<< writer->getSeparator() <<"\";\n";
	}
	
	if( grid() ){
		ss << "set grid;\n";
	}

	if( !decorate() ){
		ss << "unset xlabel;\n";
		ss << "unset ylabel;\n";
		ss << "unset xtics;\n";
		ss << "unset ytics;\n";
		ss << "unset cblabel;\n";
		ss << "unset key;\n";
		ss << "unset colorbox;\n";
	}
	else{
		string xlabel = axes.x.label;
        string ylabel = axes.y.label;
		string cblabel = axes.cb.label;
		ss << "set xlabel "<< xlabel <<";\n" ;
		ss << "set ylabel "<< ylabel <<";\n" ;
		if(axes.cb.symbol.isDefined()){
			ss << "set cblabel "<<  cblabel << ";\n";
			if( axes.cb.palette.isDefined() ){
				switch( axes.cb.palette() ){
					case( Palette::HOT ):
						ss << "set palette rgb 21,22,23;\n"; break;
					case(  Palette::AFMHOT ):
						ss << "set palette rgb 34,35,36;\n"; break;
					case(  Palette::OCEAN ):
						ss << "set palette rgb 23,28, 3;\n"; break;
					case(  Palette::GRV ):
						ss << "set palette rgb  3,11, 6;\n"; break;
					case( Palette::RAINBOW ):
						ss << "set palette rgb 33,13,10;\n"; break;
					case( Palette::GRAY ):
						ss << "set palette gray;\n"; break;
					default:
						ss << "set palette rgb  7, 5,15;\n"; break;
				}
				// TODO: import decent colorscales
// 				string palette = gnuplot->set_palette( int(axes.cb.palette()) );
// 				replace_substring(palette, "\\ \n", " ");
// 				ss << palette;
			}
			if( axes.cb.palette_reverse() )
				ss << "set palette negative;\n";
		}
		else
			ss << "unset colorbox;\n";

		// only show legend (key) when plotting multiple data columns
		if( axes.y.column_nums.size() == 1 )
			ss << "unset key;\n";
	}
	if(axes.x.logarithmic())
		ss << "set log x;\n";
	if(axes.y.logarithmic())
		ss << "set log y;\n";
	if(axes.cb.logarithmic())
		ss << "set log cb;\n";

	string min_x = "*";
	string max_x = "*";
	string min_y = "*";
	string max_y = "*";
	string min_cb = "*";
	string max_cb = "*";
	
	ostringstream oss;
	if( axes.x.min.isDefined() ){
		oss.clear(); oss.str("");
		oss << axes.x.min.get(SymbolFocus());
		min_x = oss.str();
	}
	if( axes.x.max.isDefined() ){
		oss.clear(); oss.str("");
		oss << axes.x.max.get(SymbolFocus());
		max_x = oss.str();
	}
	if( axes.y.min.isDefined() ){
		oss.clear(); oss.str("");
		oss << axes.y.min.get(SymbolFocus());
		min_y = oss.str();
	}
	if( axes.y.max.isDefined() ){
		oss.clear(); oss.str("");
		oss << axes.y.max.get(SymbolFocus());
		max_y = oss.str();
	}
	if( axes.cb.min.isDefined() ){
		oss.clear(); oss.str("");
		oss << axes.cb.min.get(SymbolFocus());
		min_cb = oss.str();
	}
	if( axes.cb.max.isDefined() ){
		oss.clear(); oss.str("");
		oss << axes.cb.max.get(SymbolFocus());
		max_cb = oss.str();
	}



    // Style
	stringstream linespoints_ss;
	switch( style() ){
		case(Style::POINTS):{
			linespoints_ss << " with points pt 7 pointsize " << pointsize();
			break;
		}
		case(Style::LINES):{
			linespoints_ss << " with lines linewidth " << linewidth();
			break;
		}
		case(Style::LINESPOINTS):{
			linespoints_ss << " with linespoints pt 7 linewidth " << linewidth() << " pointsize " << pointsize();
			break;
		}
	}

    // Range
    stringstream datarange_ss;
    if( first_line.isDefined() || last_line.isDefined() || increment.isDefined() ){
        datarange_ss << "every ";
        if( increment.isDefined() )
            datarange_ss << int(increment.get( SymbolFocus() ));
        if( first_line.isDefined() )
            datarange_ss << "::" << int(first_line.get( SymbolFocus() ));
        if( last_line.isDefined() && !first_line.isDefined() )
            datarange_ss << "::";
        if( last_line.isDefined() )
            datarange_ss << "::" << int(last_line.get( SymbolFocus() ));
        datarange_ss << " ";
    }

    // The main plot command
    // TODO:: Here we must take care to pick the proper files and offsets for the plots,
    // also considering the data might be chunked into different files for timepoints and cells.
    
    if (writer->getFileSeparation() == LoggerTextWriter::FileSeparation::TIME || writer->getFileSeparation()== LoggerTextWriter::FileSeparation::TIME_CELL) {
		// just pick the proper time points to plot
		cout << "LoggerLinePlot::plot: missing implementation" << endl;
		assert(0);
	}
    else {
// 		if (writer->getFileSeparation() == LoggerTextWriter::FileSeparation::CELL) {
// 			cout << "LoggerLinePlot::plot: missing implementation" << endl;
// 			assert(0);
// 		}
		double time = SIM::getTime();
		vector<string> datafiles;
		
		if (writer->getFileSeparation() == LoggerTextWriter::FileSeparation::NONE)
			datafiles.push_back( writer->getDataFile(time) );
		else {
			datafiles = writer->getDataFiles();
		}

		
		// set range of time points to include in plot
		stringstream timerange_x;
		stringstream timerange_cb;
		switch( timerange() ){
			case(TimeRange::ALL):{
				timerange_x << "($1 <= " << time << "?$" << (axes.x.column_num) << ":NaN)";
				timerange_cb << "($1 <= " << time << "?$" << (axes.cb.column_num) << ":NaN)";
				break;
			}
			case(TimeRange::SINCELAST):{
				timerange_x << "($1 > " << last_plot_time << " && $1 <= " << time << "?$" << (axes.x.column_num) << ":NaN)";
				timerange_cb << "($1 > " << last_plot_time << " && $1 <= " << time << "?$" << (axes.cb.column_num) << ":NaN)";
				break;
			}
			case(TimeRange::HISTORY):{
				timerange_x << "($1 > " << (time-history.get( SymbolFocus())) << " && $1 <= " << time << "?$" << (axes.x.column_num) << ":NaN)";
				timerange_cb << "($1 > " << (time-history.get( SymbolFocus())) << " && $1 <= " << time << "?$" << (axes.cb.column_num) << ":NaN)";
				break;
			}
			case(TimeRange::CURRENT):{
				timerange_x << "($1 == " << time << "?$" << (axes.x.column_num) << ":NaN)";
				timerange_cb << "($1 == " << time << "?$" << (axes.cb.column_num) << ":NaN)";
				break;
			}
		}
		
		ss << "plot ["<< min_x <<":"<< max_x <<"]["<< min_y <<":"<< max_y <<"] ";
		
		for (uint f=0; f<datafiles.size(); f++) {
			for(uint c=0; c<num_defined_symbols; c++){
				if ( ! (c==0 && f== 0) )
					ss << ", ";
					
				if (c==0)
					ss << "'./" << datafiles[f] << "' ";
				else
					ss << "'' ";
				ss << datarange_ss.str() << " us " << timerange_x.str() << ":"<< axes.y.column_nums[c] <<":" << \
				( axes.cb.defined ? timerange_cb.str() : "(0)")  << \
				linespoints_ss.str() << (axes.cb.defined ? " pal":"") << " title \"" << axes.y.symbols[c]() << "\"";
			}
		}
		ss << ";\n";
	}

	ss << "unset output;";

    // Write gnuplot commands to file
	if( logcommands() ){
		ofstream command_log;
        command_log.open(string("logger_plot_commands.gp").c_str(),ios_base::out);
		command_log << ss.str();
		command_log.close();
	}

    // Finally, execute the plot commands in Gnuplot
	gnuplot->cmd( ss.str() );
}

LoggerMatrixPlot::LoggerMatrixPlot(Logger& logger, string xml_base_path): LoggerPlotBase(logger, xml_base_path), logger(logger)
{

	string tag = "Color-bar";
	cb_axis.symbol.setXMLPath(xml_base_path+"/"+tag+"/Symbol/symbol-ref");
	logger.registerPluginParameter(cb_axis.symbol);

	cb_axis.min.setXMLPath(xml_base_path+"/"+tag+"/"+"minimum");
	logger.registerPluginParameter(cb_axis.min);
	cb_axis.max.setXMLPath(xml_base_path+"/"+tag+"/"+"maximum");
	logger.registerPluginParameter(cb_axis.max);

	cb_axis.logarithmic.setXMLPath(xml_base_path+"/"+tag+"/"+"logarithmic");
	cb_axis.logarithmic.setDefault("false");
	logger.registerPluginParameter(cb_axis.logarithmic);

	map<string, LoggerPlotBase::Palette> paletteMap;
	paletteMap["hot"]       = Palette::HOT;
	paletteMap["afmhot"]    = Palette::AFMHOT;
	paletteMap["rainbow"]   = Palette::RAINBOW;
	paletteMap["gray"]      = Palette::GRAY;
	paletteMap["ocean"]     = Palette::OCEAN;
	paletteMap["grv"]       = Palette::GRV;
	paletteMap["default"]   = Palette::DEFAULT;
	cb_axis.palette.setXMLPath(xml_base_path+"/"+tag+"/"+"palette");
	cb_axis.palette.setConversionMap( paletteMap );
	logger.registerPluginParameter(cb_axis.palette);

	cb_axis.palette_reverse.setXMLPath(xml_base_path+"/"+tag+"/"+"reverse-palette");
	cb_axis.palette_reverse.setDefault("false");
	logger.registerPluginParameter(cb_axis.palette_reverse);
}

void LoggerMatrixPlot::init()
{
	// Check a suitable output is present
	const auto& writers = logger.getWriters();
	for (auto out : writers) {
		if (dynamic_pointer_cast<LoggerTextWriter>(out)) {
			writer = dynamic_pointer_cast<LoggerTextWriter>(out);
			if (writer->getOutputFormat() != LoggerTextWriter::OutputFormat::MATRIX || writer->getFileSeparation() != LoggerTextWriter::FileSeparation::NONE)
				writer = nullptr;
			else 
				break;
		}
	}
	
	if (!writer) {
		int ret = logger.addWriter(make_shared<LoggerTextWriter>(logger, LoggerTextWriter::OutputFormat::MATRIX));
		if (ret<0)
		{
			throw string("Unable to create writer for LoggerMatrixPlot");
		}
		
		writer =  dynamic_pointer_cast<LoggerTextWriter>(logger.getWriters()[ret]);
		if (!writer)
			throw string("Unable to create writer for LoggerMatrixPlot");
		writer->init();
	}
	
			// Check whether all required input is defined
	auto& inputs = writer->getSymbols();
	
	vector<string> all_symbols;
	all_symbols.push_back(cb_axis.symbol());
	
	for (auto& sym : all_symbols) {
		bool found = false;
		for (auto& i : inputs) {
			// TODO Be tolerant to white space error
			if (i.getName() == sym) {
				found = true;
			}
		}
		if (!found) writer->addSymbol(sym);
	}
	
	
	gnuplot = make_shared<Gnuplot>();
	
	cb_axis.defined = cb_axis.symbol.isDefined();
	if( cb_axis.defined){
		cb_axis.label = cb_axis.symbol();
	}
	else
		throw string("Logger: Matrix plot requires specification of a symbol for the Colorbar");
}

void LoggerMatrixPlot::plot()
{
// 	cout << "LoggerMatrixPlot::plot" << endl;
	//throw MorpheusException("Logger/Plot: Plotting Matrix format is not implemented yet.", stored_node);

	// TODO: 
	// - Select proper input datafile according to Color-bar symbol (LoggerOutput needs store multiple output file names)
	// - Add option to select the relevant datablock (to feed to the gnuplot keyword 'index') with a MathExpression (datablocks can represent either time frames or z-slices (for 3D lattices))
	// - Add option to select colormap

    // generate filename for plot output file
	string datafile = writer->getDataFile(SIM::getTime(),cb_axis.symbol());

    string outputfilename_ext = terminal_file_extension[ terminal() ];

    ostringstream fn;	
    fn << "logger_";
	if (logger.getInstanceNum()>1)
		fn << logger.getInstanceID() << "_";
	fn << cb_axis.symbol() << "_plot_";

    if ( file_numbering() == FileNumbering::SEQUENTIAL){
        fn << setfill('0') << setw(5) << plot_num++;
		//fn << setfill('0') << setw(4) << int(rint( SIM::getTime() / logger.timeStep()));
	}
    else
        fn << SIM::getTimeName();
    fn << "." << outputfilename_ext;
    string plotfilename = fn.str();
//	cout << "LoggerPlot: plotfilename " << plotfilename << endl;

    ostringstream ss;
    string terminal_str = terminal_name[ terminal() ];
	if (!terminal_str.empty()) {
		ss << "set terminal " << terminal_str ;

		if( plotsize.isDefined() ){
			VINT size = plotsize();
			ss << " size " << size.x << ", " << size.y;
		}
		ss << ";\n";
	}
	
	if (terminal() != Terminal::SCREEN)
		ss << "set output '" << plotfilename << "';\n";

    // Separator
    if( writer->getSeparator() != "\t" ){
        ss << "set datafile separator \""<< writer->getSeparator() <<"\";\n";
    }

////////////////////////////////////////////////////////////////////////////////////

    if ( cb_axis.palette.isDefined() ){
        switch( cb_axis.palette() ){
        case( Palette::HOT ):
            ss << "set palette rgb 21,22,23;\n"; break;
        case( Palette::AFMHOT ):
            ss << "set palette rgb 34,35,36;\n"; break;
        case( Palette::OCEAN ):
            ss << "set palette rgb 23,28, 3;\n"; break;
        case( Palette::GRV ):
            ss << "set palette rgb  3,11, 6;\n"; break;
        case( Palette::RAINBOW ):
            ss << "set palette rgb 33,13,10;\n"; break;
        case( Palette::GRAY ):
            ss << "set palette gray;\n"; break;
        default:
            ss << "set palette rgb  7, 5,15;\n"; break;
        }
    }
    
    if( cb_axis.palette_reverse() )
        ss << "set palette negative;\n";

	// TODO: get labels back

	ss << "set xlabel \"" << getLabels()[0] << "\";\n";
	ss << "set ylabel \"" << getLabels()[1] << "\";\n";
    ss << "set cblabel \"" << cb_axis.label << "\";\n";

//     string xmin = (logger.getOutput()->getAxes().x.min < numeric_limits<double>::max() ? to_str(logger.getOutput()->getAxes().x.min) : "*");
//     string ymin = (logger.getOutput()->getAxes().y.min < numeric_limits<double>::max() ? to_str(logger.getOutput()->getAxes().y.min) : "*");
    string cbmin= (cb_axis.min.isDefined() ? to_str(cb_axis.min.get(SymbolFocus())): "*");
//     string xmax = (logger.getOutput()->getAxes().x.max > numeric_limits<double>::min() ? to_str(logger.getOutput()->getAxes().x.max) : "*");
//     string ymax = (logger.getOutput()->getAxes().y.max > numeric_limits<double>::min() ? to_str(logger.getOutput()->getAxes().y.max) : "*");
    string cbmax= (cb_axis.max.isDefined() ? to_str(cb_axis.max.get(SymbolFocus())): "*");
//     ss << "set xrange  [" << xmin << ":"  << xmax << "];\n";
//     ss << "set yrange  [" << ymin << ":"  << ymax << "];\n";
    ss << "set xrange  [0:*];\n";
    ss << "set yrange  [0:*];\n";
    ss << "set cbrange [" << cbmin << ":" << cbmax << "];\n";

// TODO Fix Matrix Header and Block selection

	string nonuniform;
    // main plot command
 	if( writer->hasHeader() )
 		nonuniform = " nonuniform ";

/////  DATA BLOCK:
    // - first: 0
    // - last: if( separate_files ) STATS_black-2, else 0
    // - fixed: user-specified

    // if( last_block ) // get index of last data block
    //  if( separate_file == false ) 0
    //  else
    ss << "stats \"" <<  writer->getDataFile(SIM::getTime(),cb_axis.symbol()) << "\";\n";
    ss << "block = STATS_blocks-2;\n";
    ss << "if(block<0) block = 0;\n";

    // if fixed data block
    //ss << "stats \"" << datafile << "\";\n";
    //ss << "if block = STATS_blocks-2;\n";
    //ss << "if( " << datablock() << " > last_block ) print \"Error: Specified data block not available in file."\"; else block=" << last_block << "; print \"All okay...\";\n";

//// MAIN PLOT COMMAND
    // PLOT
//     ss << "plot \"" << datafile << "\" index block " << nonuniform << " matrix with image notitle;\n";
    ss << "plot \"" << datafile << "\" index block " << nonuniform << " matrix with image notitle;\n";
    ///// SPLOT with PM3D
    // Switch off interpolation
    //ss << "set pm3d map;\n";
    ss << "set pm3d corners2color c1;\n";

    //ss << "splot \"" << datafilename_base << "\" " << nonuniform << " matrix index block notitle;\n";


    // Heatmap can also be plotted with plot (matrix with image).
    // However, this way, one cannot select a particular dataset (datablock).

////////////////////////////////////////////////////////////////////////////////////

	ss << "unset output;";

	// Write gnuplot commands to file
	if( logcommands() ){
		ofstream command_log;
		string filename("logger_");
		if (logger.getInstanceNum()>1)
			filename +=  to_str(logger.getInstanceID()) + "_";
		filename +=  "_plot_commands.gp";
		command_log.open(filename.c_str(),ios_base::out);
		command_log << ss.str();
		command_log.close();
    }

	// Finally, execute the plot commands in Gnuplot
	gnuplot->cmd( ss.str() );

}



//// -------------------------------------------------------------------------------------------

vector<string> LoggerMatrixPlot::getLabels()
{
	Granularity granularity = logger.getGranularity();
	FocusRange range(granularity, logger.getRestrictions(), logger.getDomainOnly());

	vector<string> xy_labels;	
	switch( granularity ){
		case Granularity::Global:{
			// senseless to write Global to Matrix
			break;
		}
		case Granularity::Cell:{
			xy_labels.push_back( SymbolData::CellID_symbol );
			xy_labels.push_back( SymbolData::Time_symbol );
			break;
		}
		case Granularity::MembraneNode:{
			xy_labels.push_back( SymbolData::MembraneSpace_symbol );
			if(range.dimensions()>1)
				xy_labels.push_back( SymbolData::MembraneSpace_symbol );
			else
				xy_labels.push_back( SymbolData::Time_symbol );
			break;
		}
		case Granularity::Node:{
			auto restrictions = logger.getRestrictions();
			// check whether there is a slice-restriction
			bool is_slice = false;
			FocusRangeAxis slice;
			for(auto restriction : restrictions){
				if( restriction.first == FocusRangeAxis::X ||
					restriction.first == FocusRangeAxis::Y ||
					restriction.first == FocusRangeAxis::Z ){
					is_slice = true;
					slice = restriction.first;
				}
			} 
			
			if( is_slice ){
				switch( slice ){
					case FocusRangeAxis::X:{
						// y is cols, z is rows
						xy_labels.push_back( SymbolData::Space_symbol+".y" );
						if(range.dimensions()>1)
							xy_labels.push_back( SymbolData::Space_symbol+".z" );
						else
							xy_labels.push_back( SymbolData::Time_symbol );
						break;
					}
					case FocusRangeAxis::Y:{
						// x is cols, z is rows
						xy_labels.push_back( SymbolData::Space_symbol+".x" );
						if(range.dimensions()>1)
							xy_labels.push_back( SymbolData::Space_symbol+".z" );
						else
							xy_labels.push_back( SymbolData::Time_symbol );
						break;
					}
					case FocusRangeAxis::Z:{
						// x is cols, y is rows
						xy_labels.push_back( SymbolData::Space_symbol+".x" );
						if(range.dimensions()>1)
							xy_labels.push_back( SymbolData::Space_symbol+".y" );
						else
							xy_labels.push_back( SymbolData::Time_symbol );
						break;
					}
				}
			}
			else{
				xy_labels.push_back( SymbolData::Space_symbol+".x" );
				if(range.dimensions()>1)
					xy_labels.push_back( SymbolData::Space_symbol+".y" );
				else
					xy_labels.push_back( SymbolData::Time_symbol );
			}
			break;
		}
	}
	return xy_labels;
}
