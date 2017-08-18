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

#ifndef PLUGIN_PARAMETER_H
#define PLUGIN_PARAMETER_H
// #include "function.h"

#include "string_functions.h"
#include "simulation.h"
#include "expression_evaluator.h"
#include "symbol_accessor.h"
/* Can we get an implicitely shared behavior for PluginParameters?
   That would help largely to move Parameter objects around.
   
   * We should preserve the policy based interface enrichment
   * We also have to preserve the joint virtual base-class
   
   What about a template wrapper
   
	class PluginParameterBase {
		public:
			virtual PluginParameterBasePrivate* operator->() =0;
		protected:
			PluginParameterBase() {};
// 			shared_ptr<PluginParameterBasePrivate> base_d;
		private:
	}
	
	template <class T, template <class T, class R> class XMLValueInterpreter = XMLValueReader, class RequirementPolicy = RequiredPolicy >
	class PluginParameter : public PluginParameterBase {
		public:
			PluginParameterBase() { d= make_shared<ParameterPrivate>(); }
			
			typdef PluginParameterPrivate<T, XMLValueInterpreter, RequirementPolicy> ParameterPrivate;
			
			// TODO : does this work to override to the baseclass operator ???
			ParameterPrivate* operator->() override { return d; };
		protected:
			shared_ptr<ParameterPrivate> d;
   }
   
*/

/**
 * 
 * 
 * \page PluginParameters
 * @{
 * \brief Flexible Plugin Paramters to be used in Plugins for accessing XML data
 * 
 * \section Usage
 * 
 * Reading from XML can be rather cumbersome, espacially when it comes to the interpretation of a 
 * provided string literal: Is it a value, what kind of value, or is it rather an
 * expression, which should be evaluated or do we even require a symbol which we can write to?
 *
 * The morpheus framework casts all these decisions into a policy-based template concept of PluginParameter.
 * This template class are instantiated by selecting the type and a set of policies alongside to 
 * obtain a preconfigured object of your choice:
 * 
 * Syntax: PlugParameters2\<value_type, ReaderPolicy, RequirementPolicy\> my_value;
 * 
 * Reader policies: 
 *   - XMLValueReader
 *   - XMLEvaluator
 *   - XMLNamedValueReader
 *   - XMLWriteSymbol
 *   - XMLReadWriteSymbol
 * 
 * Requirement policies:
 *   - RequiredPolicy
 *   - DefaultValPolicy
 *   - OptionalPolicy
 * 
 * For convenience, a specialised template exists for identifying a celltype via a celltype name 
 *    PluginParameterCellType\<RequirementPolicy\>
 * 
 * \subsection Examples Examples
 * 
 * \subsubsection Example1 Example 1: Reading a double value and assuming 0 for the case it is ommited
 * \verbatim  PluginParameter2<double,XMLValueReader,DefaultValPolicy> my_value;\endverbatim
 * 
 * \remark For ALL PluginParameter2 the XML path referring to the value, has to be provided  relative to the plugin path.
 * For optional XMLvalues also a default value must be set in the plugin constructor.
 \verbatim  
 MyPlugin() {
	my_value.setXMLPath("Param1/value");
	my_value.setDefault(1.5);
	addPluginParameter(&my_value);
	
 }
 \endverbatim
 * 
 * \subsubsection Example2 Example 2: Reading an Value or Expression that is optional
 \verbatim PluginParameter2<double,XMLEvaluator,OptionalPolicy> my_value; \endverbatim
 * 
 * \subsubsection Example3 Example 3: Reading a String from XML and map it to an internal value
 \verbatim PluginParameter2<map<string, any type>,XMLNamedValueReader,RequiredPolicy> my_value; \endverbatim
 * \remark In addition to the XML path, also the mapping list must be provided
 \verbatim 
 MyPlugin() {
	my_value.setXMLPath("Param1/value")
 	my_value.setConversionMap( [map<string, value_type>] my_conversion_rules);
 }
 \endverbatim
 * 
 * \subsubsection Example4 Example 4: Reading a String from XML that shall represent a writable symbol
 * \verbatim PluginParameter2<double,XMLWriteSymbol,RequiredPolicy> my_value; \endverbatim

 * \subsubsection Example4 Example 4: Reading a String from XML that shall represent a Celltype name
 * \verbatim PluginParameterCellType<RequiredPolicy> my_value; \endverbatim

*/


