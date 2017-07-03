#include "membrane_logger.h"

int MembraneLogger::instances=0;
REGISTER_PLUGIN(MembraneLogger);
	

MembraneLogger::MembraneLogger(): gnuplot(NULL) { 
	MembraneLogger::instances++; instance_id = MembraneLogger::instances; 
	
	celltype.setXMLPath("celltype");
	registerPluginParameter(celltype);

	cell_ids_str.setXMLPath("cellids");
	registerPluginParameter(cell_ids_str);
	
};

MembraneLogger::~MembraneLogger() { 
	if (gnuplot) 
		delete gnuplot;  
	MembraneLogger::instances--; 
};

void MembraneLogger::loadFromXML(const XMLNode xNode)
{
	// Define PluginParameters for all defined Output tags
	symbols.resize( xNode.nChildNode("MembraneProperty") );
	for (uint i=0; i<xNode.nChildNode("MembraneProperty"); i++) {
		
		symbols[i]->setXMLPath("MembraneProperty["+to_str(i)+"]/symbol-ref");
		registerPluginParameter(symbols[i]);
	}
	
	if( xNode.nChildNode("Plot") > 0) {
		map<string, Terminal> terminalmap;
		terminalmap["png"] = PNG;
		terminalmap["pdf"] = PDF;
		terminalmap["jpg"] = JPG;
		terminalmap["gif"] = GIF;
		terminalmap["svg"] = SVG;
		terminalmap["eps"] = EPS;
		terminalmap_rev[PNG] = "pngcairo";
		terminalmap_rev[PDF] = "pdfcairo";
		terminalmap_rev[JPG] = "jpg";
		terminalmap_rev[GIF] = "gif";
		terminalmap_rev[SVG] = "svg";
		terminalmap_rev[EPS] = "eps";

		plot.terminal.setXMLPath("Plot/terminal");
		plot.terminal.setConversionMap(terminalmap);
		registerPluginParameter(plot.terminal);
		
		plot.logcommands.setXMLPath("Plot/log-commands");
		plot.logcommands.setDefault("false");
		registerPluginParameter(plot.logcommands);

		plot.clean.setXMLPath("Plot/clean");
		plot.clean.setDefault("false");
		registerPluginParameter(plot.clean);
		
		plot.superimpose.setXMLPath("Plot/superimpose");
		plot.superimpose.setDefault("false");
		registerPluginParameter(plot.superimpose);
		
		plot.pointsize.setXMLPath("Plot/pointsize");
		registerPluginParameter(plot.pointsize);

		plot.logcommandSphere.setXMLPath("Plot/log-commands-sphere");
		plot.logcommandSphere.setDefault("false");
		registerPluginParameter(plot.logcommandSphere);
	}
	
	AnalysisPlugin::loadFromXML( xNode );

	mapping = ALL;
	if(cell_ids_str.isDefined()){
		mapping = SELECTION;
		cellids = parseCellIDs();
		cout << "MembraneLogger: CellIDs: ";
		for(uint i=0; i < cellids.size(); i++)
			cout << cellids[i] << ", ";
		cout << "\n";
	}
	
	
	
}

