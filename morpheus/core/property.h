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

#ifndef PROPERTY_H
#define PROPERTY_H

#include "interfaces.h"
#include "vec.h"
#include "symbol.h"


/** \brief Base class of all typed properties
 * 
 *  Provides the basic interface for reading ang writing Properties and Constants as plugins to XML
 */

class AbstractProperty : public virtual Plugin {

public:
	virtual void init(const Scope* scope) { init(scope, SymbolFocus::global); }
	virtual void init(const Scope * scope, const SymbolFocus& f) { if (initialized) return; Plugin::init(scope); initialized = true; };
	
	virtual void restoreData(XMLNode parent_node) = 0;
	virtual XMLNode storeData() const = 0;
	
	virtual shared_ptr<AbstractProperty> clone() const =0;
	virtual string getTypeName() const =0;
	
	string getName() const { return name;}
	string getSymbol() const { return symbolic_name; }

	bool isCellProperty() const { return is_cellproperty; }
	bool isConstant() const { return is_constant;}
	bool isDelayed() const { return is_delayed; } 
	bool isInitialized() const { return initialized; }
	
// a little bit of hacking is needed to get segmented cells running the easy way ...
	bool isSubCellular() { return sub_cellular; }
	void setSubCellular(bool value) { sub_cellular = value; };
	
protected:
	AbstractProperty(bool constant, bool cellproperty) : is_cellproperty(cellproperty), is_constant(constant), is_delayed(false), initialized(false), sub_cellular(false) {};
	AbstractProperty(string n, string s, bool constant, bool cellproperty, bool delayed ) : name(n) , symbolic_name(s), is_cellproperty(cellproperty), is_constant(constant), is_delayed(delayed), initialized(false), sub_cellular(false) {};
	~AbstractProperty() {};
	string name, symbolic_name;
	const bool is_cellproperty;
	const bool is_constant;
	const bool is_delayed;
	bool initialized;
	bool sub_cellular;

};

// Property factory: Creates typed properties through the type agnostice AbstractProperty interface.
// typedef CClassFactory<string, AbstractProperty> PropertyFactory;, true, true

// Each property should receive an implementation of getTypeName in order to provide a proper type description in the XMLable
template <class ValType = double>
class Property : public AbstractProperty {
public:
	typedef ValType Value_Type;
	static const string global_xml_name();
	static const string constant_xml_name();
	static const string property_xml_name();

protected:
	static bool type_registration;

	static Plugin* createPropertyInstance();
	static Plugin* createVariableInstance();
	static Plugin* createConstantInstance();
	Value_Type value, buffer;
	string string_val;
	Property(string name, string symbol, bool constant, bool cellproperty=false, bool delayed=false) : AbstractProperty(name, symbol, constant, cellproperty, delayed) {};
	Property(bool constant=false, bool cellproperty = false) : AbstractProperty(constant, cellproperty) {};
	
public:

	static shared_ptr< Property<ValType> > createPropertyInstance(string symbol, string name);
	static shared_ptr< Property<ValType> > createVariableInstance(string symbol, string name);
	static shared_ptr< Property<ValType> > createConstantInstance(string symbol, string name);
	
	virtual string getTypeName() const { return TypeInfo<ValType>::name(); }
    virtual string XMLName() const {return is_constant ? constant_xml_name() : (this->is_cellproperty ? property_xml_name() :  global_xml_name()); }
    string XMLDataName() const { return XMLName() +"Data"; }
    
	const Value_Type& get() const { assert(initialized); return value; }
	Value_Type& getRef() { return value; }
	virtual void set(Value_Type value) { this->value = value; initialized = true;}
	virtual void setBuffer(Value_Type value) { this->buffer = value; }
	virtual void applyBuffer() { value = buffer; }
	virtual shared_ptr<AbstractProperty> clone() const;
	virtual void loadFromXML(XMLNode node); //--> passed to any derived type without mods
	virtual void init(const Scope* scope, const SymbolFocus& f ) { AbstractProperty::init(scope, f); };
// 	virtual XMLNode saveToXML() const; //--> passed to any derived type without mods
	virtual void restoreData(XMLNode node); //--> passed to any derived type without mods
	virtual XMLNode storeData() const;//--> passed to any derived type without mods
	
	

};

template <>
void Property<double>::init(const Scope * scope, const SymbolFocus& f);

template <>
void Property<VDOUBLE>::init(const Scope * scope, const SymbolFocus& f);

class DelayProperty : public Property<double>, public ContinuousProcessPlugin
{
protected:
    DelayProperty(bool cellproperty = false);
	static bool type_registration;
	static const string property_xml_name() { return "DelayProperty"; };
	static const string global_xml_name() { return "DelayVariable"; };
	static Plugin* createPropertyInstance();
	static Plugin* createVariableInstance();
	double_queue queue;
	double delay;
	bool tsl_initialized; /// Only the Celltype Plugin shall register as a TSL, all clones are managed by the ancestor

// 	const Scope* scope;
	mutable std::set<shared_ptr<DelayProperty> > clones;

public:
	DelayProperty(string name, string symbol, bool cellproperty);

