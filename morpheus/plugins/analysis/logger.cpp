#include "logger.h"

int Logger::instances=0;
REGISTER_PLUGIN(Logger);
	

Logger::Logger(): gnuplot(NULL) { Logger::instances++; instance_id = Logger::instances; };
Logger::~Logger() {
	if (gnuplot) {
		delete gnuplot;
	}
	Logger::instances--;
};


void Logger::log(){
	ostream& output = fout.is_open() ? fout : cout;
	
	if(log_global){
		uint d=0,v=0;
		output << SIM::getTime() << "\t";
		for(uint s=0; s<SATypes.size(); s++){
			switch( SATypes[s] ){
				case( DOUBLE ):
					output << sym_d[d].get(SymbolFocus()) << "\t";
					d++;
					break;
				case( VECTOR ):
					output << sym_v[v].get(SymbolFocus()) << "\t";
					v++;
					break;
				default:
					cerr << "Logger: unknown type" << endl; exit(-1); 
					break;
			}
		}
	}
	else{
		
		switch(mapping){
			
			case(ALL):
			{
				vector<CPM::CELL_ID> cells = celltype.lock()->getCellIDs();
				for(uint c=0; c<cells.size(); c++){
					// get values for symbols
					output << SIM::getTime() << "\t" << cells[c] << "\t";
					
					uint d=0,i=0,b=0,v=0;
					for(uint s=0; s<SATypes.size(); s++){
						
						switch( SATypes[s] ){
							case( DOUBLE ):
								output << sym_d[d].get(cells[c]) << "\t";
								d++;
								break;
							case( VECTOR ):
								output << sym_v[v].get(cells[c]) << "\t";
								v++;
								break;
							default:
								cerr << "Logger: unknown type" << endl; exit(-1); 
								break;
						}
					}
					output << endl;
				}
				break;
			}
			case(AVERAGE):
			{
				vector<CPM::CELL_ID> cells = celltype.lock()->getCellIDs();
				// first, pre-allocate vectors for the average,difference, and standard deviation
				vector<double> d_ave; // double
				vector<double> d_diff;
				vector<double> d_stdev;
				vector<VDOUBLE> v_ave;	// vector
				vector<VDOUBLE> v_diff;

				if(numsym_d){				
					for(uint s=0; s<numsym_d; s++){
						d_ave.push_back(0.0);
						d_diff.push_back(0.0);
					}
				}
				if(numsym_v){				
					for(uint s=0; s<numsym_v; s++){
						v_ave.push_back(VDOUBLE(0,0,0));
						v_diff.push_back(VDOUBLE(0,0,0));
					}
				}
				
				// FIRST PASS: accumulate values from cells
				int numcells = cells.size();
				for(uint c=0; c<numcells; c++){
					
					// get values for symbols
					uint d=0,i=0,b=0,v=0;
					for(uint s=0; s<SATypes.size(); s++){
						
						switch( SATypes[s] ){
							case( DOUBLE ):
								d_ave[d] += sym_d[d].get(cells[c]);
								d++;
								break;
							case( VECTOR ):
								v_ave[v] += sym_v[v].get(cells[c]);
								v++;
								break;
							default:
								cerr << "Logger: unknown type" << endl; exit(-1); 
								break;
						}
					}
				}
				// get averages by dividing by the number of cells
				for(uint s=0; s<numsym_d; s++){
					d_ave[s] /= numcells;
				}
				for(uint s=0; s<numsym_v; s++){
					v_ave[s].x /= numcells;
					v_ave[s].y /= numcells;
					v_ave[s].z /= numcells;
				}
				
				// SECOND PASS: calculate variance
				for(uint c=0; c<numcells; c++){
					uint d=0,i=0,b=0,v=0;
					for(uint s=0; s<SATypes.size(); s++){
						
						switch( SATypes[s] ){
							case( DOUBLE ):
								d_diff[d] += sqr(sym_d[d].get(cells[c]) - d_ave[d]);
								d++;
								break;
							case( VECTOR ):{
								VDOUBLE vec = VDOUBLE(0.,0.,0.);
								vec.x = sqr(sym_v[v].get(cells[c]).x - v_ave[v].x);
								vec.y = sqr(sym_v[v].get(cells[c]).y - v_ave[v].y);
								vec.z = sqr(sym_v[v].get(cells[c]).z - v_ave[v].z);
								v_diff[v] += vec;
								v++;
								break;
							}
							default:
								cerr << "Logger: unknown type" << endl; exit(-1); 
								break;
						}
					}
				}
				
				// FINALLY: calculate standard deviation and print to file
				uint d=0,i=0,b=0,v=0;
				output << SIM::getTime() << " ";
				for(uint s=0; s<SATypes.size(); s++){
					double stdev = 0.0;
					switch( SATypes[s] ){
						case( DOUBLE ):
							stdev = sqrt( d_diff[d] / numcells ); 
							output << d_ave[d] << " " << stdev << "\t";
							d++;
							break;
						case( VECTOR ):{
							VDOUBLE v_stdev;
							v_stdev.x = sqrt( v_diff[v].x / numcells );
							v_stdev.y = sqrt( v_diff[v].y / numcells );
							v_stdev.z = sqrt( v_diff[v].z / numcells ); 
							output << v_ave[v] << " " << v_stdev << "\t";
							v++;
							break;
						}
						default:
							cerr << "Logger: unknown type" << endl; exit(-1); 
							break;
					}
				}
				output << endl;
				break;
			}
			case(SUM):
			{
				vector<CPM::CELL_ID> cells = celltype.lock()->getCellIDs();
				// first, pre-allocate vectors for the average,difference, and standard deviation
				vector<double> d_sum; // double
				vector<VDOUBLE> v_sum;	// vector

				if(numsym_d){				
					for(uint s=0; s<numsym_d; s++){
						d_sum.push_back(0.0);
					}
				}
				if(numsym_v){				
					for(uint s=0; s<numsym_v; s++){
						v_sum.push_back(VDOUBLE(0,0,0));
					}
				}
				
				// FIRST PASS: accumulate values from cells
				int numcells = cells.size();
				for(uint c=0; c<numcells; c++){
					
					// get values for symbols
					uint d=0,i=0,b=0,v=0;
					for(uint s=0; s<SATypes.size(); s++){
						
						switch( SATypes[s] ){
							case( DOUBLE ):
								d_sum[d] += sym_d[d].get(cells[c]);
								d++;
								break;
							case( VECTOR ):
								v_sum[v] += sym_v[v].get(cells[c]);
								v++;
								break;
							default:
								cerr << "Logger: unknown type" << endl; exit(-1); 
								break;
						}
					}
				}
				
				// FINALLY: print to file
				uint d=0,i=0,b=0,v=0;
				output << SIM::getTime() << " ";
				for(uint s=0; s<SATypes.size(); s++){
	// 				double stdev = 0.0;
					switch( SATypes[s] ){
						case( DOUBLE ):
							output << d_sum[d] << "\t";
							d++;
							break;
						case( VECTOR ):{
							output << v_sum[v] << "\t";
							v++;
							break;
						}
						default:
							cerr << "Logger: unknown type" << endl; exit(-1); 
							break;
					}
				}
				break;
			}
			case(MINMAX):
			{
				vector<CPM::CELL_ID> cells = celltype.lock()->getCellIDs();
				// first, pre-allocate vectors for the average,difference, and standard deviation
				vector<double> d_min, d_max; // double

				if(numsym_d){
					for(uint s=0; s<numsym_d; s++){
						d_min.push_back(std::numeric_limits<double>::max());
						d_max.push_back(std::numeric_limits<double>::min());
					}
				}
				
				// FIRST PASS: accumulate values from cells
				int numcells = cells.size();
				for(uint c=0; c<numcells; c++){
					
					// get values for symbols
					uint d=0,i=0,b=0,v=0;
					for(uint s=0; s<SATypes.size(); s++){
						
						switch( SATypes[s] ){
							case( DOUBLE ):
								if( sym_d[d].get(cells[c]) < d_min[d] )
									d_min[d] = sym_d[d].get(cells[c]);
								else if( sym_d[d].get(cells[c]) > d_max[d] )
									d_max[d] = sym_d[d].get(cells[c]);
								d++;
								break;
							default:
								cerr << "Logger: unknown type" << endl; exit(-1); 
								break;
						}
					}
				}
				
				// FINALLY: print to file
				uint d=0;
				output << SIM::getTime() << " ";
				for(uint s=0; s<SATypes.size(); s++){
	// 				double stdev = 0.0;
					switch( SATypes[s] ){
						case( DOUBLE ):
							output << d_min[d] << "\t" << d_max[d] << "\t";
							d++;
							break;
						default:
							cerr << "Logger: unknown type" << endl; exit(-1); 
							break;
					}
				}
				break;
			}
			case(SINGLE):
			{
				vector<CPM::CELL_ID> cells = celltype.lock()->getCellIDs();
				if( CPM::getCellIndex(cell_id).status == CPM::NO_CELL){
					cerr << "Logger:: cell (ID = " << cell_id << ") does not exist..." << endl;
					return;
				}
	// 			const Cell& cell = CPM::getCell( cell_id );
				
				output << SIM::getTime() << " ";
				
				// get values for symbols
				uint d=0,v=0;
				for(uint s=0; s<SATypes.size(); s++){
					
					switch( SATypes[s] ){
						case( DOUBLE ):
							output << sym_d[d].get(cell_id) << "\t";
							d++;
							break;
						case( VECTOR ):
							output << sym_v[v].get(cell_id) << "\t";
							v++;
							break;
						default:
							cerr << "Logger: unknown type" << endl; exit(-1); 
							break;
					}
				}
				//output << endl;
				break;
			}
			case(PDE_SUM):
			{
				output << SIM::getTime() << "\t";
				for(int  l=0; l<pdelayers.size(); l++)
					output << pdelayers[l]->sum() << "\t";
				output << "\n";
				break;
			}
			case(PDE_AVE):
			{
				output << SIM::getTime() << "\t";
				for(int  l=0; l<pdelayers.size(); l++)
					output << pdelayers[l]->mean() << "\t";
				output << "\n";
				break;
			}
			case(PDE_VAR):
			{
				output << SIM::getTime() << "\t";
				for(int  l=0; l<pdelayers.size(); l++)
					output << pdelayers[l]->variance() << "\t";
				output << "\n";
				break;
			}
			case(PDE_MINMAX):
			{
				output << SIM::getTime() << "\t";
				for(int  l=0; l<pdelayers.size(); l++)
					output << pdelayers[l]->min_val() << "\t" << pdelayers[l]->max_val() << "\t";
				output << "\n";
				break;
			}
			case(PDE_ALL):
			{
				VINT pos;
				VINT latticeDim = pdelayers[0]->size();

				int start=0;
				int end=latticeDim.y;
				if(pde_slice>0){
					start=pde_slice;
					end=pde_slice+1;
				}
				for (pos.y=start; pos.y<end; pos.y++){
					for (pos.x=0; pos.x<latticeDim.x; pos.x++){
						output << SIM::getTime() << "\t" << pos << "\t";
						for(int  l=0; l<pdelayers.size(); l++)
							output << pdelayers[l]->get( pos ) << "\t";
						output << "\n";
					}
				}
				break;
			}
			case(MEMBRANE_SINGLE):
			{
				if( CPM::getCellIndex(cell_id).status == CPM::NO_CELL ){
					cout << "Warning (Logger): Cell " << cell_id << " does not exist (anymore)" << endl;
					return;
				}
				
				if( SIM::getLattice()->getDimensions() == 2 ){
					for(uint a=0; a<MembraneProperty::size.x; a++){
						output << SIM::getTime() << "\t" << a;
						for(uint s=0; s<sym_d.size(); s++){
							if(sym_d[s].getLinkType() == SymbolData::CellMembraneLink){
								CellMembraneAccessor membrane = celltype.lock()->findMembrane( sym_d[s].getName() ) ;
								output << "\t" << membrane.get(cell_id, a);
							}
						}
						output << "\n";
					}
					output << "\n";
				}
				else if( SIM::getLattice()->getDimensions() == 3 ){
					cerr << "Error: Logger not implemented for MembraneProperty in 3D. Use SpaceTimeLogger instead." << endl;
					exit(-1);
				}

				break;
			}
			case(MEMBRANE_ALL):
			{
				vector<CPM::CELL_ID> cells = celltype.lock()->getCellIDs();
				for(uint c=0; c<cells.size(); c++){
					
					if( SIM::getLattice()->getDimensions() == 2 ){
						for(uint a=0; a<MembraneProperty::size.x; a++){
							output << SIM::getTime() << "\t" << c << "\t" << a;
							for(uint s=0; s<sym_d.size(); s++){
								if(sym_d[s].getLinkType() == SymbolData::CellMembraneLink){
									CellMembraneAccessor membrane = celltype.lock()->findMembrane( sym_d[s].getName() ) ;
									output << "\t" << membrane.get(cells[c], a);
								}
							}
							output << "\n";
						}
						output << "\n";
					}
					else if( SIM::getLattice()->getDimensions() == 3 ){
						cerr << "Error: Logger not implemented for MembraneProperty in 3D. Use SpaceTimeLogger instead." << endl;
						exit(-1);
					}
				}
				break;
			}
			case(MEMBRANE_AVERAGE):
			{
				vector<CPM::CELL_ID> cells = celltype.lock()->getCellIDs();
				for(uint c=0; c<cells.size(); c++){
					
					output << SIM::getTime() << "\t" << c;
					if( SIM::getLattice()->getDimensions() == 2 ){
						vector<double> aves; aves.resize(sym_d.size());
						for(uint a=0; a<MembraneProperty::size.x; a++){
							for(uint s=0; s<sym_d.size(); s++){
								if(sym_d[s].getLinkType() == SymbolData::CellMembraneLink){
									CellMembraneAccessor membrane = celltype.lock()->findMembrane( sym_d[s].getName() ) ;
									aves[s] += membrane.get(cells[c], a);
								}
							}
						}
						for(uint s=0; s<sym_d.size(); s++){
							output << "\t" << (aves[s] / double(MembraneProperty::size.x));
						}
						output << "\n";
					}
					else if( SIM::getLattice()->getDimensions() == 3 ){
						cerr << "Error: Logger not implemented for MembraneProperty in 3D. Use SpaceTimeLogger instead." << endl;
						exit(-1);
					}
				}
				break;
			}
			default:
			{
				cerr << "Logger::log: no mapping defined!" << endl;			
				exit(-1);
				break;
			}
		}
	}
	output << endl;
}

