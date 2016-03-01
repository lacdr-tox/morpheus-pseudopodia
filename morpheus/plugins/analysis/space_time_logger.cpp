#include "space_time_logger.h"


REGISTER_PLUGIN(SpaceTimeLogger);

// NOTE TODO
// - Cannot findSymbol("TIME"): 
// WORKAROUND: in in XML, define in <CellType> a statement <Equation symbol="time">TIME</Equation>
//
// - Cannot use \t as a delimiter in a simple way
// WHY? Stringstream needs it to be \\t 
// Simple solution: have tab-delimited files as a standard (works well with gnuplot)
// Difficult solution: if delimiters are \t or \n, then put in an extra '\'
//
// 
SpaceTimeLogger::SpaceTimeLogger(){};

SpaceTimeLogger::~SpaceTimeLogger() {
	for(uint i=0; i<plots.size(); i++){
		if (plots[i].gnuplot)
			delete plots[i].gnuplot;
	}
};

void SpaceTimeLogger::finish(double time)
{
	for(uint i=0; i<fout.size(); i++){
		if( fout[i]->is_open() ){
			//fout[i] << endl;
			fout[i]->close();
		}
	}
	for(uint i=0; i<plots.size(); i++){
		if (plots[i].gnuplot)
			delete plots[i].gnuplot;
	}
}

void SpaceTimeLogger::log(double time){

	switch(mapping){
		
		case(SINGLE):
		{
			if( CPM::getCellIndex(cell_id).status == CPM::NO_CELL ){
				cout << "SpaceTimeLogger::log: cell " << cell_id << " does not exist (anymore)..." << endl;
				return;
			}
			
			for(uint s=0; s<accessors.size(); s++){
				if(accessors[s].is_membrane_property){
					//cout << "Lattice " << SIM::getLattice()->getXMLName() << " has dimenions: " << SIM::getLattice()->getDimensions() << endl;
					if( SIM::getLattice()->getDimensions() == 2 ){
						for(uint a=0; a<MembraneProperty::size.x; a++){
							(*fout[s]) << SIM::getTime() << " " << time << " " << a << " " << accessors[s].membrane_accessor.get(cell_id, a) << "\n";
						}
					}
					else if( SIM::getLattice()->getDimensions() == 3 ){
						for(uint a=0; a<MembraneProperty::size.x; a++){
							for(uint b=0; b<MembraneProperty::size.y; b++){
								(*fout[s]) << SIM::getTime() << " " << time << " " << a << " " << b << " " << accessors[s].membrane_accessor.get(cell_id,a,b) << "\n";
							}
						}
					}
					(*fout[s]) << endl;
				}
			}
			
			break;
		}
		case(ALL):
		{
			uint file=0;
			//uint numsymbols = accessors.size();
			vector<CPM::CELL_ID> cells = celltype->getCellIDs();
			for(uint c=0; c<cells.size(); c++){
				
				for(uint s=0; s<accessors.size(); s++){
					if(accessors[s].is_membrane_property){
						//cout << "Lattice " << SIM::getLattice()->getXMLName() << " has dimenions: " << SIM::getLattice()->getDimensions() << endl;
						if( SIM::getLattice()->getDimensions() == 2 ){
							for(uint a=0; a<MembraneProperty::size.x; a++){
								(*fout[s]) << SIM::getTime() << " " << time << " " << a << " " << accessors[s].membrane_accessor.get(cells[c], a) << "\n";
							}
						}
						else if( SIM::getLattice()->getDimensions() == 3 ){
							for(uint a=0; a<MembraneProperty::size.x; a++){
								for(uint b=0; b<MembraneProperty::size.y; b++){
									//cout << "Getting concentration at " << pos << endl;
									(*fout[s]) << SIM::getTime() << " " << time << " " << a << " " << b << " " << accessors[s].membrane_accessor.get(cell_id,a,b) << "\n";
								}
							}
						}
						(*fout[file]) << endl;
						file++;
					}
						
				}

			}
			break;
		}
		default:
		{
			//cerr << "SpaceTimeLogger::log: no mapping defined!" << endl;			
			//exit(-1);
			break;
		}
	}
	
	// PDE Layers
	
	uint file=0;
	for(uint s=0; s<accessors.size(); s++){
		if( !accessors[s].is_membrane_property){
			
			int size = accessors[s].pde_layer->size().x;
		
/*			// now, always plot along the x-axis; in the future, more flexibility
			switch ( accessors[s].dimensions ){
				case(1):
					size = 
					break;
				case(2):
					cerr << "Spacetime logger for 2D is not implemented yet!" << endl;
					exit(-1);
					break;
				case(3):
					cerr << "Spacetime logger for 3D is not implemented yet!" << endl;
					exit(-1);
					break;
				default:
					cerr << "Spacetime logger: unknown dimensions!" << endl;
					exit(-1);
					break;
			
			}
*/

			for(uint a=0; a<size; a++){
					VINT pos( a, accessors[s].slice_y, accessors[s].slice_z);

					//cout << time << " " << a << " " << accessors[s].pde_layer->get(pos) << endl;
					(*fout[file]) << SIM::getTime() << " " << time << " " << a << " " << accessors[s].pde_layer->get(pos) << endl;
			}
			(*fout[file]) << endl;
			
			file++;
		}
	}
}