/**
 * \brief Internal interface for Plugin Parameters 
 */
/*
template <class T, template <class T, class R> class XMLValueInterpreter, class RequirementPolicy>
class PluginParameter2 ;*/

// The type agnostic interface for the integration into the plugin architecture.
class PluginParameterBase {
public:
	virtual void loadFromXML(XMLNode node) =0; // read from value, optionally from symbol
	virtual void init() =0;
	virtual void read(string value) =0;
	virtual string XMLPath() const =0;
	virtual set<SymbolDependency> getDependSymbols() const =0;
	virtual set<SymbolDependency> getOutputSymbols() const =0;
};

/** 
 * Policy class to be used to create a REQUIRED PluginParameter 
 */


class RequiredPolicy {
public:
	RequiredPolicy() {};
	bool isRequired() const { return true; }
	bool isMissing() const { return false; }
	const string& stringVal() const { return string_value; };
protected:
	void assertDefined() const {};
	void setMissing() const { throw  string("Required parameter not set!"); }
	void setStringVal(const string& val) { string_value = val; }
	// Delete Policies only from derived classes
	~RequiredPolicy() {};
private:
	string string_value;
};

/** 
 * Policy class to be used to create an OPTIONAL PluginParameter 
 */


class  OptionalPolicy {
public:
	OptionalPolicy(): is_missing(true) {}
	bool isRequired() const  { return false; }
	bool isMissing() const { return is_missing; };
	const string& stringVal() const { return string_value; };

protected:
	void assertDefined() const { if (is_missing) throw string("Optional parameter queried although it is not defined!"); };
	void setMissing() { is_missing=true; }
	void setStringVal(const string& val) { string_value = val; is_missing = false; }
	// Delete Policies only from derived classes
	~OptionalPolicy() {};
private:
	string string_value;
	bool is_missing;
}; 

/** 
 * Policy class to be used to create an PluginParameter with DEFAULT VALUE, if the parameter is ommited
 */


class DefaultValPolicy {
	public:
	DefaultValPolicy() : default_defined(false) {}
	bool isRequired() const  { return false; }
	bool isMissing() const { return false; };
	const string& stringVal() const { return string_value; };
	void setDefault(string val) { default_value = val; default_defined=true; };
protected:
	void assertDefined() const {};
	void setMissing() { if (!default_defined) throw string("PluginParameter::DefaultValPolicy: No default value provided"); string_value=default_value; }
	void setStringVal(const string& val) { string_value = val; }
	// Delete Policies only from derived classes
	~DefaultValPolicy() {};
private:
	string default_value, string_value; 
	bool default_defined;
};

/** 
 * Policy class to be used to create a read-only, fixed value PluginParameter 
 */

template <class ValType, class RequirementPolicy> 
class XMLValueReader : public RequirementPolicy
{
public:
	typename TypeInfo<ValType>::SReturn operator()()  const {
		RequirementPolicy::assertDefined();
		return const_val;
	};
	typename TypeInfo<ValType>::SReturn get()  const {
		RequirementPolicy::assertDefined();
		return const_val;
	};
	
protected:
	XMLValueReader() {};
	// Delete Policies only from derived classes
	~XMLValueReader() {};
	
	void read(const string& string_val) {
		const_val = TypeInfo<ValType>::fromString(string_val);
	};
	
	void init() {};
	
	set<SymbolDependency> getDependSymbols() const { return set<SymbolDependency>(); };
	set<SymbolDependency> getOutputSymbols() const { return set<SymbolDependency>(); };
	
private:
	ValType const_val;
};

