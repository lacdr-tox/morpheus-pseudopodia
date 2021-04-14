#include "gnuplotter.h"
#include "focusrange.h"

using namespace SIM;



const float CellPainter::transparency_value = std::numeric_limits<float>::quiet_NaN();

Gnuplotter::PlotSpec::PlotSpec() : 
field(false), cells(false), labels(false), arrows(false), vectors(false), z_slice(0), title("") {};

VDOUBLE Gnuplotter::PlotSpec::size()
{
	
	VDOUBLE latticeDim = SIM::lattice().to_orth(SIM::lattice().size());
	
	if (SIM::lattice().getStructure() == Lattice::hexagonal) {
		if (SIM::lattice().get_boundary_type(Boundary::mx) == Boundary::periodic) {
			latticeDim.x = SIM::lattice().size().x;
		}
	}
	
	return latticeDim;
}

VDOUBLE Gnuplotter::PlotSpec::view_oversize()
{
	
	VDOUBLE size;
	
	if (SIM::lattice().getStructure() == Lattice::hexagonal) {
		size.y = 0.3 ;
		if (SIM::lattice().get_boundary_type(Boundary::mx) == Boundary::periodic) {
			size.x = 0.5;
		}
	}
	
// 	if (SIM::lattice().size().y<3) {
// 		size.y = max(1.0,0.1*size.x) - size.y;
// 	}
	
	return size;
}

ArrowPainter::ArrowPainter() {
	arrow.setXMLPath("orientation");
	arrow.allowPartialSpec();
	map<string, bool> centering_map = {{"midpoint",true}, {"origin",false}};
	centering.setConversionMap(centering_map);
	centering.setXMLPath("center");
	centering.setDefault("midpoint");
}

void ArrowPainter::loadFromXML(const XMLNode node, const Scope * scope)
{
	arrow.loadFromXML(node, scope);
	centering.loadFromXML(node, scope);
	style = 3;
	getXMLAttribute(node,"style",style);
}

int ArrowPainter::getStyle() { return style; }

void ArrowPainter::init(const Scope* scope, int slice)
{
	arrow.init();
	centering.init();
	z_slice = slice;
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
	
// 	out.precision(2);
	auto celltypes = CPM::getCellTypes();
	for (uint i=0; i < celltypes.size(); i++){
		auto ct = celltypes[i].lock();
		if (ct->isMedium())
			continue;
		
// 		vector<CPM::CELL_ID> cells = ct->getCellIDs();

// 		for (uint c=0; c< cells.size(); c++){
// 			SymbolFocus f(cells[c]);
		FocusRange range(Granularity::Cell, {{FocusRangeAxis::Z, z_slice}, {FocusRangeAxis::CellType, ct->getID()}});
		for (const SymbolFocus& focus : range){
			try {
				VDOUBLE a = arrow(focus);
				VDOUBLE center = focus.cell().getCenter();
				lattice.orth_resolve (center);
				
				if (! (a.x == 0 && a.y==0) ) {
					out << center.x - (centering() ? 0.5*a.x : 0) << "\t" <<  center.y - (centering() ? 0.5*a.y : 0) << "\t" << a.x << "\t" << a.y << endl;
				}
			}
			catch (const SymbolError &e) {
			}
		}
	}
}

