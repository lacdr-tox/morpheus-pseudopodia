#include "gnuplotter.h"
using namespace SIM;



const float CellPainter::transparency_value = std::numeric_limits<float>::quiet_NaN();

Gnuplotter::PlotSpec::PlotSpec() : 
field(false), cells(false), labels(false), arrows(false), vectors(false), title("") {};

VDOUBLE Gnuplotter::PlotSpec::size()
{
	
	VDOUBLE latticeDim = SIM::lattice().to_orth(SIM::lattice().size());
	if (SIM::lattice().getStructure() == Lattice::hexagonal) {
		latticeDim.x = SIM::lattice().size().x;
		if (SIM::lattice().get_boundary_type(Boundary::mx) != Boundary::periodic) {
			latticeDim.x += ceil(double(SIM::lattice().size().y)) * 0.5;
		}
	}
	if (latticeDim.y<3) {
		latticeDim.y = max(2.0,0.1*latticeDim.x);
	}
	latticeDim += VDOUBLE(1,1,1); //VDOUBLE(1,1,1);
	
	return latticeDim;
}


void SymbolReader::init()
{
	string type_name = SIM::getSymbolType(name);
	if (type_name == TypeInfo<double>::name()) {
		type = sDouble;
		sym_d = SIM::getGlobalScope()->findSymbol<double>(name,0);
		integer = sym_d.isInteger();
		fullname = sym_d.getFullName();
		linktype = sym_d.getLinkType();
        cout << "SymbolReader::init()  Symbol " <<  name << " is double value of type " << SymbolData::getLinkTypeName(sym_d.getLinkType()) << endl;
	}
	else if (type_name == TypeInfo<VDOUBLE>::name()) {
		type = sVDOUBLE;
		sym_v = SIM::getGlobalScope()->findSymbol<VDOUBLE>(name,VDOUBLE(0,0,0));
		integer = sym_v.isInteger();
		fullname = sym_v.getFullName();
		linktype = sym_v.getLinkType();
         cout << "SymbolReader::init()  Symbol " <<  name << " is vector value of type " << SymbolData::getLinkTypeName(sym_v.getLinkType()) << endl;
	}
	else {
		throw string("SymbolReader::init: Unknown Symbol Type ") + type_name + ".";
	}
	
}


ArrowPainter::ArrowPainter() {}

void ArrowPainter::loadFromXML(const XMLNode node)
{
	arrow.setXMLPath("orientation");
	arrow.loadFromXML(node);
	style = 3;
	getXMLAttribute(node,"style",style);
}

int ArrowPainter::getStyle() { return style; }

void ArrowPainter::init()
{
	arrow.init();
}

set< SymbolDependency > ArrowPainter::getInputSymbols() const
{
	return arrow.getDependSymbols();
}


const string& ArrowPainter::getDescription() const
{
	return arrow.description();
}


void ArrowPainter::plotData(ostream& out)
{
	const Lattice& lattice = SIM::lattice();
	VDOUBLE orth_lattice_size = Gnuplotter::PlotSpec::size();
	
// 	out.precision(2);
	auto celltypes = CPM::getCellTypes();
	for (uint i=0; i < celltypes.size(); i++){
		auto ct = celltypes[i].lock();
		if (ct->isMedium())
			continue;
		
		vector<CPM::CELL_ID> cells = ct->getCellIDs();

		for (uint c=0; c< cells.size(); c++){
			SymbolFocus f(cells[c]);
			
			VINT centerl = f.cell().getCenterL();
			lattice.resolve (centerl);
			VDOUBLE center = ( lattice.to_orth(centerl) + VDOUBLE(0.5,0.5,0) ) % orth_lattice_size;
			
			VDOUBLE a = arrow(f);
			if (! (a.x == 0 && a.y==0) ) {
				out << center.x-a.x  << "\t" <<  center.y-a.y << "\t" << 2*a.x << "\t" << 2*a.y << endl;
			}
		}
	}
}

void FieldPainter::loadFromXML(const XMLNode node)
{
	field_value.setXMLPath("symbol-ref");
	field_value.loadFromXML(node);
	
	min_value.setXMLPath("min");
	min_value.loadFromXML(node);
	
	max_value.setXMLPath("max");
	max_value.loadFromXML(node);
	
	isolines.setXMLPath("isolines");
	isolines.loadFromXML(node);
	
	surface.setXMLPath("surface");
	surface.loadFromXML(node);
	
	z_slice.setXMLPath("slice");
	z_slice.setDefault("0");
	z_slice.loadFromXML(node);
	
// 	getXMLAttribute(xPlotChild, "data-cropping", plot.pde_data_cropping);
// 	getXMLAttribute(xPlotChild, "resolution", plot.pde_max_resolution);
				
	if(  node.nChildNode("ColorMap") > 0 && node.getChildNode("ColorMap").nChildNode("Color") > 0 ){
		XMLNode xColorMap = node.getChildNode("ColorMap");
		for (int j=0; j< xColorMap.nChildNode("Color"); j++) {
			XMLNode xColor = xColorMap.getChildNode("Color",j);
			string colorname=""; double value=0.;
			getXMLAttribute(xColor,"color",colorname);
			getXMLAttribute(xColor,"value",value);
			lower_case(colorname);
			color_map[value] = colorname;
		}
	}
}

void FieldPainter::init()
{
	field_value.init();
	if( min_value.isDefined() )
		min_value.init();
	if( max_value.isDefined() )
		max_value.init();
}

const string& FieldPainter::getDescription() const
{
	return field_value.description();
}

set< SymbolDependency > FieldPainter::getInputSymbols() const
{
	set<SymbolDependency> sd, sd2;
	sd2 = field_value.getDependSymbols();
	sd.insert(sd2.begin(),sd2.end());
	sd2 = min_value.getDependSymbols();
	sd.insert(sd2.begin(),sd2.end());
	sd2= max_value.getDependSymbols();
	sd.insert(sd2.begin(),sd2.end());
	return sd;
}



string FieldPainter::getValueRange() const
{
	string min_val_str(min_value.isDefined() ? to_str(min_value.get(SymbolFocus::global)) : string("*"));
	string max_val_str(max_value.isDefined() ? to_str(max_value.get(SymbolFocus::global)) : string("*"));
	return string("[")+ min_val_str + ":" + max_val_str + "]";
}

string FieldPainter::getColorMap() const
{
	stringstream color_map_string;
	if ( ! color_map.empty() ) {
		color_map_string << "set palette defined (";
		map<double,string>::const_iterator it = color_map.begin();
		while (true) {
			color_map_string << it->first << " '" << it->second +"' ";
			++it;
			if (it==color_map.end()) break;
			color_map_string << ", ";
		}
		color_map_string << ");\n";
	}
	else {
		color_map_string << "set palette rgb 2,-7,-6;\n";
	}
	return color_map_string.str();
}

