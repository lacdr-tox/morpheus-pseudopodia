#include "lattice.h"
#include "random_functions.h"




Neighborhood::Neighborhood(const vector< VINT >& neighbors, int order, const Lattice* lattice) 
  : _neighbors(neighbors), _order(order), _distance(0), _lattice(lattice)
{
	sort(_neighbors.begin(),_neighbors.end(), [&](const VINT& a, const VINT& b) {
		
		double angular_diff = _lattice->to_orth(a).angle_xy() - _lattice->to_orth(b).angle_xy();
		if (angular_diff<0) return true;
		else if (angular_diff>0) return false;
		
		double radial_diff = _lattice->to_orth(a).abs_sqr() - _lattice->to_orth(b).abs_sqr();
		if (radial_diff<0) return true;
		else if (radial_diff>0) return false;
		 
		double z_diff = _lattice->to_orth(a).z - _lattice->to_orth(b).z;
		if (z_diff<0) return true;
		else if (z_diff>0) return false;
		
		return false;
	});
	
	for(auto i : _neighbors) {
		double d = _lattice->to_orth(i).abs();
		d = d > _distance ? d : _distance;
	}
}
	
	
	

Lattice::Lattice() {
// set default boundary conditions
 	for (uint i=0; i<Boundary::nCodes; i++){
		boundaries[i] = Boundary::periodic;
	}
}

Lattice::Lattice(const Lattice::LatticeDesc& desc) : Lattice()
{
	for (auto const & bdry : desc.boundaries) {
		boundaries[bdry.first] = bdry.second;
	}
	_size = desc.size;
	default_neighborhood = getNeighborhoodByOrder(desc.neighborhood_order);
}


unique_ptr<Lattice> Lattice::createLattice(const Lattice::LatticeDesc& desc)
{
	switch (desc.structure) {
		case cubic:
			return make_unique<Cubic_Lattice>(desc);
		case hexagonal:
			return make_unique<Hex_Lattice>(desc);
		case square:
			return make_unique<Square_Lattice>(desc);
		case linear:
			return make_unique<Linear_Lattice>(desc);
		default:
			return unique_ptr<Lattice>();
	}
}