void MembraneLogger::init(const Scope* scope)
{
	celltype.init();
	const Scope* ct_scope = celltype()->getScope();
	for(uint i=0; i<symbols.size(); i++){
		symbols[i]->setScope( ct_scope );
		
// 		if( symbols[i].granularity() != SymbolData::MembraneNodeGran ){
// 			cerr << "MembraneLogger: Symbol '" <<  symbols[i].getDescription() << "' does not refer to a MembraneProperty." << endl;
// 			exit(-1);
// 		}
	}
	
	AnalysisPlugin::init(scope);
	
	if( SIM::getLattice()->getDimensions() == 2 ){
		cerr << "Error: MembraneLogger is not implemented for MembraneProperty in 2D. Use Logger or SpaceTimeLogger instead." << endl;
		exit(-1);
	}
	
	// get names of symbols
	for(uint s=0; s<symbols.size(); s++){
		symbol_names.push_back( symbols[s]->description() );
	}

	
	if( !cellids.empty() ){
		vector<CPM::CELL_ID> markedforremoval;
		for(uint c=0; c<cellids.size(); c++){
			if( !CPM::cellExists( cellids[c] ) ){
				cout << "MembraneLogger: Removing cell ID " << cellids[c] << " from list, cell does not exist!" << endl;
				markedforremoval.push_back( cellids[c] );
			}
		}
		
		for(uint m=0; m<markedforremoval.size(); m++)
			cellids.erase(remove(cellids.begin(), cellids.end(), markedforremoval[m]), cellids.end());
		
		if( cellids.empty() ){
			cerr << "MembraneLogger: No cells to plot! Note: after removing the cell IDs that did not exist." << endl;
			exit(-1);
		}
		for(uint c=0; c<cellids.size(); c++){
			cout << cellids[c] << ", ";
		}
		cout << endl;
	}
	
	if( plot.terminal.isDefined() ){
		
		try {
			gnuplot = new Gnuplot();
		}
		catch (GnuplotException e) {
			cerr << "MembraneLogger error: " << e.what() << endl;
			exit(-1);
		}
		
		plot.extension="";
		string terminal_temp;
		
		if(plot.terminal() == Terminal::PNG){
			terminal_temp="pngcairo truecolor enhanced font \"Arial,12\" size 1024,1024 crop";
			plot.extension = "png";
		}
		else if(plot.terminal() == Terminal::PDF){
			terminal_temp="pdfcairo enhanced font \"Helvetica,10\" ";
			plot.extension = "pdf";
		}
		else if(plot.terminal() == Terminal::JPG)
			plot.extension = "jpg";
		else if(plot.terminal() == Terminal::GIF)
			plot.extension = "gif";
		else if(plot.terminal() == Terminal::SVG)
			plot.extension = "svg";
		else if(plot.terminal() == Terminal::EPS){
			terminal_temp="postscript eps color enhanced font \"Helvetica,22\" ";
			plot.extension = "eps";
		}
		plot.terminal_str = terminal_temp;
		if( plot.terminal_str.empty() )
			plot.terminal_str = plot.extension;
		
		
		if(plot.superimpose.isDefined() && plot.superimpose() == true){
			plot.colornames.push_back("red");
			plot.colornames.push_back("blue");
			plot.colornames.push_back("green");
			plot.colornames.push_back("purple");
			plot.colornames.push_back("orange");
			plot.colornames.push_back("dark-green");
			plot.colornames.push_back("yellow");
			plot.colornames.push_back("light-blue");
			plot.colornames.push_back("pink");
		}
	}
}


void MembraneLogger::log(double time){
	
	bool plotted=false;
	vector<CPM::CELL_ID> cells;
	
	if(cell_ids_str.isDefined())
		cells = cellids;
	else 
		cells = celltype()->getCellIDs();

	for(uint s=0; s<symbols.size(); s++){
		//CellMembraneAccessor membrane = celltype->findMembrane( symbol_strings[s] );
		
		for(uint c=0; c<cells.size(); c++){
			// open file if filename is given
			string fn = getFileName(symbols[s]->description(), "log", cells[c]);
			fout.open(fn.c_str(), ios::out);
			if( !fout.is_open() ){
				cerr << "MembraneLogger: cannot open file '" << fn << "'." << endl;
				exit(-1);
			}
			ostream& output = fout.is_open() ? fout : cout;
			writeMembrane(symbols[s], cells[c], output);
			fout.close();
			
		}
	}
	
	
// 	for(uint c=0; c<cells.size(); c++){
// 		plotData(symbol_names, time, cells[c]);
// 	}

	if( plot.terminal.isDefined() ){ //&& time >= last_plot_time + plot_interval) {
				
		// plot data for each cell
		for(uint c=0; c<cells.size(); c++){
			plotData(symbol_names, time, cells[c]);
		}
		
		// log commands to generate spherical projection
		if(plot.logcommandSphere()){
			vector<string> filenames;
			if(plot.superimpose()){
				for(uint i=0; i < symbol_names.size(); i++){
					filenames.push_back( getFileName(symbol_names[i], "log", cells[0]) );
					cout << "filename: " << i << " = " << filenames[i] << endl;
				}
			}
			else{
				filenames.push_back( getFileName(symbol_names[0], "log", cells[0]) );
			}
			writeCommand3D( filenames );
		}
	}
	
}

void MembraneLogger::writeMembrane(const PluginParameter_Shared<double, XMLReadableSymbol, RequiredPolicy> symbol, CPM::CELL_ID id, ostream& output){
	for(uint theta=0; theta<MembraneProperty::getSize().y; theta++){
		for(uint phi=0; phi<MembraneProperty::getSize().x; phi++){
			
			double value = symbol( SymbolFocus(id, phi, theta) );
			//double inf = 1.0/0.0;
			//double nan = 0.0/0.0;
			output <<  (value != 0.0 ? value : NAN) << " ";
		}
		output << "\n";
	}
}

