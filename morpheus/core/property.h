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
#include "cell.h"
#include "celltype.h"
#include <functional>
#include <assert.h>

template <class T>
class Container;

template <class T>
class Container : virtual public Plugin {
	
public:
	enum class Mode { Constant, Variable, CellProperty };
	static string ConstantXMLName();
	static string VariableXMLName();
	static string CellPropertyXMLName();
	
	Container(Mode mode);
	string XMLName() const override;
	const string& getName() const { return name.isDefined() ? name() : symbol();}
	const string& getSymbol() const { return symbol(); }
	T getInitValue(const SymbolFocus& F);
	
	void loadFromXML(XMLNode node, Scope* scope) override;
	XMLNode saveToXML() const override;
	void init(const Scope* scope) override;
	
	static  Plugin* createConstantInstance() { return new Container<T>(Mode::Constant); };
	static  Plugin* createVariableInstance() { return new Container<T>(Mode::Variable); };
	static  Plugin* createCellPropertyInstance() { return new Container<T>(Mode::CellProperty); };

	void assert_initialized(const SymbolFocus& f = SymbolFocus::global);
	
protected:
	static bool type_registration;
	bool initialized;
	const Mode mode;
	uint property_id;
	PluginParameter2<T,XMLEvaluator,RequiredPolicy> value;
	PluginParameter2<string,XMLValueReader,RequiredPolicy> symbol;
	PluginParameter2<string,XMLValueReader,OptionalPolicy> name;
	shared_ptr<SymbolBase> _accessor;
};


/** Generic Container interfaced Property template
 * 
 * T ist the interfactial type of the Symbol attached to the property
 * V is the actual value type of the property
 */
template <class T, class V = T>
class Property : public PrimitiveProperty<V> {
public:
	Property(Container<T>* parent, V value) :
		PrimitiveProperty<V>(parent->getSymbol(), value), initialized(false), parent(parent) {};
	
	Property(const Property<T,V>& other) : PrimitiveProperty<V>(other), initialized(false), parent(other.parent) {}
	
	shared_ptr<AbstractProperty> clone() const override {
		return make_shared< Property<T,V> >(*this);
		
	}
	void init(const SymbolFocus& f) override {
		this->value = parent->getInitValue(f);
		initialized = true;
	};
	
	void assign(shared_ptr<Property<T,V>> other) {
		 PrimitiveProperty<V>::assign(static_pointer_cast<PrimitiveProperty<V> >(other));
		 initialized = other->initialized;
	}
	void assign(shared_ptr<AbstractProperty> other) override {
		auto derived = dynamic_pointer_cast<Property<T,V>>(other);
		if (!derived) throw string("Type mismatch in property assignment");
		assign(derived);
	};
	
	string XMLDataName() const override { return parent->XMLName()+"Data"; }

// private:
	bool initialized;
	Container<T>* parent;
};

template <class T>
class ConstantSymbol : public PrimitiveConstantSymbol<T> {
	public:
		ConstantSymbol( Container<T>* parent ) : PrimitiveConstantSymbol<T>(parent->getSymbol(), "", T()), parent(parent), initialized(false) { };
		std::string linkType() const override { return "ConstantLink"; }
		void init() const { this->value = parent->getInitValue(SymbolFocus::global); }
		const string& description() const override { return parent->getName(); }
		typename TypeInfo<T>::SReturn safe_get(const SymbolFocus& f) const override { if ( !initialized) init(); return this->get(f); }
	private:
		Container<T>* parent;
		bool initialized;
		friend class Container<T>;
};

template <class T>
class VariableSymbol : public PrimitiveVariableSymbol<T> {
	public:
		VariableSymbol(Container<T>* parent ) : PrimitiveVariableSymbol<T>(parent->getSymbol(), "", T()), parent(parent), initialized(false) { };
		std::string linkType() const override { return "VariableLink"; }
		void init() const { this->value = parent->getInitValue(SymbolFocus::global); }
		const string& description() const override { return parent->getName(); }
		typename TypeInfo<T>::SReturn safe_get(const SymbolFocus& f) const override { if ( !initialized) init();  return this->get(f); }
	private:
		Container<T>* parent;
		bool initialized;
		friend class Container<T>;
};