void FieldPainter::plotData(ostream& out)
{
	VINT pos;
	pos.z = z_slice.get();
	VINT size = SIM::lattice().size();
	bool is_hexagonal = SIM::lattice().getStructure() == Lattice::hexagonal;
	
	string xsep(" "), ysep("\n");

	int x_iter = 1;
	
	int max_resolution = 200;
	
	if (max_resolution) x_iter = max (1, size.x / max_resolution);
	int y_iter = 1;
	if (max_resolution) y_iter = max (1, size.y / max_resolution);
	
	valarray<float> out_data(size.x), out_data2(size.x);

	pos.z = z_slice.get();
// 	bool first_row = true;
	for (pos.y=(y_iter)/2; pos.y<size.y; pos.y+=y_iter) {
		// copy & convert the raw data
		for ( pos.x=0; pos.x<size.x; pos.x+=x_iter) {
			out_data[pos.x] = float(field_value.get(SymbolFocus(pos)));
		}

		// shifting the data to map hex coordinate system
		if (is_hexagonal) {
			out_data = out_data.cshift(-pos.y/2);
			// add an interpolation step
			if (pos.y%2==1) {
				out_data2 = out_data.cshift(-1);
				out_data+= out_data2;
				out_data/=2;
			}
		}

		// Cropping data
		if (min_value.isDefined()) {
			float fmin_value = min_value.get(SymbolFocus::global);
			for (int i = 0; i< out_data.size(); i++) {
				if (out_data[i] < fmin_value) {
					out_data[i] = fmin_value;
				}
			}
		}
		if (max_value.isDefined()) {
			float fmax_value = float(max_value.get(SymbolFocus::global));
			for (int i = 0; i<out_data.size(); i++) {
				if (out_data[i] > fmax_value) {
					out_data[i] = fmax_value;
				}
			}
		}

		out << out_data[0];
		for (int i=1; i<out_data.size() ;i++) {
			out << xsep << out_data[i];
		}
		
		out << ysep;
		
// 		if (first_row) {
// 			first_row = false;
// 			if ( ! (pos.y+y_iter)<l_size.y )
// 				pos.y-=y_iter;
// 		}
	}
}


void VectorFieldPainter::loadFromXML(const XMLNode node)
{
	value.setXMLPath("value");
	value.loadFromXML(node);
	
// 	if ( ! getXMLAttribute(node, "x-symbol-ref", x_symbol.name)) { cerr << "Undefined x-symbol-ref in GnuPlot -> VectorField"; exit(-1);}
// 	if ( ! getXMLAttribute(node, "y-symbol-ref", y_symbol.name)) { cerr << "Undefined y-symbol-ref in GnuPlot -> VectorField"; exit(-1);};
	style = 1;
	getXMLAttribute(node, "style", style);
	color = "black";
	getXMLAttribute(node, "color", color);
	coarsening = 1;
	getXMLAttribute(node, "coarsening",coarsening);
	scaling=1.0;
	getXMLAttribute(node,"scaling",scaling);
    slice = 0;
    getXMLAttribute(node,"slice",slice);
}

void VectorFieldPainter::init()
{
	value.init();
}

set< SymbolDependency > VectorFieldPainter::getInputSymbols() const
{
	return value.getDependSymbols();
}


void VectorFieldPainter::plotData(ostream& out)
{
	VINT pos(0, 0, slice);
	VINT size = SIM::lattice().size();
	for (pos.y=coarsening/2; pos.y<size.y; pos.y+=coarsening) {
		for (pos.x=coarsening/2; pos.x<size.x; pos.x+=coarsening) {
            VDOUBLE arrow = value.get(SymbolFocus(pos)) * scaling;
			out << pos.x - 0.5*arrow.x << "\t" << pos.y - 0.5*arrow.y << "\t" << arrow.x << "\t" << arrow.y << "\n";
		}
	}
}


string VectorFieldPainter::getDescription()
{
	return value.description();
}

int VectorFieldPainter::getStyle()
{
	return style;
}

string VectorFieldPainter::getColor()
{
	return color;
}


LabelPainter::LabelPainter()
{
	_fontcolor="black";
	_fontsize=12;
	_scientific=false;
	_precision=-1;
	
}



void LabelPainter::loadFromXML(const XMLNode node)
{
	if ( ! getXMLAttribute(node,"symbol-ref",symbol.name)) {
			cout << "GnuPlotter, LabelPainter::loadFromXML: symbol-ref not specified. Plotting celltype..." << endl;
		symbol.name = SymbolData::CellType_symbol;
	}
	getXMLAttribute(node,"fontcolor",_fontcolor);
	getXMLAttribute(node,"fontsize",_fontsize);
	getXMLAttribute(node,"precision", _precision);
	getXMLAttribute(node,"scientific", _scientific);
	
}



void LabelPainter::init()
{
	symbol.init();
}


const string& LabelPainter::getDescription() const {
	return symbol.fullname;
}

set<SymbolDependency> LabelPainter::getInputSymbols() const
{
	set<SymbolDependency> sd;
	SymbolDependency s =  { symbol.name, SIM::getGlobalScope() };
	sd.insert(s);
	return sd;
}


void LabelPainter::plotData(ostream& out)
{
	const Lattice& lattice = SIM::lattice();
	bool  is_hexagonal = (lattice.getStructure() == Lattice::hexagonal);
	VDOUBLE orth_lattice_size = Gnuplotter::PlotSpec::size();
	if (is_hexagonal) {
		orth_lattice_size.x-=1;
	}
	
	stringstream label_stream;
	if( _scientific )
		label_stream.setf(ios_base::scientific);
	else 
		label_stream.setf(ios_base::fixed);
	if( _precision >= 0 )
		label_stream<< setprecision(_precision);
	
	auto celltypes = CPM::getCellTypes();
	for (uint i=0; i < celltypes.size(); i++) {
		auto ct = celltypes[i].lock();
		if ( ct->isMedium() ) 
			continue;

		vector<CPM::CELL_ID> cells = ct->getCellIDs();
		for (uint c=0; c< cells.size(); c++){
			const Cell& cell = CPM::getCell(cells[c]);
			
			VINT centerl = cell.getCenterL();
			lattice.resolve (centerl);
			VDOUBLE center = ( lattice.to_orth(centerl) + (CPM::isEnabled() ? VDOUBLE(1.0,1.0,0): VDOUBLE(0.1,0.0,0.0) ) ) % orth_lattice_size;
			//cout << "cell " << cells[c] << "\t" << center  << " -> ";
			
			switch(symbol.type) {
			case SymbolReader::sDouble :
				if (symbol.linktype == SymbolData::CellMembraneLink){
					cerr << "Gnuplotter: Plotting labels is not available for a MembraneProperty" << endl;
					exit(-1);
				}
				if ( ! symbol.integer ) {
					label_stream.str("");
					label_stream << symbol.sym_d.get(cells[c]);
					out << center.x << "\t" << center.y << "\t" << label_stream.str() << endl;
				}
				else 
					out << center.x << "\t" << center.y << "\t" << int( symbol.sym_d.get(cells[c])) << endl;
				break;
			case SymbolReader::sVDOUBLE:
				label_stream.str("");
				label_stream << symbol.sym_v.get(cells[c]);
				out << center << "\t \"" <<  label_stream.str() << "\""<< endl;
				break;
			default:
				cerr << "LabelPainter::plotData(): Unknown symbol type " << symbol.name << endl;
			}
		}
	}
}


