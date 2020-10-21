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

/**
 * 
 * \page PluginDev
 * \section PluginParameters Plugin Parameters
 * 
 * Flexible constructs to mediate easy access to XML data.
 * 
 * \subsection Usage
 * 
 * Reading from XML can be rather cumbersome, espacially when it comes to the interpretation of the 
 * provided string literal: Is it a value, what kind of value, or is it rather an
 * expression, which should be evaluated or do we even require a symbol which we can write to?
 *
 * The morpheus framework casts all these decisions into a policy-based template concept of PluginParameter.
 * This template class is instantiated by selecting the parameter type and a set 
 * of policies to obtain a tailored object of your choice:
 * 
 * Syntax: PlugParameters2\<value_type, ReaderPolicy, RequirementPolicy\> my_value;
 * 
 * Reader policies: 
 *   - XMLValueReader
 *   - XMLEvaluator
 *   - XMLThreadsaveEvaluator
 *   - XMLNamedValueReader
 *   - XMLReadableSymbol
 *   - XMLWritableSymbol
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
 * \subpage Examples
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
	virtual void loadFromXML(XMLNode node, const Scope* scope) =0; // read from value, optionally from symbol
	virtual void init() =0;
	virtual void read(const string& value) =0;
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
	typename TypeInfo<ValType>::Return operator()()  const {
		RequirementPolicy::assertDefined();
		return const_val;
	};
	typename TypeInfo<ValType>::Return get()  const {
		RequirementPolicy::assertDefined();
		return const_val;
	};
	
protected:
	XMLValueReader() {};
	// Delete Policies only from derived classes
	~XMLValueReader() {};
	void setScope(const Scope * scope) {};
	void read() {
		const_val = TypeInfo<ValType>::fromString(this->stringVal());
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
		if (is_const)
			return const_expr;
		else 
			return evaluator->get(f);
	};
	
	typename TypeInfo<ValType>::SReturn safe_get(SymbolFocus f) const {
		if (!is_initialized) {
// 			cout << "Warning: Evaluator initialisation during get() for expression '" << evaluator->getExpression() << "'" << endl;
			const_cast<XMLEvaluatorBase<ValType,RequirementPolicy,Evaluator>* >(this)->init();
		}
		if (is_const)
			return const_expr;
		else
			return evaluator->safe_get(f);
	}
	
	typename TypeInfo<ValType>::SReturn operator()(SymbolFocus f) const { return get(f);}
	
	void setScope(const Scope * scope) { assert(scope); local_scope = scope; }
	void setGlobalScope() { require_global_scope=true; };
	
		/*! \brief Add a foreign Scope @p scope as namespace @p ns_name to the local variable scope.
	 * 
	 * Returns the name space reference @return id.
	 */
	void addNameSpaceScope(const string& ns_name, const Scope* scope) { 
		if (is_initialized) throw string("too late to add symbol namspaces -- expression initialized");
		namespaces.insert({ns_name, {scope, ns_name, 0} });
	}
	// Get the corresponding namespace id
	uint getNameSpaceId(const string& ns_name) {
		if (!is_initialized) throw string("getNameSpaceId(): Expression not initialized");
		return namespaces[ns_name].ns_id;
	}
	
	/// Get all symbols used from name space @p ns_id. The namespace prefix is not contained in the symbols returned.
	set<Symbol> getNameSpaceUsedSymbols(uint ns_id) const {
		if (!is_initialized) throw string("getNameSpaceUsedSymbols(): Expression not initialized");
		return evaluator->getNameSpaceUsedSymbols(ns_id);
	}
	
	/// Set the focus of name space @p ns_id
	void setNameSpaceFocus(uint ns_id, const SymbolFocus& f) const {
		if (!is_initialized) throw string("expression not initialized");
		evaluator->setNameSpaceFocus(ns_id, f);
	};
	
	
	void setLocalsTable(const vector<EvaluatorVariable>& table) { if (is_initialized) throw string("too late to modify the locals table -- expression initialized"); locals_table = table; }
	void setLocals(const double* data) { if (!is_initialized) throw string("setLocals(): xpression not initialized"); evaluator->setLocals(data); }
	void allowPartialSpec(bool allow=true) { allow_partial_spec=allow; }
	void setSpherical(bool s) {  setNotation(s ? VecNotation::SPHERE_PTR : VecNotation::ORTH); };
	void setNotation(VecNotation notation) {
		this->notation = notation;
		// reinitialize if required
		if (is_initialized) { is_initialized = false; init(); }
	};
	
	void read() {
		is_initialized = false;
	};
	
	void init()
	{
		if (is_initialized)
			return;
		if (! RequirementPolicy::isMissing()) {
			if (require_global_scope)
				local_scope = SIM::getGlobalScope();
			if (! local_scope)
				throw string("PluginParameter missing scope");

			evaluator = make_unique<Evaluator<ValType> >(this->stringVal(), local_scope, allow_partial_spec);
			evaluator->setNotation(notation);
			
			if (!locals_table.empty())
				evaluator->setLocalsTable(locals_table);
			if (!namespaces.empty()) {
				for (auto& ns : namespaces) {
					ns.second.ns_id = evaluator->addNameSpaceScope(ns.second.name,ns.second.scope);
				}
			}
			
			evaluator->init();
			
			if (evaluator->flags().space_const && evaluator->flags().time_const && locals_table.empty()) { 
				is_const =  true;
				const_expr = evaluator->safe_get(SymbolFocus::global);
			}
			is_initialized = true;
		}
	};
	
	// Forward some Expression interface
	const string& description() const  { RequirementPolicy::assertDefined(); return evaluator->getDescription(); }
	string expression() const { RequirementPolicy::assertDefined(); return evaluator->getExpression(); }
	const SymbolBase::Flags& flags() const { RequirementPolicy::assertDefined(); return evaluator->flags(); }
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
	XMLEvaluatorBase() : is_const(false), notation(VecNotation::ORTH), is_initialized(false), local_scope(nullptr), require_global_scope(false), allow_partial_spec(false) {};
	// TODO Clearify  whether a Copy constructor is required to deal with the unique_ptr evaluator
	// An assignment will leave the rhs object uninitialized !!!
	
	// Delete Policies only from derived classes
	~XMLEvaluatorBase() {};
	
