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

#ifndef SYMBOL_H
#define SYMBOL_H

#include "config.h"
#include "traits.h"
#include <stdexcept>
#include "string_functions.h"
#include "symbolfocus.h"
#include "muParser/muParser.h"

typedef std::deque<double> double_queue;

/** \defgroup SymbolSystem Symbolic Linking System
 * 
 * \section Introduction
 * The Symbolic Linking system allows for dynamic linking of information flow between the individual computational modules of the platform.
 * Links are defined by symbolic references in the XML or indirectly through numerical expressions. 
 * 
 * Ususally, there is no need understand to understand a lot about the particular implementation, but rather make use of PluginParameters or in rare cases SymbolAccessors. 
 */


/** Granularity describes the spatial resolution of a symbol within it's spatial scope
  * 
  * \li \c Global -- Symbol defines a unique value within it's spatial scope.
  * \li \c Cell -- Symbol defines a unique value per cell.
  * \li \c Node -- Symbol defines one value per lattice node.
  * \li \c MembraneNode -- Symbol defines one value per element of the MembranePDE.
  * \li \c SurfaceNode -- Access cell mebrane values per Lattice nodes, selecting only those nodes at the cell surfaces.
  */
enum class Granularity { Global, Node, Cell, MembraneNode, SurfaceNode };
bool operator<(Granularity a, Granularity b);
bool operator<=(Granularity a, Granularity b);

Granularity operator+(Granularity a, Granularity b);
Granularity& operator+=(Granularity& g, Granularity b);
ostream& operator<<(ostream& out, Granularity g);

class Scope;
class SymbolBase;
using Symbol = std::shared_ptr<const SymbolBase> ;
using SymbolDependency = Symbol;

/// The type agnostic <b> symbol interface </b> to be stored in the factory ...
class SymbolBase : public std::enable_shared_from_this<SymbolBase> {
public:
	struct Flags;
	virtual const std::string& name() const =0;  ///  Symbol identifier
	virtual const std::string& description() const =0;  ///  Descriptive name
	virtual const std::string XMLPath() const =0;  /// Path to XML declaration
	virtual const Scope* scope() const =0; /// The scope the symbol is defined in
	virtual Granularity granularity() const =0;
	virtual void setScope(const Scope* scope) =0;
// 	virtual std::string baseName() const =0; ///  Identifier of the base symbol. Usually identical to name, but differs for derived symbols.
	/** Dependencies of the symbol
	 *  In case the symbols has implicite dependencies,
	 *  i.e. the function dependencies or the referring symbol of component symbols ...
	 *  Container symbols have no dependencies.
	 */
	virtual std::set<SymbolDependency> dependencies() const =0;
	virtual std::set<SymbolDependency> leafDependencies() const;
	virtual const std::string& type() const =0;  ///  Type name derived from TypeInfo<T>::name
	virtual std::string linkType() const =0;  /// Descriptive name identifying the container type providing the symbol
	virtual const Flags& flags() const =0;  /// Meta information on the symbol

	struct Flags {
		Granularity granularity;
		bool time_const;
		bool space_const;
		bool stochastic;
		bool delayed;
		bool writable;
		bool partially_defined;
		bool integer;
		bool function;
		Flags() :
		  granularity(Granularity::Global),
		  time_const(false),
		  space_const(false),
		  stochastic(false),
		  delayed(false),
		  writable(false),
		  partially_defined(false),
		  integer(false), 
		  function(false) {};
	};
	
	virtual ~SymbolBase() {};
	
	static string
		Space_symbol,
		MembraneSpace_symbol,
		Time_symbol,
		NodeLength_symbol,
		CellType_symbol,
		CellID_symbol,
		SuperCellID_symbol,
		SubCellID_symbol,
		CellVolume_symbol,
		CellLength_symbol,
		CellSurface_symbol,
		CellCenter_symbol,
		CellOrientation_symbol,
		Temperature_symbol,
		CellPosition_symbol;
};


// No purely abstract interfaces to avoid virtual class inheritance


/** \b SymbolAccessor base implementation with \b read-only access
 * 
 * The SymbolAccessor usually does not contain the data itself. It rather mediates the data access 
 * into a container. In addition, it provides all meta information requiered for integration.
 */

template <class T>
class SymbolAccessorBase : public SymbolBase {
public:
	SymbolAccessorBase(std::string name) : SymbolBase(), symbol_name(name), _scope(nullptr) {}
	/// Unique string identifier for a the data type
	const std::string& type() const final { return TypeInfo<T>::name(); }
	/// Name of the symbol
	const std::string& name() const override { return symbol_name; }
	/// Path to XML declaration
	const std::string XMLPath() const override { return xml_path; };
	void setXMLPath(const string& path)  { xml_path = path; } 
	/// Granularity of the symbol
	Granularity granularity() const final { return flags().granularity; }
	/// Scope the Symbol is registered in
	const Scope* scope() const final { return _scope; };
	/// Dependencies of implicite symbols (i.e. Functions, Mappings)
	std::set<SymbolDependency> dependencies() const override { return std::set<SymbolDependency>(); };
	
