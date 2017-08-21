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

#ifndef SCOPE_H
#define SCOPE_H

#include "symbol.h"
// #include "symbolfocus.h"
#include <assert.h>

// #include "interfaces.h"
class Plugin;
class TimeStepListener;


class Scope {
public:
	Scope();
	~Scope()  { cout << "Deleting scope " << name << endl;} 
	// TODO Shall we use weak_ptr here ?? --> There is no Ownership concept for scopes present !!
	Scope* getParent() const {return parent;}; /// Pointer to parental scope. Null if this is the global scope.
	Scope* createSubScope(string name, CellType *ct = nullptr);
		
	CellType* getCellType() const;
	string getName() const { return name; };
	
	void registerSymbol(SymbolData data); /// Adds the symbol to the scope. Propagates the presence if this scope is a component of the parental scope.
	
	void init();
	
	void setValueOverride(string symbol, string value) { value_overrides[symbol] = value;};
	const map<string, string>& valueOverrides() const { return value_overrides; };
	void removeValueOverride(string symbol) const { value_overrides.erase(symbol); };
	const map<string, string>& unusedValueOverrides() const { return value_overrides; };
	
	/// Find a read-writable symbol with @p name. Throws an error if symbol cannot be found.
	template<class T>
	SymbolRWAccessor<T> findRWSymbol(string name) const;
	
	/// Find a readable symbol with @p name. Throws an error if symbol cannot be found.
	template<class T>
	SymbolAccessor<T> findSymbol(string name, bool allow_partial = false) const;
	
	/// Find a symbol with @p name. Return a constant symbol of @p default_val, if the symbol cannot be found.
	template<class T>
	SymbolAccessor<T> findSymbol(string name, const T& default_val) const;
	
	/// Find all symbol names of type T
	template <class T>
	set<string> getAllSymbolNames() const;
	
	string getSymbolType(string name) const;
	string getSymbolBaseName(string name) const;
	bool isSymbolDelayed(string name) const;
	
	struct DepGraphConf { set<string> exclude_symbols; set<string> exclude_plugins; };
	void write_graph(ostream& out, const DepGraphConf& config) const;


	map<string, string> graphstyle;
	
private:
	Scope(Scope* parent, string name, map<string, string> colorscheme, CellType* celltype = nullptr);
	CellType* ct_component;
	string name;
	int scope_id;
	
	/** All subscopes that represent spatial components of the paraental scope (e.g. CellTypes) can register in their parental scope to override
	 *  a parental symbol within their spatial extends.
	 * 
	 *  Currently, this is only available for CellType scopes, that override the global scope within the lattice part that they occupy
	 */
	void registerSubScopeSymbol(Scope *sub_scope, string symbol_name);
	// a symbol is registered in a ct_subscope. create a new virt. symbol, if it does not exist, and move an existing global symbol as a default there. then override the ct association for 
	Scope* parent;
	vector< shared_ptr<Scope> > sub_scopes;
	vector< shared_ptr<Scope> > component_scopes;
	mutable map<string, SymbolData> local_symbols;
	mutable map<string, string> value_overrides;
	
	void init_symbol(SymbolData* data) const;
	mutable set<SymbolData*> initializing_symbols;
	
	// INTERFACE FOR SCHEDULING THROUGH THE TimeScheduler
	friend class TimeScheduler;
	friend class TimeStepListener;
	multimap<string, TimeStepListener *> symbol_readers;
	multimap<string, TimeStepListener *> symbol_writers;
	
	
	set<TimeStepListener *> local_tsl;
	void registerSymbolWriter(TimeStepListener* tsl, string symbol);
	void registerSymbolReader(TimeStepListener* tsl, string symbol);
	void registerTimeStepListener(TimeStepListener* tsl);
	
	void propagateSourceTimeStep(string symbol, double time_step);
	void propagateSinkTimeStep(string symbol, double time_step);
	
	// Used for dependency tracking
	void addUnresolvedSymbol(string symbol);
	void removeUnresolvedSymbol(string symbol);
	bool isUnresolved(string symbol) { return unresolved_symbols.find(symbol) != unresolved_symbols.end(); };
	multiset<string> unresolved_symbols;
	
	static int max_scope_id;
	
	/// Generation of DotGraph of Dependencies
	// Filtered Copy of the scheduling elements
// 	mutable map<string, SymbolData> filtered_local_symbols;
// 	mutable multimap<string, TimeStepListener *> filtered_local_tls;
	mutable multimap<string, TimeStepListener *> filtered_symbol_readers;
	mutable multimap<string, TimeStepListener *> filtered_symbol_writers;
	void write_graph_local_variables(ostream& definitions, ostream& links, const DepGraphConf& config) const;
	string tslDotName(TimeStepListener* tsl) const;
	string pluginDotName(Plugin* p) const;
	string dotStyleForType(const string& type) const;
	
};

