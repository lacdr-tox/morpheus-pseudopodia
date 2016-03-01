#include "svgPlotter.h"

using namespace SIM;

//-------------------------------------------------------------------

Plugin* svgPlotter::createInstance()					//erzeugen einer neuen instanz
{return new svgPlotter();};

registerPlugin(svgPlotter);							//registrieren des neuen plugins

svgPlotter::svgPlotter(){};
svgPlotter::~svgPlotter(){};

//-------------------------------------------------------------------

void svgPlotter::init(double time)
{
	Analysis_Listener::init( time );
	layer = CPM::getLayer();
	latticeDim = layer->getLattice()->to_orth(layer->size());	
	
	// readjust x-size for hexagonal lattices
	if (layer->get_boundary_type("x") == Lattice::periodic) {
		latticeDim.x = layer->getLattice()->to_orth(VINT(layer->size().x,0,0)).x;
	}

	stain_property_max = 0.0;
	compress_file = false; 
	if (!css_internal)
	{
		fstream fcss;
		fcss.open(css_file.c_str(), fstream::out | fstream::trunc);
		fcss << css_string;
		fcss.close();
	}
}

//-------------------------------------------------------------------

void svgPlotter::notify(double time)
{
	Analysis_Listener::notify(time);
	svgPlotter::svg_header();
	svgPlotter::svg_body();
	svgPlotter::create_svg(time);
};

//-------------------------------------------------------------------

void svgPlotter::svg_header()
{
	svgText << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
	if(!css_internal)
		{ svgText << "<?xml-stylesheet href=\"" << css_file << "\" type=\"text/css\"?>" << endl;}
	svgText << "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\"" << endl;
	svgText << "\"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">" << endl;
	
	
	if(latticeDim.x / latticeDim.y > 1)
	{
		svgLatticeScale = 500.0 / latticeDim.x;
		svgWidth = 500;
		svgHeight = 500 / (latticeDim.x / latticeDim.y);
	}
	if(latticeDim.x / latticeDim.y <= 1)
	{
		svgLatticeScale = 500.0 / latticeDim.y;
		svgWidth = 500 * (latticeDim.x / latticeDim.y);
		svgHeight = 500;
	}
	svgText << "<svg version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" viewBox=\"0 0 " << svgWidth << " " << svgHeight << "\">" << endl << endl;

	if(css_internal)
	{
		svgText << "<defs>" << endl;
		svgText << "<style type=\"text/css\">" << endl;
		svgText << "<![CDATA[" << endl;
		svgText << css_string;
		svgText << "]]>" << endl;
		svgText << "</style>" << endl << endl;
		svgText << "</defs>" << endl;
	}
};

//-------------------------------------------------------------------

void svgPlotter::loadFromXML(const XMLNode node)	//einlesen der Daten aus der XML
{
	Analysis_Listener::loadFromXML(node);
	css_internal=false;
	getXMLAttribute(node,"css-internal",css_internal);
	getXMLAttribute(node,"CSS/internal",css_internal);

	compress_file = false;
	getXMLAttribute(node,"compress",compress_file);
	
	if (getXMLAttribute(node,"css",css_file) || getXMLAttribute(node,"CSS/file",css_file)) {		//css-file zum einfärben der zellen
		fstream fcss;
		fcss.open(css_file.c_str(), fstream::in);
		if (!fcss)	{
				cout << "Unable to open the provided css-file " << css_file << endl;
				css_file.clear();
		} 
		else {
			string line;
			while ( getline(fcss, line) ) {
				css_string.append(line);	
				css_string.append("\n");
			}
			fcss.close();
		}
	}
	if (css_file.empty()) {
		css_file = "colors.css";
		stringstream sstr_css;

		sstr_css << "polygon {stroke:black; stroke-width:1px;}" << endl;
		for(int i = 0; i < getCellTypes().size()-1; i++) {
			sstr_css << ".cellpop" << i+1 << " {fill:" << discr_color_scale_1(i) << ";}" << endl;
		}
		css_string = sstr_css.str();
	}
	
	getXMLAttribute(node, "Stain/cellproperty", stain_property_name );
	
};

