#include "lattice.h"
#include "simulation.h"
const double sin_60 = 0.86602540378443864676;

Lattice::Lattice() {
// set default boundary conditions
 	for (uint i=0; i<Boundary::nCodes; i++){
		boundaries[i] = Boundary::periodic;
	}
}

void Lattice::loadFromXML(const XMLNode xnode) {
// 	VINT xml_size, domain_size;
	
	stored_node = xnode;

	getXMLAttribute(xnode, "Size/value", _size);
	
	if (xnode.nChildNode("Domain")) {
		domain.loadFromXML(xnode.getChildNode("Domain"),_size);
		cout << "Domain: overriding lattice size with domain size " << domain.domainSize() << endl;
		_size = domain.domainSize();

	} 
	if (_size.abs() == 0) {
		throw string("undefined lattice size");
	}
	
	// set default boundary conditions
	// relevant boudaries are set to 'periodic'
	// other boundaries (i.e. -z and z in 1D and 2D models) are set to noflux
	//  the latter solves a plotting problem with cells
	for (uint i=0; i<Boundary::nCodes; i++){
		if(i < getDimensions()*2)
			boundaries[i] = Boundary::periodic;
		else
			boundaries[i] = Boundary::noflux;
	}

	if (xnode.nChildNode("BoundaryConditions")) {
		XMLNode xbcs = xnode.getChildNode("BoundaryConditions");
		for (int i=0; i< xbcs.nChildNode("Condition"); i++) {
			XMLNode xbc = xbcs.getChildNode("Condition",i);
			string code_name;
			if (! getXMLAttribute(xbc, "boundary",code_name)) 
				throw string("No boundary in BoundaryCondition");
			Boundary::Codes code = Boundary::name_to_code(code_name);

			string type_name;
			if ( ! getXMLAttribute(xbc, "type", type_name) )
				throw(string("No boundary type defined in BoundaryCondition"));
			Boundary::Type boundary_type = Boundary::name_to_type(type_name);

			
			bool set_opposite = (boundary_type == Boundary::periodic || boundaries[code] == Boundary::periodic );
			
			boundaries[code] = boundary_type;
			if (set_opposite) {
				switch (code) {
					case Boundary::mx :
						boundaries[Boundary::px] = boundary_type; break;
					case Boundary::px :
						boundaries[Boundary::mx] = boundary_type; break;
					case Boundary::my :
						boundaries[Boundary::py] = boundary_type; break;
					case Boundary::py :
						boundaries[Boundary::my] = boundary_type; break;
					case Boundary::mz :
						boundaries[Boundary::pz] = boundary_type; break;
					case Boundary::pz :
						boundaries[Boundary::mz] = boundary_type; break;
				}
			}
		}
	}
	
	if (xnode.nChildNode("Neighborhood")) {
		setNeighborhood(xnode.getChildNode("Neighborhood"));
	}
	else {
		default_neighborhood = getNeighborhoodByOrder(1);
		neighborhood_type = "Order";
		neighborhood_value = "1";
	}
	
}


XMLNode Lattice::saveToXML() {

// 	XMLNode xLattice = XMLNode::createXMLTopNode("Lattice");
// 	xLattice.addChild("Size").addAttribute("value",to_cstr(_size));
// 	xLattice.addAttribute("class",getXMLName().c_str());
// 	XMLNode xNode = XMLNode::createXMLTopNode("BoundaryConditions");
// 	for (uint i=0; i<Boundary::nCodes; i++) {
// 		XMLNode xCond = xNode.addChild("Condition");
// 		xCond.addAttribute("boundary", Boundary::code_to_name(Boundary::Codes(i)).c_str());
// 		xCond.addAttribute("type", Boundary::type_to_name(boundaries[i]).c_str());
// 	}
// 	if (xNode.nChildNode())
// 		xLattice.addChild(xNode);
// 	xLattice.addChild("Neighborhood").addChild(neighborhood_type.c_str()).addText(neighborhood_value.c_str());
	return stored_node;
}
 
// std::unique_ptr<BoundaryConstraint> Lattice::getConstraint(const XMLNode xNode) const {
// 	if (xNode.getName() == string("BoundaryCondition") || xNode.getName() == string("Condition") ) {
// 		std::unique_ptr<BoundaryConstraint> bc (new BoundaryConstraint(this, xNode));
// 		return bc;
// 	};
// 	assert(0);
// 	return std::unique_ptr<BoundaryConstraint>();
// }

