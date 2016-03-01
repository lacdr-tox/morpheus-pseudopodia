#include "membraneLogger.h"
// #include <core/cell_membrane_accessor.h>

int MembraneLogger::instances=0;
REGISTER_PLUGIN(MembraneLogger);
	

MembraneLogger::MembraneLogger(): gnuplot(NULL) { 
	MembraneLogger::instances++; instance_id = MembraneLogger::instances; 
	
	
	
	
};
MembraneLogger::~MembraneLogger() { if (gnuplot) delete gnuplot;  MembraneLogger::instances--; };

void MembraneLogger::finish(double time)
{ 
	if( fout.is_open() ){
		fout << endl;
		fout.close();
	}
	if (gnuplot)
		delete gnuplot;
}

void MembraneLogger::log(double time){
	
	bool plotted=false;
	vector<CPM::CELL_ID> cells;
	
	if(specified_cells)
		cells = cellids;
	else 
		cells = celltype->getCellIDs();

	for(uint s=0; s<symbol_strings.size(); s++){
		CellMembraneAccessor membrane = celltype->findMembrane( symbol_strings[s] ) ;
		
		for(uint c=0; c<cells.size(); c++){
			// open file if filename is given
			string fn = getFileName(symbol_strings[s], "log", cells[c]);
			fout.open(fn.c_str(), ios::out);
			if( !fout.is_open() ){
				cerr << "MembraneLogger: cannot open file '" << fn << "'." << endl;
				exit(-1);
			}
			ostream& output = fout.is_open() ? fout : cout;
			writeMembrane(membrane, cells[c], output);
			fout.close();
			
		}
	}

	for(uint c=0; c<cells.size(); c++){
		plotData(symbol_strings, time, cells[c]);
	}

	if (plot && time >= last_plot_time + plot_interval) {
		plotted = true;
		for(uint c=0; c<cells.size(); c++){
			plotData(symbol_strings, time, cells[c]);
		}
		
		if(plot_logcommandSphere){
			vector<string> filenames;
			if(plot_superimpose){
				for(uint i=0; i < symbol_strings.size(); i++){
					filenames.push_back( getFileName(symbol_strings[i], "log", cells[0]) );
					cout << "filename: " << i << " = " << filenames[i] << endl;
				}
			}
			else{
				filenames.push_back( getFileName(symbol_strings[0], "log", cells[0]) );
			}
			writeCommand3D( filenames );
		}
	}
	
	if(plotted)
		last_plot_time = time;
}

void MembraneLogger::writeMembrane(CellMembraneAccessor membrane, CPM::CELL_ID id, ostream& output){
	for(uint theta=0; theta<MembraneProperty::size.y; theta++){
		for(uint phi=0; phi<MembraneProperty::size.x; phi++){
			double value = membrane.get(id, phi, theta);
			//double inf = 1.0/0.0;
			//double nan = 0.0/0.0;
			output <<  (value != 0.0 ? value : NAN) << " ";
		}
		output << "\n";
	}
}

