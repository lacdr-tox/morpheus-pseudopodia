#include "membrane_property.h"
#include "core/focusrange.h"
// #include "celltype.h"

REGISTER_PLUGIN(MembranePropertyPlugin);

/* Make the Membrane Lattice accessible from the world.
 *  Resolution is what has to be set globally, if not no lattice can be created.
 *  Resolution symbol is what can be used to expose the lattice size to the symbol accessor infrastructure.
 */

uint MembranePropertyPlugin::resolution = 0;
string MembranePropertyPlugin::resolution_symbol("");
VINT MembranePropertyPlugin::size = VINT(0,0,0);
vector<double> MembranePropertyPlugin::node_sizes;
shared_ptr<const Lattice> MembranePropertyPlugin::membrane_lattice;


void MembranePropertySymbol::applyBuffer() const {
	// Iterate over all membranes of the referred CellType and run apply
	FocusRange range(Granularity::Cell, scope());
	for (auto focus: range) {
		getProperty(focus)->membrane_pde->swapBuffer();
	}
}


void MembranePropertyPlugin::loadMembraneLattice(const XMLNode& node, Scope* scope)
{
	if ( ! getXMLAttribute(node,"MembraneLattice/Resolution/value",resolution) )
		return ;
	if ( resolution < 2) {
		throw string("Membrane resolution specified is too small!");
	}
	
	if (getXMLAttribute(node,"MembraneLattice/Resolution/symbol",resolution_symbol)) {
			scope->registerSymbol( SymbolAccessorBase<double>::createConstant(resolution_symbol, "Membrane Lattice Size",resolution) );
	}
	
	getXMLAttribute(node,"MembraneLattice/SpaceSymbol/symbol",SymbolBase::MembraneSpace_symbol);
	// space symbol shall always be decalared.
	scope->registerSymbol(make_shared<MembraneSpaceSymbol>(SymbolBase::MembraneSpace_symbol));

	

	if (SIM::getLatticeStructure() == Lattice::square || SIM::getLatticeStructure() == Lattice::hexagonal ) {
		LatticeDesc desc;
		desc.size = VINT(resolution,1,1);
		size = desc.size;
		desc.boundaries[Boundary::mx] = Boundary::periodic;
		desc.boundaries[Boundary::my] = Boundary::periodic;
		membrane_lattice = make_shared<Linear_Lattice>(desc);
		node_sizes.push_back(1.0);
	}
	else if (SIM::getLatticeStructure() == Lattice::cubic) {
		LatticeDesc desc;
		size.y = resolution/2;
		size.x = 2 * size.y;
		size.z = 1;
		desc.size = size;
		membrane_lattice = shared_ptr<Lattice>(Square_Lattice::create(size,true));
		node_sizes.resize(size.y);
		VINT pos(0,0,0);
		for (pos.y=0; pos.y<size.y; pos.y++) {
			node_sizes[pos.y] = sin(memPosToOrientation(pos).to_radial().y);
		}
		
	}
	else {
		throw(string("cannot create membranes for 1D lattices.\nrefuse to create the membrane lattice!") );
	}
}

double MembranePropertyPlugin::nodeSize(const VINT& memPos)
{
	assert(node_sizes.size()>memPos.y);
	return node_sizes[memPos.y];
}


VINT MembranePropertyPlugin::orientationToMemPos(const VDOUBLE& direction) {
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
		VINT mem_pos(round(radial.x/(2*M_PI)*size.x),floor((radial.y/M_PI)*size.y),0);
		if (mem_pos.y==size.y) mem_pos.y=size.y-1;
		if (mem_pos.x>=size.x) mem_pos.x-=size.x;
// 		assert(mem_pos.y<size.y);
// 		assert(mem_pos.y>=0);
// 		assert(mem_pos.x<size.x);
// 		assert(mem_pos.x>=0);
		return mem_pos;
	}
}

VDOUBLE MembranePropertyPlugin::memPosToOrientation(const VINT& memPos)
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

shared_ptr< const Lattice > MembranePropertyPlugin::lattice()
{
	if ( ! membrane_lattice ) {
		throw string("Membrane resolution is not specified!\nTo use membrane properties, you must set Space/MembraneSize/Resolution");
	}
	return membrane_lattice;
}

void MembranePropertyPlugin::loadFromXML(const XMLNode xNode, Scope* scope )
{
	if (!scope->getCellType()) {
		throw string("Membrane properties can only be used within CellTypes");
	}
	symbol_name.setXMLPath("symbol");
	registerPluginParameter(symbol_name);
	Plugin::loadFromXML( xNode, scope);
	const bool surface = true;
	auto membrane_pde = shared_ptr<PDE_Layer>(new PDE_Layer( MembraneProperty::lattice() , MembranePropertyPlugin::node_length.getMeters(), surface));
 	membrane_pde->loadFromXML(xNode, scope);
	
	default_property = make_shared<MembraneProperty>(this, membrane_pde);
	auto property_id = scope->getCellType()->addProperty(default_property);
	symbol = make_shared <MembranePropertySymbol>( this, scope->getCellType() ,property_id );
	scope->registerSymbol(symbol);
	if (membrane_pde->getDiffusionRate() >0) {
		diffusion_plugin =  make_shared<Diffusion>(symbol);
	}
}


void MembranePropertyPlugin::init(const Scope* scope ) {
	Plugin::init(scope);
	if (diffusion_plugin)
		diffusion_plugin->init(scope);
	try {
		default_property->membrane_pde->init();
	} catch (...) { cout << "Warning: Could not initialize default of membrane property " << symbol->name() << "." << endl; }
}