VINT Lattice::setSize(const VINT& a) { 
	_size.x =  max(a.x,1);
	if (dimensions > 1 ) _size.y =  max(a.y,1);
	else _size.y = 1;
	if (dimensions > 2 ) _size.z =  max(a.z,1);
	else _size.z = 1;
	return _size;
}

Boundary::Type Lattice::get_boundary_type(Boundary::Codes code) const {
	return boundaries[code];
}

vector<Boundary::Type> Lattice::get_boundary_types() const {
	return vector<Boundary::Type>(&boundaries[0], &boundaries[Boundary::nCodes]);
}

VINT Lattice::node_distance(const VINT& a, const VINT& b) const {
	VINT c(a-b);
	if (boundaries[Boundary::px] == Boundary::periodic && (abs(2*c.x) > _size.x)) {
		c.x= MOD(c.x, _size.x);
		if (c.x > _size.x - c.x) c.x -= _size.x;
	}
	if (boundaries[Boundary::py] == Boundary::periodic && (abs(2*c.y) > _size.y)) {
		c.y= MOD(c.y, _size.y);
		if (c.y > _size.y - c.y) c.y -= _size.y;
	}
	if (boundaries[Boundary::pz] == Boundary::periodic && (abs(2*c.z) > _size.z)) {
		c.z= MOD(c.z, _size.z);
		if (c.z > _size.z - c.z) c.z -= _size.z;
	}
	return c;
}


bool Lattice::equal_pos(const VINT& a, const VINT& b) const { return node_distance(a,b) == VINT(0,0,0); };

VINT Lattice::getRandomPos( void ) const { 
	VINT a(0,0,0); 
	a.x= getRandomUint(_size.x-1);
	if (dimensions > 1) a.y = getRandomUint(_size.y-1);
	if (dimensions > 2) a.z = getRandomUint(_size.z-1);
	return a; 
};


std::vector<VINT> Lattice::getNeighborhood(const std::string name) const {
	vector<VINT> neighborhood = this->getNeighborhoodByName(name);
	if ( ! neighborhood.empty() ) return neighborhood;
	int float_end = name.find_first_not_of(" 0123456789.,");
	int int_end = name.find_first_not_of(" 0123456789");
	if ( name.empty() || float_end == 0) return neighborhood;
	stringstream ss(name);
	if ( int_end > 0 && int_end != float_end) {
		double distance;
		ss >> distance;
		neighborhood = getNeighborhoodByDistance(distance);
	} else {
		int order;
		ss >> order;
		neighborhood = getNeighborhoodByOrder(order);
	}
	return neighborhood;
}

void Lattice::setNeighborhood(const XMLNode node) {
	if ( node.nChildNode("Distance") ) {
		neighborhood_type = "Distance";
		double distance;
		getXMLAttribute(node,"Distance/text",distance);
		getXMLAttribute(node,"Distance/text",neighborhood_value);
		if( distance == 0 ){
		  cerr << "Neighborhood/Distance must be greater than 0." << endl;
		  exit(-1);
		}
		default_neighborhood = getNeighborhoodByDistance( distance );
	}
	else if ( node.nChildNode("Order") ) {
		neighborhood_type = "Order";
		int order;
		getXMLAttribute(node,"Order/text",order);
		getXMLAttribute(node,"Order/text",neighborhood_value);
		if( order == 0 ){
		  cerr << "Neighborhood/Order must be greater than 0." << endl;
		  exit(-1);
		}
		default_neighborhood = getNeighborhoodByOrder( order );
	}
	else if ( node.nChildNode("Name") ){
		neighborhood_type = "Name";
		neighborhood_value =  string(node.getChildNode("Name").getText());
		default_neighborhood = getNeighborhood( neighborhood_value );
	}
	else {
		cerr << "unknown neighborhood definition in " << node.getName() << endl;
		exit(-1);
	}
	
	sort(default_neighborhood.begin(),default_neighborhood.end(), [this](const VINT& a, const VINT& b){ return this->to_orth(a).angle_xy()< this->to_orth(b).angle_xy(); } );
}

