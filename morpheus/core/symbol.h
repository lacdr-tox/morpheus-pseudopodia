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

typedef std::deque<double> double_queue;

/** \defgroup SymbolSystem Symbolic Linking System
 * 
 * \section Introduction
 * The Symbolic Linking system allows for dynamic linking of information flow between the individual computational modules of the platform.
 * Links are defined by symbolic references in the XML or indirectly through numerical expressions. 
 * 
 * Ususally, there is no need understand to understand a lot about the particular implementation, but rather make use of PluginParameters or in rare cases SymbolAccessors. 
 */


/** Granularity describes the spatial resolution of a symbol within it's scope
  * 
  * \li \c Global -- Symbol defines a unique value within it's spatial Scope.
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
/// Information about a SymbolDependency
struct SymbolDependency {string name; const Scope* scope;};
inline bool operator<(const SymbolDependency& lhs, const SymbolDependency& rhs) {
	return (lhs.name < rhs.name) || (lhs.name == rhs.name && lhs.scope<rhs.scope);
}

inline bool operator==(const SymbolDependency& lhs, const SymbolDependency& rhs) {
	return (lhs.name == rhs.name && lhs.scope == rhs.scope);
}


/// The type agnostic <b> symbol interface </b> to be stored in the factory ...
class SymbolBase {
public:
	struct Flags;
	virtual const std::string& name() const =0;  ///  Symbol identifier
	virtual const std::string& description() const =0;  ///  Descriptive name
	virtual const Scope* scope() const =0; /// The scope the symbol is defined in
	virtual Granularity granularity() const =0;
	virtual void setScope(const Scope* scope) =0;
// 	virtual std::string baseName() const =0; ///  Identifier of the base symbol. Usually identical to name, but differs for derived symbols.
	virtual std::set<SymbolDependency> dependencies() const =0;
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
		Flags() :
		  granularity(Granularity::Global),
		  time_const(false),
		  space_const(false),
		  stochastic(false),
		  delayed(false),
		  writable(false),
		  partially_defined(false),
		  integer(false) {};
	};
	
// 	virtual bool is_const_in_time() const =0;
// 	virtual bool is_const_in_space() const =0;
// 	virtual bool is_stochastic() const =0;  
// 	virtual bool is_delayed() const =0;
// 	virtual bool is_writable() const =0;
	
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


// No abstract interfaces to avoid virtual class inheritance for performance reasons

// template <class T>
// class SymbolReadAccessor_I : public Symbol_I {
// public:
// 	virtual T get(const Focus<T>& f) const =0;
// };
// 
// template <class T>
// class SymbolRWAccessor_I : virtual public SymbolReadAccessor_I<T> {
// public:
// 	virtual void set(const Focus<T>& f, const T& val) const =0;
// };


/** \b SymbolAccessor base implementation with \b read-only access
 * 
 * The SymbolAccessor usually does not contain the data itself. It rather mediates the data access 
 * into a container. In addition, it provides all meta information requiered for integration.
 */

template <class T>
class SymbolAccessorBase : public SymbolBase {
public:
	SymbolAccessorBase(std::string name) : symbol_name(name), _scope(nullptr) {}
	const std::string& type() const final { return TypeInfo<T>::name(); }
	const std::string& name() const override { return symbol_name; }
	Granularity granularity() const final { return flags().granularity; }
	const Scope* scope() const final { return _scope; };
	void setScope(const Scope* scope) override { _scope = scope; }

	std::set<SymbolDependency> dependencies() const override {
		if (_scope) return std::set<SymbolDependency>({{symbol_name, _scope}});
		else return std::set<SymbolDependency>();
	};
	
	const Flags& flags() const override { return _flags; }
	Flags& flags() { return _flags; }   /// Writable access to symbol's meta information
	
	virtual typename TypeInfo<T>::SReturn get(const SymbolFocus& f) const =0;
	virtual typename TypeInfo<T>::SReturn safe_get(const SymbolFocus& f) const { return get(f); };
	
// 	static shared_ptr<SymbolAccessorBase<T> > createConstant(string name, const T& value);
	static shared_ptr<SymbolAccessorBase<T> > createConstant(string name, string description, const T& value);
	static shared_ptr<SymbolAccessorBase<T> > createVariable(string name, string description, const T& value);

protected:
	
private:
	std::string symbol_name;
	const Scope* _scope;
	Flags _flags;
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
 *  into a container. In addition, it provides all meta information requiered.
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

/**
 * \brief SymbolData is a Symbol Descriptor, with the ability to spawn a fully fledged Accessor.
 * 
 * Also contains the storage for globally predefined symbols like time and space
 */



/*
class SymbolData {
public:
	SymbolData() : integer(false), writable(false), invariant(false), time_invariant(false), is_composite(false), is_delayed(false), granularity(Granularity::Undef), link(UnLinked) {};
	string name;
	string base_name;  /// holds the name of the symbol this symbol is derived from, or the symbol name in any other case.
	string fullname;   /// More descriptive name od the symbol, allows space chars.
	string type_name;  /// type name of the symbol according to TypeInfo<your_type>::name()
	bool integer;      /// numbers are integer
	bool writable;     /// Symbol allows writable access
	bool invariant;     /// Symbol is invariant in time and space
	bool time_invariant;  /// Symbols is constant in time
	bool is_composite; /// Symbol is composed of subscope symbols, but may also have a default
	bool is_delayed;
	
// 	enum Granularity { UndefGran, GlobalGran, NodeGran, CellGran, MembraneNodeGran };
	
	enum LinkType {	GlobalLink,
					CellPropertyLink,
					CellMembraneLink,
					SingleCellPropertyLink,
					SingleCellMembraneLink,
					FunctionLink,
					VectorFunctionLink,
					PDELink,
					VectorFieldLink,
					Space,
					MembraneSpace,
					Time,
					CellTypeLink,
					PopulationSizeLink,
					CellIDLink,
					SuperCellIDLink,
					SubCellIDLink,
					CellCenterLink,
					CellOrientationLink,
					CellVolumeLink,
					CellLengthLink,
					CellSurfaceLink,
					VecXLink,
					VecYLink,
					VecZLink,
					VecAbsLink,
					VecPhiLink,
					VecThetaLink,
					PureCompositeLink,
					UnLinked};

	Granularity granularity;
	LinkType link;

	// the type agnostic interface for a constant value
	shared_ptr<AbstractProperty> const_prop;
	// the link to a Funktion object
	shared_ptr<Function> func;
	shared_ptr<VectorFunction> vec_func;
	weak_ptr<CellType> celltype;
	// the link to the subscopes overriding this symbol
	vector<Scope*> component_subscopes;
	

	template <class S>
// 	SymbolAccessor<S> spawn_accessor(const CellType* ct) const;
	bool operator ==(const SymbolData& b) { return (link == b.link && name == b.name && type_name == b.type_name); }
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
		
	
	static string getLinkTypeName(LinkType linktype);
    string getLinkTypeName() const;
	
}; 
*/
#endif // SYMBOL_H

