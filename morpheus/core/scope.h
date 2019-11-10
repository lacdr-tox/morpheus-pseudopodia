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
#include <assert.h>


/// \brief Exception class thrown in case of errors related to symbol evaluation
class SymbolError : public logic_error {
public:
	enum class Type { Undefined, WrongType, InvalidPartialSpec, InvalidDefinition};
	SymbolError(Type type, const string& what) : logic_error(what) , _type(type) {};
	SymbolError::Type type() const { return _type; };

private:
	Type _type;
};

class Plugin;
class TimeStepListener;

/// Additional type agnostic interface for composite symbols
class CompositeSymbol_I {
public:
	///  Symbol identifier
	virtual const std::string& name() const =0;  
	/// Init composite symbol to @p n_celltypes CellTypes
	virtual void init(int n_celltypes) =0;
	/// Add a default symbol. The default is the overridden by the subscope symbols.
	virtual void setDefaultValue(Symbol) =0;
	/// Add a celltype specific symbol, that may override a default value in it's celltype scope.
	virtual void addCellTypeAccessor(int celltype_id,  Symbol symbol) =0;
	/// Remove a celltype specific symbol, that may override a default value in it's celltype scope.
	virtual void removeCellTypeAccessor(Symbol symbol) =0;
	/// Get a list of component sub-scopes.
	virtual vector<const Scope*> getSubScopes() const =0;
	virtual ~CompositeSymbol_I() {};
};

class Scope {
public:
	Scope();
	~Scope();
	
	/// Scope name
	string getName() const { return name; };
	/// Unique scope ID
	int getID() const { return scope_id; }
	/// Pointer to parental scope. Null if this is the global scope.
	Scope* getParent() const { return parent; }; 
	/// List of all sub-scopes
	const vector< const Scope* > getSubScopes() const { 
		vector<const Scope*> scopes;
		for (auto& c : sub_scopes) scopes.push_back(c.get());
		return scopes;
	}
	/// List of all component sub-scopes, i.e. sub-scopes spatially tiling the global scope.
	const vector< const Scope* > getComponentSubScopes() const { 
		vector<const Scope*> scopes;
		for (auto& c : component_scopes) scopes.push_back(c.get());
		return scopes;
	}
	/// Create a new sub-scope named @p name. 
	/// If @p ct is a celltype, this sub-scope is assumed to be a spatial scope within the domain of CellType @p ct.
	Scope* createSubScope(string name, CellType *ct = nullptr);
	
	/**
	 * Access the CellType associated with the scope.
	 * Returns null_ptr if no CellType is associated, i.e. no spatial scope.
	 * Also sub-scopes of a CellType scope are associated with the CellType.
	 */ 
	CellType* getCellType() const;
	
	void init();
	
	/// Public store for value overrides from the command-line. Only available to the global scope.
	map<string,string>& value_overrides() const { return _value_overrides; };
	
	/** 
	 * Register a Symbol @p sym to the local scope.
	 * Spatial scopes (CellTypes) also propagate the symbol to the parental scope.
	 * Vector symbols trigger the automatic creation of derived component symbols (.x,.y,.z).
	 */
	void registerSymbol(Symbol sym); 
	
	void removeSymbol(Symbol sym);
	
	/// Find a symbol @p name, without any data access
	Symbol findSymbol(string name) const;
	
	/// Find a readable symbol @p name of type \<T\>. Throws an \ref SymbolError if symbol cannot be found.
	template<class T>
	SymbolAccessor<T> findSymbol(string name, bool allow_partial = false) const;
	
	/// Find a symbol @p name of type \<T\>. If the symbol of @p name is only partially defined, @p default_val is assumed in the undefined scopes. Throws a \ref SymbolError if symbol cannot be found.
	template<class T>
	SymbolAccessor<T> findSymbol(string name, const T& default_val) const;
	
	/// Find a read-writable symbol @p name of type \<T\>. Throws a \ref SymbolError if symbol cannot be found.
	template<class T>
	SymbolRWAccessor<T> findRWSymbol(string name) const;
	
	/// Find all symbols of type \<T\>
	template <class T>
	set<Symbol> getAllSymbols(bool allow_partial = false) const;
	
	/// Find names of all symbols of type \<T\>
	template <class T>
	set<string> getAllSymbolNames(bool allow_partial = false) const;
	
	/// Get all TimeStepListeners registered within the scope
	const set<TimeStepListener *>& getTSLs() const { return local_tsl;};
	
private:
	/// private constructor for creating sub-scopes -- \ref createSubScope makes use  of it
	Scope(Scope* parent, string name, CellType* celltype = nullptr);
	
	string name;
	int scope_id;
	Scope* parent;
	CellType* ct_component;
	mutable map<string, shared_ptr<SymbolBase> > symbols;
	map<string, shared_ptr<CompositeSymbol_I> > composite_symbols;
	
	mutable map<string, string> _value_overrides;
	
	/** Sub-scopes that represent spatial components of the parental scope (i.e. CellTypes) can register symbols
	 *  at their parental scope to override a parental symbol within the sub-scope's range.
	 * 
	 *  Currently, this is only available for CellType scopes, that override the global scope within the lattice part that they occupy.
	 */
	void registerSubScopeSymbol(Scope *sub_scope, Symbol symbol);
	void removeSubScopeSymbol(Symbol sym);
	