template<class T>
SymbolAccessor<T> Scope::findSymbol(string name, bool allow_partial) const {
// 	cout << "Symbol name: " << name << endl;
	if(name.empty())
		throw (string("Symbol without a name \"") + name + ("\""));

	// try to find it locally
// 	try {
		auto it = local_symbols.find(name);
		if ( it != local_symbols.end()) {
			if (TypeInfo<T>::name() != it->second.type_name) {
				throw (string("Symbol type mismatch. Cannot create an Accessor of type ")
					+ TypeInfo<T>::name() 
					+ " for Symbol " + name
					+ " of type " + it->second.type_name );
			}
			cout << "Scope: Creating Accessor for symbol " << name << " from Scope " << this->name << endl;
			
			SymbolData& data = it->second;
			// Assert dynamic symbols (i.e. functions) to be initialized
			if (data.granularity == Granularity::Undef) {
				init_symbol(&data);
			}
			
			return SymbolAccessor<T>(data, this, allow_partial);
		}
		else if (parent) {
			return parent->findSymbol<T>(name, allow_partial);
		}
		else {
 			throw (string("Symbol \"")+name+"\" is not defined. ");
		}
// 	}
// 	catch (string e) {
// // 		throw ("Global default for symbol \""+name+"\" is missing. ");
// 		stringstream sstr;
// 		sstr << "Unable to create a Symbol Accessor for Symbol " <<  name << "." << endl;
// 		sstr << e << endl; 
// // 		sstr << "Available symbols: ";
// // 		for (auto it : local_symbols) {
// // 			if (it.second.type_name == TypeInfo<T>::name())
// // 				sstr << "\""<< it.first << "\", ";
// // 		}
// 		throw (sstr.str());
// 	}
};

template<class T>
SymbolRWAccessor<T> Scope::findRWSymbol(string name) const {
// 	cout << "Symbol name: " << name << endl;
	if(name.empty())
		throw (string("Symbol without a name \"") + name + ("\""));
	
	// try to find it locally
// 	try {
		auto it = local_symbols.find(name);
		if ( it != local_symbols.end()) {
			if (TypeInfo<T>::name() != it->second.type_name) {
				throw (string("Symbol type mismatch. Cannot create an Accessor of type ")
					+ TypeInfo<T>::name() 
					+ " for Symbol " + name
					+ " of type " + it->second.type_name );
			}
			
			SymbolData& data = it->second;
			// Assert dynamic symbols (i.e. functions) to be initialized
			if (data.granularity == Granularity::Undef) {
				init_symbol(&data);
			}
			
			return SymbolRWAccessor<T>(data, this);
		}
		else if (parent) {
			return parent->findRWSymbol<T>(name);
		}
		else {
			throw (string("Requested symbol \"") + name + "\" is not defined.");
		}
// 	}
// 	catch (string e) {
// // 		throw ("Cannot find (r+w) symbol \""+name+"\". ");
// 		stringstream sstr;
// 		sstr << "Unable to create a Symbol Accessor for Symbol " <<  name << "." << endl;
// 		sstr << e << endl; 
// // 		sstr << "Available symbols: ";
// // 		for (auto it : local_symbols) {
// // 			if (it.second.type_name == TypeInfo<T>::name())
// // 				sstr << "\""<< it.first << "\", ";
// // 		}
// 		throw (sstr.str());
// 	}
};

template<class T>
SymbolAccessor<T> Scope::findSymbol(string name, const T& default_val) const {
// 	cout << "Symbol name: " << name << endl;
	if(name.empty())
		throw (string("Symbol without a name \"") + name + ("\""));
	// try to find it locally
// 	try {
		auto it = local_symbols.find(name);
		if ( it != local_symbols.end()) {
			if (TypeInfo<T>::name() != it->second.type_name) {
				throw (string("Symbol type mismatch. Cannot create an Accessor of type ")
					+ TypeInfo<T>::name() 
					+ " for Symbol " + name
					+ " of type " + it->second.type_name );
			}
			
			SymbolData& data = it->second;
			// Assert dynamic symbols (i.e. functions) to be initialized
			if (data.granularity == Granularity::Undef) {
				init_symbol(&data);
			}
			
			return SymbolAccessor<T>(data, this, default_val);
		}
		else if (parent) {
			return parent->findSymbol<T>(name, default_val);
		}
		else {
			throw (string("Requested symbol \"") + name + "\" is not defined.");
		}
// 	}
// 	catch (string e) {
// // 		throw ("Cannot find symbol \""+name+"\". ");
// 		stringstream sstr;
// 		sstr << "Unable to create a Symbol Accessor for Symbol " <<  name << "." << endl;
// 		sstr << e << endl; 
// // 		sstr << "Available symbols: ";
// // 		for (auto it : local_symbols) {
// // 			if (it.second.type_name == TypeInfo<T>::name())
// // 				sstr << "\""<< it.first << "\", ";
// // 		}
// 		throw (sstr.str());
// 	}
};

template <class T>
set<string> Scope::getAllSymbolNames() const {
	set<string> names;
	if (parent)
		names = parent->getAllSymbolNames<T>();
	for ( auto sym : local_symbols ) {
		if (TypeInfo<T>::name() == sym.second.type_name) {
			if (sym.second.link == SymbolData::PureCompositeLink) {
				bool all_subscopes_valid = true;
				for (auto sub : sym.second.component_subscopes){
					if (sub == NULL)
						all_subscopes_valid = false;
				}
				if (all_subscopes_valid)
					names.insert(sym.first);
			}
			else {
				names.insert(sym.first);
			}
		}
	}
	return names;
}

#endif