std::vector<VINT> Lattice::getNeighborhood(const XMLNode node) const {
	if ( node.nChildNode("Distance") ) {
		double distance;
		getXMLAttribute(node,"Distance/text",distance);
		if( distance == 0 ){
		  cerr << "Neighborhood/Distance must be greater than 0." << endl;
		  exit(-1);
		}
		return getNeighborhoodByDistance( distance );
	}
	else if ( node.nChildNode("Order") ) {
		int order;
		getXMLAttribute(node,"Order/text",order);
		if( order == 0 ){
		  cerr << "Neighborhood/Order must be greater than 0." << endl;
		  exit(-1);
		}
		return getNeighborhoodByOrder( order );
	}
	else if ( node.nChildNode("Name") ){
		return getNeighborhood( string(node.getChildNode("Name").getText()) );
	}
	else {
		cerr << "unknown neighborhood definition in " << node.getName() << endl;
		exit(-1);
	}
}

std::vector<VINT> Lattice::getNeighborhoodByDistance(const double dist_max) const  {
	std::vector<VINT> acc = get_all_neighbors();
	// assume all neighbors are ordered by distance and we just got to drop from the end ...
	if (dist_max>3.0) {
		throw string("Neighborhood distances larger than 3 are not supported");
	}
	while ( ! acc.empty() && orth_distance(VDOUBLE(0,0,0),to_orth(acc.back())).abs() > dist_max + 0.00001 ) {
// 		cout << acc.back() << " -> "  << orth_distance(VINT(0,0,0),to_orth(acc.back())).abs() << endl;
		acc.pop_back();
	}
	return acc;
}

std::vector<VINT> Lattice::getNeighborhoodByOrder(const uint order) const {
	std::vector<VINT> neighbors = get_all_neighbors();
	std::vector<int> neighbors_per_order = get_all_neighbors_per_order();
	if (neighbors_per_order.size()<order) {
		throw string("Maximum neighborhood order for the current lattice is ") + to_str(neighbors_per_order.size());
	}
	uint n_neighbors=0;
	for (uint i=0; i<order; i++) n_neighbors += neighbors_per_order[i]; 
	neighbors.resize(n_neighbors);
	return neighbors;
}


Hex_Lattice::Hex_Lattice(const XMLNode xNode) : Lattice() {
	dimensions = 2;
	structure = hexagonal;
	_size = VINT(50,50,1);
	loadFromXML(xNode);
	_size.z = 1;
	orth_size = VDOUBLE(_size.x, _size.y * sin_60, 1);
};

VDOUBLE Hex_Lattice::to_orth(const VDOUBLE& a) const {
	return VDOUBLE(a.x+0.5*a.y, sin_60 * a.y,0);
};

VINT Hex_Lattice::from_orth(const VDOUBLE& a) const {
	int y =  round(a.y / sin_60);
	return VINT( round(a.x - 0.5 * y) , y ,0);
}


VDOUBLE Hex_Lattice::orth_distance(const VDOUBLE& a, const VDOUBLE& b) const {

	VDOUBLE c;
	if (boundaries[Boundary::px] == Boundary::periodic  || boundaries[Boundary::py] == Boundary::periodic ) {

		// Wrapping values to hex coordinates
		VDOUBLE k(a.x - 0.5 * a.y / sin_60, a.y / sin_60, 0.0);
		VDOUBLE l(b.x - 0.5 * b.y / sin_60, b.y / sin_60, 0.0);

		// calculate their distance
		c = k-l;

		if (boundaries[Boundary::py] == Boundary::periodic  && (abs(2*c.y) > _size.y)) {
			c.y= fmod(c.y, _size.y);
			if ( 2*c.y > _size.y) c.y -= _size.y;
			if ( 2*c.y < -_size.y) c.y += _size.y;
		}

		if (boundaries[Boundary::px] == Boundary::periodic  && (abs(2*c.x) > _size.x)) {
			c.x= fmod(c.x, _size.x);
			if ( 2*c.x > _size.x) c.x -= _size.x;
			if ( 2*c.x < -_size.x) c.x += _size.x;
		}
		// Wrapping back to orth coordinates  this might not really be the minimal distance but don't care
		c = to_orth(c);

	}
	else {
		c = a-b;
	}
	return c;
}