private:
	bool is_const;
	VecNotation notation;
	bool is_initialized;
	const Scope* local_scope;
	bool require_global_scope;
	bool allow_partial_spec;
	ValType const_expr;
// 	string string_val;
	vector<EvaluatorVariable> locals_table;
	
	struct NS_Desc {
		const Scope* scope; 
		string name;
		uint ns_id;
	};
	map<string,NS_Desc> namespaces;
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
	void setValueMap(const value_map_type& value_map) { setConversionMap(value_map); }

protected:
	XMLNamedValueReader() {};
	void setScope(const Scope * scope) {};
	void read() {
		if (value_map.empty()) {
			throw string("XMLNamedValueReader::read() : Empty value map");
		}
		if (!value_map.count(this->stringVal())) {
			throw string("Invalid value '") + this->stringVal() + "' in XMLNamedValueReader";
		}
		value = value_map[this->stringVal()];
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
	typename TypeInfo<value_type>::SReturn operator()() const { return get(); };
	typename TypeInfo<value_type>::SReturn get() const {
		RequirementPolicy::assertDefined();
		if(value.expired())
			const_cast<XMLNamedValueReader< shared_ptr<const CellType>,  RequirementPolicy>* >(this)->init();
		return value.lock();
	};

protected:
	typedef weak_ptr<const CellType> int_value_type;
	typedef map<string,int_value_type> value_map_type;
	
	XMLNamedValueReader() {};
	void read() {}
	void setScope(const Scope * scope) {};
	void init() {
		if (! RequirementPolicy::isMissing()) {
			value_map_type value_map = CPM::getCellTypesMap();
			if (!value_map.count(this->stringVal())) {
				throw string("Invalid value '") + this->stringVal() + "' in XMLNamedValueReader";
			}
			value = value_map[this->stringVal()];
		}
	};
	
	set<SymbolDependency> getDependSymbols() const { return set<SymbolDependency>(); };
	set<SymbolDependency> getOutputSymbols() const { return set<SymbolDependency>(); };
	
private:
// 	string string_val;
	int_value_type value;
};