CellPainter::CellPainter()
{
	external_palette = false;
	external_range = false;
	min_val = 1e12;
	max_val = -1e12;
	z_level = 0;
	symbol.setXMLPath("value");
	symbol.setDefault("cell.type");
}

CellPainter::~CellPainter() { }

void CellPainter::loadFromXML(const XMLNode node)
{
	symbol.loadFromXML(node);	
	
	external_palette = false;
	external_range = false;
	external_range_min=false;
	external_range_max=false;
	z_level = 0;
	flooding=true;
	reset_range_per_frame = false;
	
	if (getXMLAttribute(node,"min",range_min) )
		external_range_min = true;
	if (getXMLAttribute(node,"max",range_max) )
		external_range_max = true;
	if (getXMLAttribute(node,"min",range_min) && getXMLAttribute(node,"max",range_max))
		external_range = true;

	if (string(node.getName()) == "Cells") {
		
		getXMLAttribute(node,"slice",z_level);
		getXMLAttribute(node,"flooding",flooding);
		getXMLAttribute(node,"per-frame-range", reset_range_per_frame);
		
		if(  node.nChildNode("ColorMap") > 0 ){ 
			loadPalette(node.getChildNode("ColorMap"));
		}
	}
}

void CellPainter::updateDataLayout() {
	if (flooding && symbol.granularity() != Granularity::MembraneNode)
		// this does not plot cells using points, but fills the boundary !
		// Know that this currently is based on piping data;
		data_layout = boundary_cell_wise;
	else if (is_hexagonal) 
		data_layout = point_wise;
	else
		data_layout = ascii_matrix;
}

void CellPainter::loadPalette(const XMLNode node)
{
	external_palette=true;
	if(  node.nChildNode("Color") > 0 ){
		
		for (int j=0; j< node.nChildNode("Color"); j++) {
			XMLNode xColor = node.getChildNode("Color",j);
			string colorname=""; double value=0.;
			getXMLAttribute(xColor,"color",colorname);
			getXMLAttribute(xColor,"value",value);
// 			cout << "color name = " << colorname << " => " << value << endl;
			lower_case(colorname);
			color_map[value] = colorname;
		}
	}
	else{
		cout << "Gnuplotter:: ColorMap did not define any colors" << endl;
	}
}

void CellPainter::init() {
	symbol.init();
	
	cpm_layer = CPM::getLayer();
	
	if(z_level > cpm_layer->size().z){
		throw string("CellPainter: z-slice to be plotted lies outside of lattice range = ") + to_str(cpm_layer->size()) + ".";
	}

	is_hexagonal = (SIM::lattice().getStructure() == Lattice::hexagonal);
	updateDataLayout();
}

XMLNode CellPainter::savePalette() const
{
	XMLNode node = XMLNode::createXMLTopNode("ColorMap");
	map<double,string>::const_iterator it;
	for(it=color_map.begin(); it != color_map.end(); it++) {
		XMLNode xColor = node.addChild("Color");
		xColor.addAttribute("color",it->second.c_str());
		xColor.addAttribute("value",to_cstr(it->first));
	}
	return node;
}

void CellPainter::setDefaultPalette() {
// // 	color_map.clear();
	int i = 0;
	std::map <double, string > color_template;
	color_template[i++]="red";
	color_template[i++]="yellow";
	color_template[i++]="dark-green";
	color_template[i++]="blue";
	color_template[i++]="orange";
	color_template[i++]="light-coral";
	color_template[i++]="turquoise";
	color_template[i++]="dark-magenta";
	color_template[i++]="spring-green";
	color_template[i++]="light-blue";
	
	// make a 
	int fsize = color_template.size();
	int count=max_val;
	if (count==0) count = 10;
		
	for ( i = color_map.size(); i <= count; i++ ){
		color_map[i]=color_template[i % fsize];
	}
}

string CellPainter::getPaletteCmd() {
	string title;
	stringstream cmd;
	
	if (symbol.isInteger() && color_map.empty()) {
 		if (color_map.size() < max_val)
 			setDefaultPalette();
 	}
 	
	if ( ! color_map.empty()) {
		cmd << "set cbtics; set palette defined (";
		map<double,string>::const_iterator it = color_map.begin();
		while (true) {
			cmd << it->first << " '" << it->second +"' ";
			++it;
			if (it==color_map.end()) break;
			cmd << ", ";
		}
		cmd << ");\n";
	}
	else {
		cmd << "set palette;\n";
	}
		
	if ( external_range ) {// if range is set in XML
		cmd << "set cbrange [" << range_min << ":" << range_max << "];\n"; 
	} else if ( external_range_min ){
		cmd << "set cbrange [" << range_min << ":*];\n"; 
	} else if ( external_range_max ){
		cmd << "set cbrange [*:" << range_max << "];\n"; 
	} else if ((! color_map.empty() && symbol.isInteger())) { // if plotting celltypes
		cmd << "set cbrange [0:" << color_map.size()-1 << "];\n";
	} else if (min_val < max_val){
		cmd << "set cbrange [" << min_val << ":" << max_val << "];\n"; 
	} else { // else, let gnuplot choose range
		cmd << "set cbrange [*:*];\n";
	}

	return cmd.str();
}

vector<CellPainter::CellBoundarySpec> CellPainter::getCellBoundaries() {

	// Reset the min / max value range
	if (reset_range_per_frame) {
		min_val = 1e12;
		max_val = -1e12;
	}
	
	vector<CellPainter::CellBoundarySpec> cell_boundaries;
	auto celltypes = CPM::getCellTypes();
	for (uint i=0; i < celltypes.size(); i++) {
		auto ct = celltypes[i].lock();
		if (ct->isMedium())
			continue;
		vector<CPM::CELL_ID> cells = ct->getCellIDs();
		bool is_super_cell = ( dynamic_pointer_cast<const SuperCT>(ct) != nullptr );
		for (uint c=0; c< cells.size(); c++) {
			const Cell& cell = CPM::getCell(cells[c]);
			if (! cell.nNodes()) continue;
// 			if (is_super_cell) continue;
				
			CellBoundarySpec cell_spec;
			
			if (is_super_cell)
				cell_spec.polygons = polygons( getBoundarySnippets( cell.getNodes(), & same_super_cell ) );
			else 
				cell_spec.polygons = polygons( getBoundarySnippets( cell.getNodes(), & same_cell ) );
			
            if (cell_spec.polygons.empty()) continue;

			cell_spec.value = getCellValue(cell.getID());
// 			if ( ! isnan(cell_spec.value) && cell_spec.value != transparency_value)
			cell_boundaries.push_back(cell_spec);
		}
	}
	return cell_boundaries;
}

float CellPainter::getCellValue(CPM::CELL_ID cell_id) {
// 	const Cell& cell = CPM::getCell(cell_id);
// 	if (! cell.nNodes()) return transparency_value;
	// check whether the cell is visible in the current slice
	float value;
	
	if (symbol.granularity() == Granularity::MembraneNode) {
		return transparency_value;
	}
	
	value =  symbol(SymbolFocus(cell_id));

	min_val = min(min_val,value);
	max_val = max(max_val,value);
	return value;
}


CellPainter::DataLayout CellPainter::getDataLayout()
{
	return data_layout;
}

const string& CellPainter::getDescription() const {
	return symbol.description();
}