vector<VINT> Hex_Lattice::get_all_neighbors() const {
	const VINT neighbors[] = {
	// 1st order
	VINT(1,0,0), VINT(0,1,0),  VINT(-1,1,0), VINT(-1,0,0), VINT(0,-1,0), VINT(1,-1,0),
	// 2nd order
	VINT(1,1,0), VINT(-1,-1,0),VINT(2,-1,0), VINT(1,-2,0), VINT(-2,1,0), VINT(-1,2,0),
	// 3rd order
	VINT(2,0,0), VINT(-2,0,0),VINT(0,2,0), VINT(0,-2,0), VINT(-2,2,0), VINT(2,-2,0),
	// 4th order
	VINT(2,1,0), VINT(1,2,0),VINT(-1,3,0), VINT(-2,3,0), VINT(-3,2,0), VINT(-3,1,0),
	VINT(-2,-1,0), VINT(-1,-2,0),VINT(1,-3,0), VINT(2,-3,0), VINT(3,-2,0), VINT(3,-1,0),
	// 5th order
	VINT(3,0,0), VINT(-3,0,0),VINT(0,3,0), VINT(0,-3,0), VINT(-3,3,0), VINT(3,-3,0)
	};
	
	vector<VINT> acc(c_array_begin(neighbors), c_array_end(neighbors));
	return acc;
};

vector<int> Hex_Lattice::get_all_neighbors_per_order() const {
	int neighbors[]	 = {6, 6, 6, 12 ,6};
	std::vector<int> acc(c_array_begin(neighbors), c_array_end(neighbors));
	return acc;	
}


VDOUBLE Orth_Lattice::to_orth(const VDOUBLE& a) const { return a; };

VDOUBLE Orth_Lattice::orth_distance(const VDOUBLE& a, const VDOUBLE& b) const {
	VDOUBLE c = a-b;
	if (boundaries[Boundary::px] == Boundary::periodic  && (abs(2*c.x) > _size.x)) {
		c.x= fmod(c.x, _size.x);
		if ( 2*c.x > _size.x) c.x -= _size.x;
		if ( 2*c.x < -_size.x) c.x += _size.x;
	}
	if (boundaries[Boundary::py] == Boundary::periodic  && (abs(2*c.y) > _size.y)) {
		c.y= fmod(c.y, _size.y);
		if ( 2*c.y > _size.y) c.y -= _size.y;
		if ( 2*c.y < -_size.y) c.y += _size.y;
	}
	if (boundaries[Boundary::pz] == Boundary::periodic  && (abs(2*c.z) > _size.z)) {
		c.z= fmod(c.z, _size.z);
		if ( 2*c.z > _size.z) c.z -= _size.z;
		if ( 2*c.z < -_size.z) c.z += _size.z;
	}
	return c;
}

VINT Orth_Lattice::from_orth(const VDOUBLE& a) const {
	return VINT(round(a.x), round(a.y) , round(a.z));
}

Cubic_Lattice::Cubic_Lattice(const XMLNode xNode) : Orth_Lattice(xNode) {
	structure = cubic;
	dimensions = 3;
	_size = VINT(50,50,20);
	loadFromXML(xNode);
}