string MembraneLogger::getFileName(string symbol, string extension, uint cellid){
	
	stringstream ts;
	//if(time_name)
	ts << SIM::getTimeName();
// 	else
// 		ts << setfill('0') << setw(4) <<  int(rint( SIM::getTime() / getInterval()));
	
	stringstream fn;
	fn << "MemLog_" << symbol;
	fn << "_" << cellid;
//	if (MembraneLogger::instances>1)
//		fn << "_" << instance_id;
	fn << "_" << ts.str();
	fn << "." << extension;
	return fn.str();
	
}

void MembraneLogger::plotData(vector<string> symbols, double time, uint cellid){

	// concatenate symbols into string
	stringstream ss1;
	copy(symbols.begin(), symbols.end(), std::ostream_iterator<std::string>(ss1, "_"));
	string symstr = ss1.str();
	if (!symstr.empty()) {
		symstr.resize(symstr.length() - 1); // trim trailing space
	}
	
	string filename_output= getFileName(symstr, plot.extension, cellid);

	stringstream ss;
	if( plot.terminal() == WXT || plot.terminal() == X11 || plot.terminal() == AQUA ){
		ss << "set term "<< terminalmap_rev[ plot.terminal() ] << /*(persist?" persist ":" ") <<*/ ";\n";
	}
	else{
		ss << "set output \"" << filename_output << "\";\n";
		ss << "set term " << terminalmap_rev[ plot.terminal() ] << ";\n";
	}
	ss << "unset xtics;\n";
	ss << "unset ytics;\n";
	
	ss << "set view map;\n";
	ss << "set size ratio 0.5;\n";

	if(plot.clean()){
		ss << "unset colorbox;\n";
		ss << "unset title;\n"; 
	}
		
	if(plot.superimpose()){
		ss << "set tmargin 0;\n";
		ss << "set bmargin 0;\n";
		ss << "set lmargin 0;\n";
		ss << "set rmargin 0;\n";
		ss << "set datafile missing \"nan\";\n";
		if(plot.clean()){
			ss << "unset key;\n";
		}
		else
			ss << "set key out;\n";
		ss << "splot ";
		for(uint s=0; s<symbols.size(); s++){ 
			string filename_input;
			if( mapping == SELECTION)
				filename_input = getFileName(symbols[s], "log");
			else // mapping == ALL
				filename_input = getFileName(symbols[s], "log", cellid);
			CellMembraneAccessor membrane = celltype()->findMembrane( symbols[s] ) ;
			
			ss << " \"" << filename_input <<"\" matrix w p pt 5 ps " 
				<< plot.pointsize()
				<< " lc rgb \"" << plot.colornames[s % plot.colornames.size()]
				<< "\" title \""<< membrane.getFullName() <<"\""; 
			if(s+1 < symbols.size())
				ss << ",\\\n";
			else
				ss << ";\n";
		}
		
	}
	else{
		ss << "set palette defined (0 \"white\", 0.25 \"yellow\", 1 \"red\");\n";
		if(!plot.clean()){
			ss << "set tmargin 2;\n";
			ss << "set bmargin 1;\n";
		}
		ss << "set tmargin 0;\n";
		ss << "set bmargin 0;\n";
		ss << "set lmargin 0;\n";
		ss << "set rmargin 0;\n";
		ss << "set multiplot layout " << symbols.size() << ",1;\n";
		for(uint s=0; s<symbols.size(); s++){ 
			string filename_input;
			if( mapping == SELECTION )
				filename_input = getFileName(symbols[s], "log", cellid);
			else // mapping == ALL
				filename_input = getFileName(symbols[s], "log", cellid);
			CellMembraneAccessor membrane = celltype()->findMembrane( symbols[s] ) ;
			if(!plot.clean())
				//ss << "set title\"" << membrane.getFullName() << "\" offset 0,-2;\n";
				ss << "set label 1 \"" << membrane.getFullName() << "\" at graph 0.5,0.9 center front;\n";
			ss << "plot \"" << filename_input <<"\" matrix with image not;\n";
		}
		ss << "unset multiplot;\n";
	}
	ss << "unset output;\n";
	
	gnuplot->cmd( ss.str() );

	if(plot.logcommands()){
		ofstream foutcmd;
		foutcmd.open( "membraneLogger_command.plt", fstream::out);
		foutcmd << ss.str() << endl;
		foutcmd.close();
	}
	
}