set<SymbolDependency> CellPainter::getInputSymbols() const
{
	return this->symbol.getDependSymbols();
}

void CellPainter::writeCellLayer(ostream& out)
{
	VINT size = cpm_layer->size();
	assert(z_level < size.z);
	
    if (view.empty()) {
		view.resize(size.y,vector<float>(size.x,transparency_value));
	}
    
	// Reset the min / max value range
	if (reset_range_per_frame) {
		min_val = 1e12;
		max_val = -1e12;
	}

	// drawback of plotting cell properties is that we need to reserve a 'double' array

	// find the id's and types of the named property propnum in all celltypes	
	VINT p;
	p.z = z_level;
    
	for (p.y = 0; p.y < size.y; ++p.y) {
		for (p.x = 0; p.x < size.x; ++p.x) {

			CPM::STATE state = cpm_layer->get(p);

			if( state != CPM::getEmptyState()){ // if not medium
				if (symbol.granularity() == Granularity::MembraneNode && ! CPM::isBoundary(p)) {
					
					view[p.y][p.x] = transparency_value;
				}
				else {
					view[p.y][p.x] = float( symbol(SymbolFocus(state.cell_id, p) ) );
				}

				if (min_val > view[p.y][p.x] && view[p.y][p.x] != transparency_value) min_val = view[p.y][p.x];
				if (max_val < view[p.y][p.x] && view[p.y][p.x] != transparency_value) max_val = view[p.y][p.x];
			}
			else {
				view[p.y][p.x] = transparency_value; // medium
			}
		}
	}

	if (data_layout == point_wise ) {
		for (int y = 0; y < size.y; ++y) {
			for (int x = 0; x < size.x; ++x) {
				if(is_hexagonal)
					out << MOD(x+double(y)/2,double(size.x)) << "\t" << double(y)*sin(M_PI/3);
				else
					out << x << "\t" << y; 
                if (isnan(view[y][x]) || view[y][x] == transparency_value)
                    out << "\t" << "NaN" << "\n";
                else
                    out << "\t" << view[y][x] << "\n";
			}
			out << "\n";
		}
	}
	else if (data_layout == ascii_matrix) {
		for (int y = 0; y < size.y; ++y) {
			for (int x = 0; x < size.x; ++x) {
				if (isnan(view[y][x]) || view[y][x] == transparency_value)
                    out << "NaN" << "\t";
                else
                    out << view[y][x] << "\t";
			}
			out << "\n";
		}
	}
}


vector<CellPainter::boundarySegment> CellPainter::getBoundarySnippets(const Cell::Nodes& node_list, bool (*comp)(const CPM::STATE& a, const CPM::STATE& b))
{
	VDOUBLE latticeDim = Gnuplotter::PlotSpec::size();
	latticeDim -= VDOUBLE(1.0,1.1,0);
	
	// we assume that the neighbors are sorted either clockwise or anti-clockwise.
	// we could also add a sorting step after the filtering of nodes belonging to the plane
	vector<VINT> neighbors = SIM::lattice().getNeighborhood(1);
	if( SIM::lattice().getStructure() == Lattice::linear ) {
		neighbors.resize(4);
		neighbors[0] = VINT(1, 0, 0);
		neighbors[1] = VINT(0, 1, 0);
		neighbors[2] = VINT(-1, 0, 0);
		neighbors[3] = VINT(0, -1, 0);
	}

	vector<VINT> plane_neighbors;
	vector<VDOUBLE> orth_neighbors;
	for  ( vector< VINT >::iterator n = neighbors.begin(); n!=neighbors.end();n++) {
		if ( n->z == 0) {
			plane_neighbors.push_back(*n);
			orth_neighbors.push_back(SIM::lattice().to_orth(*n));
		}
	}

	// start and end  point of the line snippet towards neighbor i relative to the node center
	vector<VDOUBLE> snippet_begin_offset; 
	vector<VDOUBLE> snippet_end_offset;
	for  ( int i=0; i<orth_neighbors.size(); i++) {
			// those offsets serve all lattice structures
			snippet_begin_offset.push_back((orth_neighbors[i] + orth_neighbors[MOD(i+1,orth_neighbors.size())]) / plane_neighbors.size() * 2.0);
			snippet_end_offset.push_back((orth_neighbors[i] + orth_neighbors[MOD(i-1,orth_neighbors.size())]) / plane_neighbors.size() * 2.0);
	}

	vector<boundarySegment> snippets;

	Cell::Nodes::const_iterator it;
	for (it = node_list.begin(); it != node_list.end(); it++ )
	{
		VINT pos = (*it);
		cpm_layer->lattice().resolve(pos);
		if (pos.z != z_level) continue;
		
		// get the proper position in the drawing area
		VDOUBLE orth_pos  = cpm_layer->lattice().to_orth(pos) % latticeDim;

		const CPM::STATE& node = CPM::getNode(pos);
		for(int i = 0; i < plane_neighbors.size(); i++)
		{
			const CPM::STATE& neighborNode = CPM::getNode((pos + plane_neighbors[i]));

			if ( comp (neighborNode, node) ) {
				// identical states
				VDOUBLE orth_nei = (orth_pos + orth_neighbors[i]) % latticeDim;
				if ( (orth_pos-orth_nei).abs() <= 1.1) {
					// neighbor is inside of the drawing area
					continue;
				}
			}
			// create the edge : this alg. serves all lattice structures
			boundarySegment boundary;
			boundary.pos1 = orth_pos + snippet_begin_offset[i];
			boundary.pos2 = orth_pos + snippet_end_offset[i];
			snippets.push_back(boundary);
		}
	}
	return snippets;
};

vector< vector<VDOUBLE> > CellPainter::polygons(vector<boundarySegment> v_snippets){

	list<boundarySegment>::iterator start, current;
	list<boundarySegment> snippets(v_snippets.begin(), v_snippets.end());
	vector< vector<VDOUBLE> > polygons;

	while ( ! snippets.empty() )
	{
		vector<VDOUBLE> polygon;
		polygon.push_back(snippets.front().pos1);
		polygon.push_back(snippets.front().pos2);
		snippets.pop_front();
		start = current = snippets.begin();

		while ( (( polygon.front() - polygon.back()).abs_sqr() > 0.01) )
		{
			if ( (polygon.back() - current->pos1).abs_sqr() < 0.01)
			{
				// found a fitting snippet
				polygon.push_back(current->pos2);
				snippets.erase(current++);
				if (current == snippets.end()) current = snippets.begin();
				start = current;
			}
			else if ( (polygon.back() - current->pos2).abs_sqr() < 0.01)
			{
				// found a fitting snippet
				polygon.push_back(current->pos1);
				snippets.erase(current++);
				if (current == snippets.end()) current = snippets.begin();
				start = current;
			}
			else
			{
				current++;
				if (current == snippets.end()) current = snippets.begin();
				if (current == start) {
					// break through !!;
					cout << "Cannot close polygon !" << endl;
					cout << polygon.front() << "  - - - " << polygon.back() << endl;
// 					for (current = snippets.begin(); current != snippets.end(); current++) {
// 						cout << "("<< current->pos1 << "; " << current->pos2 << "),";
// 					}
					break;
				}
			}
		}
		polygons.push_back(polygon);
	}
	return polygons;
};


