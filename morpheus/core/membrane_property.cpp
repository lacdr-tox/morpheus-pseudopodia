#include "membrane_property.h"
#include "celltype.h"

REGISTER_PLUGIN(MembraneProperty);


/* Make the Membrane Lattice accessible from the world.
 *  Resolution is what has to be set globally, if not no lattice can be created.
 *  Resolution sysmbol is what can be used to expose the lattice size to the symbol accessor infrastructure.
 */


uint MembraneProperty::resolution = 0;
string MembraneProperty::resolution_symbol("");
shared_ptr<const Lattice> MembraneProperty::membrane_lattice = shared_ptr<const Lattice>();
VINT MembraneProperty::size = VINT(0,0,0);

vector<double> MembraneProperty::node_sizes;

void MembraneProperty::loadMembraneLattice(const XMLNode& node)
{
	if ( ! getXMLAttribute(node,"MembraneLattice/Resolution/value",MembraneProperty::resolution) )
		return ;
	
	if (getXMLAttribute(node,"MembraneLattice/Resolution/symbol",MembraneProperty::resolution_symbol)) {
			shared_ptr<Property<double> > p = Property<double>::createConstantInstance(MembraneProperty::resolution_symbol, "Membrane Lattice Size");
			p->set(MembraneProperty::resolution);
			SIM::defineSymbol(p);
	}
	
	SymbolData symbol;
	if (getXMLAttribute(node,"MembraneLattice/SpaceSymbol/symbol",SymbolData::MembraneSpace_symbol) ) {
		symbol.link = SymbolData::MembraneSpace;
		symbol.granularity = Granularity::MembraneNode;
		symbol.name = SymbolData::MembraneSpace_symbol;
		symbol.fullname = "membrane coordinates";
		getXMLAttribute(node,"MembraneLattice/SpaceSymbol/name",symbol.fullname); 
		symbol.type_name = TypeInfo<VDOUBLE>::name();
		symbol.integer = false;
		symbol.invariant = false;
		symbol.time_invariant = true;
		SIM::defineSymbol(symbol);
	}
	
	if ( MembraneProperty::resolution < 2) {
		throw string("Membrane resolution specified is too small!");
	}
	if (SIM::getLattice()->getDimensions()==2) {
		size = VINT(MembraneProperty::resolution,1,1);
		membrane_lattice = shared_ptr<Lattice>(Linear_Lattice::create(size,true));
		node_sizes.push_back(1.0);
	}
	else if (SIM::getLattice()->getDimensions()==3) {

		size.y = MembraneProperty::resolution/2;
		size.x = 2 * size.y;
		size.z = 1;
		membrane_lattice = shared_ptr<Lattice>(Square_Lattice::create(size,true));
		node_sizes.resize(size.y);
		VINT pos(0,0,0);
		for (pos.y=0; pos.y<size.y; pos.y++) {
			node_sizes[pos.y] = sin(MembraneProperty::memPosToOrientation(pos).to_radial().y);
		}
		
	}
	else {
		throw(string("cannot create membranes for 1D lattices.\nrefuse to create the membrane lattice!")  +  to_str( SIM::getLattice()->getDimensions() ) );
	}
}

double MembraneProperty::nodeSize(const VINT& memPos)
{
	assert(MembraneProperty::node_sizes.size()>memPos.y);
	return MembraneProperty::node_sizes[memPos.y];
}


VINT MembraneProperty::orientationToMemPos(const VDOUBLE& direction) {
	if (size.x == 0) {
		return VINT(0,0,0);
	}
	if (size.y <= 1) {
		int x = round(direction.angle_xy() / (2*M_PI) * size.x);
		if (x >= size.x) 
			x-= size.x;
		return VINT(x,0,0);
	}
	else {
		VDOUBLE radial = direction.to_radial();
		VINT mem_pos(round(radial.x/(2*M_PI)*size.x),round((radial.y/M_PI)*size.y-0.5),0);
		if (mem_pos.x >= size.x) mem_pos.x-=size.x;
		if (mem_pos.y<0) mem_pos.y = 0;
		return mem_pos;
	}
}

VDOUBLE MembraneProperty::memPosToOrientation(const VINT& memPos)
{
	if (size.x == 0) {
		return VDOUBLE(0,0,0);
	}
	else if (size.y <= 1) {
		double angle_xy = memPos.x * 2 * M_PI / size.x;
		return VDOUBLE(cos(angle_xy), sin(angle_xy), 0);
	}
	else {
		return VDOUBLE::from_radial(VDOUBLE( 2 * M_PI * memPos.x / size.x, M_PI * ((memPos.y+0.5)  / size.y - 0.5),1));
	}
	
}

shared_ptr< const Lattice > MembraneProperty::lattice()
{
	if ( ! membrane_lattice ) {
		throw string("Membrane resolution is not specified!\nTo use membrane properties, you must set Space/MembraneSize/Resolution");
	}
	return membrane_lattice;
}

void MembraneProperty::loadFromXML(const XMLNode xNode )
{
	Plugin::loadFromXML( xNode );
	const bool surface = true;
	pde_layer = shared_ptr<PDE_Layer>(new PDE_Layer( lattice() , node_length.getMeters(), surface));
	
 	pde_layer->loadFromXML(xNode);
}

shared_ptr<PDE_Layer> MembraneProperty::getPDE(){
	return pde_layer;
}

string MembraneProperty::getSymbolName(){
	if (pde_layer)
		return pde_layer->getSymbol();
	else 
		return "";
}

string MembraneProperty::getName(){
	if (pde_layer)
		return pde_layer->getName();
	else 
		return "";
}