vector<VINT> Cubic_Lattice::get_all_neighbors() const {
const VINT neighbors[] = {
	//Flächennnachbarn
	VINT(1,0,0),VINT(0,1,0),VINT(0,0,1),VINT(-1,0,0),VINT(0,-1,0),VINT(0,0,-1),
	// Kantennachbarn
	VINT(1,1,0), VINT(-1,1,0), VINT(1,0,1), VINT(1,0,-1), VINT(0,1,1), VINT(0,-1,1),
	VINT(1,-1,0),VINT(-1,-1,0),VINT(-1,0,1),VINT(-1,0,-1),VINT(0,1,-1),VINT(0,-1,-1),
	// Eckennachbarn
	VINT(1,1,1),    VINT(-1,1,1),  VINT(1,-1,1), VINT(1,1,-1),
	VINT(-1,-1,-1), VINT(1,-1,-1), VINT(-1,1,-1), VINT(-1,-1,1),
	//2nd order=
	//Flächennachbarn der Flächennachbarn 1. Ordnung
	VINT(2,0,0),VINT(-2,0,0),VINT(0,0,2),VINT(0,0,-2),VINT(0,2,0),VINT(0,-2,0),
	//Flächennachbarn der Kantennachbarn 1. Ordnung
	VINT(2,1,0), VINT(-2,1,0), VINT(2,0,1), VINT(2,0,-1), VINT(0,2,1), VINT(0,-2,1),
	VINT(2,-1,0),VINT(-2,-1,0),VINT(-2,0,1),VINT(-2,0,-1),VINT(0,2,-1),VINT(0,-2,-1),
	VINT(1,2,0), VINT(-1,2,0), VINT(1,0,2), VINT(1,0,-2), VINT(0,1,2), VINT(0,-1,2),
	VINT(1,-2,0),VINT(-1,-2,0),VINT(-1,0,2),VINT(-1,0,-2),VINT(0,1,-2),VINT(0,-1,-2),
	// Flächennachbarn der Eckennachbarn 1. Ordnung 
	VINT(2,1,1),  VINT(-2,1,1),  VINT(2,-1,1), VINT(2,1,-1), VINT(-2,-1,-1), VINT(2,-1,-1), VINT(-2,1,-1), VINT(-2,-1,1),
	VINT(1,2,1),  VINT(-1,2,1),  VINT(1,-2,1), VINT(1,2,-1), VINT(-1,-2,-1), VINT(1,-2,-1), VINT(-1,2,-1), VINT(-1,-2,1),
	VINT(1,1,2),  VINT(-1,1,2),  VINT(1,-1,2), VINT(1,1,-2), VINT(-1,-1,-2), VINT(1,-1,-2), VINT(-1,1,-2), VINT(-1,-1,2),
	// Kantennachbarn von 1.Ordnung
	VINT(2,2,0),  VINT(-2,2,0), 
	VINT(2,-2,0), VINT(-2,-2,0),
	VINT(2,0,2),  VINT(2,0,-2), 
	VINT(-2,0,2), VINT(-2,0,-2),
	VINT(0,2,2),  VINT(0,-2,2), 
	VINT(0,2,-2), VINT(0,-2,-2),
	// Kantennachbarn von 2.Ordnung
	VINT(2,2,1),VINT(2,2,-1),
	VINT(-2,2,1),VINT(-2,2,-1),
	VINT(2,1,2),VINT(2,-1,2),
	VINT(2,1,-2),VINT(2,-1,-2),
	VINT(1,2,2),VINT(-1,2,2),
	VINT(1,-2,2), VINT(-1,-2,2),
	VINT(2,-2,1),VINT(2,-2,-1),
	VINT(-2,-2,1),VINT(-2,-2,-1),
	VINT(-2,1,2),VINT(-2,-1,2),
	VINT(-2,1,-2),VINT(-2,-1,-2),
	VINT(1,2,-2),VINT(-1,2,-2),
	VINT(1,-2,-2),VINT(-1,-2,-2),
	
	VINT(3,0,0), VINT(-3,0,0),
	VINT(0,3,0), VINT(0,-3,0),
	VINT(0,0,3), VINT(0,0,-3),
	//Eckennachbarn
	VINT(2,2,2),    VINT(-2,2,2),  VINT(2,-2,2),  VINT(2,2,-2),
	VINT(-2,-2,-2), VINT(2,-2,-2), VINT(-2,2,-2), VINT(-2,-2,2)
	};
	vector<VINT> acc(c_array_begin(neighbors), c_array_end(neighbors));
	return acc;
}

std::vector<int> Cubic_Lattice::get_all_neighbors_per_order() const {
	int neighbors[]	 = {6, 12, 8, 6, 24, 24, 12, 30, 8};
	std::vector<int> acc(c_array_begin(neighbors), c_array_end(neighbors));
	return acc;	
};

std::vector<VINT> Cubic_Lattice::getNeighborhoodByName(std::string name) const {
	std::vector<VINT> acc;
	if (lower_case(name) == "fchc") {
		VINT neighbors[] = {
		//Flächennnachbarn
		VINT(1,0,0),VINT(-1,0,0),VINT(0,0,1),VINT(0,0,-1),VINT(0,1,0),VINT(0,-1,0),
		VINT(1,0,0),VINT(-1,0,0),VINT(0,0,1),VINT(0,0,-1),VINT(0,1,0),VINT(0,-1,0),
		// Kantennachbarn
		VINT(1,1,0), VINT(-1,1,0), VINT(1,0,1), VINT(1,0,-1), VINT(0,1,1), VINT(0,-1,1),
		VINT(1,-1,0),VINT(-1,-1,0),VINT(-1,0,1),VINT(-1,0,-1),VINT(0,1,-1),VINT(0,-1,-1)
		};
		return std::vector<VINT>(c_array_begin(neighbors), c_array_end(neighbors));
	}
	return std::vector<VINT>();
};