string MembraneLogger::getFileName(string symbol, string extension, uint cellid){
	
	stringstream ts;
	if(time_name)
		ts << SIM::getTimeName();
	else
		ts << setfill('0') << setw(4) <<  int(rint( SIM::getTime() / getInterval()));
	
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
	
	string filename_output= getFileName(symstr, plot_extension, cellid);

	stringstream ss;
	if( terminal == "wxt" || terminal == "x11" || terminal == "aqua" ){
		ss << "set term "<< terminal << (persist?" persist ":" ") << ";\n";
	}
	else{
		ss << "set output \"" << filename_output << "\";\n";
		ss << "set term " << terminal << ";\n";
	}
	ss << "unset xtics;\n";
	ss << "unset ytics;\n";
	
	ss << "set view map;\n";
	ss << "set size ratio 0.5;\n";

	if(clean){
		ss << "unset colorbox;\n";
		ss << "unset title;\n"; 
	}
		
	if(plot_superimpose){
		ss << "set tmargin 0;\n";
		ss << "set bmargin 0;\n";
		ss << "set lmargin 0;\n";
		ss << "set rmargin 0;\n";
		ss << "set datafile missing \"nan\";\n";
		if(clean){
			ss << "unset key;\n";
		}
		else
			ss << "set key out;\n";
		ss << "splot ";
		for(uint s=0; s<symbols.size(); s++){ 
			string filename_input;
			if( mapping == SINGLE )
				filename_input = getFileName(symbols[s], "log");
			else
				filename_input = getFileName(symbols[s], "log", cellid);
			CellMembraneAccessor membrane = celltype->findMembrane( symbols[s] ) ;
			
			ss << " \"" << filename_input <<"\" matrix w p pt 5 ps " 
				<< plot_pointsize 
				<< " lc rgb \"" << colornames[s%colornames.size()] 
				<< "\" title \""<< membrane.getFullName() <<"\""; 
			if(s+1 < symbols.size())
				ss << ",\\\n";
			else
				ss << ";\n";
		}
		
	}
	else{
		ss << "set palette defined (0 \"white\", 0.25 \"yellow\", 1 \"red\");\n";
		if(!clean){
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
			if( mapping == SINGLE )
				filename_input = getFileName(symbols[s], "log", cellid);
			else
				filename_input = getFileName(symbols[s], "log", cellid);
			CellMembraneAccessor membrane = celltype->findMembrane( symbols[s] ) ;
			if(!clean)
				//ss << "set title\"" << membrane.getFullName() << "\" offset 0,-2;\n";
				ss << "set label 1 \"" << membrane.getFullName() << "\" at graph 0.5,0.9 center front;\n";
			ss << "plot \"" << filename_input <<"\" matrix with image not;\n";
		}
		ss << "unset multiplot;\n";
	}
	ss << "unset output;\n";
	
	gnuplot->cmd( ss.str() );

	if(plot_logcommand){
		ofstream foutcmd;
		foutcmd.open( "membraneLogger_command.plt", fstream::out);
		foutcmd << ss.str() << endl;
		foutcmd.close();
	}
	
}

void MembraneLogger::notify(double time)
{
	Analysis_Listener::notify(time);

	this->log(time);
	
}

set< string > MembraneLogger::getDependSymbols()
{
	set< string > symbols;
	for(uint p=0; p<symbol_strings.size(); p++){
		symbols.insert(symbol_strings[p]);
	}
	return symbols;
}