//-------------------------------------------------------------------
XMLNode svgPlotter::saveToXML() const
{
	XMLNode xNode = Analysis_Listener::saveToXML();
	XMLNode xCSS = xNode.addChild("CSS");
	xCSS.addAttribute("internal",to_cstr( css_internal ));
	if ( ! css_file.empty()) xCSS.addAttribute("file",css_file.c_str());

	if ( ! stain_property_name.empty() )
		xNode.addChild("Stain").addAttribute("cellproperty",stain_property_name.c_str());
	return xNode;
};

//-------------------------------------------------------------------

void svgPlotter::svg_body()
{
	VINT pos;
	vector<const CellType*> vec_ct = getCellTypes();
	
	// grep all the cellproperty related information and create a color code
	map<string, double> svgclass_properties;
	Cell_Property::Property_Type property_type;
	for(int i = 0; i < vec_ct.size(); i++)
	{
		uint property_id;
		bool has_property = vec_ct[i]->findCellProperty(stain_property_name, property_type, property_id);
// 		cout << "Stain: " << stain_property_name <<  " "  << (has_property ? "YES" : "NO") <<endl;
		if (has_property) {
			for(int j = 0; j < vec_ct[i]->getNumberOfCells(); j++)
			{
				string class_name = string("cellpop") + to_str(i) +"-cell" + to_str(j);
				double val;
				switch (property_type) {
					case Cell_Property::Boolean : val = vec_ct[i]->getCell(j).getProperty(property_id).b * 1.0; break;
					case Cell_Property::Double : val = vec_ct[i]->getCell(j).getProperty(property_id).d; break;
					case Cell_Property::Integer : val = vec_ct[i]->getCell(j).getProperty(property_id).i; break;
					case Cell_Property::Vector : val = vec_ct[i] -> getCell(j).getProperty(property_id).v.angle_xy(); break;
				}
				svgclass_properties[class_name] = val;
			}
		}
	}
	
	// Put the cell colors derived from the cellproperties into a style section

	if ( ! svgclass_properties.empty() ) {
		if (property_type == Cell_Property::Double) {
			for ( std::map< string, double >::const_iterator c = svgclass_properties.begin(); c  != svgclass_properties.end(); c++ ) {
				stain_property_max = max( c->second , stain_property_max );
			}
		}

		svgText << "<defs>" << endl;
		svgText << "<style type=\"text/css\">" << endl;
		svgText << "<![CDATA[" << endl;
		for ( std::map< string, double >::const_iterator c = svgclass_properties.begin(); c  != svgclass_properties.end(); c++ ) {
			svgText << '.' << c->first << " { fill:";
			if (property_type == Cell_Property::Double)
				svgText << cont_color_scale_1(c->second);
			else
				svgText << discr_color_scale_1(int(c->second));
			svgText << ";}" << endl;
		}
		svgText << "]]>" << endl;
		svgText << "</style>" << endl << endl;
		svgText << "</defs>" << endl;
	}
	
	svgText << "<g>" << endl;;
	for(int i = 0; i < vec_ct.size(); i++)
	{
		if( i != CPM::getEmptyState().celltype )
		{
			// Legend
			svgText << "<rect style=\"stroke-width:1\" class=\"cellpop" << i << "\" x=\"" << svgWidth + 30 << "\" y=\"" << 25 + (20 * i) << "\" width=\"10\" height=\"10\"/>" << endl;
			svgText << "<text x=\"" << svgWidth + 45 << "\" y=\"" << 35 + (20 * i) << "\">"<< vec_ct[i]->getName() << "</text>" << endl;
			
			svgText << "<g class=\"cellpop" << i << "\" transform=\"translate(0,"<< svgHeight-0.5 << ") scale(1,-1)\">" << endl;
			svgText.precision(5);
			for(int j = 0; j < vec_ct[i]->getNumberOfCells(); j++)
			{
				svgText << "	<g class=\"cellpop" << i << "-cell" << j << "\">" << endl;
				vector<boundarySegment> vec_allBound;
				// if there is a periodic lattice we have to draw lines through cells crossing the boundary
				if (SIM::getLattice()->get_boundary_type("x")== periodic || SIM::getLattice()->get_boundary_type("y")== periodic) {
					vec_allBound = svg_bounds(vec_ct[i]->getCell(j).getNodes());
				} else {
					vec_allBound = svg_bounds(vec_ct[i]->getCell(j).getNodes());
				}
				svg_polygons(vec_allBound);
				svgText << "	</g>" << endl;
			}
			svgText << "</g>" << endl;
		}
	}
	svgText << "</g>" << endl;
	svgText << "<rect x=\"" << 0 << "\" y=\"" << 0 << "\" width=\"" << svgWidth << "\" height=\"" << svgHeight << "\" style=\"fill: none; stroke: black;\"/>" << endl;

	svgText << "</svg>" << endl;
};