void SpaceTimeLogger::plotData(plot p, double time){	
	
	stringstream ss;	

    if(p.extension.size()){
  		if (p.time_name){
	       ss << "set output 'spacetime_"<< files[p.plot_file_index].symbol << "_" << SIM::getTimeName() << "." << p.extension <<"';\n";
		}
		else 
	       ss << "set output 'spacetime_"<< files[p.plot_file_index].symbol << "_" <<  setfill('0') << setw(4) << int(rint( time / p.plot_interval)) << "." << p.extension <<"';\n";
	}
 
	// splot "< cut -d' ' -f2- membrane.log" matrix  us ($1):($2*10):3 pal not
	//splot 'membrane_3_a.log' us 2:1:($1>5?$3:1/0) pal pt 5	
	//cout << "p.plot_file = " << p.plot_file << endl;
	//cout << "files[p.plot_file].filename  = " << files[p.plot_file].filename << endl;

	ss << "set view map;\n" ;
	ss << "set size ratio 2;\n" ;
        ss << "set title \"" << files[p.plot_file_index].symbol << "\";\n";
	ss << "set xl 'space (" << SIM::getLengthScaleUnit() << ")';\n" ;
	ss << "set yl 'time (" << SIM::getTimeScaleUnit() << ")';\n" ;
	ss << "set zl '["<< files[p.plot_file_index].symbol << "]';\n" ;
	
	if( p.plot_min < p.plot_max)
		ss << "set cbrange [" << p.plot_min << ":" << p.plot_max<< "];\n";
	
	if( p.plot_max_time > 0 )
		ss << "set yrange [0:" << p.plot_max_time <<"] reverse;\n";
	else
		ss << "set yrange [0:*] reverse;\n";
		
	//cout << files[p.plot_file].filename << endl;
	ss << "splot \"" << files[p.plot_file_index].filename << "\" us ($3*" << SIM::getLengthScaleValue() <<"):1:($1<="<<time<<"?$4:NaN) every " << p.every << " w pm3d not;"; //w p pt 5 pal not;";
	
	p.gnuplot->cmd( ss.str() );
	
	if( p.write_commands ){
		ofstream command_log;
		if (p.time_name)
			command_log.open((string("spacetime_commands")+ SIM::getTimeName() + ".plt").c_str(),ios_base::app);
		else 
			command_log.open((string("spacetime_commands")+ to_str(time/p.plot_interval,4) + ".plt").c_str(),ios_base::app);
		command_log << ss.str();
		command_log.close();
	}

	//cout << ss.str() << endl;
	
	//if(!p.extension.size())
	//	cout << "Time: " << mcs << "\t Saving 'spacetime_"<< files[p.plot_file].symbol << "_" << mcs << "." << p.extension <<"';\n";

}

void SpaceTimeLogger::notify(double time)
{
	Analysis_Listener::notify(time);
	log(time);	
	
	for(uint i=0; i<plots.size(); i++){

		if (time >= plots[i].plot_last_time + plots[i].plot_interval) {
			plots[i].plot_last_time=time;
			plotData(plots[i], time);
		}
		
	}
}