/** 
 * Policy class to be used to create a read-only but evaluated value PluginParameter 
 */

template <class ValType, class RequirementPolicy, template <class V> class Evaluator> 
class XMLEvaluatorBase : public RequirementPolicy {
public:
	typename TypeInfo<ValType>::SReturn get(SymbolFocus f) const 
	{ 
		RequirementPolicy::assertDefined();
		
		if (!is_initialized) {
			cout << "Warning: Evaluator initialisation during get() for expression '" << evaluator->getExpression() << "'" << endl; const_cast<XMLEvaluatorBase<ValType,RequirementPolicy,Evaluator>* >(this)->init(); // may throw ...
		}
		if (is_const)
			return const_expr;
		else 
			return evaluator->get(f);
	};
	
	typename TypeInfo<ValType>::SReturn operator()(SymbolFocus f) const { return get(f);}
	
	void setScope(const Scope * scope) { assert(scope); this->scope = scope; }
	void setGlobalScope() { require_global_scope=true;};
	void allowPartialSpec(bool allow=true) { allow_partial_spec=allow; }
	
	void init()
	{
		if (! RequirementPolicy::isMissing()) {
			if (scope)
				evaluator->init(scope);
			else if (require_global_scope)
				evaluator->init(SIM::getGlobalScope());
			else
				evaluator->init(SIM::getScope());
			
			if (evaluator->isConst()) { 
				is_const =  true;
				const_expr = evaluator->get(SymbolFocus::global);
			}
			is_initialized = true;
		}
	};
	
	const string& description() const  { RequirementPolicy::assertDefined(); return evaluator->getDescription(); }
	string expression() const { RequirementPolicy::assertDefined(); return evaluator->getExpression(); }
	
	Granularity granularity() const { RequirementPolicy::assertDefined(); return evaluator->getGranularity();}
	bool isInteger() const { RequirementPolicy::assertDefined(); return evaluator->isInteger(); }
	
	set<SymbolDependency> getDependSymbols() const { 
		if (RequirementPolicy::isMissing())
			return set<SymbolDependency>();
		if (evaluator)
			return evaluator->getDependSymbols(); 
		cout << "Retrieving Dependencies of undefined evaluator " << endl;
		return set<SymbolDependency>();
		
	};
	set<SymbolDependency> getOutputSymbols() const { return set<SymbolDependency>(); };
	
protected:
	XMLEvaluatorBase() : is_const(false), is_initialized(false), scope(NULL), require_global_scope(false), allow_partial_spec(false) {};
	// TODO Clearify  whether a Copy constructor is required to deal with the unique_ptr evaluator
	// An assignment will leave the rhs object uninitialized !!!
	
	
	bool read(const string& string_val){
		evaluator = make_unique<Evaluator<ValType> >(string_val, allow_partial_spec);
		return true;
	};
	
	// Delete Policies only from derived classes
	~XMLEvaluatorBase() {};
	
private:
	bool is_const;
	bool is_initialized;
	const Scope* scope;
	bool require_global_scope;
	bool allow_partial_spec;
	ValType const_expr;
	unique_ptr< Evaluator<ValType> > evaluator;
};


template <class ValType, class RequirementPolicy >
using XMLEvaluator = XMLEvaluatorBase< ValType, RequirementPolicy, ExpressionEvaluator >;

template <class ValType, class RequirementPolicy >
using XMLThreadsaveEvaluator = XMLEvaluatorBase< ValType, RequirementPolicy, ThreadedExpressionEvaluator >;

/** 
 * Policy class to be used to create a read-only and mapped-from-string value PluginParameter 
 */

template <class ValType, class RequirementPolicy> 
class XMLNamedValueReader : public RequirementPolicy {
public:
	typedef ValType value_type;
	typedef map<string,value_type> value_map_type;
	