void Logger::plotData(){	
		
	stringstream ss;

	bool use_cb=false;
	if ( atoi(col_cb.c_str()) > - 1)
		use_cb = true;

	stringstream fn;
	if(!extension.empty()){
		fn << "logger_";
		if (Logger::instances>1)
			fn << "_" << instance_id;
		for(uint i=0; i<sym_d.size(); i++){
			fn << "_" << sym_d[i].getName() ;
		}
		if (time_name)
			fn << SIM::getTimeName() << "." << extension;
		else {
			fn << setfill('0') << setw(4) <<  int(rint( SIM::getTime() / plot_interval ))  << "." << extension;
		}
	}
	filename_img = fn.str().c_str();
	
	ss << "set output '" << filename_img << "';\n";
	
	//ss << "set key out;\n" ;
	ss << "set size 1.0,1.0;\n" ;

	if(clean){
		ss << "unset xlabel;\n";
		ss << "unset ylabel;\n";
		ss << "unset xtics;\n";
		ss << "unset ytics;\n";
		ss << "unset cblabel;\n";
		ss << "unset key;\n";
	}
	else{
		string escape_char = "_^";
		string xlabel = headers[ atoi(col_x.c_str()) -1 ];
		size_t pos = xlabel.find_first_of(escape_char);
		while (pos != string::npos) {
			xlabel[pos] = ' ';
			pos = xlabel.find_first_of(escape_char,pos);
		}
		vector<string> ylabels;
		for(int i=0; i<column_tokens.size(); i++){
			string ylabel =  headers[ atoi(column_tokens[i].c_str()) -1 ];
			pos = ylabel.find_first_of(escape_char);
			while (pos != string::npos) {
				ylabel[pos] = ' ';
				pos = ylabel.find_first_of(escape_char,pos);
			}
			ylabels.push_back(ylabel);
		}
		string cblabel;
		if(use_cb){
			cblabel = headers[ atoi(col_cb.c_str()) -1 ];
			pos = cblabel.find_first_of(escape_char);
			while (pos != string::npos) {
				cblabel[pos] = ' ';
				pos = cblabel.find_first_of(escape_char,pos);
			}
		}
		
		ss << "set xlabel "<< xlabel <<"; \n" ;
		ss << "set ylabel " ;
		for(int i=0; i<column_tokens.size(); i++){
			ss << ylabels[i];
			if(column_tokens.size() > i+1)
					ss << ", ";
		}
		ss << ";\n";
		if(use_cb)
			ss << "set cblabel \""<<  cblabel << "\"; \n" ;
		else
			ss << "unset colorbox;\n";
	}

	if(logscale_x)
		ss << "set log x;\n";
	if(logscale_y)
		ss << "set log y;\n";
	if(logscale_cb)
		ss << "set log cb;\n";
	
	
	ss << "plot ["<< (min_val_x.size()>0?min_val_x:"*") <<":"<< (max_val_x.size()>0?max_val_x:"*") <<"]["<< (min_val_y.size()>0?min_val_y:"*") <<":"<< (max_val_y.size()>0?max_val_y:"*") <<"] './" << filename_log << "' us ($1>" << SIM::getTime() << "?NaN:$"<<col_x<<"):"<<column_tokens[0]<<":"<< ( use_cb ? col_cb:"(0)")<< " every " << (every.size()?every:"1") <<" w p pt 7 ps 0.5 "<< (use_cb?"pal":"") << " ti col";
	for(uint i=1; i< column_tokens.size(); i++){
		ss << ", '' us ($1>" << SIM::getTime() << "?NaN:$"<<col_x<<"):"<<column_tokens[i]<<":"<<( use_cb ?col_cb:"(0)")<<" every " << (every.size()?every:"1")  <<" w p pt 7 ps 0.5 "<< (use_cb?"pal":"") << " ti col";
	}
	//cout << ss.str() << endl << endl; using "<<col_x<<":
	//gnuplot->reset_plot();
	
	
	if( write_commands ){
		ofstream command_log;
// 		if (time_name)
			command_log.open((string("logger_commands")+ SIM::getTimeName() + ".log").c_str(),ios_base::app);
// 		else 
// 			command_log.open((string("logger_commands")+ to_str(time/getInterval(),4) + ".log").c_str(),ios_base::app);
		command_log << ss.str();
		command_log.close();
	}
	
	gnuplot->cmd( ss.str() );
	//cout << "Time: " << mcs << ", Logger/plot\n";

}