void FieldPainter::loadFromXML(const XMLNode node, const Scope * scope)
{
	field_value.setXMLPath("symbol-ref");
	field_value.loadFromXML(node, scope);
	
	coarsening.setXMLPath("coarsening");
	coarsening.setDefault("1");
	coarsening.loadFromXML(node, scope);
	if (coarsening()<0)
		throw MorpheusException("Invalid coarsening value.", node);
	
	min_value.setXMLPath("min");
	min_value.loadFromXML(node, scope);
	
	max_value.setXMLPath("max");
	max_value.loadFromXML(node, scope);
	
	isolines.setXMLPath("isolines");
	isolines.loadFromXML(node, scope);
	
	surface.setXMLPath("surface");
	surface.loadFromXML(node, scope);
	
// 	z_slice.setXMLPath("slice");
// 	z_slice.setDefault("0");
// 	z_slice.loadFromXML(node, scope);
// 	if (z_slice()<0 || z_slice() >= SIM::lattice().size().z)
// 		throw MorpheusException("Invalid z_slice.", node);
		
	
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

void FieldPainter::init(const Scope* scope, int slice)
{
	z_slice = slice;
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

int FieldPainter::getCoarsening() const
{
	return coarsening();
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
	bool is_hexagonal = SIM::lattice().getStructure() == Lattice::hexagonal;

	VINT size = SIM::lattice().size();
	VINT out_size = size / coarsening();
	
	VINT view_size(Gnuplotter::PlotSpec::size());
	VDOUBLE view_out_size = VDOUBLE(view_size) / double(coarsening());
	
	string xsep(" "), ysep("\n");
		
	VINT out_pos;
	VINT pos(0,0, z_slice);
	
	valarray<float> out_data(out_size.x), out_data2(out_size.x);
	valarray<float> out_data_count(out_size.x);
	
	if (out_size.y < 2) out_size.y = 2;
	for (out_pos.y=0; out_pos.y<out_size.y; out_pos.y+=1) {
		
		pos.y = min( size.y-1, out_pos.y*coarsening());
			
		if ( coarsening()>1) {
			// reset the output buffer
			out_data = 0;
			out_data_count = 0;
			// accumulate data
			for (int y_c=0; y_c<coarsening(); y_c++, pos.y++ ) {
				if (pos.y>=size.y) pos.y = size.y-1;
				int x_c=0;
				out_pos.x=0;
				for ( pos.x=0; pos.x<size.x; pos.x++, x_c++) {
					if (x_c==coarsening()) {
						x_c=0;
						out_pos.x++;
					}
					out_data[out_pos.x] += float(field_value.get(SymbolFocus(pos)));
					out_data_count[out_pos.x] ++;
				}
			}
			// averaging data
			out_data /= out_data_count;
		}
		else {
			// copy & convert the raw data
			for ( pos.x=0; pos.x<size.x; pos.x++) {
				out_data[pos.x] = float(field_value.get(SymbolFocus(pos)));
			}

		}
		
		// Cropping data
		if (min_value.isDefined()) {
			float fmin_value = min_value.get(SymbolFocus::global);
			for (uint i = 0; i< out_data.size(); i++) {
				if (out_data[i] < fmin_value) {
					out_data[i] = fmin_value;
				}
			}
		}
		if (max_value.isDefined()) {
			float fmax_value = float(max_value.get(SymbolFocus::global));
			for (uint i = 0; i<out_data.size(); i++) {
				if (out_data[i] > fmax_value) {
					out_data[i] = fmax_value;
				}
			}
		}
		
		if (is_hexagonal) {
			for (int x = 0; x < view_out_size.x*2; ++x) {
				int x_l = int(pmod((0.5*x - 0.5*out_pos.y)+0.01, view_out_size.x));
				if (x_l<0 || x_l>= out_data.size())
					out << "Nan" << "\t";
				else
					out << out_data[x_l] << "\t";
			}
		}
		else {
			out << out_data[0];
			for (uint i=1; i<out_data.size() ;i++) {
				out << xsep << out_data[i];
			}
		}
		
		out << ysep;
	}
}


void VectorFieldPainter::loadFromXML(const XMLNode node, const Scope* scope)
{
	value.setXMLPath("value");
	value.loadFromXML(node, scope);
	map<string, bool> centering_map = {{"midpoint",true}, {"origin",false}};
	centering.setConversionMap(centering_map);
	centering.setXMLPath("center");
	centering.setDefault("midpoint");
	centering.loadFromXML(node, scope);
	
// 	if ( ! getXMLAttribute(node, "x-symbol-ref", x_symbol.name)) { cerr << "Undefined x-symbol-ref in GnuPlot -> VectorField"; exit(-1);}
// 	if ( ! getXMLAttribute(node, "y-symbol-ref", y_symbol.name)) { cerr << "Undefined y-symbol-ref in GnuPlot -> VectorField"; exit(-1);};
	style = 1;
	getXMLAttribute(node, "style", style);
	color = "black";
	getXMLAttribute(node, "color", color);
	coarsening = 1;
	getXMLAttribute(node, "coarsening",coarsening);
}

void VectorFieldPainter::init(const Scope* scope, int slice)
{
	value.init();
	centering.init();
	z_slice = slice;
}

set< SymbolDependency > VectorFieldPainter::getInputSymbols() const
{
	return value.getDependSymbols();
}


void VectorFieldPainter::plotData(ostream& out)
{
	auto& lattice = SIM::lattice();
	VINT pos(0, 0, z_slice);
	VINT size = lattice.size();
	for (pos.y=coarsening/2; pos.y<size.y; pos.y+=coarsening) {
		for (pos.x=coarsening/2; pos.x<size.x; pos.x+=coarsening) {
            VDOUBLE arrow = value.get(SymbolFocus(pos));
			VDOUBLE opos = lattice.to_orth(pos);
			if (lattice.getStructure() == Lattice::hexagonal) {
				lattice.orth_resolve(opos);
			}
			out << opos.x - (centering() ? 0.5*arrow.x : 0) << "\t" << opos.y - (centering() ? 0.5*arrow.y : 0)  << "\t" << arrow.x << "\t" << arrow.y << "\n";
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
	z_slice = -1;
	value.setXMLPath("value");
	value.setUndefVal("");
}



void LabelPainter::loadFromXML(const XMLNode node, const Scope* scope)
{
	value.loadFromXML(node, scope);

	getXMLAttribute(node,"fontcolor",_fontcolor);
	getXMLAttribute(node,"fontsize",_fontsize);
	
	int prec=0;
	getXMLAttribute(node,"precision", prec);
	if (prec>=0) {
		value.setPrecision(prec);
	}
	bool scientific=false;
	getXMLAttribute(node,"scientific", scientific);
	if  (scientific) 
		value.setFormat(ios_base::scientific);
	else
		value.setFormat(ios_base::fixed);
	
}



void LabelPainter::init(const Scope* scope, int slice)
{
	value.init();
	this->z_slice = slice;
}


const string& LabelPainter::getDescription() const {
	return value.description();
}

set<SymbolDependency> LabelPainter::getInputSymbols() const
{
	return value.getDependSymbols();
}


void LabelPainter::plotData(ostream& out)
{
	const Lattice& lattice = SIM::lattice();
	bool  is_hexagonal = (lattice.getStructure() == Lattice::hexagonal);
	VDOUBLE orth_lattice_size = Gnuplotter::PlotSpec::size();
	if (is_hexagonal) {
		orth_lattice_size.x-=1;
	}
	
	auto celltypes = CPM::getCellTypes();
	for (uint i=0; i < celltypes.size(); i++) {
		auto ct = celltypes[i].lock();
		if ( ct->isMedium() ) 
			continue;
		
// 		vector<CPM::CELL_ID> cells = ct->getCellIDs();
		FocusRange range(Granularity::Cell, {{FocusRangeAxis::Z, z_slice}, {FocusRangeAxis::CellType, ct->getID()}});
		for (const SymbolFocus& focus : range){
			
			string val = value.get(focus);
			if ( ! val.empty() ) {
				VDOUBLE centerl = focus.cell().getCenterL();
				lattice.resolve (centerl);
				VDOUBLE center = ( lattice.to_orth(centerl) + (CPM::isEnabled() ? VDOUBLE(1.0,1.0,0): VDOUBLE(0.1,0.0,0.0) ) ) % orth_lattice_size;
				
				out << center.x << "\t" << center.y << "\t \"" << val << "\"\n";
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
	fill_opacity = 1.0;
	z_level = 0;
	symbol.setXMLPath("value");
	symbol.setDefault("cell.type");
	symbol.setPartialSpecDefault(transparency_value);
}

CellPainter::~CellPainter() { }

void CellPainter::loadFromXML(const XMLNode node, const Scope* scope)
{
	symbol.loadFromXML(node,scope);	
	
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
	external_range = external_range_min && external_range_max;
	
	getXMLAttribute(node,"opacity", fill_opacity);
	fill_opacity = cpp17::clamp(fill_opacity, 0.0, 1.0);

	if (string(node.getName()) == "Cells") {
		
// 		getXMLAttribute(node,"slice",z_level);
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
// 	else if (is_hexagonal) 
// 		data_layout = point_wise;
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

void CellPainter::init(const Scope* scope, int slice) {
	symbol.init();
	z_level = slice;
	cpm_layer = CPM::getLayer();
	
	if(z_level > cpm_layer->size().z || z_level < 0 ){
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
		cmd << "set cbrange [" << color_map.begin()->first << ":" << color_map.rbegin()->first << "];\n";
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
#ifdef HAVE_SUPERCELLS
		bool is_super_cell = ( dynamic_pointer_cast<const SuperCT>(ct) != nullptr );
#else
		bool is_super_cell = false;
#endif
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

void CellPainter::setDataLayout(CellPainter::DataLayout layout) {
	data_layout = layout;
}

const string& CellPainter::getDescription() const {
	return symbol.description();
}

set<SymbolDependency> CellPainter::getInputSymbols() const
{
	return { symbol.accessor() };
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
				if (symbol.granularity() == Granularity::MembraneNode && ! CPM::isSurface(p)) {
					
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
					out << pmod(x+double(y)/2,double(size.x)) << "\t" << double(y)*sin(M_PI/3);
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
		auto view_size = Gnuplotter::PlotSpec::size();
		for (int y = 0; y < max(2,size.y); ++y) {
			int y_l = min(y,size.y-1);
			if (is_hexagonal) {
				if (SIM::lattice().get_boundary_type(Boundary::mx) == Boundary::periodic) {
					for (int x = 0; x < view_size.x*2+1; ++x) {
						if (y_l%2==0 && x==size.x*2) {
							out << "Nan" << "\t";
						}
						else if (y_l%2==1 && x==0) {
							out << "Nan" << "\t";
						}
						else {
							int x_l = int(pmod(0.5*x - 0.5*y_l+0.01,double(size.x)));
							if (isnan(view[y_l][x_l]) || view[y_l][x_l] == transparency_value)
								out << "Nan" << "\t";
							else
								out << view[y_l][x_l] << "\t";
						}
					}
				}
				else {
					for (int x = 0; x < view_size.x*2+1; ++x) {
						int x_l = 0.5*x - 0.5*y_l+0.01;
						if (x_l<0 || x_l>=size.x)
							out << "Nan" << "\t";
						else if (isnan(view[y_l][x_l]) || view[y_l][x_l] == transparency_value)
							out << "Nan" << "\t";
						else
							out << view[y_l][x_l] << "\t";
					}
				}
			}
			else {
				for (int x = 0; x < size.x; ++x) {
					if (isnan(view[y_l][x]) || view[y_l][x] == transparency_value)
						out << "NaN" << "\t";
					else
						out << view[y_l][x] << "\t";
				}
			}
			out << "\n";
		}
	}
}


vector<CellPainter::boundarySegment> CellPainter::getBoundarySnippets(const Cell::Nodes& node_list, bool (*comp)(const CPM::STATE& a, const CPM::STATE& b))
{
	VDOUBLE view_size = Gnuplotter::PlotSpec::size();
	view_size -= VDOUBLE(0.01,0.01,0);
	auto lattice = SIM::getLattice();
	bool is_linear = lattice->getStructure() == Lattice::linear;
	
	// we assume that the neighbors are sorted either clockwise or anti-clockwise.
	// we could also add a sorting step after the filtering of nodes belonging to the plane
	vector<VINT> neighbors = SIM::lattice().getNeighborhoodByOrder(1).neighbors();
	if( is_linear ) {
		neighbors.resize(4);
		neighbors[0] = VINT( 1, 0, 0);
		neighbors[1] = VINT( 0, 1, 0);
		neighbors[2] = VINT(-1, 0, 0);
		neighbors[3] = VINT( 0,-1, 0);
	}

	vector<VINT> plane_neighbors;
	vector<VDOUBLE> orth_neighbors;
	for  ( vector< VINT >::iterator n = neighbors.begin(); n!=neighbors.end();n++) {
		if ( n->z == 0) {
			plane_neighbors.push_back(*n);
			orth_neighbors.push_back(lattice->to_orth(*n));
		}
	}

	// start and end  point of the line snippet towards neighbor i relative to the node center
	vector<VDOUBLE> snippet_begin_offset; 
	vector<VDOUBLE> snippet_end_offset;
	for  ( uint i=0; i<orth_neighbors.size(); i++) {
			// those offsets serve all lattice structures
			snippet_begin_offset.push_back((orth_neighbors[i] + orth_neighbors[pmod(int(i)+1,orth_neighbors.size())]) / plane_neighbors.size() * 2.0);
			snippet_end_offset.push_back((orth_neighbors[i] + orth_neighbors[pmod(int(i)-1,orth_neighbors.size())]) / plane_neighbors.size() * 2.0);
	}

	vector<boundarySegment> snippets;

	for ( auto it = node_list.begin(); it != node_list.end(); it++ )
	{
		VINT pos = (*it);
		lattice->resolve(pos);
		if (pos.z != z_level) continue;
		
		// get the proper position in the drawing area
		VDOUBLE orth_pos  = cpm_layer->lattice().to_orth(pos);
		cpm_layer->lattice().orth_resolve(orth_pos);

		const CPM::STATE& node = CPM::getNode(pos);
		for (uint i = 0; i < plane_neighbors.size(); i++)
		{
			if ( is_linear && (pos + plane_neighbors[i]).y != 0) {
				// virtual 1d neighbor
			}
			else {
				const CPM::STATE& neighborNode = CPM::getNode((pos + plane_neighbors[i]));

				if ( comp (neighborNode, node) ) {
					VDOUBLE orth_nei = ((orth_pos + orth_neighbors[i]));
					if ((orth_nei.x>=0) &&
						(orth_nei.x<=view_size.x) &&
						(orth_nei.y>=0 ) &&
						(orth_nei.y<=view_size.y) &&
						(orth_nei.z>=0) &&
						(orth_nei.z<=view_size.z))
					{
						// neighbor is outside of the drawing area
						continue;
					}
				}
			}
			// create the edge
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
					for (current = snippets.begin(); current != snippets.end(); current++) {
						cout << "("<< current->pos1 << "; " << current->pos2 << "),";
					}
					cout << endl;
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
	
	terminal.setXMLPath("Terminal/name");
	
	map<string, Terminal> term_map;
	term_map["screen"] = Terminal::SCREEN;
	term_map["windows"] = Terminal::SCREEN;
	term_map["wxt"] = Terminal::SCREEN;
	term_map["aqua"] = Terminal::SCREEN;
	term_map["qt"] = Terminal::SCREEN;
	term_map["x11"] = Terminal::SCREEN;
	term_map["png"] = Terminal::PNG;
	term_map["jpg"] = Terminal::JPG;
	term_map["jpeg"] = Terminal::JPG;
	term_map["svg"] = Terminal::SVG;
	term_map["pdf"] = Terminal::PDF;
	term_map["eps"] = Terminal::EPS;
	term_map["postscript"] = Terminal::EPS;
	terminal.setConversionMap(term_map);
	terminal.setDefault("png");
	registerPluginParameter(terminal);
	
	terminal_size.setXMLPath("Terminal/size");
	registerPluginParameter(terminal_size);
	
// 	pointsize.setXMLPath("Terminal/pointsize");
// 	pointsize.setDefault("0.5");
// 	registerPluginParameter(pointsize);
	
// 	cell_opacity.setXMLPath("Terminal/opacity");
// 	registerPluginParameter(cell_opacity);
	
	
	
	TerminalSpec term;
	term.name = "wxt"; // Will get overridden during init
	term.visual = true;
	term.vectorized = false;
	term.size = VINT(1200,600,0);
	term.font_size = 60;
	term.line_width = 2;
	term.font = "Verdana";
	term.extension = "";
	
	terminal_defaults[Terminal::SCREEN]  = term;
	
	term.visual = false;
	term.extension = "png";
	
	term.name = "pngcairo";
	terminal_defaults[Terminal::PNG]  = term;
	
	term.extension = "jpg";
	term.name = "jpeg";
	terminal_defaults[Terminal::JPG] = term;
	
	term.extension = "gif";
	term.name = "gif";
	terminal_defaults[Terminal::GIF]  = term;

	term.vectorized = true;
	term.name = "svg";
	term.extension = "svg";
	term.font_size = 30;
	terminal_defaults[Terminal::SVG]  = term;
	
	term.extension = "pdf";
	term.name = "pdfcairo";
	term.size = VINT(20,10,0);
	term.font_size = 40;
	term.line_width = 6;
	terminal_defaults[Terminal::PDF]  = term;
	
	term.extension = "eps";
	term.name = "epscairo";
	term.font = "Helvetica";
	term.font_size  = 40;
	term.line_width = 1.5;
	terminal_defaults[Terminal::EPS] = term;
	
};
Gnuplotter::~Gnuplotter() { if (gnuplot) delete gnuplot; Gnuplotter::instances--;};

void Gnuplotter::loadFromXML(const XMLNode xNode, Scope* scope)
{
	AnalysisPlugin::loadFromXML(xNode, scope);

	decorate = true;
	getXMLAttribute(xNode,"decorate",decorate);

// 	interpolation_pm3d = true;
// 	getXMLAttribute(xNode,"interpolation",interpolation_pm3d);
	
	log_plotfiles = false;
	getXMLAttribute(xNode,"log-commands",log_plotfiles);
	pipe_data = ! log_plotfiles;

	string plot_tag = "Plot";
	for (int i=0; i<xNode.nChildNode(plot_tag.c_str()); i++) {
		XMLNode xPlot =  xNode.getChildNode(plot_tag.c_str(),i);
		PlotSpec plot;
		getXMLAttribute(xPlot, "title", plot.title);
		getXMLAttribute(xPlot, "slice", plot.z_slice);
		if (plot.z_slice<0 || plot.z_slice >= SIM::lattice().size().z)
			throw MorpheusException("Invalid z_slice.", xPlot);
		
		for (int j=0; j<xPlot.nChildNode(); j++) {
			XMLNode xPlotChild = xPlot.getChildNode(j);
			string node_name = xPlotChild.getName();
			if (node_name == "Cells"){
				plot.cell_painter = shared_ptr<CellPainter>(new CellPainter());
				plot.cell_painter->loadFromXML(xPlotChild, scope);
				plot.cells = true;
			}
			else if (node_name == "CellLabels"){
				plot.label_painter = shared_ptr<LabelPainter>(new LabelPainter());
				plot.label_painter->loadFromXML(xPlotChild, scope);
				plot.labels = true;
			}
			else if (node_name == "CellArrows") {
				plot.arrows = true;
				plot.arrow_painter = shared_ptr<ArrowPainter>(new ArrowPainter());
				plot.arrow_painter->loadFromXML(xPlotChild, scope);
			}
			else if (node_name == "VectorField") {
				plot.vectors = true;
				plot.vector_field_painter = shared_ptr<VectorFieldPainter>(new VectorFieldPainter());
				plot.vector_field_painter->loadFromXML(xPlotChild, scope);
			}
			else if (node_name == "Field") {
				plot.field = true;
				plot.field_painter = shared_ptr<FieldPainter>(new FieldPainter());
				plot.field_painter->loadFromXML(xPlotChild, scope);
			}
			
		}
		plots.push_back(plot);
	}
}

void Gnuplotter::init(const Scope* scope) {
	AnalysisPlugin::init(scope);
	
	// Check gnuplot has cairo available
	auto gnu_terminals = Gnuplot::get_terminals();
	if (gnu_terminals.count("pngcairo")==0 ) {
		terminal_defaults[Terminal::PNG].name = "png";
		cout << "Gnuplot: Falling back to plain png terminal " << endl;
	}
	if (gnu_terminals.count("pdfcairo")==0 ) {
		terminal_defaults[Terminal::PDF].name = "pdf";
	}
	if (gnu_terminals.count("epscairo")==0 ) {
		terminal_defaults[Terminal::EPS].name = "postscript";
	}
	
	// Replace the screen placeholder and non-available screen terminals
	// with the default screen terminal
	if (terminal() == Terminal::SCREEN) {
		if (terminal.stringVal() == "screen") {
			terminal_defaults[Terminal::SCREEN].name = Gnuplot::get_screen_terminal();
		}
		else {
			if (gnu_terminals.count(terminal.stringVal()))
				terminal_defaults[Terminal::SCREEN].name = terminal.stringVal();
			else {
				terminal_defaults[Terminal::SCREEN].name = Gnuplot::get_screen_terminal();
				cout << "Gnuplotter: Requested terminal " << terminal.stringVal() <<
					" is not available. Switching to " << Gnuplot::get_screen_terminal() << endl;
			}
		}
	}
	

	try {
		gnuplot = new Gnuplot();
		auto terminals = gnuplot->get_terminals();
    
		for (uint i=0;i<plots.size();i++) {
			if (plots[i].cells) {
				plots[i].cell_painter->init(scope, plots[i].z_slice);
				registerInputSymbols( plots[i].cell_painter->getInputSymbols() );
			}
			if (plots[i].label_painter) {
				plots[i].label_painter->init(scope, plots[i].z_slice);
				registerInputSymbols( plots[i].label_painter->getInputSymbols() );
			}
			if (plots[i].arrow_painter) {
				plots[i].arrow_painter->init(scope, plots[i].z_slice);
				registerInputSymbols( plots[i].arrow_painter->getInputSymbols() );
			}
			if (plots[i].vector_field_painter) {
				plots[i].vector_field_painter->init(scope, plots[i].z_slice);
				registerInputSymbols( plots[i].vector_field_painter->getInputSymbols() );
			}
			if (plots[i].field) {
				plots[i].field_painter->init(scope, plots[i].z_slice);
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
		for (uint i=0;i<plots.size();i++) {
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
	

	TerminalSpec terminal_spec = terminal_defaults[terminal()];
	if ( terminal_size.isDefined() ) {
		terminal_spec.size = terminal_size();
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

	string points_pm3d = " with image ";

	

	/* Generate Gnuplot plot
		*/
	//plot->reset_plot();
	stringstream command;

// 	Gnuplot& command = *gnuplot;
	
	if( log_plotfiles ) {
		string time_id = (file_numbering() == FileNumbering::TIME) ? SIM::getTimeName() :  to_str(int(time/timeStep()));
		gnuplot->setLogfile(string("gnuplot_commands_") + (to_str(instance_id) + "_") + time_id + ".gp");
	}

	//    SETTING UP THE TERMINAL
	command << "unset multiplot; reset; \n";

	// assumes that the default terminal size has aspect ratio 2:1
	double terminal_scaling =  max(terminal_spec.size.x/(terminal_defaults[terminal()].size.x * plot_layout.cols) , terminal_spec.size.y/(terminal_defaults[terminal()].size.y * 2 * plot_layout.rows));
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
	terminal_cmd << "set term "<< terminal_spec.name << (terminal_spec.visual ? " persist ":" ") << " enhanced "
				 << "size " << terminal_spec.size.x << "," << terminal_spec.size.y << " "
				 << "font \"" << terminal_spec.font << "," << terminal_spec.font_size << "\" ";
				 
	if ( ! terminal_spec.visual )
		terminal_cmd << "lw " << terminal_spec.line_width << " ";

	if ( ! terminal_spec.vectorized && ! terminal_spec.visual)
		terminal_cmd << "truecolor ";

// 	if (terminal == Terminal::EPS)
// 		terminal_cmd << "color ";
	
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
		DEFAULT STYLES
	*/
	
	double cell_contour_width = (plot_layout.plots[0].right - plot_layout.plots[0].left) * terminal_spec.size.x / 4 / (Gnuplotter::PlotSpec::size().x+50);
	double cell_arrow_width = 1.6 * cell_contour_width;
	double cell_node_size = 0.54 * cell_contour_width;
	double cell_label_scaling = terminal_spec.line_width;
	
	if ( ! terminal_spec.vectorized ) {
		cell_contour_width = (cell_contour_width<1.3 ?  1.0 : cell_contour_width)/terminal_spec.line_width;
		cell_arrow_width = (cell_arrow_width<1.3 ? 1.0 : cell_arrow_width)/terminal_spec.line_width;
	}

	command << "set multiplot;\n"
			<< "unset tics;\n"
			<< "set datafile missing \"NaN\";\n"
			<< "set view map;\n"
			<< "set style arrow 1 head   filled   size screen 0.015,15,45  lc 0 lw " << cell_arrow_width << ";\n"
			<< "set style arrow 2 head   nofilled size screen 0.015,15,135 lc 0 lw " << cell_arrow_width << ";\n"
			<< "set style arrow 3 head   filled   size screen 0.015,15,45  lc 0 lw " << cell_arrow_width << ";\n"
			<< "set style arrow 4 head   filled   size screen 0.015,15,90  lc 0 lw " << cell_arrow_width << ";\n"
			<< "set style arrow 5 head   nofilled size screen 0.015,15,135 lc 0 lw " << cell_arrow_width << ";\n"
			<< "set style arrow 6 heads  filled   size screen 0.015,15,135 lc 0 lw " << cell_arrow_width << ";\n"
			<< "set style arrow 7 heads  nofilled size screen 0.004,90,90  lc 0 lw " << cell_arrow_width << ";\n"
			<< "set style arrow 8 nohead nofilled                          lc 0 lw " << cell_arrow_width << ";\n"
			<< "mod(a,b) = (a-int(a/b)*b + (a<0)*b);\n";
			
	/*
		MULTI PLOTS
	*/
	for (uint i=0; i<plots.size(); i++ ) {
		command << "set lmargin at screen " << plot_layout.plots[i].left << ";\n"
				<< "set rmargin at screen " << plot_layout.plots[i].right << ";\n"
				<< "set tmargin at screen " << plot_layout.plots[i].top << ";\n"
				<< "set bmargin at screen " << plot_layout.plots[i].bottom << ";\n";
		command << "set style fill transparent solid 1;\n";
		
		bool suppress_xlabel = int(i) < plot_layout.cols*plot_layout.rows-1;
		
		VDOUBLE origin(0.5,0.5,0);
		
		/*
			PLOT PDE SURFACE + ISOLINES
		*/
		stringstream s;
		VDOUBLE view_size = PlotSpec::size() + PlotSpec::view_oversize();
		origin.y += 0.25 * PlotSpec::view_oversize().y;
		s << "[" << - origin.x << ":" << view_size.x - origin.x << "]"
		  <<  "[" << - origin.y << ":" << view_size.y - origin.y << "]";
		string plot_range = s.str();
		s.str("");
// 		view_size.y = lattice().size().y + PlotSpec::view_oversize().y;
		
		s << "[" << - origin.x << ":" << view_size.x - origin.x << "]"
		  <<  "[" << - origin.y << ":" << view_size.y - origin.y << "]";
		string field_range = s.str();
		bool is_hexagonal = (SIM::lattice().getStructure() == Lattice::hexagonal);
		
		if (plots[i].field) {
// 			if (!interpolation_pm3d)
// 				command << "set pm3d corners2color c4;\n";
			if (! decorate || plots[i].cells || plots[i].vectors)
				command << "unset colorbox;\n" << "unset title;\n" << "unset xlabel;\n";			
			else{
				string plot_title;
				if ( ! plots[i].title.empty())
					plot_title = Gnuplot::sanitize(plots[i].title);
				else 
					plot_title = Gnuplot::sanitize(plots[i].field_painter->getDescription());

				command << "set colorbox; set cbtics\n";
				if (suppress_xlabel)
					command << "unset xlabel\n";
				else 
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

				command << "splot "<< field_range << " " << plots[i].field_painter->getValueRange();

				if (pipe_data)
					command << " '-' ";
				else
					command << " \'"<< outputDir << "/" << plots[i].field_data_file.c_str() << "' ";
				
				if (plots[i].field_painter->getCoarsening() != 1 || is_hexagonal) {
					auto c = plots[i].field_painter->getCoarsening();
					auto cx = c * (is_hexagonal ? 0.5:1);
					auto cy = c * (is_hexagonal ? 0.866025:1);
					command <<  "u (" << cx <<  "*$1):(" << cy << "*$2):3 ";
				}
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
						<< "splot " << field_range << "[]";
				if (pipe_data)
					command << " '-' ";
				else
					command << " '" << outputDir << "/" << plots[i].field_data_file.c_str() << "' ";
				if (plots[i].field_painter->getCoarsening() != 1 || is_hexagonal) {
					auto c = plots[i].field_painter->getCoarsening();
					auto cx = c * (is_hexagonal ? 0.5:1);
					auto cy = c * (is_hexagonal ? 0.866025:1);
					command <<  "u (" << cx <<  "*$1):(" << cy << "*$2):3 ";
				}
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
			if (! decorate || plots[i].cells)
				command << "unset title;\n" << "unset xlabel;\n";
			else{
				string plot_title;
				if ( ! plots[i].title.empty())
					plot_title = Gnuplot::sanitize(plots[i].title);
				else 
					plot_title = Gnuplot::sanitize(plots[i].vector_field_painter->getDescription());

				if (suppress_xlabel)
					command << "unset xlabel\n";
				else 
					command << "set xlabel '" << time << " " /*<< SIM::getTimeScaleUnit() */<< "' offset 0,0 ;\n";
				command << "set title \"" << plot_title << "\" offset 0,-0.5 ;\n";
			}

			command << "plot " << plot_range << " ";
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
			
			if (plots[i].cell_painter->getOpacity() < 1.0)
				command << "set style fill transparent solid " << plots[i].cell_painter->getOpacity() << " noborder;\n";
			//command << "set cbrange ["<< plots[i].cell_painter->getMinVal() << ":"<< plots[i].cell_painter->getMaxVal() << "]" << endl;
			command << "set style line 40 lc rgb \"black\" lw " << cell_contour_width << "\n";
			if ( ! decorate)
				command << "unset colorbox;\n";
			else {
				string plot_title;
				if ( ! plots[i].title.empty())
					plot_title = Gnuplot::sanitize(plots[i].title);
				else 
					plot_title = Gnuplot::sanitize(plots[i].cell_painter->getDescription());
				
				command << "set colorbox; set cbtics; set clabel;\n";
				
				if (suppress_xlabel)
					command << "unset xlabel\n";
				else 
					command << "set xlabel '" << time << " " /*<< SIM::getTimeScaleUnit() */ << "' offset 0," << (using_splot ? "0.1" : "2") << ";\n";

				if (plots[i].z_slice > 0)
					command << "set title '" << plot_title << " (z-slice: " << plots[i].z_slice << ")' offset 0,-0.5;\n";
				else
					command << "set title '" << plot_title << "' offset 0,-0.5;\n";
			}
			
			
			if (CellPainter::boundary_cell_wise == plots[i].cell_painter->getDataLayout()) {
				
				command << "plot " << plot_range << " ";
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
						command << " us 1:2 w filledc c lt pal cb " << cell_boundary_data[p].value << " lw " << 1.0/terminal_spec.line_width <<" notitle";
						command << ",\\\n''";
					}
					if (!pipe_data)
						command << " index " << current_index << ":" << current_index + cell_boundary_data[p].polygons.size()-1;
					command << " us 1:2 w l ls 40 notitle";
					current_index +=cell_boundary_data[p].polygons.size();
				}
				
                // assert that the plot command is valid ...
                if (cell_boundary_data.size() == 0) {
					command << " 0 notitle";
				}
			}
			else {
				// TODO Try to use plot with image here to get rid of the pixel size issue
				command << "splot " << plot_range;
				if (pipe_data) 
					command << " '-' ";
				else
					command << " '" << outputDir << "/" << plots[i].cells_data_file << "' " ;
				
				if (plots[i].cell_painter->getDataLayout() == CellPainter::ascii_matrix) {
					command << " matrix";
					if (is_hexagonal) {
						// half size pixel approximation for in hexagonal lattice data
						command << " u (0.5*$1-0.25):(0.866*$2):3";
					}
					command << " w image pal not";
				}
				else  /* if (plots[i].cell_painter->getDataLayout() == CellPainter::point_wise) */ {
					command << " using 1:2:3 ";
					command << " w p pt 5 ps " << cell_node_size << " pal not";
				}
				
				if (pipe_data) 
					command << ", '-' ";
				else 
					command << ", '" << outputDir << "/" << plots[i].membranes_data_file << "' ";
				command << "using 1:2:(0) w l ls 40 notitle";
			}

			if ( plots[i].labels ){
				if(pipe_data)
					command << ", '-' ";
				else{
					command << ", '" << outputDir << "/" << plots[i].labels_data_file << "' ";
				}
				command << " using ($1):($2)"<< (using_splot ? "" : ":(0)") << ":3 with labels font \"Helvetica,"
						<< plots[i].label_painter->fontsize() * cell_label_scaling << "\" textcolor rgb \"" << plots[i].label_painter->fontcolor().c_str() << "\" notitle";
			}

			if ( plots[i].arrows ){
				if(pipe_data)
					command << ", '-' ";
				else{
					command << ", '" << outputDir << "/" << plots[i].arrow_data_file << "' ";
				}
				command << " u ($1):($2)"<< (using_splot ? "" : ":(0)") << ":3:4" <<  (using_splot ? "" : ":(0)")  << " ";
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
								command << cell_boundary_data[o].polygons[p][q].x << "\t" << cell_boundary_data[o].polygons[p][q].y << "\n";
							}
							command << "\n\n";
						}
						command << "e\n";
						if ( ! isnan(cell_boundary_data[o].value) && cell_boundary_data[o].value != CellPainter::getTransparentValue()) {
							for (uint p=0; p<cell_boundary_data[o].polygons.size(); p++) {
								for (uint q=0; q < cell_boundary_data[o].polygons[p].size(); q++) {
									command << cell_boundary_data[o].polygons[p][q].x << "\t" << cell_boundary_data[o].polygons[p][q].y << "\n";
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
								command << cell_boundary_data[o].polygons[p][q].x << "\t" << cell_boundary_data[o].polygons[p][q].y << "\n";
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
							out_file << cell_boundary_data[o].polygons[p][q].x << "\t" << cell_boundary_data[o].polygons[p][q].y << "\n";
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


	gnuplot->cmd(command.str());
	
}

Gnuplotter::plotLayout Gnuplotter::getPlotLayout( uint plot_count, bool border )
{
	plotLayout layout;
	plotPos p = {0,1,1,0};

	VDOUBLE lattice_size = PlotSpec::size();
	VDOUBLE plot_size = lattice_size;
	double view_aspect_ratio = (plot_size.y / plot_size.x); 
	if (view_aspect_ratio < 1.0/20) {
		view_aspect_ratio = 1.0/20;
		plot_size.y = plot_size.x * view_aspect_ratio;
	}
	if (view_aspect_ratio > 20) {
		view_aspect_ratio = 20;
		plot_size.x =  plot_size.y / view_aspect_ratio;
	}
	
	double x_margin = 0;
	double y_margin = 0;
	if (border) {
		double view_extend =  max(lattice_size.x, lattice_size.y);
		x_margin = 0.3 * view_extend;
		plot_size.x += x_margin;
		
		y_margin = 0.15 * view_extend;
		plot_size.y += y_margin;
	}
	
	layout.plot_aspect_ratio = plot_size.y / plot_size.x;
// 	cout << "Plot size " << plot_size.x <<"x"<<plot_size.y<< " -> " << layout.plot_aspect_ratio << endl;
	
	layout.rows = max(1,int(floor(sqrt(plot_count/layout.plot_aspect_ratio))));
	layout.cols = ceil(double(plot_count) / double(layout.rows));
	layout.layout_aspect_ratio = (layout.plot_aspect_ratio * layout.rows) / layout.cols;
	uint x_panel = 0;
	uint y_panel = 0;
	
	for (uint i=0; i<plot_count; i++){  
	  
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
// 		cout << "Plot " << p.left <<"--"<<p.right<< "," << p.top << "--" << p.bottom << endl;
		layout.plots.push_back(p);

		// set plot dimensions rowsfirst
		x_panel++;
		if (x_panel >= layout.cols){
			y_panel++;
			x_panel = 0;
		}
	    
	} 
	
	// plot size shrinkage

	return layout;
}