template <class T>
class PropertySymbol : public PrimitivePropertySymbol<T> {
	public:
		PropertySymbol(Container<T>* parent, const CellType* ct, uint pid) : PrimitivePropertySymbol<T>(parent->getSymbol(), ct, pid), parent(parent) { }
		std::string linkType() const override { return "CellPropertyLink"; }
		const string& description() const override { if (parent) return parent->getName();  return this->name();}
		typename TypeInfo<T>::SReturn get(const SymbolFocus& f) const override { return getCellProperty(f)->value; }
		typename TypeInfo<T>::SReturn safe_get(const SymbolFocus& f) const override {
			auto p=getCellProperty(f); 
			if (!p->initialized) 
				p->value = parent->getInitValue(f);
			return p->value;
		}
		void init() const { 
			assert(this->celltype->default_properties.size()>this->property_id);
			auto p = dynamic_pointer_cast<Property<T,T>>(this->celltype->default_properties[this->property_id]);
			assert(p);
			try { 
				static_cast<Property<T,T>*>(this->celltype->default_properties[this->property_id].get())->value = parent->getInitValue(SymbolFocus::global); 
			} catch (...) { cout << "Warning: Could not intialize default property "<<  this->name() << " of celltype " << this->celltype->getName() << "." << endl; }
			
		}
		void set(const SymbolFocus& f, typename TypeInfo<T>::Parameter value) const override { getCellProperty(f)->value = value; };
		void setBuffer(const SymbolFocus& f, typename TypeInfo<T>::Parameter value) const override { getCellProperty(f)->buffer = value; };
		void applyBuffer() const override;
		void applyBuffer(const SymbolFocus& f) const override {  getCellProperty(f)->value =  getCellProperty(f)->buffer; };
	private:
		Property<T,T>* getCellProperty(const SymbolFocus& f) const { 
			assert(f.cell().getCellType() == this->celltype);
			return static_cast<Property<T,T>*>(f.cell().properties[this->property_id].get()); 
		}
		Container<T>* parent;
		friend class Container<T>;
};

typedef Property<double,deque<double>> DelayProperty;
template <>
void DelayProperty::init(const SymbolFocus& f) ;

class DelayPropertyPlugin : public Container<double>, public ContinuousProcessPlugin
{
protected:
	DelayPropertyPlugin(Mode mode);
	static bool type_registration;
	static const string CellPropertyXMLName() { return "DelayProperty"; };
	static const string VariableXMLName() { return "DelayVariable"; };
	static Plugin* createVariableInstance();
	static Plugin* createCellPropertyInstance();

	double delay;
	bool tsl_initialized;

public:
	string XMLName() const override { if (mode==Mode::Variable) return VariableXMLName(); else return  CellPropertyXMLName(); }

	void setTimeStep(double t) override;
	void prepareTimeStep() override {};
	void executeTimeStep() override;
	
	void loadFromXML(XMLNode node, Scope* scope) override;
	void init(const Scope* scope) override;
};

class DelayPropertySymbol : public SymbolRWAccessorBase<double> {
public:
	DelayPropertySymbol(DelayPropertyPlugin* parent, const CellType* ct, uint pid) : 
		SymbolRWAccessorBase<double>(parent->getSymbol()), parent(parent), celltype(ct), property_id(pid)
	{ 
		this->flags().delayed = true;
		this->flags().granularity = Granularity::Cell;
	};
	std::string linkType() const override { return "DelayPropertyLink"; }
	const string& description() const override { if (parent) return parent->getName();  return this->name();}
	double get(const SymbolFocus& f) const override { return getCellProperty(f)->value.front(); }
	double safe_get(const SymbolFocus& f) const override {
		auto p=getCellProperty(f); 
		if (!p->initialized)
			p->init(f);
		return p->value.front();
	}
	void init() { 
		DelayProperty* p = static_cast<DelayProperty*>(celltype->default_properties[property_id].get());
		if (!p->initialized) 
			try {
				p->init(SymbolFocus::global);
			} catch (...) { cout << "Warning: Could not initialize default property " << this->name() << " of CellType " << celltype->getName() << "." << endl;}
	}
	void set(const SymbolFocus& f, double value) const override { getCellProperty(f)->value.back() = value; };
	void setBuffer(const SymbolFocus& f, double value) const override { getCellProperty(f)->value.back() = value; };
	void applyBuffer() const override {};
	void applyBuffer(const SymbolFocus&) const override {};
private:
	DelayProperty* getCellProperty(const SymbolFocus& f) const { 
		return static_cast<DelayProperty*>(f.cell().properties[property_id].get()); 
	}
	DelayPropertyPlugin* parent;
	const CellType* celltype;
	uint property_id;
	friend class DelayPropertyPlugin;
};

class DelayVariableSymbol : public SymbolRWAccessorBase<double>  {
public:
	DelayVariableSymbol(DelayPropertyPlugin* parent) : SymbolRWAccessorBase<double>(parent->getSymbol()), parent(parent), property(parent, deque<double>()) {
		this->flags().delayed = true;
	};
	std::string linkType() const override { return "DelayVariableLink"; }
	const string& description() const override { if (parent) return parent->getName();  return this->name();}
	double get(const SymbolFocus&) const override { return property.value.front(); }
	double safe_get(const SymbolFocus&) const override {
		if (!property.initialized) {
			property.init(SymbolFocus::global);
		}
		return property.value.front();
	}
	void init() { if (!property.initialized) property.init(SymbolFocus::global); }
	void set(const SymbolFocus&, double value) const override { property.value.back() = value; };
	void setBuffer(const SymbolFocus&, double value) const override { property.value.back() = value; };
	void applyBuffer() const override {};
	void applyBuffer(const SymbolFocus&) const override {};
private:
	DelayPropertyPlugin* parent;
	mutable DelayProperty property;
	friend class DelayPropertyPlugin;
};




