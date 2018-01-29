#include "histogram_logger.h"

REGISTER_PLUGIN(HistogramLogger);
	
HistogramLogger::~HistogramLogger() { if (gnuplot) { delete gnuplot; gnuplot = nullptr;} };

HistogramLogger::HistogramLogger(): gnuplot(NULL){

	numbins.setXMLPath("number-of-bins");
	numbins.setDefault("10");
	registerPluginParameter(numbins);
	
	normalized.setXMLPath("normalized");
	normalized.setDefault("false");
	registerPluginParameter(normalized);
	
	minimum_fixed.setXMLPath("minimum");
	registerPluginParameter(minimum_fixed);
	
	maximum_fixed.setXMLPath("maximum");
	registerPluginParameter(maximum_fixed);
	
	logarithmic_x.setXMLPath("logarithmic-bins");
	logarithmic_x.setDefault("false");
	registerPluginParameter(logarithmic_x);
	
	logarithmic_y.setXMLPath("logarithmic-freq");
	logarithmic_y.setDefault("false");
	registerPluginParameter(logarithmic_y);
	
	map<string, Terminal> terminalmap;
	terminalmap["png"] = PNG;
	terminalmap["pdf"] = PDF;
	terminalmap["jpg"] = JPG;
	terminalmap["gif"] = GIF;
	terminalmap["svg"] = SVG;
	terminalmap["eps"] = EPS;
	
	plot.terminal.setXMLPath("Plot/terminal");
	plot.terminal.setConversionMap(terminalmap);
	registerPluginParameter(plot.terminal);
	
	plot.logcommands.setXMLPath("Plot/log-commands");
	plot.logcommands.setDefault("false");
	registerPluginParameter(plot.logcommands);
	
	y_minimum.setXMLPath("Plot/minimum");
	y_minimum.setDefault("0");
	registerPluginParameter(y_minimum);
	
	y_maximum.setXMLPath("Plot/maximum");
	registerPluginParameter(y_maximum);

};

void HistogramLogger::loadFromXML(const XMLNode xNode, Scope* scope)
{
	cout << "HistogramLogger::loadFromXML " << endl;
	
	// Define PluginParameters for all defined Output tags
	for (uint i=0; i<xNode.nChildNode("Column"); i++) {
		shared_ptr<Column> c( new Column());
		c->symbol.setXMLPath("Column["+to_str(i)+"]/symbol-ref");
		registerPluginParameter(c->symbol);
		c->label.setXMLPath("Column["+to_str(i)+"]/label");
		registerPluginParameter(c->label);
		c->celltype.setXMLPath("Column["+to_str(i)+"]/celltype");
		registerPluginParameter(c->celltype);
		columns.push_back(c);
	}

    AnalysisPlugin::loadFromXML( xNode, scope );
	
}

void HistogramLogger::init(const Scope* scope) {
	// Preinitialize celltype parameters to transfer their celltype scopes to the symbol reference
	for (auto c : columns) {
		if (c->celltype.isDefined()) {
			c->celltype.init(scope);
			cout << "celltype " << c->celltype()->getName() << "defined for symbol " << c->symbol.stringVal();
			c->symbol.setScope(c->celltype()->getScope());
		}
	}

	AnalysisPlugin::init(scope);
	
	if( logarithmic_x() && ( (minimum_fixed.isDefined() && minimum_fixed() == 0) || (maximum_fixed.isDefined() && maximum_fixed() == 0)) ){
		throw MorpheusException("HistogramLogger: Cannot use Logarithmic bins with min=0 or max=0", stored_node);
	}
	if( logarithmic_y() && (y_minimum() == 0 || y_maximum() == 0) ) {
		throw MorpheusException("HistogramLogger: Cannot use Logarithmic/Plot with min=0 or max=0", stored_node);
	}

	plot.enabled = plot.terminal.isDefined();
	if ( plot.enabled ){
		gnuplot = new Gnuplot();

		plot.extension="";
		string terminal_temp;
		terminal_temp=plot.terminal();

		if(plot.terminal() == Terminal::PNG){
			terminal_temp="png truecolor enhanced font \"Helvetica,12\""; /*1024,786*/
			plot.extension = "png";
		}
		else if(plot.terminal() == Terminal::PDF){
			terminal_temp="pdfcairo enhanced font \"Helvetica,10\" ";
			plot.extension = "pdf";
		}
		else if (plot.terminal() == Terminal::JPG)
			plot.extension = "jpg";
		else if (plot.terminal() == Terminal::GIF)
			plot.extension = "gif";
		else if (plot.terminal() == Terminal::SVG)
			plot.extension = "svg";
		else if (plot.terminal() == Terminal::EPS){
			terminal_temp="postscript eps color enhanced font \"Helvetica,22\" ";
			plot.extension = "eps";
		}
		
		stringstream ss;
		// on screen terminals ..
		if( plot.terminal() == Terminal::WXT || 
			plot.terminal() == Terminal::X11 || 
			plot.terminal() == Terminal::AQUA )
			ss << "set term "<< plot.terminal() << (plot.persist?" persist ":" ") << ";\n";
		// file output terminals
		else
			ss << "set term " << terminal_temp << ";\n";

		if(logarithmic_x())
			ss << "set log x; set mxtics\n";
		if(logarithmic_y())
			ss << "set log y; set mytics\n";
		
		gnuplot->cmd(ss.str());

	}

}

