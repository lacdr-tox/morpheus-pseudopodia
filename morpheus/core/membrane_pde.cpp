#include "membrane_pde.h"
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

shared_ptr< const Lattice > MembraneProperty::lattice()
{
	if ( ! membrane_lattice ) {
// #pragma omp critical
		if ( ! membrane_lattice ) {
			if ( MembraneProperty::resolution < 2) {
				throw string("Membrane resolution is not specified!\nTo use membrane properties, you must set Space/MembraneSize/Resolution");
			}
			if (SIM::getLattice()->getDimensions()==2) {
				size = VINT(MembraneProperty::resolution,1,1);
				membrane_lattice = shared_ptr<Lattice>(Linear_Lattice::create(size,true));
			}
			else if (SIM::getLattice()->getDimensions()==3) {

				size.y = MembraneProperty::resolution/2;
				size.x = 2 * size.y;
				size.z = 1;
				membrane_lattice = shared_ptr<Lattice>(Square_Lattice::create(size,true));
			}
			else {
// 				cout << "cannot create membranes for 1D lattices.\nrefuse to create the membrane lattice!" << SIM::getLattice()->getDimensions() << endl;
				throw(string("cannot create membranes for 1D lattices.\nrefuse to create the membrane lattice!")  +  to_str( SIM::getLattice()->getDimensions() ) );
			}
		}
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