Square_Lattice* Square_Lattice::create(VINT resolution, bool spherical) {
        XMLNode xLattice = XMLNode::createXMLTopNode("Lattice");
        xLattice.addChild("Size").addAttribute("value",to_cstr(resolution));
        if  (spherical) {
                XMLNode xBoundary = xLattice.addChild("BoundaryConditions");
                XMLNode xCondition = xBoundary.addChild("Condition");
                xCondition.addAttribute("boundary","x");
                xCondition.addAttribute("type","periodic");
                XMLNode yCondition = xBoundary.addChild("Condition");
                yCondition.addAttribute("boundary","y");
                yCondition.addAttribute("type","noflux");
                yCondition = xBoundary.addChild("Condition");
                yCondition.addAttribute("boundary","-y");
                yCondition.addAttribute("type","noflux");
        }
        return new Square_Lattice(xLattice);
}

Square_Lattice::Square_Lattice(const XMLNode xnode) : Orth_Lattice(xnode) {
	structure = square;
	dimensions = 2;
	_size = VINT (50,50,1);
	loadFromXML(xnode);
	_size.z = 1;
}

vector<VINT> Square_Lattice::get_all_neighbors() const {
const VINT neighbors[] = {
	//Kantennachbarn
	VINT(1,0,0), VINT(0,1,0), VINT(-1,0,0),VINT(0,-1,0),
	// Eckennachbarn
	VINT(1,1,0), VINT(-1,1,0), VINT(1,-1,0),VINT(-1,-1,0),
	//2nd order
	//Kantennachbarn der Kantennachbarn 1. Ordnung
	VINT(2,0,0),VINT(-2,0,0),VINT(0,2,0),VINT(0,-2,0),
	//Flächennachbarn der Kantennachbarn 1. Ordnung
	VINT(2,1,0), VINT(-2,1,0),
	VINT(2,-1,0),VINT(-2,-1,0),
	VINT(1,2,0), VINT(-1,2,0), 
	VINT(1,-2,0),VINT(-1,-2,0),
	// Eckennachbarn der Eckennachbarn 1.Ordnung
	VINT(2,2,0), VINT(-2,2,0), VINT(2,-2,0), VINT(-2,-2,0),
	// 
	VINT(3,0,0), VINT(0,3,0), VINT(-3,0,0), VINT(0,-3,0)
	};
	
	vector<VINT> acc(c_array_begin(neighbors), c_array_end(neighbors));
	return acc;
}

std::vector<int> Square_Lattice::get_all_neighbors_per_order() const {
	int neighbors[]	 = {4, 4, 4, 8, 4, 4};
	std::vector<int> acc(c_array_begin(neighbors), c_array_end(neighbors));
	return acc;	
};

Linear_Lattice::Linear_Lattice(const XMLNode xNode): Orth_Lattice(xNode)
{
	dimensions=1;
	structure = linear;
	_size=VINT(100,1,1);
	loadFromXML(xNode);
	_size.y=1;
	_size.z=1;
}


Linear_Lattice* Linear_Lattice::create(VINT resolution, bool periodic) {
	XMLNode xLattice = XMLNode::createXMLTopNode("Lattice");
	xLattice.addChild("Size").addAttribute("value",to_cstr(resolution));
	if  (periodic) {
		XMLNode xBoundary = xLattice.addChild("BoundaryConditions");
		XMLNode xCondition = xBoundary.addChild("Condition");
		xCondition.addAttribute("boundary","x");
		xCondition.addAttribute("type","periodic");
	}
	return new Linear_Lattice(xLattice);
}


vector< VINT > Linear_Lattice::get_all_neighbors() const {
	const VINT neighbors[] = {
	VINT(1,0,0), VINT(-1,0,0),
	VINT(2,0,0),VINT(-2,0,0),
	VINT(3,0,0),VINT(-3,0,0)
	};
	vector<VINT> acc(c_array_begin(neighbors), c_array_end(neighbors));
	return acc;
}


vector< int > Linear_Lattice::get_all_neighbors_per_order() const {
	int neighbors[]	 = {2, 2, 2};
	std::vector<int> acc(c_array_begin(neighbors), c_array_end(neighbors));
	return acc;	 
}