void SpaceTimeLogger::loadFromXML(const XMLNode Node)
{
    Analysis_Listener::loadFromXML( Node );
	
	mapping = NONE;
	
	if(Node.nChildNode("Membranes") > 0){
		XMLNode mNode = Node.getChildNode("Membranes",0);
		
		string membrane_symbol_str;
		getXMLAttribute(mNode,"symbol-ref",membrane_symbol_str);
		data_format.append(" ");
		data_format.append(membrane_symbol_str);
				 
		ostringstream fn;
		fn << "spacetime_membrane_" << membrane_symbol_str << ".log";
		filename = fn.str();
// 		if( mNode.nChildNode("Filename") )
// 			getXMLAttribute(mNode,"Filename/name",filename);
// 		if( !filename.size() )
// 			cout << "SpaceTimeLogger::loadFromXML: No filename provided, writing to stdout " << endl;
// 		else
// 			cout << "SpaceTimeLogger::loadFromXML: Writing to file '" << filename << "'." << endl;

		//===		
		if( mNode.nChildNode("Cell") ){
			
			if( !getXMLAttribute(mNode,"Cell/mapping", mapping_str) )
				mapping_str = "all";
			
			if(lower_case(mapping_str) == "all"){ 
				mapping = ALL;
				cout << "SpaceTimeLogger::loadFromXML: Logging  all cells " << endl;
			}
			else if(lower_case(mapping_str) == "single"){
				mapping = SINGLE;
				cout << "SpaceTimeLogger::loadFromXML: Logging  single cell " << endl;
			}
		}
				
		if(mapping == SINGLE){
			getXMLAttribute(mNode,"Cell/cell_id", cell_id);
		}
		if(mapping == SINGLE || mapping == ALL){
			getXMLAttribute(mNode,"Cell/celltype", celltype_str);
		}

		// optional Format string to write multiple MembraneProps to file
		if( mNode.nChildNode("Format") ){
			getXMLAttribute(mNode,"Format/string",data_format);
		}
		if( !data_format.size() ){
			cerr << "SpaceTimeLogger::loadFromXML: No <Format> tag provided, logger does not make sense..." << endl;
			exit(-1);
		}
		else
			cout << "SpaceTimeLogger::loadFromXML: Logging  string  '"<< data_format << "' ." << endl;
		
	}
	
	//cout << "MAPPING = " << mapping_str << endl;
	
	for(uint i=0; i<Node.nChildNode("Layer");i++){
		XMLNode lNode = Node.getChildNode("Layer",i);
		string layer_symbol_str;
		getXMLAttribute(lNode,"symbol-ref",layer_symbol_str);
		data_format.append(" ");
		data_format.append(layer_symbol_str);
	}
			
	do_plots=false;
	for(int i=0; i < Node.nChildNode("Plot"); i++){
		plot p;
		do_plots=true;
		p.persist=false;
		XMLNode plotNode = Node.getChildNode("Plot",i);
		
		getXMLAttribute(plotNode, "terminal", p.terminal);
		getXMLAttribute(plotNode, "persist", p.persist);
		getXMLAttribute(plotNode, "every", p.every);
		p.plot_interval = 1;
		getXMLAttribute(plotNode, "interval", p.plot_interval);
		p.plot_cell_id = cell_id;
		p.plot_symbol =  tokenize( data_format, " " )[0];
		//getXMLAttribute(plotNode, "cell_id", p.plot_cell_id);
		//getXMLAttribute(plotNode, "symbol-ref", p.plot_symbol);
		p.plot_min=0.; 
		p.plot_max=0.;
		getXMLAttribute(plotNode, "min", p.plot_min);
		getXMLAttribute(plotNode, "max", p.plot_max);
		p.plot_max_time=0;
		getXMLAttribute(plotNode, "maxtime", p.plot_max_time);
		p.write_commands = false;
		getXMLAttribute(plotNode, "log-commands", p.write_commands);
		p.time_name = true;
		getXMLAttribute(plotNode,"timename", p.time_name);

		plots.push_back(p);
	}
		
}

set< string > SpaceTimeLogger::getDependSymbols()
{
	set< string > symbols;
	for(uint i=0; i < symbol_strings.size(); i++){
		symbols.insert(symbol_strings[i]);
	}
	return symbols;
}