	typename TypeInfo<value_type>::SReturn operator()() const { RequirementPolicy::assertDefined();  return value; };
	typename TypeInfo<value_type>::SReturn get() const { RequirementPolicy::assertDefined();  return value; };
	void setConversionMap(const value_map_type& value_map) { this->value_map = value_map; };

protected:
	XMLNamedValueReader() {};
	void read(const string& string_val) {
		if (value_map.empty()) {
			throw string("XMLNamedValueReader::read() : Empty value map");
		}
		if (!value_map.count(string_val)) {
			throw string("Invalid value '") + string_val + "' in XMLNamedValueReader";
		}
		value = value_map[string_val];
	}
	void init() {};
	set<SymbolDependency> getDependSymbols() const { return set<SymbolDependency>(); };
	set<SymbolDependency> getOutputSymbols() const { return set<SymbolDependency>(); };
	
private:
	value_map_type value_map;
	value_type value;
};


/** 
 * Policy class to be used to create a read-only and mapped-from-string value PluginParameter 
 * 
 * This is the explicit specialisation for selecting a CellType via XML
 */


template <class RequirementPolicy> 
class XMLNamedValueReader< shared_ptr<const CellType>,  RequirementPolicy> : public RequirementPolicy {
public:
	typedef shared_ptr<const CellType> value_type;
	typename TypeInfo<value_type>::SReturn operator()() const { RequirementPolicy::assertDefined();  return get(); };
	typename TypeInfo<value_type>::SReturn get() const { RequirementPolicy::assertDefined();  return value.lock(); };

protected:
	typedef weak_ptr<const CellType> int_value_type;
	typedef map<string,int_value_type> value_map_type;
	
	XMLNamedValueReader() {};
	void read(const string& val) {
		this->string_val = val;
	}
	
	void init() {
		if (! RequirementPolicy::isMissing()) {
			auto ct_vec = CPM::getCellTypes();
			value_map_type value_map;
			for (auto ct : ct_vec) {
				value_map[ct.lock()->getName()] = ct;
			}
			
			if (value_map.empty()) {
				throw string("XMLNamedValueReader::read() : Empty value map");
			}
			if (!value_map.count(string_val)) {
				throw string("Invalid value '") + string_val + "' in XMLNamedValueReader";
			}
			
			value = value_map[string_val];
		}
	};
	
	set<SymbolDependency> getDependSymbols() const { return set<SymbolDependency>(); };
	set<SymbolDependency> getOutputSymbols() const { return set<SymbolDependency>(); };
	
private:
	string string_val;
	int_value_type value;
};

/** 
 * Policy class to be used to create readable accessible PluginParameter to a platform symbol 
 */

template <class ValType, class RequirementPolicy> 
class XMLReadableSymbol : public RequirementPolicy {
public:
	void read(const string& string_val) { symbol_name = string_val; if (symbol_name.empty()) throw string("Missing Symbol name in XMLReadableSymbol::read()");};
	void init() {
		if (! RequirementPolicy::isMissing()) {
			if (scope)
				_accessor = scope->findSymbol<ValType>(symbol_name);
			else if (require_global_scope) 
				_accessor = SIM::getGlobalScope()->findSymbol<ValType>(symbol_name);
			else
				_accessor = SIM::findSymbol<ValType>(symbol_name);
		}
	}
	
	void requireGlobalScope() { require_global_scope=true;};
	void setScope(const Scope * scope) { this->scope = scope; }
	
	string name() const { RequirementPolicy::assertDefined(); return symbol_name; }
	const string& description() const  { RequirementPolicy::assertDefined(); return _accessor.getFullName(); }
	Granularity granularity() const { RequirementPolicy::assertDefined(); return _accessor.getGranularity(); }
	bool isInteger() const { RequirementPolicy::assertDefined(); return _accessor.isInteger(); }
	
	const SymbolAccessor<ValType>& accessor() const { RequirementPolicy::assertDefined(); return _accessor; }
	