void HistogramLogger::analyse(double time)
{
	stringstream ss;
	ss << "histogram_";
	for ( auto c : columns){
		ss << c->symbol.name() << "_";
	}
	ss << setfill('0') << setw(6) << time << ".log";

	filename = ss.str();
	fout.open(filename.c_str(), fstream::out );
	if( !fout.is_open() ){
		cout << "Error opening file " << ss.str() << endl;
		return;
	}

	this->writelog(time);

	if (plot.enabled) {
		this->plotData(time);
	}
}

void HistogramLogger::writelog(double time) {

	minimum = std::numeric_limits<double>::max();
	maximum = std::numeric_limits<double>::min();
	
	for(uint i=0; i<columns.size(); i++){

		Column& c = *columns[i];

		// collect values
		FocusRange range(c.symbol.granularity(), c.celltype.isDefined() ? c.celltype()->getScope() : this->scope());
		c.values.resize(range.size(),0);
		unsigned j=0;
		for (auto focus : range ){
			c.values[j] = c.symbol( focus );
			if (c.values[j]<minimum) minimum = c.values[j];
			if (c.values[j]>maximum) maximum = c.values[j];
			j++;
		}
	}

	// set minimum and maximum values
	if (minimum_fixed.isDefined()) {
		minimum=minimum_fixed();
	}
	if (maximum_fixed.isDefined()) {
		maximum=maximum_fixed();
	}

	Column& c0 = *columns[0];

	//create bins
	c0.bins.clear();
	double w = (maximum-minimum)/(double)numbins();

	vector<double> ws(numbins()+1);
	if( logarithmic_x() ){
		double logMin = log10(minimum);	
		double logMax = log10(maximum);
		double delta = (logMax-logMin) / (double)numbins();
		double accDelta=0;
		for (int i = 0; i<ws.size(); i++)
		{
			ws[i] = pow(10.0, logMin + accDelta);
// 				cout << "Bin: " << i << " min = " << ws[i] << " accDelta "<< accDelta << " delta " << delta << " numbins " << numbins << endl;
			accDelta += delta;
		}
	}
		
		
	for(uint i=0; i<columns.size(); i++){
		Column& c = *columns[i];
		// do the first bin separately, in case all values are identical
		c.bins.clear();
		for (int j = 0; j < numbins(); j++)
		{
			Bin b;
			if( logarithmic_x() ){
				b.minimum = ws[j];
				b.maximum = ws[j+1];
			}
			else{
				b.minimum = minimum+j*w;
				b.maximum = minimum+(j+1)*w;
			}
			b.frequency = 0; // initialize frequency
			c.bins.push_back(b);
		}

		//populate the probability density function
		double weight = 1.0;
		if( normalized() )
			weight = 1./(double)c.values.size();

		// for each bin
		for( auto& bin : c.bins){
			// get elements that fall in this bin
			for (vector<double>::iterator jt = c.values.begin(); jt < c.values.end(); jt++){
				if (*jt >= bin.minimum && *jt < bin.maximum){
					bin.frequency += weight;
				}
				else if (bin.minimum == bin.maximum){
					bin.frequency += weight;
				}
			}
		}
	}

    // Write to file in this format:

    // bin1_min bin1_max freq_p1 freq2_p2 ...
    // bin2_min bin1_max freq_p1 freq2_p2 ...
    // bin3_min bin1_max freq_p1 freq2_p2 ...
    // ...  ...     ...      ...

	for(uint b=0; b<numbins(); b++){

		// bins are determined by first property in the list!
		fout << c0.bins[b].minimum << "\t" << c0.bins[b].maximum;

		for(uint i=0; i<columns.size(); i++){
			if( logarithmic_y() ){
				fout << "\t" << columns[i]->bins[b].frequency;
			}
			else
				fout << "\t" << columns[i]->bins[b].frequency;
		}
		fout << "\n";
	}
	
	if(maximum == minimum){
		maximum += 1.0;
	}
	fout << endl;
	fout.close();

}