int Gnuplotter::instances=0;

REGISTER_PLUGIN(Gnuplotter);

Gnuplotter::Gnuplotter(): AnalysisPlugin(), gnuplot(NULL) {
	Gnuplotter::instances++;
	instance_id = Gnuplotter::instances; 
	
	file_numbering.setDefault("time");
	file_numbering.setXMLPath("file-numbering");
	map<string, FileNumbering> file_numbering_map;
	file_numbering_map["time"] = FileNumbering::TIME;
	file_numbering_map["sequential"] = FileNumbering::SEQUENTIAL;
	file_numbering.setConversionMap(file_numbering_map);
	registerPluginParameter(file_numbering);
	
};
Gnuplotter::~Gnuplotter() { if (gnuplot) delete gnuplot; Gnuplotter::instances--;};

void Gnuplotter::loadFromXML(const XMLNode xNode)
{
	AnalysisPlugin::loadFromXML(xNode);

	terminal="none";
	pointsize=0.5;
	persist=false;
	cell_opacity = 1.0;
	if( xNode.nChildNode("Terminal") ){
		getXMLAttribute(xNode,"Terminal/name",terminal);
		getXMLAttribute(xNode,"Terminal/pointsize",pointsize);
		getXMLAttribute(xNode,"Terminal/persist",persist);
		getXMLAttribute(xNode,"Terminal/opacity",cell_opacity);
		getXMLAttribute(xNode,"Terminal/size",terminal_size);
	}
	
	decorate = true;
	getXMLAttribute(xNode,"decorate",decorate);

	interpolation_pm3d = true;
	getXMLAttribute(xNode,"interpolation",interpolation_pm3d);
	
	log_plotfiles = false;
	getXMLAttribute(xNode,"log-commands",log_plotfiles);
	pipe_data = ! log_plotfiles;

	string plot_tag = "Plot";
	for (int i=0; i<xNode.nChildNode(plot_tag.c_str()); i++) {
		XMLNode xPlot =  xNode.getChildNode(plot_tag.c_str(),i);
		PlotSpec plot;
		getXMLAttribute(xPlot, "title", plot.title);
		for (int j=0; j<xPlot.nChildNode(); j++) {
			XMLNode xPlotChild = xPlot.getChildNode(j);
			string node_name = xPlotChild.getName();
			if (node_name == "Cells"){
				plot.cell_painter = shared_ptr<CellPainter>(new CellPainter());
				plot.cell_painter->loadFromXML(xPlotChild);
				plot.cells = true;
			}
			else if (node_name == "CellLabels"){
				plot.label_painter = shared_ptr<LabelPainter>(new LabelPainter());
				plot.label_painter->loadFromXML(xPlotChild);
				plot.labels = true;
			}
			else if (node_name == "CellArrows") {
				plot.arrows = true;
				plot.arrow_painter = shared_ptr<ArrowPainter>(new ArrowPainter());
				plot.arrow_painter->loadFromXML(xPlotChild);
			}
			else if (node_name == "VectorField") {
				plot.vectors = true;
				plot.vector_field_painter = shared_ptr<VectorFieldPainter>(new VectorFieldPainter());
				plot.vector_field_painter->loadFromXML(xPlotChild);
			}
			else if (node_name == "Field") {
				plot.field = true;
				plot.field_painter = shared_ptr<FieldPainter>(new FieldPainter());
				plot.field_painter->loadFromXML(xPlotChild);
			}
			
		}
		plots.push_back(plot);
	}
}

void Gnuplotter::init(const Scope* scope) {
	AnalysisPlugin::init(scope);

	try {
		gnuplot = new Gnuplot();
    
		for(int i=0;i<plots.size();i++) {
			if (plots[i].cells) {
				plots[i].cell_painter->init();
				registerInputSymbols( plots[i].cell_painter->getInputSymbols() );
			}
			if (plots[i].label_painter) {
				plots[i].label_painter->init();
				registerInputSymbols( plots[i].label_painter->getInputSymbols() );
			}
			if (plots[i].arrow_painter) {
				plots[i].arrow_painter->init();
				registerInputSymbols( plots[i].arrow_painter->getInputSymbols() );
			}
			if (plots[i].vector_field_painter) {
				plots[i].vector_field_painter->init();
				registerInputSymbols( plots[i].vector_field_painter->getInputSymbols() );
			}
			if (plots[i].field) {
				plots[i].field_painter->init();
				registerInputSymbols( plots[i].field_painter->getInputSymbols() );
			}
		}
	}
	catch (string e) {
		throw MorpheusException(e,this->stored_node);
	}
	catch (GnuplotException e) {
		throw MorpheusException(e.what(), this->stored_node);
	}
};