/** 
 * Policy class to be used to create readable accessible PluginParameter to a platform symbol 
 */

template <class ValType, class RequirementPolicy> 
class XMLReadableSymbol : public RequirementPolicy {
public:
	/// read the XML-specified value
	void read() { if (this->stringVal().empty()) throw string("Missing Symbol name in XMLReadableSymbol::read()"); symbol_name = this->stringVal(); };
	/// Initialize the symbol
	void init() {
		if (! RequirementPolicy::isMissing()) {
			if (require_global_scope)
				local_scope = SIM::getGlobalScope();
			if (! local_scope)
				throw string("PluginParameter missing scope");
			
			if (partial_spec_val_set)
				_accessor = local_scope->findSymbol<ValType>(symbol_name, partial_spec_val);
			else
				_accessor = local_scope->findSymbol<ValType>(symbol_name);
		}
	}
	
	/// Require a Symbol from another scope (i.e. not the lexical scope)
	void setScope(const Scope * scope) { local_scope = scope; }
	/// Require a globally valid Symbol
	void setGlobalScope() { require_global_scope =true; }
	
	/** Set a default value for partially defined symbols.
	 * 
	 *  Sets a value to be assumed in spatial regions where the symbol is not specified.
	 *  This value does work as a default for unspecified symbols.
	 */
	void setPartialSpecDefault(const ValType& val) {
		partial_spec_val_set = true;
		partial_spec_val = val;
	}
	
	void unsetPartialSpecDefault() { partial_spec_val_set = false; if (local_scope) init();}
	
	string name() const { RequirementPolicy::assertDefined(); return symbol_name; }
	const string& description() const  { RequirementPolicy::assertDefined(); return _accessor->description(); }
	Granularity granularity() const { RequirementPolicy::assertDefined(); return _accessor->granularity(); }
	bool isInteger() const { RequirementPolicy::assertDefined(); return _accessor->flags().integer; }
	
	const SymbolAccessor<ValType>& accessor() const { RequirementPolicy::assertDefined(); return _accessor; }
	
	typename TypeInfo<ValType>::SReturn operator()(SymbolFocus f) const {
		RequirementPolicy::assertDefined(); 
		return this->_accessor->get(f);
	};
	
	typename TypeInfo<ValType>::SReturn get(SymbolFocus f) const {
		RequirementPolicy::assertDefined(); 
		return this->_accessor->get(f);
	};
	
	std::set<SymbolDependency> getDependSymbols() const {
		if (this->_accessor) {
			return { this->_accessor };
		}
		else
			return { };
	}
	
	std::set<SymbolDependency> getOutputSymbols() const { 
		std::set<SymbolDependency> s;  
		return s; 
	}

protected:
	XMLReadableSymbol() {};
	~XMLReadableSymbol() {}
	
	SymbolAccessor<ValType> _accessor;
private:
	string symbol_name;
	const Scope* local_scope = NULL;
	bool require_global_scope = false;
	bool partial_spec_val_set = false;
	ValType partial_spec_val;
};



/** 
 * Policy class to be used to create write accessible PluginParameter to a platform symbol 
 */

template <class ValType, class RequirementPolicy> 
class XMLWritableSymbol : public RequirementPolicy {
public:
	void read() {  if (this->stringVal().empty()) throw string("Missing Symbol name in XMLWritableSymbol::read()"); symbol_name = this->stringVal(); };
	void init() {
		if (! RequirementPolicy::isMissing()) {
			if (require_global_scope)
				local_scope = SIM::getGlobalScope();
			if (! local_scope)
				throw string("PluginParameter missing scope");
			_accessor = local_scope->findRWSymbol<ValType>(symbol_name);
		}
	}
	void setScope(const Scope* scope) { local_scope = scope; }
	void setGlobalScope() { require_global_scope = true; }
	