void Lattice::loadFromXML(const XMLNode xnode) {
// 	VINT xml_size, domain_size;
	
	stored_node = xnode;

	getXMLAttribute(xnode, "Size/value", _size);
	
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
	
	if (xnode.nChildNode("Domain")) {
		domain.loadFromXML(xnode.getChildNode("Domain"), this);
	} 
	if (_size.abs() == 0) {
		throw string("undefined lattice size");
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
		c.x= pmod(c.x, _size.x);
		if (c.x > _size.x - c.x) c.x -= _size.x;
	}
	if (boundaries[Boundary::py] == Boundary::periodic && (abs(2*c.y) > _size.y)) {
		c.y= pmod(c.y, _size.y);
		if (c.y > _size.y - c.y) c.y -= _size.y;
	}
	if (boundaries[Boundary::pz] == Boundary::periodic && (abs(2*c.z) > _size.z)) {
		c.z= pmod(c.z, _size.z);
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


Neighborhood Lattice::getNeighborhood(const std::string name) const {
	Neighborhood neighborhood = this->getNeighborhoodByName(name);
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
	default_neighborhood = getNeighborhood(node);
}

Neighborhood Lattice::getNeighborhood(const XMLNode node) const {
	if ( node.nChildNode("Distance") ) {
		double distance;
		getXMLAttribute(node,"Distance/text",distance);
		if( distance == 0 ){
		  throw string("Neighborhood/Distance must be greater than 0.");
		}
		return getNeighborhoodByDistance( distance );
	}
	else if ( node.nChildNode("Order") ) {
		int order;
		getXMLAttribute(node,"Order/text",order);
		if( order == 0 ){
		  throw string("Neighborhood/Order must be greater than 0.");
		}
		return getNeighborhoodByOrder( order );
	}
	else if ( node.nChildNode("Name") &&  node.getChildNode("Name").getText()) {
		return getNeighborhood( string(node.getChildNode("Name").getText()) );
	}
	else {
		throw string("Unknown neighborhood definition in ") + node.getName();
	}
}

Neighborhood Lattice::getNeighborhoodByDistance(const double dist_max) const  {
	std::vector<VINT> neighbors = get_all_neighbors();
	// assume all neighbors are ordered by distance and we just got to drop from the end ...
	if (dist_max>=4.0) {
		throw string("Neighborhood distances equal or larger than 4 are not supported");
	}
	uint order=0;
	int n_neighbors=0;
	std::vector<int> neighbors_per_order = get_all_neighbors_per_order();
	
	for (order=0; order<neighbors_per_order.size(); order++) {
		if ( orth_distance(VDOUBLE(0,0,0),to_orth(neighbors[n_neighbors])).abs() < dist_max + 0.00001) {
			n_neighbors+= neighbors_per_order[order];
		}
		else {
			break;
		}
	}
	neighbors.resize(n_neighbors);
	return Neighborhood(neighbors,order,this);
}

Neighborhood Lattice::getNeighborhoodByOrder(const uint order) const {
	std::vector<VINT> neighbors = get_all_neighbors();
	std::vector<int> neighbors_per_order = get_all_neighbors_per_order();
	if (neighbors_per_order.size()<order) {
		throw string("Maximum neighborhood order for the current lattice is ") + to_str(neighbors_per_order.size()) + string(" requested ") + to_str(order);
	}
	uint n_neighbors=0;
	for (uint i=0; i<order; i++) n_neighbors += neighbors_per_order[i]; 
	neighbors.resize(n_neighbors);
	return Neighborhood(neighbors,order, this);
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

/* vector<VINT> Hex_Lattice::get_all_neighbors() const {
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
}; */


vector<VINT> Hex_Lattice::get_all_neighbors() const {
	const VINT neighbors[] = {
	// 1st order CCW
	VINT(1,0,0), VINT(0,1,0),  VINT(-1,1,0), 
	VINT(-1,0,0), VINT(0,-1,0), VINT(1,-1,0),
	// 2nd order CCW
	VINT(1,1,0), VINT(-1,2,0), VINT(-2,1,0), 
	VINT(-1,-1,0), VINT(1,-2,0), VINT(2,-1,0), 
	// 3rd order CCW
	VINT(2,0,0), VINT(0,2,0),  VINT(-2,2,0), 
	VINT(-2,0,0), VINT(0,-2,0), VINT(2,-2,0),
	// 4rd order CCW
	VINT(2,1,0),  VINT(1,2,0),  VINT(-1,3, 0), VINT(-2,3,0), VINT(-3,2,0), VINT(-3,1,0),
	VINT(-2,-1,0),VINT(-1,-2,0),VINT(1,-3,0), VINT(2,-3,0), VINT(3,-2,0), VINT(3,-1,0),
	// 5th order CCW
	VINT(3,0,0), VINT(0,3,0), VINT(-3,3,0),
	VINT(-3,0,0),VINT(0,-3,0),VINT(3,-3,0),
	// 6th order CCW
	VINT(2,2,0), VINT(-2,4,0),VINT(-4,2,0),
	VINT(-2,-2,0),VINT(2,-4,0),VINT(4,-2,0),
	// 7th order CCW
	VINT(3,1,0),  VINT(1,3,0),  VINT(-1,4,0),VINT(-3,4,0),VINT(-4,3,0),VINT(-4,1,0),
	VINT(-3,-1,0),VINT(-1,-3,0),VINT(1,-4,0),VINT(3,-4,0),VINT(4,-3,0),VINT(4,-1,0),
	// 8th order CCW
	VINT(4,0,0), VINT(0,4,0), VINT(-4,4,0),
	VINT(-4,0,0),VINT(0,-4,0),VINT(4,-4,0),
	// 9th order CCW
	VINT(3,2,0),  VINT(2,3,0),  VINT(-2,5,0),VINT(-3,5,0),VINT(-5,3,0),VINT(-5,2,0),
	VINT(-3,-2,0),VINT(-2,-3,0),VINT(2,-5,0),VINT(3,-5,0),VINT(5,-3,0),VINT(5,-2,0),
	// 10th order CCW
	VINT(4,1,0),  VINT(1,4,0),  VINT(-1,5,0),VINT(-4,5,0),VINT(-5,4,0),VINT(-5,1,0),
	VINT(-4,-1,0),VINT(-1,-4,0),VINT(1,-5,0),VINT(4,-5,0),VINT(5,-4,0),VINT(5,-1,0),
	// 11th order CCW
	VINT(5,0,0), VINT(0,5,0), VINT(-5,5,0),
	VINT(-5,0,0),VINT(0,-5,0),VINT(5,-5,0)
	};
	vector<VINT> acc(c_array_begin(neighbors), c_array_end(neighbors));
	return acc;
};

/* vector<int> Hex_Lattice::get_all_neighbors_per_order() const {
	int neighbors[]	 = {6, 6, 6, 12 ,6};
	std::vector<int> acc(c_array_begin(neighbors), c_array_end(neighbors));
	return acc;	
} */

vector<int> Hex_Lattice::get_all_neighbors_per_order() const {
	int neighbors[]	 = { 6, 6, 6, 12, 6 , 6, 12, 6, 12, 12, 6 };
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

Neighborhood Cubic_Lattice::getNeighborhoodByName(std::string name) const {
	std::vector<VINT> acc;
	if (lower_case(name) == "fchc") {
		std::vector<VINT> neighbors = {
		//Flächennnachbarn
		VINT(1,0,0),VINT(-1,0,0),VINT(0,0,1),VINT(0,0,-1),VINT(0,1,0),VINT(0,-1,0),
		VINT(1,0,0),VINT(-1,0,0),VINT(0,0,1),VINT(0,0,-1),VINT(0,1,0),VINT(0,-1,0),
		// Kantennachbarn
		VINT(1,1,0), VINT(-1,1,0), VINT(1,0,1), VINT(1,0,-1), VINT(0,1,1), VINT(0,-1,1),
		VINT(1,-1,0),VINT(-1,-1,0),VINT(-1,0,1),VINT(-1,0,-1),VINT(0,1,-1),VINT(0,-1,-1)
		};
		return Neighborhood(neighbors,2,this);
	}
	return Neighborhood();
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

