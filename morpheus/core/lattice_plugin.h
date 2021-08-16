#ifndef LATTICE_PLUGIN_H
#define LATTICE_PLUGIN_H

#include "config.h"
#include "interfaces.h"
#include "lattice.h"

class LatticePlugin : public Plugin {
public:
	DECLARE_PLUGIN("Lattice");
	LatticePlugin();
	void loadFromXML(XMLNode xnode, Scope* scope) override;
	Lattice::Structure getStructure() { return structure(); };
	void init(const Scope* scope) override;

	shared_ptr<Lattice> getLattice() { return lattice; };
	static NeighborhoodDesc parseNeighborhood(XMLNode xnode);
	const Length_Scale& getNodeLength() { return node_length; }
	
private:
	PluginParameter<Lattice::Structure, XMLNamedValueReader, RequiredPolicy> structure;
	PluginParameter<VDOUBLE, XMLEvaluator, RequiredPolicy> size;
	PluginParameter<string, XMLValueReader, OptionalPolicy> size_symbol_name;
	
	Length_Scale node_length;
	LatticeDesc lattice_desc;
	shared_ptr<Lattice> lattice;
	shared_ptr<SymbolRWAccessorBase<VDOUBLE>> size_symbol;
	Scope* lattice_scope;
};

#endif // LATTICE_PLUGIN_H