void MembraneLogger::loadFromXML(const XMLNode Node)
{
	Analysis_Listener::loadFromXML( Node );
	
	time_name = true;
	getXMLAttribute(Node, "timename", time_name);
	getXMLAttribute(Node, "celltype", celltype_str);
	
	mapping = ALL;
	
	string cellids_str;
	getXMLAttribute(Node, "cellids", cellids_str);
	specified_cells = false;
	if(!cellids_str.empty()){
		mapping = SINGLE;
		specified_cells = true;
		vector<string> cell_ids1;
		cell_ids1 = tokenize(cellids_str, ",");
		for(uint i=0; i < cell_ids1.size(); i++){
			//cout << cell_ids1[i] << "\n";
			vector<string> cell_ids2 = tokenize(cell_ids1[i], "-");
			if(cell_ids2.size() == 2){
				uint s = atoi(cell_ids2[0].c_str());
				uint e = atoi(cell_ids2[1].c_str());
				for(uint j = s; j <= e; j++){
					//cout << j << "\n";
					cellids.push_back( j );
				}
			}
			else
				cellids.push_back( atoi(cell_ids1[i].c_str()) );
		}
		cout << "MembraneLogger: CellIDs: ";
		for(uint i=0; i < cellids.size(); i++)
			cout << cellids[i] << ", ";
		cout << "\n";
	}
	
	string symbol_str;
	for(uint p=0; p<Node.nChildNode("MembraneProperty"); p++){
		XMLNode mpNode = Node.getChildNode("MembraneProperty", p);
		getXMLAttribute(mpNode, "symbol-ref", symbol_str);
		symbol_strings.push_back(symbol_str);
	}
	
		
	plot=false;
	if( Node.nChildNode("Plot") ){
		plot=true;
		persist=false;
		XMLNode plotNode = Node.getChildNode("Plot",0);
		
		getXMLAttribute(plotNode, "terminal", terminal);
		clean = false;
		getXMLAttribute(plotNode, "clean", clean);
		getXMLAttribute(plotNode, "persist", persist);
		plot_interval = 1;
		getXMLAttribute(plotNode, "interval", plot_interval);	
		plot_superimpose = false;
		getXMLAttribute(plotNode, "superimpose", plot_superimpose);
		plot_pointsize = 1.0;
		getXMLAttribute(plotNode, "pointsize", plot_pointsize);
		plot_logcommand = false;
		getXMLAttribute(plotNode, "commands", plot_logcommand);
		plot_logcommandSphere = false;
		getXMLAttribute(plotNode, "commandSphere", plot_logcommandSphere);
	}
	
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
		oss << "if(usecellshape==0) system(\"superimpose=" << (plot_superimpose?1:0) << "; awk -v nan=$superimpose -f ~/bin/morphConvertMatrixToXYZ.awk \".datafilename.\" > membraneData_XYZ.txt\");\\\n";
		oss << "else system(\"superimpose=" << (plot_superimpose?1:0) << "; awk -v nan=$superimpose -f ~/bin/morphConvertMatrixToXYRZ.awk \".cellshapefile.\" \".datafilename.\" > membraneData_XYRZ.txt\");\n\n";
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
	if(!plot_superimpose){
		//oss2 << "if(terminal eq \"wxt\") set palette gray; else set palette defined (0 \"white\", 0.5 \"yellow\", 1 \"red\")\n";
		oss2 << "if(terminal eq \"wxt\") set palette defined (0 \"black\", 0.5 \"red\", 1 \"yellow\"); else set palette defined (0 \"white\", 0.5 \"yellow\", 1 \"red\");\n";
	}
	else{
		oss2 << "set palette maxcolors " << logfilenames.size() << ";\n";
		oss2 << "set palette defined ( ";
		for(uint i=0; i<logfilenames.size();i++){
			oss2 << i << " \"" << colornames[i%colornames.size()] <<"\"";
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


void MembraneLogger::init(double time)
{
	Analysis_Listener::init(time);

	celltype = CPM::findCellType( celltype_str );

	for(uint p=0; p<symbol_strings.size(); p++){
		SymbolAccessor<double> sym = SIM::findSymbol<double>(symbol_strings[p]);
		if ( (sym.getLinkType() != SymbolData::CellMembraneLink) || (!celltype->findMembrane( symbol_strings[p] ).valid()) ){
			cerr << "MembraneLogger: Symbol '" <<  symbol_strings[p] << "' does not refer to a MembraneProperty." << endl;
			exit(-1);
		}
	}
	
	if( SIM::getLattice()->getDimensions() == 2 ){
		cerr << "Error: MembraneLogger is not implemented for MembraneProperty in 2D. Use Logger or SpaceTimeLogger instead." << endl;
		exit(-1);
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
	
	
	if(plot){
		
		try {
			gnuplot = new Gnuplot();
		}
		catch (GnuplotException e) {
                        cerr << "MembraneLogger error: " << e.what() << endl;
			exit(-1);
		}
		
		last_plot_time = time;
		
		plot_extension="";
		string terminal_temp;
		terminal_temp=terminal;
		if(terminal == "png"){
			terminal_temp="pngcairo truecolor enhanced font \"Arial,12\" size 1024,1024 crop";
			plot_extension = "png";
		}
		else if(terminal == "pdf"){
			terminal_temp="pdfcairo enhanced font \"Helvetica,10\" ";
			plot_extension = "pdf";
		}
		else if (terminal == "jpeg")
			plot_extension = "jpg";
		else if (terminal == "gif")
			plot_extension = "gif";
		else if (terminal == "svg")
			plot_extension = "svg";
		else if (terminal == "postscript"){
			terminal_temp="postscript eps color enhanced font \"Helvetica,22\" ";
			plot_extension = "eps";
		}
		//else extension = "log";
		
		terminal = terminal_temp;
		
		if(plot_superimpose){
			colornames.push_back("red");
			colornames.push_back("blue");
			colornames.push_back("green");
			colornames.push_back("purple");
			colornames.push_back("orange");
			colornames.push_back("dark-green");
			colornames.push_back("yellow");
			colornames.push_back("light-blue");
			colornames.push_back("pink");
		}

	}
}