void Logger::analyse(double time)
{
	//cout << "Logger::analyse(" << time << ")" << endl;
	//AnalysisPlugin::analyse(time);
	this->log();

	if (plot && last_plot_time + plot_interval  < time  + 10e-12 && !plot_endstate){
		this->plotData();
		last_plot_time = time;
	}
	if( plot && plot_endstate &&  time + 10e-12 >= SIM::getStopTime()) {
		//cout << "Logger: plotting data at endstate (t = " << time << ")" << endl;
		plotData();
	}
}

void Logger::finish(double time)
{
	if( fout.is_open() ){
		fout << endl;
		fout.close();
	}
	
	
	if (gnuplot) {
		delete gnuplot;
		gnuplot = NULL;
	}
}


// XMLNode Logger::saveFromXML(){
// 	return stored_node;
// }

void Logger::loadFromXML(const XMLNode Node)
{
	AnalysisPlugin::loadFromXML( Node );
		
	//===		
	log_global = false;
	log_cell= false;
	log_pde = false;
	XMLNode inNode = getXMLNode(Node, "Input");
	if( inNode.nChildNode("Global") ){
		log_global=true;
	}
	if( inNode.nChildNode("Cell") ){
		log_cell=true;
		getXMLAttribute(inNode,"Cell/mapping", mapping_str);
		if(lower_case(mapping_str) == "all"){ 
			mapping = ALL;
// 			cout << "Logger::loadFromXML: Logging  all cells " << endl;
		}
		else if(lower_case(mapping_str) == "average"){
			mapping = AVERAGE;
// 			cout << "Logger::loadFromXML: Logging average of cells " << endl;
		}
		else if(lower_case(mapping_str) == "sum"){
			mapping = SUM;
// 			cout << "Logger::loadFromXML: Logging sum of cells " << endl;
		}
		else if(lower_case(mapping_str) == "min_max"){
			mapping = MINMAX;
// 			cout << "Logger::loadFromXML: Logging minimum and maximum " << endl;
		}
		else if(lower_case(mapping_str) == "single"){
			mapping = SINGLE;
// 			cout << "Logger::loadFromXML: Logging  single cell " << endl;
		}
		if(mapping == SINGLE){
			getXMLAttribute(Node,"Input/Cell/cell_id", cell_id);
		}
		if(mapping == SINGLE || mapping == AVERAGE ||  mapping == SUM || mapping == ALL || mapping == MINMAX){
			getXMLAttribute(Node,"Input/Cell/celltype", celltype_str);
		}
	}
	else if( inNode.nChildNode("PDE")){
		log_pde = true;
		getXMLAttribute(inNode,"PDE/mapping", mapping_str);
		if(lower_case(mapping_str) == "all"){ 
			mapping = PDE_ALL;
// 			cout << "Logger::loadFromXML: Logging all pde " << endl;
		}
		else if(lower_case(mapping_str) == "sum"){
			mapping = PDE_SUM;
// 			cout << "Logger::loadFromXML: Logging sum of pde " << endl;
		}
		else if(lower_case(mapping_str) == "min_max"){
			mapping = PDE_MINMAX;
// 			cout << "Logger::loadFromXML: Logging minimum and maximum of pde " << endl;
		}
		pde_slice = -1;
		getXMLAttribute(Node,"Input/PDE/slice", pde_slice);
		//getXMLAttribute(Node, "PDE/symbol-ref",pdesymbol_str);
	}
	
	if(log_pde) log_cell = false;
	else log_pde = false;
	
	//===
	if( Node.nChildNode("Format") ){
		getXMLAttribute(Node,"Format/string",data_format);
	}
	log_header = true;
	if( Node.nChildNode("Format") ){
		getXMLAttribute(Node,"Format/header",log_header);
	}
	
	
	if( !data_format.size() ){
		cerr << "Logger::loadFromXML: No <Format> tag provided, logger does not make sense..." << endl;
		exit(-1);
	}
	else
// 		cout << "Logger::loadFromXML: Logging  string  '"<< data_format << "' ." << endl;
	
// 	<Logger>
// 		<Plot terminal="" every="">
// 			<X-axis min="" max="" column="1" />
// 			<Y-axis min="" max="" columns="2 3 4"/>
// 		</Plot>
// 	</Logger>
	
	plot=false;
	if( Node.nChildNode("Plot") ){
		plot=true;
		persist=false;
		clean=false;
		XMLNode plotNode = Node.getChildNode("Plot",0);
		
		time_name = true;
		getXMLAttribute(plotNode,"timename",time_name);

		plot_endstate = false;
		getXMLAttribute(plotNode,"endstate",plot_endstate);
	
		getXMLAttribute(plotNode, "terminal", terminal);
		getXMLAttribute(plotNode, "persist", persist);
		getXMLAttribute(plotNode, "clean", clean);
		getXMLAttribute(plotNode, "every", every);
		write_commands = false;
		getXMLAttribute(plotNode, "log-commands", write_commands);
		plot_interval = 1;
		getXMLAttribute(plotNode, "interval", plot_interval);
		
		logscale_x=false; 
		getXMLAttribute(plotNode, "X-axis/min", min_val_x);
		getXMLAttribute(plotNode, "X-axis/max", max_val_x);
		getXMLAttribute(plotNode, "X-axis/column", col_x);
		getXMLAttribute(plotNode, "X-axis/logscale", logscale_x);
		
		logscale_y=false; 
		getXMLAttribute(plotNode, "Y-axis/min", min_val_y);
		getXMLAttribute(plotNode, "Y-axis/max", max_val_y);
		getXMLAttribute(plotNode, "Y-axis/columns", cols_y);
		getXMLAttribute(plotNode, "Y-axis/logscale", logscale_y);
		
		col_cb = "-1";
		logscale_cb=false; 
		getXMLAttribute(plotNode, "color-bar/min", min_val_cb);
		getXMLAttribute(plotNode, "color-bar/max", max_val_cb);
		getXMLAttribute(plotNode, "color-bar/column", col_cb);
		getXMLAttribute(plotNode, "color-bar/logscale", logscale_cb);
		
		if( atoi(col_x.c_str()) < 1){ cerr << "Logger: Column number for X-axis does not exist (must be larger than 0)" << endl; exit(-1); }
	}
	
	
}

