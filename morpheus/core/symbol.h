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

#include "string_functions.h"
#include <vector>
#include <deque>
typedef deque<double> double_queue;
#include "vec.h"
// #include "cpm_layer.h"

/** \defgroup SymbolSystem Symbolic Linking System
 * 
 * \section Introduction
 * The Symbolic Linking system allows for dynamic linking of information flow between the individual computational modules of the platform.
 * Links are defined by symbolic references in the XML or indirectly through numerical expressions. 
 * 
 * Ususally, there is no need understand to understand a lot about the particular implementation, but rather make use of PluginParameters or in rare cases SymbolAccessors. 
 */


// Global type dependent switches ...
template <class T>
struct TypeInfo {
	typedef const T& Return;
	typedef T SReturn; 
	typedef const T& Parameter; 
	typedef T& Reference; 
	static SReturn fromString(const string& val) { stringstream s(val); T ret; s >> ret; if (s.fail()) { throw string("Unable to read value from string \'") + val + "'";} return ret; }
	static const string name();
	
};


template <>
struct TypeInfo<double> {
	typedef double Return;
	typedef double SReturn;
	typedef double Parameter; 
	typedef double& Reference; 
	static SReturn fromString(const string& val) { stringstream s(val); double ret; s >> ret; if (s.fail()) { throw string("Unable to read value from string \'") + val + "'";} return ret; }
	static const string name();
};

template <>
struct TypeInfo<float> {
	typedef float Return;
	typedef float SReturn;
	typedef float Parameter; 
	typedef float& Reference; 
	static SReturn fromString(const string& val) { stringstream s(val); double ret; s >> ret; if (s.fail()) { throw string("Unable to read value from string \'") + val + "'";} return ret; }
	static const string name();
};

template <>
struct TypeInfo<bool> {
	typedef bool Return;
	typedef bool SReturn;
	typedef bool Parameter; 
	typedef bool& Reference; 
	static bool fromString(string val) { lower_case(val); if ( val == "true" ) return true; else if ( val == "false" ) return false; else throw string("Unable to read value from string \'") + val + "'"; }
	static const string name();
};

template <>
struct TypeInfo<string> {
	typedef const string& Return;
	typedef string SReturn;
	typedef const string& Parameter; 
	typedef string& Reference; 
	static string fromString(string val) { return val; }
	static const string name() { return "String";};
};
// this guy takes care of making the real connections and thus needs all the platform as includes
template <class T> class ReadOnlyAccess;
template <class T> class ReadWriteAccess;
template <class T, template <class> class AccessPolicy = ReadOnlyAccess> class SymbolAccessorBase;

template <class T>
using SymbolAccessor = SymbolAccessorBase<T,ReadOnlyAccess>;

template <class T> class SymbolRWAccessor;
class Function;
class VectorFunction;
class CellType;
class AbstractProperty;

// Forward declarations for platform provided computation symbols
extern string sym_RandomUni;
extern string sym_RandomNorm;
extern string sym_RandomBool;
extern string sym_RandomGamma;
extern string sym_Modulo;

/**
 * \brief SymbolData is a Symbol Descriptor, with the ability to spawn a fully fledged Accessor.
 * 
 * Also contains the storage for globally predefined symbols like time and space
 */

class Scope;

enum class Granularity {
	 Undef, Global, Node, Cell, MembraneNode, SurfaceNode
};

bool operator<(Granularity a, Granularity b);
bool operator<=(Granularity a, Granularity b);

Granularity operator+(Granularity a, Granularity b);
Granularity& operator+=(Granularity& g, Granularity b);
ostream& operator<<(ostream& out, Granularity g);

class SymbolError : public logic_error {
public:
	enum class Type { Undefined, InvalidLink, InvalidPartialSpec, InvalidDefinition };
	SymbolError(Type type, const string& what) : logic_error(what) , _type(type) {};
	SymbolError::Type type() const { return _type; };

private:
	Type _type;
};

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
#endif // SYMBOL_H