	string XMLName() const override { return this->is_cellproperty ? property_xml_name() : global_xml_name(); }

    void setTimeStep(double t) override;
	void prepareTimeStep() override {};
	void executeTimeStep() override;

// 	virtual Value_Type& getRef() { return queue.front(); }
	void set(Value_Type value) override { queue.back() = value; }
	void setBuffer(Value_Type value) override { buffer = value; }
	void applyBuffer() override { queue.back() = buffer;}
	shared_ptr<AbstractProperty> clone() const;
	void loadFromXML(XMLNode node) override; //--> passed to any derived type without mods
	void init(const Scope* scope) override;
    void init(const Scope* scope, const SymbolFocus& f) override;
// 	virtual XMLNode saveToXML() const; //--> passed to any derived type without mods
	void restoreData(XMLNode node) override; //--> passed to any derived type without mods
	XMLNode storeData() const override;//--> passed to any derived type without mods
};

namespace SIM {
	void defineSymbol(shared_ptr<AbstractProperty>);

	template <class T>
	void defineSymbol(shared_ptr< Property<T> > p) {
		defineSymbol(static_pointer_cast<AbstractProperty>(p));
	}
}

//-------------------------------------------------------------------------
//------------------- TEMPLATE IMPLEMENTATION

template <class T>
shared_ptr<AbstractProperty> Property<T>::clone() const {
	return shared_ptr<AbstractProperty>(new Property<T>(*this) );
}

template <class T> 
Plugin* Property<T>::createPropertyInstance() { return new Property<T>(false, true); }
template <class T> 
shared_ptr< Property<T> > Property<T>::createPropertyInstance(string symbol, string name) { return shared_ptr< Property<T> >(new Property<T>(name, symbol, false, true) ); }
template <class T> 
Plugin* Property<T>::createVariableInstance() { return new Property<T>(false); }
template <class T> 
shared_ptr< Property<T> > Property<T>::createVariableInstance(string symbol, string name) { return shared_ptr< Property<T> >(new Property<T>(name, symbol,false) ); }
template <class T>
Plugin* Property<T>::createConstantInstance() { return new Property<T>(true); }
template <class T>
shared_ptr< Property<T> > Property<T>::createConstantInstance(string symbol, string name) { return shared_ptr< Property<T> >(new Property<T>(name, symbol, true) ); }


template <class T> 
bool Property<T>::type_registration = PluginFactory::RegisterCreatorFunction( Property<T>::global_xml_name(), Property<T>::createVariableInstance) 
								   && PluginFactory::RegisterCreatorFunction( Property<T>::constant_xml_name(), Property<T>::createConstantInstance)
								   && PluginFactory::RegisterCreatorFunction( Property<T>::property_xml_name(), Property<T>::createPropertyInstance);

template <class T> 
XMLNode Property<T>::storeData() const {
	XMLNode node = XMLNode::createXMLTopNode(XMLDataName().c_str());
	node.addAttribute("symbol-ref",symbolic_name.c_str());
	node.addAttribute("value",to_cstr(value));
	return node;
};

template <class T> 
void Property<T>::restoreData(XMLNode parent_node){
	XMLNode node = parent_node.getChildNodeWithAttribute(XMLDataName().c_str(),"symbol-ref",symbolic_name.c_str());
	if (!node.isEmpty()) {
		if ( getXMLAttribute(node,"value",value,false)) {
			cout << symbolic_name << "=" << value << "; ";
		} else {
			cerr << "Property<T>::restoreData: Cannot restore PropertyData of referenced symbol \"" << symbolic_name << endl;
		}
	}
}


template <class T>
void Property<T>::loadFromXML(const XMLNode node) {
	Plugin::loadFromXML(node);
	if (this->XMLName() != node.getName()) {
		cout << this->XMLName() << " != " << node.getName() << endl;
	}
	assert(this->XMLName() == node.getName());

	if ( ! Property<T>::type_registration ) {
		// Do not remove this check, since it ensures that property registration actually takes place.
		// The check depends on Property<T>::type_registration.
		throw string("Property type ") + getTypeName() + " is not registered.\nThis is a fatal error!";
	}
		
	if ( ! getXMLAttribute(node, "symbol", symbolic_name) ) {
		throw string("Missing symbol while loading ") + this->XMLName();
	}
	
	if ( ! getXMLAttribute(node, "value",string_val)) {
		throw string("Missing value while loading ") + this->XMLName() + " with symbol: \"" + symbolic_name + "\".";
	}
	
	if (TypeInfo<T>::name() !=TypeInfo< double >::name() && TypeInfo<T>::name() !=TypeInfo< VDOUBLE >::name()) {
		stringstream s(string_val);
		s >> value;
	}
	else {
		initialized = false;
	}

	getXMLAttribute(node, "name",  name);
};


#endif // PROPERTY_H