void Gnuplotter::analyse(double time) {
// 		binary=false; // override binary switch
	if (plots.empty())
		return;
	
	if ( ! pipe_data ) {
		for(int i=0;i<plots.size();i++) {
			stringstream sstr;
			sstr << "_";
			if (Gnuplotter::instances>1)
				sstr << instance_id << ",";
			sstr << i << "_";
			string plot_id = sstr.str();
			if (plots[i].cells) {
				sstr.str("");
				sstr << "cells" << plot_id << SIM::getTimeName() << ".log";
				plots[i].cells_data_file = sstr.str();
				
				sstr.str("");
				sstr << "membranes" << plot_id << SIM::getTimeName() << ".log";
				plots[i].membranes_data_file = sstr.str();
			}
			
			if (plots[i].labels) {
				sstr.str("");
				sstr << "labels" << plot_id << SIM::getTimeName() << ".log";
				plots[i].labels_data_file = sstr.str();
			}
			
			if (plots[i].arrows) {
				sstr.str("");
				sstr << "arrows" << plot_id << SIM::getTimeName() << ".log";
				plots[i].arrow_data_file = sstr.str();
			}

			if (plots[i].vectors) {
				sstr.str("");
				sstr << "vectors" << plot_id << SIM::getTimeName() << ".log";
				plots[i].vector_field_file = sstr.str();
			}

			if (plots[i].field) {
				// write_PDE_layer
				sstr.str("");
				sstr << "field" << plot_id << SIM::getTimeName() << ".log";
				plots[i].field_data_file = sstr.str();
			}
		}
	}


	// SETTING UP THE PLOT LAYOUT
	plotLayout plot_layout = getPlotLayout(plots.size(),decorate);
	
	map<string,TerminalSpec> term_defaults;
	TerminalSpec term;
	term.visual = true;
	term.vectorized = false;
	term.size = VINT(1600,800,0);
	term.font_size = 60;
	term.line_width = 2;
	term.font = "Verdana";
	term.extension = "";
	term_defaults[string("wxt")]  = term;
	
	term_defaults[string("aqua")] = term;
	
	term_defaults[string("qt")]   = term;
	
	term_defaults[string("x11")]  = term;
	
	term.visual = false;
	term.extension = "png";
	term_defaults[string("png")]  = term;
	
	term.extension = "jpg";
	term_defaults[string("jpeg")] = term;
	
	term.extension = "gif";
	term_defaults[string("gif")]  = term;

	term.vectorized = true;
	term.extension = "svg";
	term.font_size = 30;
	term_defaults[string("svg")]  = term;
	
	term.extension = "pdf";
	term.size = VINT(20,10,0);
	term.font_size = 40;
	term.line_width = 6;
	term_defaults[string("pdf")]  = term;
	
	term.extension = "eps";
	term.font = "Helvetica";
	term.font_size  = 40;
	term.line_width = 1.5;
	term_defaults[string("postscript")] = term;
	

	TerminalSpec terminal_spec;
	if (term_defaults.find(terminal) == term_defaults.end()) {
		terminal = "png";
	}
	terminal_spec = term_defaults[terminal];

	if (terminal_size.abs() != 0) {
		terminal_spec.size = terminal_size;
	}
	
	double term_aspect_ratio = terminal_spec.size.y / terminal_spec.size.x;
	if (term_aspect_ratio < plot_layout.layout_aspect_ratio  )  {
		// terminal is too wide for the layout
		terminal_spec.size.x = terminal_spec.size.y / plot_layout.layout_aspect_ratio;
	} else {
		// terminal is too high for the layout
		terminal_spec.size.y = terminal_spec.size.x * plot_layout.layout_aspect_ratio;
	}
	
	if ( ! terminal_spec.vectorized ) {
		terminal_spec.size.x = floor(terminal_spec.size.x);
		terminal_spec.size.y = floor(terminal_spec.size.y);
	}
	const static string outputDir = ".";

	string points_pm3d = " with pm3d ";

	

	/* Generate Gnuplot plot
		*/
	//plot->reset_plot();
	stringstream command;
	

	//    SETTING UP THE TERMINAL
	command << "unset multiplot; reset; \n";

	// assumes that the default terminal size has aspect ratio 2:1
	double terminal_scaling =  max(terminal_spec.size.x/(term_defaults[terminal].size.x * plot_layout.cols) , terminal_spec.size.y/(term_defaults[terminal].size.y * 2 * plot_layout.rows));
	if ( terminal_spec.vectorized) {
		terminal_spec.font_size  *= terminal_scaling;
		terminal_spec.line_width *= terminal_scaling;
	}
	else {
		terminal_spec.size.x     = floor(terminal_spec.size.x);
		terminal_spec.size.y     = floor(terminal_spec.size.y);
		terminal_spec.font_size  = pow( terminal_spec.font_size*terminal_scaling, 0.8 );
		terminal_spec.line_width = max(1.0, terminal_spec.line_width * terminal_scaling);
	}
	
	stringstream terminal_cmd;
	terminal_cmd << setprecision(2) << setiosflags(ios::fixed);

	terminal_cmd << "set term "<< terminal << (persist && terminal_spec.visual ? " persist ":" ") << " enhanced "
				 << "size " << terminal_spec.size.x << "," << terminal_spec.size.y << " "
				 << "font \"" << terminal_spec.font << "," << terminal_spec.font_size << "\" ";
				 
	if ( ! terminal_spec.visual )
		terminal_cmd << "lw " << terminal_spec.line_width << " ";

	if ( ! terminal_spec.vectorized && ! terminal_spec.visual)
		terminal_cmd << "truecolor ";

	if (terminal == "postscript")
		terminal_cmd << "color ";
	
	terminal_cmd << ";\n";
	
	command << terminal_cmd.str();

	if ( ! terminal_spec.visual ) {
		stringstream plot_file_name;
		plot_file_name << "plot";
		if (Gnuplotter::instances>1)
			plot_file_name << "-" << instance_id;
		if (file_numbering() == FileNumbering::TIME)
			plot_file_name << "_" << SIM::getTimeName();
		else {
			plot_file_name << setfill('0') << setw(4) <<  int(rint( time / timeStep()));
		}
		plot_file_name << "." << terminal_spec.extension;
		command << "set output '" << plot_file_name.str() << "'\n";

		cout << "GnuPlotter: Saving " <<  plot_file_name.str() << endl;
	}
	

	/*
		DEFAULT ARROW STYLES
	*/

	command << "set multiplot;\n"
			<< "unset tics;\n"
			<< "set datafile missing \"NaN\";\n"
			<< "set view map;\n"
			<< "set style arrow 1 head   filled   size screen 0.015,15,45  lc 0 lw 1.5;\n"
			<< "set style arrow 2 head   nofilled size screen 0.015,15,135 lc 0 lw 1.5;\n"
			<< "set style arrow 3 head   filled   size screen 0.015,15,45  lc 0 lw 1.5;\n"
			<< "set style arrow 4 head   filled   size screen 0.015,15,90  lc 0 lw 1.5;\n"
			<< "set style arrow 5 head   nofilled size screen 0.015,15,135 lc 0 lw 1.5 ;\n"
			<< "set style arrow 6 heads  filled   size screen 0.015,15,135 lc 0 lw 1.5;\n"
			<< "set style arrow 7 heads  nofilled size screen 0.004,90,90  lc 0 lw 1.5;\n"
			<< "set style arrow 8 nohead nofilled                          lc 0 lw 1.5;\n";
			
	/*
		MULTI PLOTS
	*/
	for (uint i=0; i<plots.size(); i++ ) {
		command << "set lmargin at screen " << plot_layout.plots[i].left << ";\n"
				<< "set rmargin at screen " << plot_layout.plots[i].right << ";\n"
				<< "set tmargin at screen " << plot_layout.plots[i].top << ";\n"
				<< "set bmargin at screen " << plot_layout.plots[i].bottom << ";\n";
		command << "set style fill transparent solid 1;\n";

		
		/*
			PLOT PDE SURFACE + ISOLINES
		*/

		if (plots[i].field) {
			if (!interpolation_pm3d)
				command << "set pm3d corners2color c4;\n";
			if (! decorate)
				command << "unset colorbox;\n" << "unset title;\n";
			else if (plots[i].cells)
				command << "unset colorbox;\n" << "unset title;\n" << "unset xlabel;\n";			
			else{
				string plot_title;
				if ( ! plots[i].title.empty())
					plot_title = plots[i].title;
				else 
					plot_title = plots[i].field_painter->getDescription();
				string escape_char = "_^";
				size_t pos = plot_title.find_first_of(escape_char);
				while (pos != string::npos) {
// 					plot_title.replace(pos,1,string("\\") + plot_title[pos]);
// 					pos+=2;
					plot_title[pos] = ' ';
					pos = plot_title.find_first_of(escape_char,pos);
				}
				command << "set colorbox; set cbtics\n";
				command << "set xlabel '" << time << " "/* << SIM::getTimeScaleUnit()*/ << "' offset 0,2 ;\n";
				command << "set title \"" << plot_title << "\" offset 0,-0.5 ;\n";
			}

			command << "set cbrange " << plots[i].field_painter->getValueRange() << ";\n";
			
			if (plots[i].field_painter->getSurface() ) {
				command << "unset contour;\n"
						<< "set surface;\n";
						
				command << plots[i].field_painter->getColorMap();
						
				// set background pattern to ease data / no-data discrimination
				//command << "set object 1 rectangle from graph 0, graph 0 to graph 1, graph 1 behind fc rgb 'light-grey' fs pattern 2\n";

				command << "splot [][]" << plots[i].field_painter->getValueRange();

				if (pipe_data)
					command << " '-' ";
				else
					command << " \'"<< outputDir << "/" << plots[i].field_data_file.c_str() << "' ";
				command << " matrix " << points_pm3d << " pal not\n";
				
				if (pipe_data) {
					plots[i].field_painter->plotData(command);
					command << "\ne" << endl;
				}
			}
			
			if (plots[i].field_painter->getIsolines() ) {
				command << "set contour base;\n"
						<< "set cntrparam linear;\n"
						<< "set cntrparam levels auto "<< plots[i].field_painter->getIsolines() <<" ;\n"
						<< "unset surface;\n"
						<< "unset clabel;\n"
						<< "splot [][][]";
				if (pipe_data)
					command << " '-' ";
				else
					command << " '" << outputDir << "/" << plots[i].field_data_file.c_str() << "' ";
				command << " matrix w l lw 1 lc rgb \"red\"  not;\n" << endl;
				if (pipe_data) {
					plots[i].field_painter->plotData(command);
					command << "\ne"<< endl;
				}
			}
				
			// write_PDE_layer data to file ...
			if (!pipe_data) {
				ofstream file;
				file.open(plots[i].field_data_file.c_str(), ios::out);
				plots[i].field_painter->plotData(file);
				file.close();
			}
		}

		if ( plots[i].vectors ) {
			command << "unset colorbox;\n";
			if (plots[i].cells)
				command << "unset title;\n" << "unset xlabel;\n";
			else{
				string plot_title;
				if ( ! plots[i].title.empty())
					plot_title = plots[i].title;
				else 
					plot_title = plots[i].vector_field_painter->getDescription();
				string escape_char = "_^";
				size_t pos = plot_title.find_first_of(escape_char);
				while (pos != string::npos) {
// 					plot_title.replace(pos,1,string("\\") + plot_title[pos]);
// 					pos+=2;
					plot_title[pos] = ' ';
					pos = plot_title.find_first_of(escape_char,pos);
				}
				command << "set xlabel '" << time << " " /*<< SIM::getTimeScaleUnit() */<< "' offset 0,0 ;\n";
				command << "set title \"" << plot_title << "\" offset 0,-0.5 ;\n";
			}

			command << "plot ";
			if(pipe_data)
				command << "'-' ";
			else{
				command << "'" << outputDir << "/" << plots[i].vector_field_file << "' ";
			}
			command << " w vectors  arrowstyle " << plots[i].vector_field_painter->getStyle() << /*" lc rgb \'" << plots[i].vector_field_painter->getColor() <<  "\'" << */ " notitle;\n";

			if (pipe_data) {
				plots[i].vector_field_painter->plotData(command);
				command << "\ne" << endl;

			}
			else {
				ofstream out_file;
				out_file.open(plots[i].vector_field_file.c_str());
				plots[i].vector_field_painter->plotData(out_file);
				out_file.close();
			}
		}
		
		/*
			PLOTS CELLS (types, ids, properties, membrane properties)
		*/
		if (plots[i].cells) {
			
			vector<CellPainter::CellBoundarySpec> cell_boundary_data = plots[i].cell_painter->getCellBoundaries();
			bool using_splot = CellPainter::boundary_cell_wise == plots[i].cell_painter->getDataLayout();
			stringstream cell_pixel_data;
			if ( plots[i].cell_painter->getDataLayout() != CellPainter::boundary_cell_wise ) {
				 plots[i].cell_painter->writeCellLayer(cell_pixel_data);
			}
			
			command << plots[i].cell_painter->getPaletteCmd() << endl;
			command << "unset contour;\n";
			
			if (cell_opacity < 1.0)
				command << "set style fill transparent solid " << cell_opacity << " noborder;\n";
			//command << "set cbrange ["<< plots[i].cell_painter->getMinVal() << ":"<< plots[i].cell_painter->getMaxVal() << "]" << endl;

			if ( ! decorate)
				command << "unset colorbox;\n";
			else {
				string plot_title;
				if ( ! plots[i].title.empty())
					plot_title = plots[i].title;
				else 
					plot_title = plots[i].cell_painter->getDescription();
				string escape_char = "_^";
				size_t pos = plot_title.find_first_of(escape_char);
				while (pos != string::npos) {
// 					plot_title.replace(pos,1,string("\\") + plot_title[pos]);
// 					pos+=2;
					plot_title[pos] = ' ';
					pos = plot_title.find_first_of(escape_char,pos);
				}
				command << "set colorbox; set cbtics; set clabel;\n";
				
				command << "set xlabel '" << time << " " /*<< SIM::getTimeScaleUnit() */ << "' offset 0," << (using_splot ? "0.1" : "2") << ";\n";

				if (plots[i].cell_painter->getSlice() > 0)
					command << "set title '" << plot_title << " (z-slice: " << plots[i].cell_painter->getSlice() << ")' offset 0,-0.5;\n";
				else
					command << "set title '" << plot_title << "' offset 0,-0.5;\n";
			}
			
			
			if (CellPainter::boundary_cell_wise == plots[i].cell_painter->getDataLayout()) {
				
				VDOUBLE s = PlotSpec::size();
				command << "plot [0.2:" << s.x+0.2 << "][0:" << s.y << "] ";
				uint current_index = 0;
				for (uint p=0; p<cell_boundary_data.size(); p++) {
					if (p>0)
						command << ",\\\n'' ";
					else if ( pipe_data )
						command << "'-' ";
					else 
						command << "'" << outputDir << "/" << plots[i].membranes_data_file << "' "; 
					if ( ! isnan(cell_boundary_data[p].value) && cell_boundary_data[p].value != CellPainter::getTransparentValue()) {
						if (!pipe_data )
							command << " index " << current_index << ":" << current_index + cell_boundary_data[p].polygons.size()-1;
						command << " us 1:2 w filledc c lt pal cb "  << cell_boundary_data[p].value << " notitle";
						command << ",\\\n''";
					}
					if (!pipe_data)
						command << " index " << current_index << ":" << current_index + cell_boundary_data[p].polygons.size()-1;
					command << " us 1:2 w l lt -1 notitle";
					current_index +=cell_boundary_data[p].polygons.size();
				}
				
                // assert that the plot command is valid ...
                if (cell_boundary_data.size() == 0) {
					command << " 0 notitle";
				}
			}
			else {
				
				string layout = "";
				if (plots[i].cell_painter->getDataLayout() == CellPainter::ascii_matrix)
					layout = "matrix";
				// TODO Try to use plot with image here to get rid of the pixel size issue
				command << "splot [0:"<< PlotSpec::size().x << "][0:" << PlotSpec::size().y << "][] ";
				if (pipe_data) 
					command << " '-' ";
				else
					command << " '" << outputDir << "/" << plots[i].cells_data_file << "' " ;
				command << layout << " using (1+$1):(1+$2):3 ";
				command << " w p pt 5 ps " << pointsize << " pal not";
				if (pipe_data) 
					command << ", '-' ";
				else 
					command << ", '" << outputDir << "/" << plots[i].membranes_data_file << "' ";
				command << "using 1:2:(0) w l ls -1 notitle";
			}

			if ( plots[i].labels ){
				if(pipe_data)
					command << ", '-' ";
				else{
					command << ", '" << outputDir << "/" << plots[i].labels_data_file << "' ";
				}
				command << " using (1.0+$1):(1.0+$2)"<< (using_splot ? "" : ":(0)") << ":3 with labels font \"Helvetica,"
						<< plots[i].label_painter->fontsize() << "\" textcolor rgb \"" << plots[i].label_painter->fontcolor().c_str() << "\" notitle";
			}

			if ( plots[i].arrows ){
				if(pipe_data)
					command << ", '-' ";
				else{
					command << ", '" << outputDir << "/" << plots[i].arrow_data_file << "' ";
				}
				command << " u (0.5+$1):(0.5+$2)"<< (using_splot ? "" : ":(0)") << ":3:4" <<  (using_splot ? "" : ":(0)")  << " ";
				command << "w vectors arrowstyle " << plots[i].arrow_painter->getStyle() << " notitle";
			}
			
			command << "\n" << endl;
			
			
			// writing data ...
			// put cell boundaries ...
			if ( pipe_data ) {
				if (CellPainter::boundary_cell_wise == plots[i].cell_painter->getDataLayout()) {
					for (uint o=0; o<cell_boundary_data.size(); o++) {
						for (uint p=0; p<cell_boundary_data[o].polygons.size(); p++) {
							for (uint q=0; q < cell_boundary_data[o].polygons[p].size(); q++) {
								command << cell_boundary_data[o].polygons[p][q].x+1 << "\t" << cell_boundary_data[o].polygons[p][q].y+1 << "\n";
							}
							command << "\n\n";
						}
						command << "e\n";
						if ( ! isnan(cell_boundary_data[o].value) && cell_boundary_data[o].value != CellPainter::getTransparentValue()) {
							for (uint p=0; p<cell_boundary_data[o].polygons.size(); p++) {
								for (uint q=0; q < cell_boundary_data[o].polygons[p].size(); q++) {
									command << cell_boundary_data[o].polygons[p][q].x+1 << "\t" << cell_boundary_data[o].polygons[p][q].y+1 << "\n";
								}
								command << "\n\n";
							}
							command << "e\n";
						}
					}
				}
				else {
					// put cell point matrix ...
// 					plots[i].cell_painter->writeCellLayer(command);
					command << cell_pixel_data.str();
					command << "\ne\n";
					for (uint o=0; o<cell_boundary_data.size(); o++) {
						for (uint p=0; p<cell_boundary_data[o].polygons.size(); p++) {
							for (uint q=0; q < cell_boundary_data[o].polygons[p].size(); q++) {
								command << cell_boundary_data[o].polygons[p][q].x+1 << "\t" << cell_boundary_data[o].polygons[p][q].y+1 << "\n";
							}
							command << "\n\n";
						}
					}
					command << "e\n";
				}
			}
			else {
				
				ofstream out_file;
				if ( plots[i].cell_painter->getDataLayout() != CellPainter::boundary_cell_wise ) {
					// put cell point matrix ...
					out_file.open(plots[i].cells_data_file.c_str());
					out_file << cell_pixel_data.str();
					out_file.close();
				}
				// Cell Boundary polygons
				out_file.open(plots[i].membranes_data_file.c_str());
				for (uint o=0; o<cell_boundary_data.size(); o++) {
					for (uint p=0; p<cell_boundary_data[o].polygons.size(); p++) {
						for (uint q=0; q < cell_boundary_data[o].polygons[p].size(); q++) {
							out_file << cell_boundary_data[o].polygons[p][q].x +1 << "\t" << cell_boundary_data[o].polygons[p][q].y+1 << "\n";
						}
						out_file << "\n\n";
					}
				}
				out_file.close();
			}
			if (plots[i].labels ) {
				if (pipe_data) {
					plots[i].label_painter->plotData(command);
					command << "\ne" << endl;
				}
				else{
					ofstream out_file;
					out_file.open(plots[i].labels_data_file.c_str());
					plots[i].label_painter->plotData(out_file);
					out_file.close();
				}
			}
			if (plots[i].arrows ) {
				if (pipe_data) {
					plots[i].arrow_painter->plotData(command);
					command << "\ne" << endl;
					
				}
				else {
					ofstream out_file;
					out_file.open(plots[i].arrow_data_file.c_str());
					plots[i].arrow_painter->plotData(out_file);
					out_file.close();
				}
			}
		}
	}

	command << "unset multiplot;" << endl;
	command << "unset output;" 	  << endl;
	//command << "system(\"gnuplot --version\");" << endl;
	//cout << (log_plotfiles?"log_plotfiles = true\n":"log_plotfiles = false \n") << command.str() << endl;
    if( log_plotfiles ){
		ofstream command_log;
		if (file_numbering() == FileNumbering::TIME)
            command_log.open((string("gnuplot_commands_")+ SIM::getTimeName() + ".gp").c_str(),ios_base::app);
		else 
            command_log.open((string("gnuplot_commands_")+ to_str(time/timeStep(),4) + ".gp").c_str(),ios_base::app);
		command_log << command.str();
		command_log.close();
	}

	gnuplot->cmd(command.str());
}

