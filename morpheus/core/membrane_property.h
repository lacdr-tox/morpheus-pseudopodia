//////
//
// This file is part of the modelling and simulation framework 'Morpheus',
// and is made available under the terms of the BSD 3-clause license (see LICENSE
// file that comes with the distribution or https://opensource.org/licenses/BSD-3-Clause).
//
// Authors:  Joern Starruss and Walter de Back
// Copyright 2009-2016, Technische Universität Dresden, Germany
//
//////

#ifndef MEMBRANE_PDE_H
#define MEMBRANE_PDE_H

#include "core/property.h"
#include "core/field.h"
#include "core/diffusion.h"
#include "core/interfaces.h"
#include "core/scales.h"


/**
\defgroup ML_MembraneProperty MembraneProperty
\ingroup ML_CellType
\ingroup Symbols

Symbol with a variable scalar field, mapped to a cell membrane that associates a scalar value to every lattice site in the domain.

A MembraneProperty is a circular (2D) or spherical (3D) lattice mapped to the surface nodes of a cell using polar coordinates.

- \b value: \b initial condition for the scalar field. May be given as a \ref MathExpressions. Use the \b SpaceSymbol of the \ref ML_MembraneLattice to define spatial the distribution.

Optionally, a \b Diffusion rate may be specified.

- \b rate: diffusion coefficient [(node length)² per (global time)]
- \b well-mixed (optional): if true, homogenizes scalar field. Requires rate=0.

**/

class MembranePropertyPlugin;
class MembraneProperty;
class MembranePropertySymbol;


/** Wrapper class that creates a membrane pde from XML data.
 *
 *  The membrane pde is included into the celltype a template and default membrane
*/
class MembranePropertyPlugin : public Plugin
{
public:
	DECLARE_PLUGIN("MembraneProperty");
	
	void loadFromXML(const XMLNode, Scope * scope ) override;
	void init(const Scope* scope) override;
	const string& getSymbol() { return symbol_name(); };
	
	static void loadMembraneLattice(const XMLNode& node, Scope* scope);
	static uint getResolution() { return resolution; }
	static VINT getSize() { return size; }
	static VINT orientationToMemPos(const VDOUBLE& direction);
	static VDOUBLE memPosToOrientation(const VINT& memPos);
	static double nodeSize(const VINT& memPos);
	static shared_ptr<const Lattice> lattice();

private:
	PluginParameter2<string, XMLValueReader, RequiredPolicy> symbol_name;
	shared_ptr<MembranePropertySymbol> symbol;
	shared_ptr<Diffusion> diffusion_plugin;
	shared_ptr<MembraneProperty> default_property;
	
	static shared_ptr<const Lattice>  membrane_lattice;
	static VINT size;
// 	static string size_symbol;
	static uint resolution;
	static string resolution_symbol;
	static vector<double> node_sizes;
	Length_Scale node_length;
};

/// Container class holding membrane property data
class MembraneProperty : public AbstractProperty {
public:
	MembraneProperty(MembranePropertyPlugin* parent, shared_ptr<PDE_Layer> membrane) : AbstractProperty(), membrane_pde(membrane), initialized(false), initializing(false), parent(parent) {};
	const MembraneProperty& operator=(const MembraneProperty& other) { membrane_pde->data = other.membrane_pde->data; return *this; }
	shared_ptr<AbstractProperty> clone() const override {
		return make_shared<MembraneProperty>(parent,membrane_pde->clone());
	};
	void init(const SymbolFocus& f) override { 
		if (!initialized)  { 
			if (initializing) {
				throw string("Detected circular dependencies in intitialization of symbol '") + parent->getSymbol() + "'!";
			}
			initializing = true;
			membrane_pde->init(f);
			initializing = false;
			initialized = true;
		} 
	}

	const string& symbol() const override { return  parent->getSymbol(); };

	const string& type() const override { return TypeInfo<double>::name();}
	void assign(std::shared_ptr<AbstractProperty> other) override {
		auto other_mem = dynamic_pointer_cast<MembraneProperty>(other);
		if (other_mem) {
			this->operator=(*other_mem);
		}
		else if (dynamic_pointer_cast<MembraneProperty >(other)) {
			membrane_pde->data = static_pointer_cast<MembraneProperty>(other)->membrane_pde->data;
		}
		else if (dynamic_pointer_cast<Property<double,double>>(other)) {
			membrane_pde->data = static_pointer_cast<Property<double,double>>(other)->value;
		}
		else 
			throw string("Failed to assign non-matching Property to MembraneProperty ");
	};
	void assign(const MembraneProperty& other) { this->operator=(other); };
	
	
	string XMLDataName() const override { return "MembranePropertyData"; }
	void restoreData(XMLNode node) override { membrane_pde->restoreData(node); };
	XMLNode storeData() const override { 
		auto mem_node = membrane_pde->storeData();
		mem_node.updateName(XMLDataName().c_str());
		mem_node.addAttribute("symbol-ref", symbol().c_str());
		return mem_node;
	};