void MembraneLogger::analyse(double time)
{
	//AnalysisPlugin::analyse(time);
	this->log(time);
	
}

set< string > MembraneLogger::getDependSymbols()
{
	set< string > symbols;
	for(uint p=0; p<symbol_names.size(); p++){
		symbols.insert(symbol_names[p]);
	}
	return symbols;
}




void MembraneLogger::writeCommand3D(vector<string> logfilenames){
	
	// rotate.plt
	ostringstream oss;
	oss << "#!/usr/bin/gnuplot\n\n";
	oss << "# >>> user input <<<\n";
	oss << "# output terminal (wxt, png, gif)\n";
	oss << "terminal=\"gif\"\n";
	oss << "# number of frames\n";
	oss << "frames=180\n\n";
	oss << "datafilename = '" << logfilenames[0] << "'\n";
	oss << "usecellshape = 1; \n";
	oss << "cellshapefile = '" << logfilenames[0] << "' # file should contain radii from cell center\n";
	oss << "reset\n";
	
	oss << "set object 1 rectangle from screen 0,0 to screen 1,1 fillcolor rgb \"black\" behind;\n";
	oss << "print sprintf(\"%s\",terminal)\n";
	oss << "if(terminal eq \"wxt\") set term wxt size 400,400;\\\n";
	oss << "else if(terminal eq \"gif\") set term gif animate size 400,400; set output \"sphere.gif\";\\\n";
	oss << "else if(terminal eq \"png\") set term pngcairo transparent size 400,400 crop;\\\n";
	oss << "else print sprintf(\"Terminal not found: %s\", terminal); exit; \n";
	
	oss << "# convert data form matrix format into X,Y,Z format\n\n";
	
	for(uint i=0; i<logfilenames.size(); i++){
		oss << "if(usecellshape==0) system(\"superimpose=" << (plot.superimpose() ?1:0) << "; awk -v nan=$superimpose -f ~/bin/morphConvertMatrixToXYZ.awk \".datafilename.\" > membraneData_XYZ.txt\");\\\n";
		oss << "else system(\"superimpose=" << (plot.superimpose()?1:0) << "; awk -v nan=$superimpose -f ~/bin/morphConvertMatrixToXYRZ.awk \".cellshapefile.\" \".datafilename.\" > membraneData_XYRZ.txt\");\n\n";
	}
	
	//ostringstream awk;
	//awk << "BEGIN{ y=0; }{ v1=$(1); yc = (180/(NF/2))*(y)-90+0.5; if (NR==1) yc=-90; if (NR==NF/2) yc=90; for(x=1; x < NF; x++){ v=$(x);  if(v==\\\"nan\\\") v=0; print ((360/NF)*(x-1))-180, yc, v;  } print 180, yc, (v1==\\\"nan\\\"?\"0\\\":v1); print \"\"; y++; }";
	//oss << "system(\"awk -f \'" << awk.str() << "\' " << logfilename <<  " > membraneData_XYZ.txt\");\n";
	
	oss << "rotation=720.0/(frames+0.0);\n";
	oss << "shift=90.0/(frames+0.0);\n";
	oss << "i=0.0; j=50.0;\n";
	oss << "num=0;\n";
	oss << "set view j,i\n";
	
	oss << "load \"membraneLogger_command_iterate.plt\"\n";
	oss << "unset output\n";
	
	fstream file_rotate;
	string filename = "membraneLogger_command_rotate.plt";
	file_rotate.open(filename.c_str(), ios::out);
	if(file_rotate.is_open()){
		file_rotate << oss.str() << endl;
		file_rotate.close();
	}

	// iterate.plt
	ostringstream oss3;
	oss3 << "if(terminal eq \"png\") set output sprintf(\"sphere_%04d.png\",num);\n";
	oss3 << "set view j,i\n";
	oss3 << "i=i+rotation; j=j+shift; if(i >= 360 && shift>0) i=0; shift=-shift;\n";	
	oss3 << "load 'membraneLogger_command_sphere.plt'\n";
	oss3 << "num=num+1\n";
	oss3 << "print \"frame \", num, \" of \", frames, \" view: \", j,\", \", i;\n";
	oss3 << "if(num < frames) reread;\n";
	fstream file_iterate;
	filename = "membraneLogger_command_iterate.plt";
	file_iterate.open(filename.c_str(), ios::out);
	if(file_iterate.is_open()){
		file_iterate << oss3.str() << endl;
		file_iterate.close();
	}
	
	// sphere.plt
	ostringstream oss2;
	
	oss2 << "if(!exist(\"terminal\")) terminal=\"wxt\"; set terminal wxt size 400,400\n";
	oss2 << "set size 1,1\n";
	
	oss2 << "unset key\n";
	oss2 << "unset border\n";
	oss2 << "set tics scale 0\n";
	oss2 << "unset xtics\n";
	oss2 << "unset ytics\n";
	oss2 << "set lmargin screen 0\n";
	oss2 << "set bmargin screen 0\n";
	oss2 << "set rmargin screen 1\n";
	oss2 << "set tmargin screen 1\n";
	oss2 << "set format ''\n";
	
	oss2 << "set mapping spherical\n";
	oss2 << "set angles degrees\n";
	oss2 << "set pm3d depthorder\n";
	oss2 << "set pm3d corners2color c1 #prevent interpolation\n";
	oss2 << "# Set xy-plane to intersect z axis at -1 to avoid an offset between the lowest z\n";
	oss2 << "# value and the plane\n";
	oss2 << "set xyplane at -1\n";
	
	oss2 << "set parametric\n";
	oss2 << "set isosamples 25\n";
	oss2 << "set urange[0:360]\n";
	oss2 << "set vrange[-90:90]\n";
	oss2 << "set xrange[-1:1]\n";
	oss2 << "set yrange[-1:1]\n";
	oss2 << "set zrange[-1:1]\n";
	
	oss2 << "set datafile missing \"nan\"\n";
	if(!plot.superimpose()){
		//oss2 << "if(terminal eq \"wxt\") set palette gray; else set palette defined (0 \"white\", 0.5 \"yellow\", 1 \"red\")\n";
		oss2 << "if(terminal eq \"wxt\") set palette defined (0 \"black\", 0.5 \"red\", 1 \"yellow\"); else set palette defined (0 \"white\", 0.5 \"yellow\", 1 \"red\");\n";
	}
	else{
		oss2 << "set palette maxcolors " << logfilenames.size() << ";\n";
		oss2 << "set palette defined ( ";
		for(uint i=0; i<logfilenames.size();i++){
			oss2 << i << " \"" << plot.colornames[i%plot.colornames.size()] <<"\"";
			if(logfilenames.size()> (i+1))
				oss2 << ", ";
		}
		oss2 << ");\n";
	}
		
	oss2 << "# terminal gif is treated differently, because of artefacts in pm3d rendering\n";
	
	//oss2 << "splot './membraneData_XYZ.txt' us 1:2:(0.99):3 w pm3d;";
	oss2 << "splot './membraneData_XYRZ.txt' us 1:2:3:4 w pm3d;";

// 	oss2 << "splot " <<;
// 	for(uint i=0; i<logfilenames.size(); i++){
// 		oss2 << "'./membraneData_XYZ_" << i << ".txt' us 1:2:(0.99):3 w pm3d";
// 		if(logfilenames.size() > i+1)
// 			oss2 << ",";
// 		else
// 			oss2 << ";\n";
// 	}
	
	
			
	fstream file_sphere;
	filename = "membraneLogger_command_sphere.plt";
	file_sphere.open(filename.c_str(), ios::out);
	if(file_sphere.is_open()){
		file_sphere << oss2.str() << endl;
		file_sphere.close();
	}

}



vector<CPM::CELL_ID> MembraneLogger::parseCellIDs(){
	string s = cell_ids_str();
	vector<CPM::CELL_ID> ids;
	vector<string> tokens;
	tokens = tokenize(cell_ids_str(), ",");
	for(uint i=0; i < tokens.size(); i++){
		//cout << cell_ids[i] << "\n";
		vector<string> cell_ids2 = tokenize(tokens[i], "-");
		if(cell_ids2.size() == 2){
			uint s = atoi(cell_ids2[0].c_str());
			uint e = atoi(cell_ids2[1].c_str());
			for(uint j = s; j <= e; j++){
				//cout << j << "\n";
				ids.push_back( j );
			}
		}
		else
			ids.push_back( atoi(tokens[i].c_str()) );
	}
	return ids;
}

void MembraneLogger::finish(double time)
{ 
	if( fout.is_open() ){
		fout << endl;
		fout.close();
	}
	if (gnuplot)
		delete gnuplot;
}