	typename TypeInfo<ValType>::SReturn operator()(SymbolFocus f) const {
		RequirementPolicy::assertDefined(); 
		return this->_accessor.get(f);
	};
	
	typename TypeInfo<ValType>::SReturn get(SymbolFocus f) const {
		RequirementPolicy::assertDefined(); 
		return this->_accessor.get(f);
	};
	
	std::set<SymbolDependency> getDependSymbols() const {
		std::set<SymbolDependency> s;
		if (this->_accessor.valid()) {
			SymbolDependency dep = {this->_accessor.getBaseName(), this->_accessor.getScope()};
			cout << "Plugin Param dep " << dep.name << " scope " << dep.scope << endl; 
			s.insert(dep);
		}
		return s;
	}
	
	std::set<SymbolDependency> getOutputSymbols() const { 
		std::set<SymbolDependency> s;  
		return s; 
	}

protected:
	XMLReadableSymbol() : require_global_scope(false) , scope(NULL) {};
	~XMLReadableSymbol() {}
	
	SymbolAccessor<ValType> _accessor;
private:
	string symbol_name;
	bool require_global_scope;
	const Scope* scope;
};


// Allow to move the default value as into the symbol to be queried
template <> void XMLReadableSymbol<double,DefaultValPolicy>::init();
template <> void XMLReadableSymbol<VDOUBLE,DefaultValPolicy>::init();


/** 
 * Policy class to be used to create write accessible PluginParameter to a platform symbol 
 */

template <class ValType, class RequirementPolicy> 
class XMLWritableSymbol : public RequirementPolicy {
public:
	void read(const string& string_val) { symbol_name = string_val; if (symbol_name.empty()) throw string("Missing Symbol name in XMLWritableSymbol::read()");};
	void init() {
		if (! RequirementPolicy::isMissing()) {
			if (scope)
				_accessor = scope->findRWSymbol<ValType>(symbol_name);
			else if (require_global_scope) 
				_accessor = SIM::getGlobalScope()->findRWSymbol<ValType>(symbol_name);
			else
				_accessor = SIM::findRWSymbol<ValType>(symbol_name);
		}
	}
	void setScope(const Scope* scope) { this->scope = scope; }
	void setGlobalScope() { scope=NULL; require_global_scope=true;};
	
	string name() { RequirementPolicy::assertDefined(); return symbol_name; }
	const string& description() const { RequirementPolicy::assertDefined(); return _accessor.getFullName(); }
	Granularity granularity() const { RequirementPolicy::assertDefined(); return _accessor.getGranularity(); }
	
	const SymbolRWAccessor<ValType>& accessor() const { RequirementPolicy::assertDefined(); return _accessor; }
	
	bool set(SymbolFocus f, typename TypeInfo<ValType>::Parameter value) const {  RequirementPolicy::assertDefined(); return _accessor.set(f,value); };
	bool set(CPM::CELL_ID cell_id, typename TypeInfo<ValType>::Parameter value) const {  RequirementPolicy::assertDefined(); return _accessor.set(cell_id,value); };
	
	std::set<SymbolDependency> getOutputSymbols() const { 
		std::set<SymbolDependency> s;  
		if (_accessor.valid()) {
			SymbolDependency dep = {_accessor.getBaseName(), _accessor.getScope()};
			cout << "Plugin Param out " << dep.name << " scope " << dep.scope << endl; 
			s.insert(dep);
		}
		return s; 
	}

	std::set<SymbolDependency> getDependSymbols() const {
		std::set<SymbolDependency> s;
		return s;
	}
	
protected:
	XMLWritableSymbol() : scope(NULL), require_global_scope(false) {};
	~XMLWritableSymbol() {}
	
	SymbolRWAccessor<ValType> _accessor;
private:
	string symbol_name;
	const Scope* scope;
	bool require_global_scope;
};


/** 
 * Policy class to be used to create read-write accessible PluginParameter to a platform symbol 
 */