	static uint getResolution() { return MembranePropertyPlugin::getResolution(); }
	static VINT getSize() { return MembranePropertyPlugin::getSize(); }
	static VINT orientationToMemPos(const VDOUBLE& direction) { return MembranePropertyPlugin::orientationToMemPos(direction); }
	static VDOUBLE memPosToOrientation(const VINT& memPos) { return MembranePropertyPlugin::memPosToOrientation(memPos); }
	static double nodeSize(const VINT& memPos) { return MembranePropertyPlugin::nodeSize(memPos); }
	static shared_ptr<const Lattice> lattice() { return MembranePropertyPlugin::lattice(); }
	
	shared_ptr<PDE_Layer> membrane_pde;
	
private:
	bool initialized; bool initializing;
	MembranePropertyPlugin* parent;
	friend class MembranePropertySymbol;
};



/** Writable Symbol Accessor for Membrane Properties 
 * 
 */
class MembranePropertySymbol : public SymbolRWAccessorBase<double> {
public:
	MembranePropertySymbol(MembranePropertyPlugin* parent, CellType* celltype, uint pid) :  SymbolRWAccessorBase<double>(parent->getSymbol()), property_id(pid), celltype(celltype), parent(parent) {
		this->flags().granularity = Granularity::MembraneNode;
	}
	const string&  description() const override { return parent->getDescription(); }
	const string XMLPath() const override { return getXMLPath(parent->saveToXML()); } 
	
	typename TypeInfo<double>::SReturn get(const SymbolFocus & f) const override { return getProperty(f)->membrane_pde->get(f.membrane_pos()); };
	
	typename TypeInfo<double>::SReturn safe_get(const SymbolFocus& f) const override {
		if (!this->flags().initialized) {
			this->safe_init();
		}
		
		auto p=getProperty(f); 
		if (!p->initialized)  {
			if (p->initializing) {
				throw string("Detected circular dependencies in evaluation of symbol '") + this->name() + "'!";
			}
			p->initializing = true;
			p->init(f);
			p->initializing = false;
			p->initialized = true;
		}
		return p->membrane_pde->get(f.membrane_pos());
	}
	
	void init() override {
		for ( auto cell_id : celltype->getCellIDs() ) {
			SymbolFocus f(cell_id);
			getProperty(f)->init(f);
		}
	}
	
	void set(const SymbolFocus & f, double val) const override { getProperty(f)->membrane_pde->set(f.membrane_pos(),val); };
	
	void setInitializer(shared_ptr<ExpressionEvaluator<double>> initializer, SymbolFocus f) const {
		auto p = getProperty(f)->membrane_pde;
		p->setInitializer( initializer );
	}
		
	void setBuffer(const SymbolFocus & f, double val) const override { getProperty(f)->membrane_pde->setBuffer(f.membrane_pos(),val); }
	
	void applyBuffer() const override;
	void applyBuffer(const SymbolFocus & f) const override { getProperty(f)->membrane_pde->applyBuffer(f.membrane_pos()); }
	PDE_Layer* getField(const SymbolFocus& f) const { 
		assert(f.cell().getCellType() == celltype);return static_pointer_cast<MembraneProperty>( f.cell().properties[property_id] )->membrane_pde.get(); }
	string linkType() const override { return "MembranePropertyLink"; }
private:
	MembraneProperty* getProperty(const SymbolFocus& f) const { return static_cast<MembraneProperty*>(f.cell().properties[property_id].get()); };
	uint property_id;
	CellType* celltype;
	MembranePropertyPlugin* parent;
};


/** Symbol for Membrane coordinates 
 */
class MembraneSpaceSymbol : public SymbolAccessorBase<VDOUBLE> {
public:
	MembraneSpaceSymbol(string symbol) : SymbolAccessorBase<VDOUBLE>(symbol) {
		flags().granularity = Granularity::MembraneNode;
	}
	const string& description() const override { static const string descr = "Membrane Position"; return descr;}
	typename TypeInfo<VDOUBLE>::SReturn get(const SymbolFocus & f) const override { return MembraneProperty::memPosToOrientation(f.membrane_pos()); }
	string linkType() const override { return "MembraneSpaceLink"; }
	
};

#endif 