	vector< shared_ptr<Scope> > sub_scopes;
	vector< shared_ptr<Scope> > component_scopes;
	
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
	friend class DependencyGraph;
// 	// Filtered Copy of the scheduling elements
// 	mutable multimap<string, TimeStepListener *> filtered_symbol_readers;
// 	mutable multimap<string, TimeStepListener *> filtered_symbol_writers;

	
};


template <class T>
class CompositeSymbol : public SymbolAccessorBase<T>, public CompositeSymbol_I {
public:
	CompositeSymbol(string name, SymbolAccessor<T> default_val = nullptr) : 
		SymbolAccessorBase<T>(name)
	{ 
		if (default_val) {
			setDefaultValue(default_val);
		}
		this->flags().space_const = false;
	};
	~CompositeSymbol() {};
	const std::string& name() const override { return SymbolAccessorBase<T>::name(); };
	std::string linkType() const override { return "CompositeLink"; }; 
	const string& description() const override { return _description;}
	std::set<SymbolDependency> dependencies() const override {
		std::set<SymbolDependency> dep;
		for (auto & ac : celltype_accessors) {
			if (ac && ac->scope())
				dep.insert(ac);
		}
		return dep;
	};

	void addCellTypeAccessor(int celltype_id,  Symbol symbol) override {
		if (celltype_id>=celltype_accessors.size())
			celltype_accessors.resize(celltype_id+1,default_val);
		
		celltype_accessors[celltype_id] = dynamic_pointer_cast<const SymbolAccessorBase< T > >(symbol);
		
		if (_description.empty())
			_description = symbol->description();
		combineFlags(symbol->flags());
	};
	
	void removeCellTypeAccessor(Symbol symbol) override {
		for (auto& acc : celltype_accessors) {
			if (acc == dynamic_pointer_cast<const SymbolAccessorBase< T > >(symbol)) 
				acc = default_val;
		}
	};
	
	void setDefaultValue(Symbol d) override {
		if (default_val)
			throw SymbolError(SymbolError::Type::InvalidDefinition, "Duplicate definition of symbol " + this->name());
		
		if ( ! dynamic_pointer_cast<const SymbolAccessorBase<T> >(d) )
			throw SymbolError(SymbolError::Type::InvalidDefinition, "Incompatible types in definition of composite symbol " + this->name());
		
		default_val = dynamic_pointer_cast<const SymbolAccessorBase<T> >(d);
		for (auto& s : celltype_accessors) if (!s) s = default_val;
		
		
		if (_description.empty())
			_description = d->description();
		combineFlags(default_val->flags());
	}
	
	void init(int n_cts) override {
		celltype_accessors.resize(n_cts, default_val);
		this->flags().partially_defined = false;
		for (auto& ct : celltype_accessors) { if ( !ct ) this->flags().partially_defined=true; }
		if (this->flags().partially_defined)
			cout << "Symbol " << this->name()  << " is only partially defined " << endl;
	}
	
	typename TypeInfo<T>::SReturn get(const SymbolFocus & f) const override {
		assert(celltype_accessors[f.celltype()]);
		return celltype_accessors[f.celltype()]->get(f);
	}
	typename TypeInfo<T>::SReturn safe_get(const SymbolFocus & f) const override{
		if (!celltype_accessors[f.celltype()])
			throw SymbolError(SymbolError::Type::InvalidPartialSpec,"Symbol not defined in subscope");
		return celltype_accessors[f.celltype()]->safe_get(f);
	}

	vector<const Scope*> getSubScopes() const override {
		return this->scope()->getComponentSubScopes();
	}
	
private:
	
	void combineFlags(const SymbolBase::Flags& of) {
		if (!initialized) {
			auto& f = this->flags();
			f = of;
			f.space_const = false;
			initialized=true;
		}
		else {
			auto& f = this->flags();
			f.granularity += of.granularity;
			f.time_const = f.time_const && of.time_const;
			f.stochastic = f.stochastic || of.stochastic;
			f.integer = f.integer && of.integer;
			f.delayed = f.delayed && of.delayed;
		}
	}
	bool initialized=false;
	
	vector<SymbolAccessor<T> > celltype_accessors;
	string _description;
	SymbolAccessor<T> default_val;
};