void HistogramLogger::plotData(double time){
		
	stringstream ss;

	if(!plot.extension.empty()){
		ss << "set output 'plot_histogram_";
		for(uint i=0; i<columns.size(); i++){
			ss << columns[i]->symbol.name() << "_";
		}
		ss << setfill('0') << setw(6) << time << "." << plot.extension << "';\n";
	}

	ss << "set xlabel '" <<columns[0]->symbol.name();
	for(uint i=1; i<columns.size(); i++){
		ss << ", " << columns[i]->symbol.name() ;
	}
	ss << "';\n";

	ss << "set ylabel '" << (normalized() ? "Rel. freq.":"Frequency") << "';\n";
	ss << "set grid mxtics xtics;\n";
	ss << "set grid mytics ytics;\n";

	double min2 = (minimum==maximum?minimum-1:minimum);
	double max2 = (minimum==maximum?minimum+1:maximum);
	double boxwidth = ((0.8) / columns.size()) * (max2-min2)/numbins();
	ss << "set style fill solid border;\n";
	if(logarithmic_x())
		ss << "set boxwidth;\n";
	else
		ss << "set boxwidth " << boxwidth << ";\n";

	vector<string> colors;
	colors.push_back("red");
	colors.push_back("blue");
	colors.push_back("dark-green");
	colors.push_back("cyan");
	colors.push_back("yellow");
	colors.push_back("violet");
	colors.push_back("orange");
	colors.push_back("black");

	// set keys for histogram plot with more than 1 property
	vector<string> titles;
	titles.push_back("notitle");
	if( columns.size() > 1 ){

		ss << "set key top right;\n";

		titles.clear();
		for(uint i=0; i<columns.size(); i++){
			stringstream tss;
			tss << "title '" << ( columns[i]->label.isDefined() ?  columns[i]->label() :columns[i]->symbol.name()) << "'";
			titles.push_back( tss.str() );
		}
	}

	double shift = boxwidth;
	ss << "plot ["<< min2 <<":"<< max2 <<"]["<<y_minimum()<<":"<< (y_maximum.isDefined() ? y_maximum.stringVal() : "*") <<"] '";
	ss << filename << "' us ($1+($2-$1)/2):3 with boxes lc rgb '"<<colors[0]<<"' " << titles[0];
	for(uint i=1; i<columns.size(); i++){
		ss << ",\\\n'' us (($1+$2)/2+"<<(i*shift)<<"):"<<i+3<<" with boxes lc rgb '"<<( i<colors.size() ? colors[i] : colors[i%colors.size()] )<<"' "<< titles[i];
	}
	ss <<";\n";

	//cout << ss.str() << endl;
	gnuplot->cmd( ss.str() );

	if(plot.logcommands()){
		ofstream foutcmd;
		foutcmd.open( "histogram_command.plt", fstream::out);
		foutcmd << ss.str() << endl;
		foutcmd.close();
	}

}


void HistogramLogger::finish()
{
	if( fout.is_open() ){
		fout << endl;
		fout.close();
	}
}