Gnuplotter::plotLayout Gnuplotter::getPlotLayout( uint plot_count, bool border )
{
	plotLayout layout;
	plotPos p = {0,1,1,0};

	VDOUBLE lattice_size = PlotSpec::size();
	VDOUBLE plot_size = lattice_size;
	double x_margin = 0;
	double y_margin = 0;
	if (border) {
		x_margin = 0.2 * max(lattice_size.x, (lattice_size.y + lattice_size.x)/2);
		plot_size.x = lattice_size.x + x_margin;
		
		y_margin = 0.15 * max(lattice_size.y, (lattice_size.y + lattice_size.x)/2);
		plot_size.y = lattice_size.y + y_margin;
	}
	layout.plot_aspect_ratio = (plot_size.y / plot_size.x);
	layout.rows = max(1,int(floor(sqrt(plot_count/layout.plot_aspect_ratio))));
	layout.cols = ceil(double(plot_count) / double(layout.rows));
	layout.layout_aspect_ratio = (layout.plot_aspect_ratio * layout.rows) / layout.cols;

	uint x_panel = 0;
	uint y_panel = 0;
	
	for(int i=0; i<plot_count; i++){  
	  
		p.left   = x_panel * plot_size.x;
		p.right  = (x_panel+1) * plot_size.x;
		p.top    = (layout.rows - y_panel) * plot_size.y;
		p.bottom = (layout.rows-y_panel-1) * plot_size.y;
  
		if (border) {
			p.left   += x_margin * 0.1;
			p.right  -= x_margin * 0.9;
			p.top    -= y_margin * 0.55;
			p.bottom += y_margin * 0.45;
		}

		p.left /= (layout.cols * plot_size.x);
		p.right /= (layout.cols * plot_size.x);
		p.top /= (layout.rows * plot_size.y);
		p.bottom /= (layout.rows * plot_size.y);
		
		layout.plots.push_back(p);

		// set plot dimensions rowsfirst
		x_panel++;
		if(x_panel >= layout.cols){
			y_panel++;
			x_panel = 0;
		}
	    
	} 
	
	// plot size shrinkage

	return layout;
}

