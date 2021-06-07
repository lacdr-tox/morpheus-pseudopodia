#include "lattice_plugin.h"

REGISTER_PLUGIN(LatticePlugin);

LatticePlugin::LatticePlugin() : node_length("NodeLength",1)
{
	structure.setXMLPath("class");
	map<string, Lattice::Structure> structure_map {
		{"linear", Lattice::linear},
		{"square", Lattice::square},
		{"hexagonal", Lattice::hexagonal},
		{"cubic", Lattice::cubic}
	};
	structure.setConversionMap(structure_map);
	registerPluginParameter(structure);
	
	size.setXMLPath("Size/value");
	registerPluginParameter(size);
	
	size_symbol_name.setXMLPath("Size/symbol");
	registerPluginParameter(size_symbol_name);
}


void LatticePlugin::loadFromXML(XMLNode xnode, Scope* scope) {
	
	lattice_scope = scope;
	
	Plugin::loadFromXML(xnode, scope);
	
	if (size_symbol_name.isDefined()) {
		size_symbol = SymbolRWAccessorBase<VDOUBLE>::createVariable(size_symbol_name(), "Lattice size", lattice_desc.size);
		lattice_scope->registerSymbol( size_symbol );
	}
	
	if (xnode.nChildNode("NodeLength")) {
		node_length.loadFromXML(xnode.getChildNode("NodeLength"), scope);
	}

	// set default boundary conditions
	// relevant boudaries are set to 'periodic'
	// other boundaries (i.e. -z and z in 1D and 2D models) are set to noflux
	//  the latter solves a plotting problem with cells

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
			
			lattice_desc.boundaries[code] = boundary_type;
			
// 			bool set_opposite = (boundary_type == Boundary::periodic || boundaries[code] == Boundary::periodic );
// 			
// 			boundaries[code] = boundary_type;
// 			if (set_opposite) {
// 				switch (code) {
// 					case Boundary::mx :
// 						boundaries[Boundary::px] = boundary_type; break;
// 					case Boundary::px :
// 						boundaries[Boundary::mx] = boundary_type; break;
// 					case Boundary::my :
// 						boundaries[Boundary::py] = boundary_type; break;
// 					case Boundary::py :
// 						boundaries[Boundary::my] = boundary_type; break;
// 					case Boundary::mz :
// 						boundaries[Boundary::pz] = boundary_type; break;
// 					case Boundary::pz :
// 						boundaries[Boundary::mz] = boundary_type; break;
// 				}
// 			}
		}
	}
	structure.init();
	lattice_desc.structure = structure();
	
	if (xnode.nChildNode("Domain")) {
		lattice_desc.domain = make_shared<Domain>();
		lattice_desc.domain->loadFromXML(xnode.getChildNode("Domain"),scope, lattice_desc);
	} 

	
	if (xnode.nChildNode("Neighborhood")) {
		lattice_desc.default_neighborhood = parseNeighborhood(xnode.getChildNode("Neighborhood"));
	}
	else {
		NeighborhoodDesc desc;
		desc.mode = NeighborhoodDesc::Order;
		desc.order = 1;
	}
}

void LatticePlugin::init(const Scope* scope)
{
	Plugin::init(scope);
	if (size.granularity() != Granularity::Global) {
		throw string ("Lattice size must be constant in time and space" );
	}
	
	lattice_desc.size = VINT(size(SymbolFocus::global));

	// Override lattice size to fit at least the domain size
	if (lattice_desc.domain->domainType() != Domain::none) {
		VINT domain_size = lattice_desc.domain->size();
		if (lattice_desc.structure == Lattice::hexagonal) {
			domain_size.y = ceil(domain_size.y / 0.866);
		}
		lattice_desc.size = max(lattice_desc.size, domain_size );
	}
	
	if (size_symbol_name.isDefined()) {
		size_symbol->set(SymbolFocus::global, lattice_desc.size);
	}
	
	lattice_desc.node_length = node_length();
	if (lattice_desc.node_length<=0)
		throw MorpheusException(string("Invalid NodeLength ") + to_str(lattice_desc.node_length), stored_node);
	
	lattice = Lattice::createLattice(lattice_desc);
	if (lattice->size().x<=0 || lattice->size().y<=0 || lattice->size().z<=0)
		throw MorpheusException(string("Invalid Lattice extends ") + to_str(lattice->size()), stored_node);
}


NeighborhoodDesc LatticePlugin::parseNeighborhood(XMLNode xnode)
{
	NeighborhoodDesc desc;
	if ( xnode.nChildNode("Distance") ) {
		double distance;
		getXMLAttribute(xnode,"Distance/text",distance);
		if( distance == 0 ){
		  throw string("Neighborhood/Distance must be greater than 0.");
		}
		desc.mode = NeighborhoodDesc::Distance;
		desc.distance = distance;
	}
	else if ( xnode.nChildNode("Order") ) {
		int order;
		getXMLAttribute(xnode,"Order/text",order);
		if( order == 0 ){
		  throw string("Neighborhood/Order must be greater than 0.");
		}
		desc.mode = NeighborhoodDesc::Order;
		desc.order = order;
	}
	else if ( xnode.nChildNode("Name") ) {
		string name = xnode.getChildNode("Name").getText();
		if (name.empty())
			throw string("Neighborhood/Name must contain a valid name.");
		desc.mode = NeighborhoodDesc::Name;
		desc.name = name;
	}
	else {
		throw string("Unknown neighborhood definition in ") + xnode.getName();
	}
	return desc;
}
