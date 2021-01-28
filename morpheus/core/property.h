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

/**
\defgroup ML_Constant Constant
\ingroup ML_Global
\ingroup ML_CellType
\ingroup ML_Contact
\ingroup ML_System
\ingroup ML_Event
\ingroup ML_Analysis
\ingroup Symbols

Symbol with a fixed scalar value given by a \ref MathExpressions.
**/
/**
\defgroup ML_ConstantVector ConstantVector
\ingroup ML_Global
\ingroup ML_CellType
\ingroup ML_System
\ingroup ML_Event
\ingroup ML_Analysis
\ingroup Symbols

Symbol with a fixed 3D vector value, given by a \ref MathExpressions.

Syntax is comma-separated as given by \b notation :
	orthogonal - x,y,z
	radial     - r,φ,θ
	or radial  - φ,θ,r
**/
/**
\defgroup ML_Variable Variable
\ingroup ML_Global
\ingroup ML_CellType
\ingroup Symbols

Symbol with a variable scalar value. The initial value is given by a \ref MathExpressions.
**/
/**
\defgroup ML_VariableVector VariableVector
\ingroup ML_Global
\ingroup ML_CellType
\ingroup Symbols

Symbol with a variable 3D vector value. The initial value is given by a \ref MathExpressions.

Syntax is comma-separated as given by \b notation :
  - orthogonal - x,y,z
  - radial     - r,φ,θ
  - or radial  - φ,θ,r
**/

/**
\defgroup ML_DelayVariable DelayVariable
\ingroup ML_Global
\ingroup Symbols

Symbol with a scalar value and a \b delay time until an assigned values become current. The initial value and history is given by a \ref MathExpressions.
**/

/**
\defgroup ML_Property Property
\ingroup ML_CellType
\ingroup Symbols


Symbol with a cell-bound, variable scalar value. The initial value is given by a \ref MathExpressions and may contain stochasticity to create diversity.
**/

/**
\defgroup ML_DelayProperty DelayProperty
\ingroup ML_CellType
\ingroup Symbols


Symbol with a cell-bound scalar value and a \b delay time until values become current. The initial value and history is given by a \ref MathExpressions
**/

/**
\defgroup  ML_PropertyVector PropertyVector
\ingroup ML_CellType
\ingroup Symbols

Symbol with cell-bound, variable 3D vector value. The initial value and history is  given by a \ref MathExpressions.

Syntax is comma-separated as given by \b notation :
  - orthogonal - x,y,z
  - radial     - r,φ,θ
  - or radial  - φ,θ,r
**/





/** 
 * Plugin Frontend for the configuration of Properties
 * 
 * This is the handler for interfacing Properties with the XML model definition.
 * The handler reads the XML, creates Properties (incl. Constants and Variables)
 * and provides the initializers. 
 * 
 */

template <class T>
class Container : virtual public Plugin {
	
public:
	enum class Mode { Constant, Variable, CellProperty };
	// manual replacement for the DECLARE_PLUGIN macro
	static string ConstantXMLName();
	static string VariableXMLName();
	static string CellPropertyXMLName();
	
	static  Plugin* createConstantInstance() { return new Container<T>(Mode::Constant); };
	static  Plugin* createVariableInstance() { return new Container<T>(Mode::Variable); };
	static  Plugin* createCellPropertyInstance() { return new Container<T>(Mode::CellProperty); };
	
	Container(Mode mode);
	~Container() { 
		if (_accessor && _accessor->scope()) 
			const_cast<Scope*>(_accessor->scope())->removeSymbol(_accessor); }
	string XMLName() const override;
	const string& getName() const { return name.isDefined() ? name() : symbol();}
	const string& getSymbol() const { return symbol(); }
	T getInitValue(const SymbolFocus& F);
	string getInitExpression() { return value.stringVal(); };
	
	void loadFromXML(XMLNode node, Scope* scope) override;
	XMLNode saveToXML() const override;
	void init(const Scope* scope) override;
	

	void assert_initialized(const SymbolFocus& f = SymbolFocus::global);
	
protected:
	static bool type_registration;
	bool initialized;
	const Mode mode;
	uint property_id;
	PluginParameter2<T,XMLEvaluator,RequiredPolicy> value;
	PluginParameter2<string,XMLValueReader,RequiredPolicy> symbol;
	PluginParameter2<string,XMLValueReader,OptionalPolicy> name;
	PluginParameter2<VecNotation,XMLNamedValueReader,DefaultValPolicy> notation;
	shared_ptr<SymbolBase> _accessor;
};


/** Generic Container-interfaced Property
 * 
 * This template class stores one value, and initializes the value using the Container
 * XML-interface class. A Data tag name for snapshoting/loading of the value is derived from the 
 * Container XML tag name by appending 'Data'.
 * 
 * T is the interfacial type of the Symbol attached to the property
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
	
	void restoreData(XMLNode node) override { 
		PrimitiveProperty<V>::restoreData(node);
		initialized = true;
	}
	
	void init(const SymbolFocus& f) override {
		if (! initialized) {
			if (initializer)
				this->value = initializer->get(f);
			else
				this->value = parent->getInitValue(f);
		}
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
	shared_ptr<ExpressionEvaluator<V>> initializer;
	Container<T>* parent;
};


/** Symbol (accessor) for constant values
 * 
 * The ConstantSymbol template attaches a constant value to a scope.
 * It provides all the necessary meta-information (type, constness, symbol name)
 * and mediates access to the value.
 */