set< string > Logger::getDependSymbols()
{
	set< string > symbols;
	for (uint i=0; i<symbol_strings.size(); i++) {
		symbols.insert(symbol_strings[i]);
	}
	return symbols;
}



void Logger::init(const Scope* scope)
{
	AnalysisPlugin::init(scope);
	
	const Scope* logging_scope = SIM::getGlobalScope();

	if(log_cell){
		// get cell type (must be given)
		celltype = CPM::findCellType(celltype_str);
		if (!celltype.lock()){
			cerr << "Logger::init: Celltype '" << celltype_str << "' does not exist! " << endl;
			exit(-1);
		}
		logging_scope =  celltype.lock()->getScope();
	}
	// get symbol accessors to symbols in data_format
	symbol_strings = tokenize( data_format, " " );
	
	set<SymbolDependency> sd;
	
	// check if there are symbols referring to MembraneProperty and change mapping accordingly
	for(uint i=0; i < symbol_strings.size(); i++){

		SymbolAccessor<double> sym =logging_scope->findSymbol<double>(symbol_strings[i]);
		SymbolDependency s = { sym.getName(), sym.getScope() };
		sd.insert(s);

		if ( sym.getLinkType() == SymbolData::CellMembraneLink ){
			if( celltype.lock()->findMembrane( symbol_strings[i] ).valid() ){
				cout << "Logger: Symbol '" <<  symbol_strings[i] << "' refers to MembraneProperty." << endl;
// 				if( mapping != ALL && mapping != SINGLE ){
// 					cerr << "Logger: Mapping " << mapping << " is not available for MembraneProperty." << endl;
// 					exit(-1);
// 				}
				if (mapping == ALL){
					mapping = MEMBRANE_ALL;
					cout << "Logger: mapping = MEMBRANE_ALL." << endl;
				}
				if (mapping == SINGLE){
					mapping = MEMBRANE_SINGLE;
					cout << "Logger: mapping = MEMBRANE_SINGLE "<< endl;
					break;
				}
				if (mapping == AVERAGE){
					mapping = MEMBRANE_AVERAGE;
					cout << "Logger: mapping = MEMBRANE_AVERAGE "<< endl;
					break;
				}
			}
		}
		//}
	}
	registerInputSymbols( sd );
	
	headers.push_back("\"Time\"");
	if( (mapping == ALL && ! log_global) || mapping == MEMBRANE_ALL){
	  headers.push_back("\"cellID\"");
	}
	if( mapping == MEMBRANE_SINGLE || mapping == MEMBRANE_ALL ){
		headers.push_back("\"x\"");
		cout << "Logger: header size = " << headers.size() << endl;
	}
	if( mapping == PDE_ALL ){
	  headers.push_back("\"x\"");
	  headers.push_back("\"y\"");
	  headers.push_back("\"z\"");
	}
	uint standard_columns = headers.size();
	
	for(uint i=0; i < symbol_strings.size(); i++){
		//double
		if( log_pde ){
			shared_ptr< PDE_Layer > pde = SIM::findPDELayer(symbol_strings[i]);
			if( mapping == PDE_SUM ) 
				headers.push_back("\"sum ("+ pde->getSymbol() +")\"" );
			else if( mapping == PDE_AVE ) 
				headers.push_back("\"ave ("+ pde->getSymbol() +")\"" );
			else if( mapping == PDE_VAR ) 
				headers.push_back("\"var ("+ pde->getSymbol() +")\"" );
			else if( mapping == PDE_MINMAX ){
				headers.push_back("\"min("+pde->getSymbol()+")\"\t");
				headers.push_back("\"max("+pde->getSymbol()+")\"\t");
			}
			else
				headers.push_back( string("\"")+pde->getSymbol()+ "\"");
			pdelayers.push_back( pde );
			continue;
		}
		
		if( logging_scope->getSymbolType(symbol_strings[i]) == TypeInfo<double>::name()){
				SATypes.push_back( DOUBLE );
				sym_d.push_back(  logging_scope->findSymbol<double>(symbol_strings[i]) );
				
				string fullname = logging_scope->findSymbol<double>(symbol_strings[i]).getFullName();
				headers.push_back( string("\"") + fullname + "\"" ); 
				
				if( mapping == AVERAGE ) 
					headers.push_back("\"sd\"");
				else if (mapping == MINMAX )
					headers.push_back("\"max\"");
		}
		// integer
// 		else if( SIM::getSymbolType(symbol_strings[i]) == Property<int>().getTypeName() ){
// 				SATypes.push_back( INTEGER );
// 				sym_i.push_back(  SIM::findSymbol<int>(symbol_strings[i]) );
// 				if( mapping == AVERAGE ) 
// 					output << sym_i.back().getFullName() << " sd" << "\t";
// 				else
// 					output << sym_i.back().getFullName() << " ";
// 		}
		//boolean
		/*else if( SIM::getSymbolType(symbol_strings[i]) == Property<bool>().getTypeName() ){
				SATypes.push_back( BOOLEAN );
				sym_b.push_back(  SIM::findSymbol<bool>(symbol_strings[i]) );
				if( mapping == AVERAGE ) 
					output << sym_b.back().getFullName() << " sd" << "\t";
				else
					output << sym_b.back().getFullName() << " ";
					
		}*/
		//vector
		else if( logging_scope->getSymbolType(symbol_strings[i]) == TypeInfo<VDOUBLE>::name() ){
				SATypes.push_back( VECTOR );
				sym_v.push_back(  logging_scope->findSymbol<VDOUBLE>(symbol_strings[i]) );
				string fullname = sym_v.back().getFullName();
				headers.push_back(  string("\"") + fullname + "\""  );
				if( mapping == AVERAGE ) 
					headers.push_back("\"sd\"");
				else if (mapping == MINMAX )
					headers.push_back("\"max\"");
				
				lower_case(plugin_name);
		}
		else{
				cerr << "Logger: Cannot use logger for " << logging_scope->getSymbolType(symbol_strings[i]) << endl;
				exit(-1);
		}
// 		cout << symbol_strings[i] << " ";
	}

	stringstream fn;
	fn << "logger";
	if (Logger::instances>1)
		fn << "_" << instance_id;
	for(uint i=0; i<sym_d.size(); i++){
		fn << "_" << sym_d[i].getName();
	}
	fn << "_" << mapping_str;

	filename_noextension = fn.str().c_str();
	fn << ".log";
	filename_log = fn.str();
	
	// open file if filename is given
	fout.open(filename_log.c_str(), ios::out);
	if( !fout.is_open() ){
		cerr << "Logger: cannot open file '" << fn.str().c_str() << "'." << endl;
		exit(-1);
	}
	
	if( log_header ){
		// write headers to file
		ostream& output = fout.is_open() ? fout : cout;
		for(int i=0; i< headers.size(); i++)
			output << headers[i] << "\t";
		output << endl;
	}
	
	numsym_d=0; numsym_v=0;
	for(uint s=0; s<SATypes.size(); s++){
		if( SATypes[s] == DOUBLE )
			numsym_d++;
		if( SATypes[s] == VECTOR )
			numsym_v++;
			
	}

	if(plot){
		
		try {
			gnuplot = new Gnuplot();
		}
		catch (GnuplotException e) {
			cerr << "Logger error: " << e.what() << endl;
			exit(-1);
		}
		
		last_plot_time = SIM::getTime();
		column_tokens = tokenize(cols_y, " ");
		if(column_tokens.size() == 0){		
			cerr << "Logger::init: Failed to find Y columns to plot!" << endl;
			exit(-1);
		}
		if( atoi(col_x.c_str()) > (((mapping != AVERAGE && mapping != MINMAX)? symbol_strings.size() :  symbol_strings.size() *2 ) + standard_columns)){
			cerr << "Logger::init: The specified X column \"" << atoi(col_x.c_str()) << "\" exceeds the number of columns in log file (which is \""<< (symbol_strings.size()  + standard_columns)<<"\")." << "\n\nHeaders:\n";
			for(uint i=0; i<headers.size(); i++)
				cerr << headers[i] << "\t";
			cerr << endl;
			exit(-1);			
		}
		for(uint i=0; i< column_tokens.size(); i++){
			if( atoi(column_tokens[i].c_str()) > (((mapping != AVERAGE && mapping != MINMAX) ? symbol_strings.size() :  symbol_strings.size() *2 )+ standard_columns) ){
				cerr << "Logger::init: The specified Y column \"" << atoi(column_tokens[i].c_str()) << "\" exceeds the number of columns in log file (which is \""<< (symbol_strings.size() + standard_columns)<<"\")." << "\n\nHeaders:\n";
				for(uint i=0; i<headers.size(); i++)
					cerr << headers[i] << "\t";
				cerr << endl;
					exit(-1);			
				}
		}	
		
		extension="";
		string terminal_temp;
		terminal_temp=terminal;
		if(terminal == "png"){
			terminal_temp="png truecolor enhanced font \"Arial,12\""; /*1024,786*/
			extension = "png";
		}
		else if(terminal == "pdf"){
			terminal_temp="pdfcairo enhanced font \"Helvetica,10\" ";
			extension = "pdf";
		}
		else if (terminal == "jpeg")
			extension = "jpg";
		else if (terminal == "gif")
			extension = "gif";
		else if (terminal == "svg")
			extension = "svg";
		else if (terminal == "postscript"){
			terminal_temp="postscript eps color enhanced font \"Helvetica,22\" ";
			extension = "eps";
		}
		//else extension = "log";
		
		stringstream ss;
		// on screen terminals ..
		if( terminal == "wxt" || terminal == "x11" || terminal == "aqua" )
			ss << "set term "<< terminal << (persist?" persist ":" ") << ";\n";
		// file output terminals
		else
			ss << "set term " << terminal_temp << ";\n";
		
		
		gnuplot->cmd(ss.str());

	}
}