	string name() { RequirementPolicy::assertDefined(); return symbol_name; }
	const string& description() const { RequirementPolicy::assertDefined(); return _accessor->description(); }
	Granularity granularity() const { RequirementPolicy::assertDefined(); return _accessor->flags().granularity; }
	
	const SymbolRWAccessor<ValType>& accessor() const { RequirementPolicy::assertDefined(); return _accessor; }
	
	void set(SymbolFocus f, typename TypeInfo<ValType>::Parameter value) const {  RequirementPolicy::assertDefined(); _accessor->set(f,value); };
	
	std::set<SymbolDependency> getOutputSymbols() const { 
		if ( ! RequirementPolicy::isMissing()) return { _accessor };
		else return {};
	}

	std::set<SymbolDependency> getDependSymbols() const {
		return {};
	}
	
protected:
	XMLWritableSymbol() : local_scope(NULL), require_global_scope(false) {};
	~XMLWritableSymbol() {}
	
	SymbolRWAccessor<ValType> _accessor;
private:
	string symbol_name;
	const Scope* local_scope;
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
		return this->_accessor->get(f);
	};
	
	typename TypeInfo<ValType>::SReturn get(SymbolFocus f) const {
		RequirementPolicy::assertDefined(); 
		return this->_accessor->get(f);
	};
	
	std::set<SymbolDependency> getDependSymbols() const {
		if ( ! RequirementPolicy::isMissing()) return { this->_accessor };
		else return { };
	}
};


// Make a distinction between declaration and functional class of the PluginParamter Template ...

/** Creates PluginParameter that is coupled to the XML with a statically inherited policy for string value interpretation
 * 
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
	PluginParameter2( const PluginParameter2& ) = delete;
	const PluginParameter2& operator=( const PluginParameter2& ) = delete;
	
	void setXMLPath(string xml_path) { this->xml_path = xml_path; }
	string XMLPath() const override { return this->xml_path; }
	void loadFromXML(XMLNode node, const Scope* scope) override {
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
				XMLValueInterpreter<T, RequirementPolicy>::read();
			
			XMLValueInterpreter<T, RequirementPolicy>::setScope(scope);
		}
		catch (string e) {
			auto tokens = tokenize(xml_path,"/");
			if (!tokens.empty()) tokens.pop_back();
			throw (MorpheusException(e, getXMLPath(stored_node)+"/"+join(tokens,"/")));
		}
	}
	
	void read(const string& value) override { XMLValueInterpreter<T, RequirementPolicy>::setStringVal(value); XMLValueInterpreter<T, RequirementPolicy>::read(); }
	// void name() { RequirementPolicy::stringVal(); }
	void init() override {
		try { XMLValueInterpreter<T, RequirementPolicy>::init();}
		catch (string e) {
			auto tokens = tokenize(xml_path,"/");
			if (!tokens.empty()) tokens.pop_back();
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
		void read() { };
		void init() {
			type = Type::Undef;
			if (! RequirementPolicy::isMissing()) {
				if (! local_scope)
					throw string("PluginParameter missing scope");

				try {
					double_expr.allowPartialSpec();
					double_expr.read(this->stringVal());
					double_expr.setScope(local_scope);
					double_expr.init();
					type = Type::D;
				}
				catch (...) {
					// "something went rong"
					type = Type::Undef;
					try {
						vdouble_expr.allowPartialSpec();
						vdouble_expr.read(this->stringVal());
						vdouble_expr.setScope(local_scope);
						vdouble_expr.init();
						type = Type::VD;
					}
					catch (...) {
						// "something went rong"
						type = Type::Undef;
						throw string("Can't evaluate ") + this->stringVal() + " and convert to string";
					}
				}
			}
		}
	
		void setScope(const Scope * scope) { local_scope = scope; }
	
// 		string name() { RequirementPolicy::assertDefined(); return symbol_name; }
		const string& description() const  { if (type == Type::D) return double_expr.description(); if (type == Type::VD)  return vdouble_expr.description(); return this->stringVal(); }

		Granularity granularity() const { if (type == Type::D) return double_expr.granularity(); if (type == Type::VD)  return vdouble_expr.granularity(); return Granularity::Global; }
		
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
					default:
						throw string("Can't evaluate ") + this->stringVal() + " and convert to string";
				}
			}
			catch (const SymbolError& e) {
				if (e.type() == SymbolError::Type::Undefined || e.type() == SymbolError::Type::InvalidPartialSpec) {
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
		XMLStringifyExpression() : local_scope(nullptr), type(Type::Undef), undef_val("") {};
		~XMLStringifyExpression() {};
	
	private:
// 		string string_val;
		string undef_val;
		const Scope* local_scope;
		
		mutable stringstream out;
		Type type;
		PluginParameter2<double, XMLEvaluator, RequiredPolicy> double_expr;
		PluginParameter2<VDOUBLE, XMLEvaluator, RequiredPolicy> vdouble_expr;
};

/** PluginParameter -- parses a parameter from the XML and provides values
 * 	FocusRange r(Granularity::Cell, parent->scope());
	for (auto& f:r) {
		
	}
 *  Using template meta programming, PluginParameter assembles an accessor class based on policy classes.
 *  @tparam T the value type
 *  @tparam XMLValueInterpreter one of the following Reader Policies:
 *    - XMLValueReader -- Just reads the value from XML and converts it via operator>> 
 *    - XMLEvaluator -- Reads numerical expressions (VDOUBLE/double type only) and interpretes them via muParser
 *    - XMLNamedValueReader -- Reads strings from XML and converts them via a user provided conversion map
 *    - XMLReadWriteSymbol -- Reads a string that represents a symbol and generates RW access to the symbol
 *  @tparam RequirementPolicy one of RequiredPolicy / OptionalPolicy
 * 
 * For reading in a CellType, the PluginParameterCellType class is a preconfigured specialization.
 * 
 * Actually, PluginParameter is a wrapper around the PluginParamter_internal class, such that it has implicitely shared behavior.
 * Thus, because we don't know in advance the interface of PluginParamter_internal accessing it's members has to be done via the pointer operator '->'.
 */ 