template <class T>
class ConstantSymbol : public PrimitiveConstantSymbol<T> {
	public:
		ConstantSymbol( Container<T>* parent ) : PrimitiveConstantSymbol<T>(parent->getSymbol(), "", T()), parent(parent), initialized(false) { };
		std::string linkType() const override { return "ConstantLink"; }
		void init() const { this->value = parent->getInitValue(SymbolFocus::global);  initialized = true;}
		const string& description() const override { return parent->getDescription(); }
		const std::string XMLPath() const override { return getXMLPath(parent->saveToXML()); };
		typename TypeInfo<T>::SReturn safe_get(const SymbolFocus& f) const override { if ( !initialized) init(); return this->get(f); }
	private:
		Container<T>* parent;
		mutable bool initialized;
		friend class Container<T>;
};

/** Symbol (accessor) for variable values
 * 
 * The VariableSymbol template attaches a variable value to a scope.
 * It provides all the necessary meta-information (type, constness, symbol name)
 * and mediates access to the value. 
 * 
 * When snapshoting, the value is copied back to the XML interfacing Container.
 */
template <class T>
class VariableSymbol : public PrimitiveVariableSymbol<T> {
	public:
		VariableSymbol(Container<T>* parent ) : PrimitiveVariableSymbol<T>(parent->getSymbol(), "", T()), parent(parent), initialized(false) { };
		std::string linkType() const override { return "VariableLink"; }
		void init() const { this->value = parent->getInitValue(SymbolFocus::global); initialized = true; cout << "set init value " << this->name() << "=" << this->value << endl; }
		const string& description() const override { return parent->getDescription(); }
		const std::string XMLPath() const override { return getXMLPath(parent->saveToXML()); };
		typename TypeInfo<T>::SReturn safe_get(const SymbolFocus& f) const override { if ( !initialized) init();  return this->get(f); }
	private:
		Container<T>* parent;
		mutable bool initialized;
		friend class Container<T>;
};

/** Symbol (accessor) for cell-attached Properties
 * 
 * Alike the PrimitivePropertySymbol, the PropertySymbol attaches Properties to a scope.
 * In addition, it interfaces to a Container plugin to mediate initialization, snapshoting and loading of values
 * 
 * When snapshoting, the XML data tag name is derived from the XML interfacing Container tag name by appending 'Data'.
 */
template <class T>
class PropertySymbol : public PrimitivePropertySymbol<T> {
	public:
		PropertySymbol(Container<T>* parent, const CellType* ct, uint pid) : PrimitivePropertySymbol<T>(parent->getSymbol(), ct, pid), parent(parent) { }
		std::string linkType() const override { return "CellPropertyLink"; }
		const string& description() const override { return parent->getDescription();}
		const std::string XMLPath() const override { return getXMLPath(parent->saveToXML()); };
// 		typename TypeInfo<T>::SReturn get(const SymbolFocus& f) const override { return getCellProperty(f)->value; }
		typename TypeInfo<T>::SReturn safe_get(const SymbolFocus& f) const override {
			auto p=getCellProperty(f); 
			if (!p->initialized)  {
				p->value = parent->getInitValue(f);
				p->initialized = true;
			}
			return p->value;
		}
		
		void setInitializer(shared_ptr<ExpressionEvaluator<T>> initializer, SymbolFocus f) const {
			auto p = getCellProperty(f);
			p->initializer = initializer;
		}
		
		void init() const { 
			assert(this->celltype->default_properties.size()>this->property_id);
			auto p = dynamic_pointer_cast<Property<T,T>>(this->celltype->default_properties[this->property_id]);
			assert(p);
			try { 
				static_cast<Property<T,T>*>(this->celltype->default_properties[this->property_id].get())->value = parent->getInitValue(SymbolFocus::global);
				static_cast<Property<T,T>*>(this->celltype->default_properties[this->property_id].get())->initialized = true;
			} catch (...) { cout << "Warning: Could not initialize default property "<<  this->name() << " of celltype " << this->celltype->getName() << "." << endl; }
			
		}
	protected:
		/// Provide a Property<T,T> associated with the SymbolFocus @p f
		Property<T,T>* getCellProperty(const SymbolFocus& f) const { 
			assert(f.cell().getCellType() == this->celltype);
			return static_cast<Property<T,T>*>(f.cell().properties[this->property_id].get()); 
		}
		Container<T>* parent;
// 		friend class Container<T>;
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
	notation.setXMLPath("notation");
	notation.setDefault("x,y,z");
	notation.setConversionMap(VecNotationMap());
	registerPluginParameter(notation);
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

// template <> string Container<double>::ConstantXMLName();
// template <> string Container<VDOUBLE>::ConstantXMLName();
// template <> string Container<double>::VariableXMLName();
// template <> string Container<VDOUBLE>::VariableXMLName();
// template <> string Container<double>::CellPropertyXMLName();
// template <> string Container<VDOUBLE>::CellPropertyXMLName();


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
	
	value.setNotation(notation());
	
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
		case Mode::Variable : {
			_accessor = make_shared<VariableSymbol<T>>(this);
			auto val_override = scope->value_overrides().find(symbol());
			if ( val_override != scope->value_overrides().end()) {
				value.read(val_override->second);
				scope->value_overrides().erase(val_override);
			}
			break;
		}
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
		stored_node.addAttribute("value",TypeInfo<T>::toString(static_pointer_cast<SymbolAccessorBase<T>>(_accessor)->get(SymbolFocus::global)).c_str());
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

// template<class T> void PropertySymbol<T>::applyBuffer() const
// {
// 	auto cells = this->celltype->getCellIDs();
// 	for (auto cell : cells) {
// 		auto p = getCellProperty(SymbolFocus(cell));
// 		p->value = p->buffer;
// 	}
// }


#endif // PROPERTY_H