void SpaceTimeLogger::init(double time){
    Analysis_Listener::init(time);

	//mapping = NONE;
	// get symbol accessors to symbols in data_format
	symbol_strings = tokenize( data_format, " " );
	for(uint i=0; i < symbol_strings.size(); i++){
		accessor a;
		a.symbol = symbol_strings[i];
		
		// first, check whether the symbol refers to a PDE lattice
		SymbolAccessor<double> pde_accessor = SIM::findSymbol<double>( symbol_strings[i] );
		//cout << "Symbol: " << pde_accessor.getFullName() << " has linktype: " << pde_accessor.getLinkType() << endl;
		if( pde_accessor.getLinkType() != SymbolData::PDELink ){
			//cout << "SpaceTimeLogger::init: Symbol \"" << pde_accessor.getName() << "\" (" << pde_accessor.getFullName() << ") is not a PDE layer!!" << endl;		
			
			// get cell type (must be given)	
			celltype = CPM::findCellType(celltype_str);
			//cout << "celltype = " << celltype->getName() << endl;
			if ( ! celltype ) {
				cerr << "SpaceTimeLogger::init: Celltype '" << celltype_str << "' does not exist! " << endl;
				exit(-1);
			}

			CellMembraneAccessor membrane = celltype->findMembrane( symbol_strings[i] ) ;
			if( !membrane.valid( ) ){
				cerr << "SpaceTimeLogger::init: Symbol \"" << membrane.getSymbol() << "\" (" << membrane.getFullName() << ") is not a MembraneProperty!" << endl;
				exit(-1);
			}
			else if( membrane.valid() ){
				a.is_membrane_property = true;
				a.membrane_accessor = membrane;
				a.symbol = symbol_strings[i];
				a.fullname = membrane.getFullName();
				accessors.push_back(a);
			}
		}
		else{
			shared_ptr< PDE_Layer > pde = SIM::findPDELayer( symbol_strings[i] );
			a.pde_layer  = pde;
			a.fullname 	 = pde->getName();
			a.dimensions = pde->getLattice()->getDimensions();
			a.pde_size	 = pde->getLattice()->size();
			a.is_membrane_property = false;
			a.slice_y 	 = 0; 
			a.slice_z 	 = 0;
			
			if( a.dimensions == 1 ){
				// for 1D, plot the concentration along the x-axis, at y=0 and z=0
				cout << "SpaceTimeLogger::init: Symbol \"" << pde_accessor.getName() << "\" (" << pde->getName() << ") is a linear lattice layer!!" << endl;
			}
			else if( a.dimensions == 2 ){
				// for 2D, plot the concentration along the x-axis, at y=L/2 and z=0
				a.slice_y = floor((double)pde->getLattice()->size().y / 2.0); 
				cout << "SpaceTimeLogger::init: Symbol \"" << pde_accessor.getName() << "\" (" << pde->getName() << ") is a 2D lattice layer!!" << endl;
			}
			else if( a.dimensions == 3 ){
				// for 3D, plot the concentration along the x-axis, at y=L/2 and z=L/2
				a.slice_y = floor((double)pde->getLattice()->size().y / 2.0); 
				a.slice_z = floor((double)pde->getLattice()->size().z / 2.0); 
				cout << "SpaceTimeLogger::init: Symbol \"" << pde_accessor.getName() << "\" (" << pde->getName() << ") is a 3D lattice layer!!" << endl;
			}
			
			accessors.push_back(a);
		}		
	}

	// open file if filename is given
	if( filename.size() ){
		//cout << "MAPPING = " << mapping << " (ALL = " << ALL  << ")"<<endl;
		if( mapping == SpaceTimeLogger::SINGLE ){
			//fout.reserve(symbol_strings.size());
			for (int s=0; s < accessors.size(); s++ ){
				if( accessors[s].is_membrane_property ){
					fout.push_back(new ofstream(filename.c_str()));
					
					fileData fd;
					fd.filename = filename;
					fd.cell_id  = cell_id;
					fd.celltype = celltype_str;
					fd.symbol   = symbol_strings[s];
					fd.symbol_name = accessors[s].fullname;
					files.push_back(fd);
				}
			}
		}
		else if(mapping == SpaceTimeLogger::ALL){
			
			vector<string> filename_tokenized;
			filename_tokenized = tokenize(filename, ".");
			
			string basename = filename_tokenized[0];
			string extension = filename_tokenized[ filename_tokenized.size() - 1];
			
			vector<CPM::CELL_ID> cell_ids = celltype -> getCellIDs();
			for (int c=0; c < cell_ids.size(); c++ ) {
				for (int s=0; s < accessors.size(); s++ ){
					if( accessors[s].is_membrane_property ){
						
						// generate filename: [basename]_[cellid]_[symbol].[extension]
						string fn = basename;
						fn.append( "_" );
						fn.append( to_str(cell_ids[c]) );
						fn.append( "_" );
						fn.append( symbol_strings[s]);
						fn.append( "." );
						fn.append( extension );
						fout.push_back(new ofstream(fn.c_str()));
						
						fileData fd;
						fd.filename = fn;
						fd.cell_id  = cell_ids[c];
						fd.celltype = celltype_str;
						fd.symbol	= symbol_strings[s];
						fd.symbol_name = accessors[s].fullname;
						cout << "symbol: " << fd.symbol << endl;
						files.push_back(fd);
					}
				}
			}
		}
	}
	
	
	// Open files for pde layers
	for (int s=0; s < accessors.size(); s++ ){
		if( !accessors[s].is_membrane_property ){
			stringstream fn;
			fn << "space_time_"<< accessors[s].symbol << ".log";
			fout.push_back(new ofstream(fn.str().c_str()));
			
			fileData fd;
			fd.filename = fn.str();
			fd.symbol_name = accessors[s].fullname;
			fd.symbol = accessors[s].symbol;
			fd.celltype = "";
			files.push_back(fd);
		}
	}
		
			
	for(uint i=0; i< plots.size(); i++){

		for(uint f=0; f< files.size(); f++){
			if(files[f].symbol == plots[i].plot_symbol  ){
				plots[i].plot_file_index = f;
// 				if( !files[f].celltype.empty() && files[f].cell_id == plots[i].plot_cell_id ){ // if membrane
// 					plots[i].plot_file_index = f;
// 					cout << " SpaceTimeLogger::init: plot file: " << plots[i].plot_file_index << endl;
// 				}
// 				else if (files[f].celltype.empty()) // if pde
// 					plots[i].plot_file_index = f;
// 
			}
			cout << " SpaceTimeLogger::init: plot file: " << plots[i].plot_file_index << endl;
		}

		try {
			plots[i].gnuplot = new Gnuplot();
		}
		catch (GnuplotException e) {
			cerr << "SpaceTimeLogger error: " << e.what() << endl;
			exit(-1);
		} 
		plots[i].plot_last_time=time;
		plots[i].extension="";
		string terminal_temp;
		terminal_temp=plots[i].terminal;
		if(plots[i].terminal == "png"){
			terminal_temp="png truecolor enhanced font \"Arial,12\""; /*1024,786*/
			plots[i].extension = "png";
		}
		else if(plots[i].terminal == "pdf"){
			terminal_temp="pdfcairo enhanced font \"Helvetica,10\" ";
			plots[i].extension = "pdf";
		}
		else if (plots[i].terminal == "jpeg")
			plots[i].extension = "jpg";
		else if (plots[i].terminal == "gif")
			plots[i].extension = "gif";
		else if (plots[i].terminal == "svg")
			plots[i].extension = "svg";
		else if (plots[i].terminal == "postscript"){
			terminal_temp="postscript eps color enhanced font \"Helvetica,22\" ";
			plots[i].extension = "eps";
		}
		//else plots[i].extension = "dat";

		stringstream ss;
		if( plots[i].terminal == "wxt" || plots[i].terminal == "x11" || plots[i].terminal == "aqua" )
			ss << "set term "<< plots[i].terminal << (plots[i].persist?" persist ":" ") << " size 500,1000 title \"Space-Time plot\";\n";
		else 
			ss << "set term " << terminal_temp << " size 500,1000;\n"; //  title \"Space-Time plot\";\n";
		ss << "set palette rgb 2,-7,-6;\n" ;
		
		plots[i].gnuplot->cmd(ss.str());
		cout << ss.str() << endl;
	}
}