template <class T, template <class S, class R> class XMLValueInterpreter, class RequirementPolicy>
using PluginParameter_internal = PluginParameter2<T, XMLValueInterpreter, RequirementPolicy>;

template <class T, template <class S, class R> class XMLValueInterpreter = XMLValueReader, class RequirementPolicy = RequiredPolicy >
class PluginParameter_Shared {
public:
	typedef PluginParameter_internal<T, XMLValueInterpreter, RequirementPolicy> ParamType;
	
private:
	shared_ptr< ParamType > d;
	
public:
	PluginParameter_Shared() { this->d = make_shared< ParamType >(); }
	/// Dereference to the underlying shared object.
	ParamType& operator*() { return *d; }
	ParamType* operator->() { return d.get(); }
	const ParamType& operator*() const { return *d; }
	const ParamType* operator->() const { return d.get(); }

	/// Convenience parenthesis operators for accessing the parameter value
	template <typename... Arguments> 
	typename TypeInfo<typename ParamType::ValType>::SReturn operator()(Arguments... params) const { return d->get(params ...); }

	PluginParameter_Shared<T, XMLValueInterpreter, RequirementPolicy> clone();
};


template <class T, template <class S, class R> class XMLValueInterpreter, class RequirementPolicy>
using PluginParameter = PluginParameter2<T, XMLValueInterpreter, RequirementPolicy>;

template < class RequirementPolicy >
using PluginParameterCellType = PluginParameter2< shared_ptr<const CellType>, XMLNamedValueReader, RequirementPolicy >;

template < class RequirementPolicy >
using PluginParameterCellType_Shared = PluginParameter_Shared< shared_ptr<const CellType>, XMLNamedValueReader, RequirementPolicy >;


///  @}

#endif