//-------------------------------------------------------------------


vector<svgPlotter::boundarySegment> svgPlotter::svg_bounds(const Cell::Nodes& node_list)
{

	// we assume that the neighbors are sorted either clockwise or anti-clockwise. 
	// we could also add a sorting step after the filtering of nodes belonging to the plane
	vector<VINT> neighbors = SIM::getLattice()->getNeighborhood(1);
	
	vector<VINT> plane_neighbors;
	vector<VDOUBLE> orth_neighbors;
	
	for  ( vector< VINT >::iterator n = neighbors.begin(); n!=neighbors.end();n++) {
		if ( n->z == 0) {
			plane_neighbors.push_back(*n);
			orth_neighbors.push_back(SIM::getLattice()->to_orth(*n));
		}
	}

	vector<boundarySegment> vec_bound;
	 neighborNode;
	boundarySegment boundary;
	
	Cell::Nodes::const_iterator it;
	vector<boundarySegment> vec_allBound, vec_tmp;
				vector<boundarySegment>::iterator it_allBound;
				
	for (it = node_list.begin(); it != node_list.end(); it++ )
	{
		VINT pos = *it;
		if (pos.z != 0) continue;
		layer->getLattice()->resolve(pos);
		// get the proper position in the viewbox
		VDOUBLE orth_pos  = layer->getLattice()->to_orth(pos) % latticeDim;
		
		
		const & node = CPM::getNode(pos);
		for(int i = 0; i < plane_neighbors.size(); i++)
		{
			neighborNode = CPM::getNode((pos + plane_neighbors[i]));
			// create the potential edge : this alg. serves for all lattice structures
			boundary.pos1 = orth_pos + (orth_neighbors[i] + orth_neighbors[MOD(i+1,orth_neighbors.size())]) / plane_neighbors.size() * 2.0;
			boundary.pos2 = orth_pos + (orth_neighbors[i] + orth_neighbors[MOD(i-1,orth_neighbors.size())]) / plane_neighbors.size() * 2.0;
			if (neighborNode != node )
			{
				vec_bound.push_back(boundary);
			}
			// if the neighbor will not appear next to this node then put the edge anyways
			else {
				VDOUBLE orth_nei(orth_pos + orth_neighbors[i]);
				if (orth_nei.x < 0 or orth_nei.x >= latticeDim.x-0.01 or orth_nei.y < 0 or orth_nei.y >= latticeDim.y-0.01 ) 
					vec_bound.push_back(boundary);
			}
		}
	}
	return vec_bound;
};

//-------------------------------------------------------------------

void svgPlotter::svg_polygons(vector<boundarySegment>& vec_bound)
{
	uint start;
	vector<bool> list_forbidden = vector<bool>(vec_bound.size(), false);

	while (1)
	{  //repeat until all segments are moved to a polygon
		bool b_all = true;
		for(uint z = 0; z < vec_bound.size(); z++)
		{
			if ( ! list_forbidden[z] )
			{
				start = z; 
				list_forbidden[z] = true;
				b_all = false;
				break;
			}
		}
		if (b_all) { break; }
		
		svgText << "		<polygon" << endl;
		svgText << "			points=\"";
		svgText << vec_bound[start].pos1.x * svgLatticeScale << "," << vec_bound[start].pos1.y* svgLatticeScale << " ";

		uint position = start;
		while( ! ((vec_bound[position].pos2 - vec_bound[start].pos1).abs() < 0.01) )
		{
			for(uint z = 0; z < vec_bound.size(); z++)
			{
				if ( (vec_bound[z].pos1 - vec_bound[position].pos2).abs() < 0.01 && ! list_forbidden[z])
				{
					list_forbidden[z] = true;
					position = z;
					svgText << vec_bound[z].pos1.x* svgLatticeScale << "," << vec_bound[z].pos1.y* svgLatticeScale << " ";
					break;
				}
				if ( z == vec_bound.size() - 1 )
				{
					cout << "Cannot close polygone !" << endl;
					vec_bound[position].pos2 = vec_bound[start].pos1;
// 					exit(-1);
				}
			}
		}

		svgText << "\"/>" << endl;
	}
};

