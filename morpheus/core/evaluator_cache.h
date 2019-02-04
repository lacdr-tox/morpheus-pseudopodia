#ifndef EVALUATOR_CACHE_H
#define EVALUATOR_CACHE_H

#include "scope.h"
#include "muParser/muParser.h"
#include <functional>

/**
 * \brief Value cache for the Expression evaluators
 *  
 * * Provides a variable factory based on 
 *   * the local scope symbols
 *   * namespace scopes symbol
 *   * cache local symbols
 *   * (parser variables)
 * * Fetches Symbols from scopes
 * * Stores local symbols
 * * Permits scalar expansion on expressions for vector operands
 * * Efficient assignment of locals through array
 */

class EvaluatorCache 
{
public:
	
	/// Descriptor for the cache layout of cache local variables.
	struct LocalSymbolDesc {
		string symbol;
		enum Type { DOUBLE, VECTOR } type;
	};
	
	EvaluatorCache(const Scope * scope, bool allow_partial_spec=false) : local_scope(scope), allow_partial_spec(allow_partial_spec) {};
	
	EvaluatorCache(const EvaluatorCache& other) ; // We need to adjust the cache links of expansions and namespace symbols.
	
	const EvaluatorCache& operator=(const EvaluatorCache& other) = delete;
	
	const Scope* getScope() { return local_scope; }

	/// Symbol factory to be used with muParser
	static mu::value_type* registerSymbol(const mu::char_type* symbol, void* ptr);
	
	/// Add a local symbol @p name to the cache and return it's cache position
	int addLocal(string name, double value);
	
	/// Add a local symbol @p name to the cache and return it's cache position
	int addLocal(string name, VDOUBLE value);
	
	int getLocalIdx(string name) {
		if (locals.count(name)) return locals[name];
		if (v_locals.count(name)) return v_locals[name];
		return -1;
	}
	
	/// Set a cache local symbol @p name to @p value
	void setLocal(string name, double value) {
		assert(locals.count(name));
		locals_cache[locals[name]] = value;
	}
	
	/// Set a cache local symbol at position @p cache_pos to @p value
	void setLocal(uint cache_pos, double value) {
		locals_cache[cache_pos] = value;
	}
	
	/// Set a cache local symbol @p name to @p value
	void setLocal(string name, VDOUBLE value) {
		assert(v_locals.count(name));
		auto pos = v_locals[name];
		locals_cache[pos] = value.x;
		locals_cache[pos+1] = value.y;
		locals_cache[pos+2] = value.z;
	}
	
	/// Set a cache local symbol at position @p cache_pos to @p value
	void setLocal(uint cache_pos, VDOUBLE value) {
		locals_cache[cache_pos] = value.x;
		locals_cache[cache_pos+1] = value.y;
		locals_cache[cache_pos+2] = value.z;
	}
	
	/*! \brief Add a foreign Scope @p scope as namespace @p ns_name to the local variable scope.
	 * 
	 * Returns the name space reference @return id. */
	uint addNameSpaceScope(const string& ns_name, const Scope* scope);
	/// Get all symbols used from name space @p ns_id. The namespace prefix is not contained in the symbols returned.
	set<Symbol> getNameSpaceUsedSymbols(uint ns_id) const;
	/// Set the focus of name space @p ns_id
	void setNameSpaceFocus(uint ns_id, const SymbolFocus& f) const;
	
	
	/// Get the memory layout of local symbols. Using that table, setLocals will update all local data at once.
	const vector<LocalSymbolDesc>& getLocalsTable() const { return locals_table; }
	
	/// Set the table of local symbols in a compact way.
	void setLocalsTable(const vector<LocalSymbolDesc>& layout);
	/** Set all local data at once using @p data
	 *  
	 * Update the scalar expansion wrappers must be done manually by calling setExpansionIndex
	 */
	void setLocals(const double *data) {
// 		if (sizeof(data) != sizeof(double) * locals_cache.size() ) cout << sizeof(data) << " != " << locals_cache.size() <<  " * " << sizeof(double) << endl;
// 		assert(sizeof(data) == sizeof(double) * locals_cache.size() );
		memcpy( &locals_cache[0] , data, sizeof(double) * locals_cache.size() );
	}
	