	/// Symbol's meta information
	const Flags& flags() const override { return _flags; }
	/// Writable access to symbol's meta information
	Flags& flags() { return _flags; }   
	
	/// Access data at SymbolFoxus @p f 
	virtual typename TypeInfo<T>::SReturn get(const SymbolFocus& f) const =0;
	/**
	 * Access data at SymbolFoxus @p f
	 * Also take care that any dependend symbols are initialized. 
	 * Use this method during the initialization phase.
	 */
	virtual typename TypeInfo<T>::SReturn safe_get(const SymbolFocus& f) const { return get(f); };
	
	/// Static creator method for a constant symbol not associated with the XML, that may be registered in a scope.
	static shared_ptr<SymbolAccessorBase<T> > createConstant(string name, string description, const T& value);
	/// Static creator method for a variable symbol not associated with the XML, that may be registered in a scope.
	static shared_ptr<SymbolAccessorBase<T> > createVariable(string name, string description, const T& value);

protected:
	/// Scope the Symbol is registered in
	void setScope(const Scope* scope) override { _scope = scope; }
	friend class Scope;
	
private:
	std::string xml_path;
	std::string symbol_name;
	const Scope* _scope;
	Flags _flags;
};

/// Interface class for the parametric function call
template <class T>
class FunctionAccessor {
	public:
		virtual int parameterCount() const =0;
		virtual mu::fun_class_generic* getCallBack() const =0;
		virtual typename TypeInfo<T>::SReturn get(double parameters[], const SymbolFocus& focus) const =0;
		virtual typename TypeInfo<T>::SReturn safe_get(double parameters[], const SymbolFocus& focus) const =0;
};


template <class T>
class PrimitiveConstantSymbol : public SymbolAccessorBase<T> {
public:
	PrimitiveConstantSymbol(string name, string description, const T& value) : SymbolAccessorBase<T>(name),
		descr(description), value(value) {
			this->flags().time_const = true;
			this->flags().space_const = true;
		};
	typename TypeInfo<T>::SReturn get(const SymbolFocus&) const override { return value; };
	const string& description() const override { return descr; }
	std::string linkType() const override { return "PrimitiveConstant"; }
	
protected:
	const string descr;
	mutable T value;
};

template <class T>
shared_ptr<SymbolAccessorBase<T> > SymbolAccessorBase<T>::createConstant(string name, string description, const T& value) {
	return make_shared< PrimitiveConstantSymbol<T> >(name,description, value);
}



/** \b SymbolRWAccessor base implementation with \b Read/Write access
 * 
 *  The SymbolRWAccessor usually does not contain the data itself. It rather mediates the data access 
 *  into a property, field or whatsoever.
 * 
 *  Interface to a buffer implementation is provided for buffering synchronous system updates
 */

template <class T>
class SymbolRWAccessorBase : public SymbolAccessorBase<T> {
public:
	SymbolRWAccessorBase(std::string name) : SymbolAccessorBase<T>(name) {
		this->flags().writable = true;
	}
	virtual void set(const SymbolFocus& f, typename TypeInfo<T>::Parameter val) const =0;
	virtual void setBuffer(const SymbolFocus& f, typename TypeInfo<T>::Parameter value) const =0;
	virtual void applyBuffer() const =0;
	virtual void applyBuffer(const SymbolFocus& f) const =0;
	static shared_ptr<SymbolRWAccessorBase<T> > createVariable(string name, string descr, const T& value);
};

template <class T>
class PrimitiveVariableSymbol : public SymbolRWAccessorBase<T> {
public:
	PrimitiveVariableSymbol(string name, string description, const T& value) : SymbolRWAccessorBase<T>(name),
		descr(description), value(value) {};
	typename TypeInfo<T>::SReturn get(const SymbolFocus&) const override { return value; };
	void set(const SymbolFocus&, typename TypeInfo<T>::Parameter val) const override { value = val; };
	void setBuffer(const SymbolFocus& f, typename TypeInfo<T>::Parameter value) const override { buffer = value; }
	void applyBuffer() const override { value = buffer; };
	void applyBuffer(const SymbolFocus& f) const override { value = buffer; };
	const string& description() const override { return descr; }
	std::string linkType() const override { return "PrimitiveVariable"; }
	
protected:
	const string descr;
	mutable T value, buffer;
};

template <class T>
shared_ptr<SymbolRWAccessorBase<T> > SymbolRWAccessorBase<T>::createVariable(string name, string descr, const T& value) {
	return make_shared< PrimitiveVariableSymbol<T> >(name,descr,value);
}


/// Convenience type definitions for using the SymbolAccessors
using Symbol = std::shared_ptr<const SymbolBase> ;

template <class T>
using SymbolAccessor = std::shared_ptr<const  SymbolAccessorBase<T>> ;

template <class T>
using SymbolRWAccessor = std::shared_ptr<const  SymbolRWAccessorBase<T>> ;


// Forward declarations for platform provided computation symbols
extern string sym_RandomUni;
extern string sym_RandomInt;
extern string sym_RandomNorm;
extern string sym_RandomBool;
extern string sym_RandomGamma;
extern string sym_Modulo;

#endif // SYMBOL_H