//-------------------------------------------------------------------

/*void svgPlotter::svg_index()
{
	//entscheidung bei welchen randbedingungen, welche ränder gesetzt werden
	if(getLattice().get_boundary_type('x') != "periodic" OR getLattice().get_boundary_type('x') != "reflecting")
	{
		svgText << "<rect x=\"" << 0 << "\" y=\"" << 15 << "\" width=\"10\" height=\"" << latticeDim.y * 10 << "\" style=\"fill: rgb(0, 238, 0); stroke: black;\"/>" << endl;
		svgText << "<rect x=\"" << latticeDim.x * 10 + 20 << "\" y=\"" << 15 << "\" width=\"10\" height=\"" << latticeDim.y * 10 << "\" style=\"fill: rgb(0, 238, 0); stroke: black;\"/>" << endl;
		svgText << "<rect x=\"" << 15 << "\" y=\"" << 0 << "\" width=\"" << latticeDim.x * 10 << "\" height=\"10\" style=\"fill: rgb(238, 0, 0); stroke: black;\"/>" << endl;
		svgText << "<rect x=\"" << 15 << "\" y=\"" << latticeDim.y * 10 + 20 << "\" width=\"" << latticeDim.x * 10 << "\" height=\"10\" style=\"fill: rgb(238, 0, 0); stroke: black;\"/>" << endl;
	}
}*/

//-------------------------------------------------------------------

void svgPlotter::create_svg(double double time)
{
	stringstream sstr_svg;
	sstr_svg << "cpm_" << setfill('0') << setw(6) << double time << ".svg";
	if (compress_file) sstr_svg << "z";
	cout << "Saving " << sstr_svg.str() << endl;
	if (compress_file) {
		gzFile file = gzopen(sstr_svg.str().c_str(), "wb");
		if (Z_NULL != file) {
			gzwrite(file, svgText.str().c_str(), svgText.str().size());
			gzclose(file);
		}
		else  cout<<"Cannot open file " << sstr_svg.str() << "!"  << endl;
	}
 	else {
		fstream fs;
		fs.open((sstr_svg.str()).c_str(), fstream::out);
		if(fs.is_open())
		{
			fs.write((svgText.str()).c_str(), (svgText.str()).length());
			fs.close();
		}
		else cout << "Cannot open file  " << sstr_svg.str() << "!" << endl;
	}
	svgText.str("");
};


string svgPlotter::discr_color_scale_1(int val ){
	static string c_colors[] = {"green","red","yellow","blue","magenta","orange","lime","brown","aqua","navy","pink","lime","teal"};
	static vector<string> colors(c_array_begin(c_colors), c_array_end(c_colors) );
	val %= colors.size();
	if (val>=0 && val<colors.size()) return colors[val];
	return "gray";
};
		
string svgPlotter::cont_color_scale_1(double val) {
	if (stain_property_max != 0.0) val/stain_property_max;
	VINT rgb = hsl_to_rgb( VDOUBLE(val,0.3,0.5)) * 255.0;
	return string("rgb(") + to_str(rgb.x) + "," + to_str(rgb.y) + ","  + to_str(rgb.z) + ")" ;
}

VDOUBLE svgPlotter::hsl_to_rgb(const VDOUBLE a) {
	// color space is 0...1
	// x=h, y=s, z=l
	double q = a.z < 0.5 ? a.z * ( 1 + a.y) : a.z + a.y - ( a.z * a.y);
	double p = 2 * a.z - q;
	VDOUBLE R;
	double t,r;
	for (int i =0; i<3; i++) {
		switch (i) {
			case 0: t= MOD(a.x + 1.0/3,1.0); break;
			case 1: t= a.x; break;
			case 2: t= MOD(a.x - 1.0/3,1.0); break;
		}
		if ( t<1.0/6 ) r = p + ((q-p)*6*t);
		else if (t < 0.5) r = q;
		else if (t<2.0/3) r = p + ((q-p) * 6 * (2.0/3 - t));
		else r=p;
		switch (i) {
			case 0: R.x=r; break;
			case 1: R.y=r; break;
			case 2: R.z=r; break;
		}
	}
	return R;
}
//-------------------------------------------------------------------

void svgPlotter::finish(double time){};