template <class ValType, class RequirementPolicy> 
class XMLReadWriteSymbol : public XMLWritableSymbol<ValType, RequirementPolicy> {
public:
	
	typename TypeInfo<ValType>::SReturn operator()(SymbolFocus f) const {
		RequirementPolicy::assertDefined(); 
		return this->_accessor.get(f);
	};
	
	typename TypeInfo<ValType>::SReturn get(SymbolFocus f) const {
		RequirementPolicy::assertDefined(); 
		return this->_accessor.get(f);
	};
	
	std::set<SymbolDependency> getDependSymbols() const {
		std::set<SymbolDependency> s;
		if (this->_accessor.valid()) {
			SymbolDependency dep = {this->_accessor.getBaseName(), this->_accessor.getScope()};
// 			cout << "Plugin Param dep " << dep.name << " scope " << dep.scope << endl; 
			s.insert(dep);
		}
		return s;
	}
};


// Make a distinction between declaration and functional class of the PluginParamter Template ...

/** Creating a PluginParameter that is coupled to the XML with a statically inherited policy for string value interpretation
 * 
 *  This is really neat meta programming ... 
 *  @tparam T the value type
 *  @tparam XMLValueInterpreter one of the following Reader Policies:
 *    - XMLValueReader -- Just reads the value from XML and converts it via operator>> 
 *    - XMLEvaluator -- Reads numerical expressions (VDOUBLE/double type only) and interpretes them via muParser
 *    - XMLNamedValueReader -- Reads strings from XML and converts them via a user provided conversion map
 *    - XMLReadWriteSymbol -- Reads a string that represents a symbol and generates RW access to the symbol
 *  @tparam RequirementPolicy one of RequiredPolicy / OptionalPolicy
 * 
 */
template <class T, template <class S, class R> class XMLValueInterpreter = XMLValueReader, class RequirementPolicy = RequiredPolicy >
class PluginParameter2 : public PluginParameterBase, public XMLValueInterpreter<T, RequirementPolicy> {
public:
	typedef  T ValType;
	PluginParameter2() : PluginParameterBase(), xml_path("") {};
	void setXMLPath(string xml_path) { this->xml_path = xml_path; }
	string XMLPath() const override { return this->xml_path; }
	void loadFromXML(XMLNode node) override {
		stored_node = node;
		try {
			string raw_string;
			if (xml_path.empty()) {
				throw string("PluginParameter: No XMLPath set. Setting parameter to missing.");
			}
			
			if (! getXMLAttribute(node, xml_path, raw_string,true)) {
				XMLValueInterpreter<T, RequirementPolicy>::setMissing();
			}
			else {
				XMLValueInterpreter<T, RequirementPolicy>::setStringVal(raw_string);
			}
			if (! XMLValueInterpreter<T, RequirementPolicy>::isMissing())
				XMLValueInterpreter<T, RequirementPolicy>::read(XMLValueInterpreter<T, RequirementPolicy>::stringVal());
		}
		catch (string e) {
			auto tokens = tokenize(xml_path,"/");
			tokens.pop_back();
			throw (MorpheusException(e, getXMLPath(stored_node)+"/"+join(tokens,"/")));
		}
	}
	
	void read(string value) override { XMLValueInterpreter<T, RequirementPolicy>::read(value); }
	// void name() { RequirementPolicy::stringVal(); }
	void init() override {
		try { XMLValueInterpreter<T, RequirementPolicy>::init();}
		catch (string e) {
			auto tokens = tokenize(xml_path,"/");
			tokens.pop_back();
			throw (MorpheusException(e, getXMLPath(stored_node)+"/"+join(tokens,"/")));
		}
	};
	bool isDefined() const { return  ! XMLValueInterpreter<T, RequirementPolicy>::isMissing(); }
	
	set<SymbolDependency> getDependSymbols() const override { return XMLValueInterpreter<T, RequirementPolicy>::getDependSymbols(); } 
	set<SymbolDependency> getOutputSymbols() const override { return XMLValueInterpreter<T, RequirementPolicy>::getOutputSymbols(); } 

private:
	string xml_path;
	XMLNode stored_node;
};