//-------------------------------------------------------------------------
//------------------- TEMPLATE IMPLEMENTATION
//-------------------------------------------------------------------------



template <class T>
Container<T>::Container(Mode mode) : mode(mode), initialized(false) {
	name.setXMLPath("name");
	this->registerPluginParameter(name);
	symbol.setXMLPath("symbol");
	this->registerPluginParameter(symbol);
	value.setXMLPath("value");
	this->registerPluginParameter(value);
};


template <class T>
string Container<T>::XMLName() const {
	switch (mode) {
		case Mode::Constant :
			return ConstantXMLName();
		case Mode::Variable :
			return VariableXMLName();
		case Mode::CellProperty :
			return CellPropertyXMLName();
	}
	return "NoName";
}

template <> string Container<double>::ConstantXMLName();
template <> string Container<VDOUBLE>::ConstantXMLName();
template <> string Container<double>::VariableXMLName();
template <> string Container<VDOUBLE>::VariableXMLName();
template <> string Container<double>::CellPropertyXMLName();
template <> string Container<VDOUBLE>::CellPropertyXMLName();

// template <class T> 
// bool Container<T>::type_registration = PluginFactory::RegisterCreatorFunction( Container<T>::ConstantXMLName(),Container<T>::createConstantInstance) 
// 								   && PluginFactory::RegisterCreatorFunction( Container<T>::VariableXMLName(), Container<T>::createVariableInstance)
// 								   && PluginFactory::RegisterCreatorFunction( Container<T>::CellPropertyXMLName(), Container<T>::createCellPropertyInstance);


template <class T>
void Container<T>::loadFromXML(XMLNode node, Scope* scope)
{
	if (this->XMLName() != node.getName()) {
		cout << this->XMLName() << " != " << node.getName() << endl;
		assert(this->XMLName() == node.getName());
	}
	if ( ! Container<T>::type_registration ) {
		// Do not remove this check, since it ensures that property registration actually takes place.
		// The check depends on Property<T>::type_registration.
		throw string("Container type ") + XMLName() + " is not registered.\nThis is a fatal error!";
	}
	
	Plugin::loadFromXML(node, scope);
	
	switch (mode) {
		case Mode::Constant : {
			_accessor = make_shared<ConstantSymbol<T>>(this);
			auto val_override = scope->value_overrides().find(symbol());
			if ( val_override != scope->value_overrides().end()) {
				value.read(val_override->second);
				scope->value_overrides().erase(val_override);
			}
			break;
		}
		case Mode::Variable : 
			_accessor = make_shared<VariableSymbol<T>>(this);
			break;
		case Mode::CellProperty : 
			auto ct =  scope->getCellType();
			if (!ct)
				throw MorpheusException("Cell properties can only be defined for CellTypes", node);
			auto property_id = ct->addProperty(make_shared<Property<T,T>>(this,T()));
			_accessor = make_shared<PropertySymbol<T>>(this, ct, property_id);
			break;
	}
	
	scope->registerSymbol(_accessor);
};

template <class T>
XMLNode Container<T>::saveToXML() const {
	if (mode == Mode::Variable) {
		stored_node.updateAttribute("value",to_cstr(static_pointer_cast<VariableSymbol<T>>(_accessor)->value));
	}
	return stored_node;
};

template <class T>
void Container<T>::init(const Scope* scope) {
	if (initialized) 
		return;
	
	Plugin::init(scope);
	initialized = true;
	
	switch(mode) {
		case Mode::Constant:
			static_pointer_cast<ConstantSymbol<T>>(_accessor)->init();
			break;
		case Mode::Variable:
			static_pointer_cast<VariableSymbol<T>>(_accessor)->init();
			break;
		case  Mode::CellProperty:
			// try to initialized the celltype bound property temlate
			static_pointer_cast<PropertySymbol<T>>(_accessor)->init();
			break;
	}
	
}

template <class T>
T Container<T>::getInitValue(const SymbolFocus& f) { 
	if (! initialized)
		init(local_scope);
	return value.safe_get(f);
}

template<class T> void PropertySymbol<T>::applyBuffer() const
{
	auto cells = this->celltype->getCellIDs();
	for (auto cell : cells) {
		auto p = getCellProperty(SymbolFocus(cell));
		p->value = p->buffer;
	}
}


#endif // PROPERTY_H