	void addParserLocal(const string& name) { parser_symbols.insert(name); }
	
	/// Get the current value of symbol @p name
	double get(const string& name); 
	/// Get the current value of symbol at @p idx
	double getLocalD(uint idx) { return locals_cache[idx]; }; 
	/// Get the current value of symbol at @p idx
	VDOUBLE getLocalV(uint idx) { return VDOUBLE(locals_cache[idx],locals_cache[idx+1],locals_cache[idx+2]); }; 
	
	/// Struct describing the used external symbols (i.e. dependencies) and wether scalar expansion was used to resolve the symbols.
	struct ParserDesc { set<Symbol> ext_symbols; set<string> loc_symbols; bool requires_expansion; };
	
	/// Attach the variables of a parser to the cache.
	void attach(mu::Parser *parser);
	/// Returns a dependency descriptor of the parser's expression
	EvaluatorCache::ParserDesc getDescriptor(mu::Parser *parser);
	
	
	/// Fill the cache with data wrt. @p focus
	void fetch(const SymbolFocus& focus, const bool safe=false);
	
	/// A list of used external symbols.
	std::set<Symbol> getExternalSymbols();
	
	/// Set current index @p idx for the scalar vector expansion (@p idx < 3)
	void setExpansionIndex(uint idx) noexcept {
		if (! scalar_expansion || idx>2) return;
		for (auto& exp : expansions) {
			if (exp.second.source == ExpansionDesc::LOCAL)
				exp.second.val = locals_cache[exp.second.local_source_idx + idx];
			else {
// 				exp.second.val = (&exp.second.source_desc->second.valV.x)[idx];
				if (idx==0) exp.second.val = exp.second.source_desc->valV.x;
				else if (idx==1) exp.second.val = exp.second.source_desc->valV.y;
				else if (idx==2) exp.second.val = exp.second.source_desc->valV.z;
			}
		}
	}
	/// Allow to perform scalar expansion
	void permitScalarExpansion(bool allow) noexcept { scalar_expansion_permitted = allow; }
	
	/// Scalar vector expansion in use
	bool getPartialSpec() noexcept { return allow_partial_spec; };
	
	/// Get a string of all values in the cache
	string toString();
	
protected:
	
	struct SymbolDesc {
		double valD;
		VDOUBLE valV;
		enum Type { DOUBLE, VECTOR } type;
		enum Source {NS, Ext} source;
		Symbol sym;
	};

	struct ExpansionDesc {
		enum {LOCAL, EXTERNAL} source;
		uint local_source_idx;
		SymbolDesc* source_desc;
		double val;
	};

	/** @brief Symbol namespace referring to a scope
	 * 
	 * Namespaces may provide access to other scopes than the expressions main scope. 
	 * Use \ref setFocus() to set the current \ref SymbolFocus of the namespace.
	 * Variable registration is done on demand, so that namespace may not be used at all ...
	 */ 
	class NS {
		private:
			const Scope* scope;
			string ns_name;
			map<string, SymbolDesc* > used_symbols;
			
			friend EvaluatorCache;
			
		public:
			NS(string name, const Scope* scope);
			const Scope* getScope() const { return scope; };
			const string& getNameSpace() const { return ns_name; };
			/// Set the current focus of the namespace scope
			void setFocus(const SymbolFocus& f) const;
			
			set<Symbol> getUsedSymbols() const;
	};
	
	/// internal implementation of the symbol factory 
	mu::value_type* registerSymbol_internal(const mu::char_type* symbol);
	
	const Scope* local_scope;
	bool initialized = false;
	bool allow_partial_spec;
	double no_val = 0;
	SymbolFocus current_focus;
	
	// Local variable storage
	map<string, uint> locals;
	map<string, uint> v_locals;
	vector<LocalSymbolDesc> locals_table;
	vector<double> locals_cache;
	
	// External variable storage
	map<string, SymbolDesc> externals;
	vector< NS > externals_namespaces;
	
	// infrastructure for vector symbol expansion
	bool scalar_expansion_permitted = false;
	bool scalar_expansion = false;
	map<string, ExpansionDesc> expansions;

	// parser local symbols, i.e. used for Function parameters
	set<string> parser_symbols;
};


#endif