template <class ValType, class RequirementPolicy> 
class XMLStringifyExpression;


// no implementation other than for string type :-) 
template <class RequirementPolicy> 
class XMLStringifyExpression<string,RequirementPolicy>  : public RequirementPolicy {
	private:
			enum class Type {D, VD, Undef };
	// Just try to forward to either XMLEvaluator for double or VDOUBLE, as required
	public:
		void read(const string& string_val) { this->string_val = string_val; };
		void init() {
			type = Type::Undef;
			if (! RequirementPolicy::isMissing()) {
				const Scope* local_scope = SIM::getScope();
				if (scope)
					local_scope = scope;
				else if (require_global_scope) 
					local_scope = SIM::getGlobalScope();

				try {
					double_expr.allowPartialSpec();
					double_expr.read(string_val);
					double_expr.setScope(local_scope);
					double_expr.init();
					type = Type::D;
				}
				catch (...) {
					// "something went rong"
					type = Type::Undef;
					try {
						vdouble_expr.allowPartialSpec();
						vdouble_expr.read(string_val);
						vdouble_expr.setScope(local_scope);
						vdouble_expr.init();
						type = Type::VD;
					}
					catch (...) {
						// "something went rong"
						type = Type::Undef;
						throw string("Can't evaluate ") + string_val + " and convert string";
					}
				}
			}
		}
	
		void requireGlobalScope() { require_global_scope=true;};
		void setScope(const Scope * scope) { this->scope = scope; }
	
// 		string name() { RequirementPolicy::assertDefined(); return symbol_name; }
		const string& description() const  { if (type == Type::D) return double_expr.description(); if (type == Type::VD)  return vdouble_expr.description(); return string_val; }

		Granularity granularity() const { if (type == Type::D) return double_expr.granularity(); if (type == Type::VD)  return vdouble_expr.granularity(); return Granularity::Undef; }
		
		/// set the precision of numerical value output
		void setPrecision(int prec) { out.precision(prec); };
		
		void  setFormat(const std::ios_base::fmtflags format) { out.setf(format); };
		
		/// set the string encoding an undefined symbol
		void setUndefVal(const string& undef) { undef_val=undef; };
		
		string operator()(SymbolFocus f) const {
			out.clear();
			out.str("");
			try {
				switch (type) {
					case Type::D :
						out << this->double_expr.get(f);
						return out.str();
					case Type::VD :
						out << this->vdouble_expr.get(f);
						return out.str();
				}
			}
			catch (const SymbolError& e) {
				if (e.type() == SymbolError::Type::Undefined){
					return undef_val;
				}
			}
			out << "null";
			return out.str();
		};
	
		string get(SymbolFocus f) const {
			return this->operator()(f);
		};
	
		std::set<SymbolDependency> getDependSymbols() const {
			switch (type) {
				case Type::D :
					return double_expr.getDependSymbols();
				case Type::VD :
					return vdouble_expr.getDependSymbols();
				default:
					return std::set<SymbolDependency>();
			}
		}
	
		std::set<SymbolDependency> getOutputSymbols() const { 
			return std::set<SymbolDependency> ();
		}

	protected:
		XMLStringifyExpression() : require_global_scope(false) , scope(NULL), type(Type::Undef), undef_val("") {};
		~XMLStringifyExpression() {}
	
	private:
		mutable stringstream out;
		Type type;
		string string_val;
		string undef_val;
		const Scope* scope;
		bool require_global_scope;
		PluginParameter2<double, XMLEvaluator, RequiredPolicy> double_expr;
		PluginParameter2<double, XMLEvaluator, RequiredPolicy> vdouble_expr;
};

template < class RequirementPolicy >
using PluginParameterCellType = PluginParameter2< shared_ptr<const CellType>, XMLNamedValueReader, RequirementPolicy >;



///  @}

#endif