class VectorComponentAccessor : public SymbolAccessorBase<double> {
public:
	enum class Component { X, Y, Z, PHI, THETA, R};
	string subSymbol(Component comp) { 
		switch(comp) {
			case Component::X: return "x";
			case Component::Y: return "y";
			case Component::Z: return "z";
			case Component::PHI: return "phi";
			case Component::THETA: return "theta";
			case Component::R: return "abs";
		}
		return "";
	};
	VectorComponentAccessor(SymbolAccessor<VDOUBLE> v_sym, Component comp) : SymbolAccessorBase<double>(v_sym->name()+"."+subSymbol(comp)), comp(comp), v_sym(v_sym) {}
	const SymbolBase::Flags & flags() const override {
		return v_sym->flags();
	};
	const string& description() const override { return v_sym->description(); }
	string linkType() const override { return "VectorComponentLink"; }
	std::set<SymbolDependency> dependencies() const override { return { v_sym }; }
	typename TypeInfo<double>::SReturn get(const SymbolFocus & f) const override {
		switch(comp) {
			case Component::X: return v_sym->get(f).x;
			case Component::Y: return v_sym->get(f).y;
			case Component::Z: return v_sym->get(f).z;
			case Component::PHI: return v_sym->get(f).angle_xy();
			case Component::THETA: return v_sym->get(f).to_radial().y;
			case Component::R: return v_sym->get(f).abs();
		}
		return 0;
	};
	typename TypeInfo<double>::SReturn safe_get(const SymbolFocus & f) const override {
		switch(comp) {
			case Component::X: return v_sym->safe_get(f).x;
			case Component::Y: return v_sym->safe_get(f).y;
			case Component::Z: return v_sym->safe_get(f).z;
			case Component::PHI: return v_sym->safe_get(f).angle_xy();
			case Component::THETA: return v_sym->safe_get(f).to_radial().y;
			case Component::R: return v_sym->safe_get(f).abs();
		}
		return 0;
	};
private: 
	Component comp;
	SymbolAccessor<VDOUBLE> v_sym;
};


/////////////////////////////////////////////////
/// \b Scope template method implementation   ///
/////////////////////////////////////////////////

// #include "property.h"

template <class T>
SymbolAccessor<T> Scope::findSymbol(string name, bool allow_partial) const
{
	if(name.empty())
		throw SymbolError(SymbolError::Type::Undefined, string("Requesting symbol without a name \"") + name + ("\""));
	auto it = symbols.find(name);
	if ( it != symbols.end()) {
		if (TypeInfo<T>::name() != it->second->type()) {
			throw SymbolError(SymbolError::Type::WrongType, string("Cannot create an accessor of type ")
				+ TypeInfo<T>::name() 
				+ " for Symbol " + name
				+ " of type " + it->second->type() );
		}
		if (it->second->flags().partially_defined && ! allow_partial)
			throw SymbolError(SymbolError::Type::InvalidPartialSpec, string("Composite symbol ") + name + " is not defined in all subscopes and has no global default.");
			
		cout << "Scope: Creating Accessor for symbol " << name << " from Scope " << this->name << endl;
		
		auto s = dynamic_pointer_cast<const SymbolAccessorBase<T> >(it->second);
		if (!s)
			throw SymbolError(SymbolError::Type::Undefined, string("Unknown error while creating symbol accessor for symbol ") + name +".");
		return s;
	}
	else if (parent) {
		return parent->findSymbol<T>(name, allow_partial);
	}
	else {
		throw SymbolError(SymbolError::Type::Undefined, string("Symbol \"")+name+"\" is not defined in Scope " + this->getName() );
	}
};


template <class T>
SymbolRWAccessor<T> Scope::findRWSymbol(string name) const
{
	auto sym = findSymbol<T>(name,false);;
	if ( ! sym->flags().writable )
		SymbolError(SymbolError::Type::WrongType, string("Symbol ") + name + " is not writable.");
	auto r = dynamic_pointer_cast<const SymbolRWAccessorBase<T> >(sym);
	if (!r)
		throw SymbolError(SymbolError::Type::Undefined, string("Unknown error while creating writable symbol accessor for symbol ") + name +".");
	return r;
};

template <class T>
SymbolAccessor<T> Scope::findSymbol(string name, const T& default_val) const
{
// 	try {
		auto s = findSymbol<T>(name, true);
		if (dynamic_pointer_cast<const CompositeSymbol<T> >(s) && s->flags().partially_defined) {
			auto new_s = make_shared<CompositeSymbol<T> >(*dynamic_pointer_cast<const CompositeSymbol<T> >(s));
			new_s->setDefaultValue( SymbolAccessorBase<T>::createConstant(name, name, default_val) );
			return new_s;
		}
		return s;
		
// 	}
// 	catch (...) {
// 		// create a default accessor
// 		auto c = SymbolAccessorBase<T>::createConstant(name, name, default_val);
// 		return c;
// 	}
};

template <class T>
set<Symbol> Scope::getAllSymbols(bool allow_partial) const {
	set<Symbol> r_symbols;
	if (parent)
		r_symbols = parent->getAllSymbols<T>();
	for ( auto sym : symbols ) {
		if (TypeInfo<T>::name() == sym.second->type()) {
			if (sym.second->flags().partially_defined && ! allow_partial)
				continue;
			r_symbols.insert(sym.second);
		}
	}
	return r_symbols;
}

template <class T>
set<string> Scope::getAllSymbolNames(bool allow_partial) const {
	set<string> names;
	if (parent)
		names = parent->getAllSymbolNames<T>();
	for ( auto sym : symbols ) {
		if (TypeInfo<T>::name() == sym.second->type()) {
			if (sym.second->flags().partially_defined && ! allow_partial)
				continue;
			names.insert(sym.first);
		}
	}
	return names;
}

#endif
