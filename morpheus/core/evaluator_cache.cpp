#include "evaluator_cache.h"

EvaluatorCache::NS::NS(string name, const Scope* scope) :
 scope(scope), ns_name(name) {}

set<Symbol> EvaluatorCache::NS::getUsedSymbols() const
{
	set<Symbol> dep;
	for (const auto& sym : used_symbols) {
		dep.insert(sym.second->sym);
	}
	return dep;
}

void EvaluatorCache::NS::setFocus(const SymbolFocus& f) const
{
	for (const auto& sym : used_symbols) {
		auto sd = sym.second;
		if (sd->type == SymbolDesc::DOUBLE) {
			auto accessor = static_pointer_cast<const SymbolAccessorBase<double>>(sd->sym);
			sd->valD = accessor->get(f);
		}
		else {
			auto accessor = static_pointer_cast<const SymbolAccessorBase<VDOUBLE>>(sd->sym);
			sd->valV = accessor->get(f);
		}
	}
}

uint EvaluatorCache::addNameSpaceScope(const string& ns_name, const Scope* scope)
{
	uint ns_id = externals_namespaces.size();
	externals_namespaces.push_back(NS(ns_name,scope));
	return ns_id;
}

set<Symbol> EvaluatorCache::getNameSpaceUsedSymbols(uint ns_id) const {
	if (ns_id>externals_namespaces.size())
		throw string("EvaluatorCache: Invalid namespace id");
	return externals_namespaces[ns_id].getUsedSymbols();
}

void EvaluatorCache::setNameSpaceFocus(uint ns_id, const SymbolFocus& f) const {
	if (ns_id>externals_namespaces.size())
		throw string("EvaluatorCache: Invalid namespace id");
	externals_namespaces[ns_id].setFocus(f);
}


EvaluatorCache::EvaluatorCache(const EvaluatorCache& other)
{
	local_scope = other.local_scope ;
	initialized = other.initialized;
	allow_partial_spec = other.allow_partial_spec;
	no_val = other.no_val;
	current_focus = other.current_focus;
	
	// Local variable storage
	locals = other.locals;
	v_locals = other.v_locals;
	locals_table = other.locals_table;
	locals_cache = other.locals_cache;
	
	// External variable storage
	map<string, SymbolDesc> externals = other.externals;
	vector< NS > externals_namespaces = other.externals_namespaces;
	
	// infrastructure for vector symbol expansion
	scalar_expansion_permitted  = other.scalar_expansion_permitted;
	scalar_expansion = other.scalar_expansion;
	expansions = other.expansions;

	// parser local symbols, i.e. used for Function parameters
	parser_symbols = other.parser_symbols;
	
	// Rewire cached SymbolDesc
	for (auto& ns : externals_namespaces) {
		for (auto& sym : ns.used_symbols) {
			sym.second = &externals[sym.first];
		}
	}
	
	for (auto& exp : expansions) {
		if (exp.second.source == ExpansionDesc::EXTERNAL) {
			exp.second.source_desc = &externals[exp.first];
		}
	}
}


mu::value_type* EvaluatorCache::registerSymbol(const mu::char_type* symbol, void* ptr) {
	auto that = reinterpret_cast<EvaluatorCache*>(ptr);
	return that->registerSymbol_internal(symbol);
}

mu::value_type* EvaluatorCache::registerSymbol_internal(const mu::char_type* sym) {
	initialized = true;
// 	if (locals_callback) {
// 		// Try to dynamically add the requested symbol to the locals
// 		locals_callback(symbol,this);
// 	}
	string symbol = sym;
	// Check local definitions first, since they might override externals
	if (locals.count(symbol)) {
		return &locals_cache[locals[symbol]];
	}
	
	if (scalar_expansion_permitted) {
		if (v_locals.count(symbol)) {
			/// create the scalar expansion wrapper
			ExpansionDesc exp;
			exp.source = ExpansionDesc::LOCAL;
			exp.local_source_idx = v_locals[symbol];
			exp.val = 0;
			auto exp_it = expansions.insert( {symbol, exp} ).first;
			scalar_expansion = true;
			return &exp_it->second.val;
		}
	}
	
	// Check registered external symbols
	if (externals.count(symbol)) {
		auto& ext = externals[symbol];
		if (ext.type == SymbolDesc::DOUBLE)
			return &ext.valD;
		else if ( scalar_expansion_permitted ) {
			assert(expansions.count(symbol));
			return &expansions[symbol].val;
		}
	}
	
	// Try to register the symbol dynamically
	
	const Scope* search_scope = local_scope;
	string search_symbol = symbol;
	NS* ns = nullptr;
	// From Namespaces
	if (symbol.find('.') != string::npos) {
		string ns_name = symbol.substr(0,symbol.find('.'));
		string ns_symbol = symbol.substr(symbol.find('.')+1, string::npos);
		// Try to find a suitible name space
		for ( auto& l_ns : externals_namespaces ) {
			if ( l_ns.ns_name == ns_name ) {
				ns = &l_ns;
				search_scope = l_ns.scope;
				search_symbol = ns_symbol;
			}
		}
	}
	// From search scope
	if (search_scope->getAllSymbolNames<double>(allow_partial_spec).count(search_symbol)) {
		SymbolDesc sd;
		sd.sym  = search_scope->findSymbol<double>(search_symbol, allow_partial_spec);
		sd.valD = 0;
		sd.type = SymbolDesc::DOUBLE;
		sd.source = ns ? SymbolDesc::NS : SymbolDesc::Ext;
		auto ext_it = externals.insert({ symbol, sd }).first;
		if (ns) ns->used_symbols.insert({search_symbol, &ext_it->second});
		return &ext_it->second.valD;
	}
	if (scalar_expansion_permitted && search_scope->getAllSymbolNames<VDOUBLE>(allow_partial_spec).count(search_symbol)) {
		/// create the scalar expansion wrapper
		auto ext_it = externals.find(symbol);
		// not registered yet
		if (ext_it == externals.end()) {
			
			SymbolDesc sd;
			sd.sym = search_scope->findSymbol<VDOUBLE>(search_symbol, allow_partial_spec);
			sd.valV = VDOUBLE(0,0,0);
			sd.type = SymbolDesc::VECTOR;
			sd.source = ns ? SymbolDesc::NS : SymbolDesc::Ext;
			ext_it = externals.insert({symbol,sd}).first;
			if (ns) ns->used_symbols.insert({search_symbol, &ext_it->second});
		}
		ExpansionDesc exp;
		exp.source = ExpansionDesc::EXTERNAL;
		exp.source_desc = &ext_it->second;
		exp.val = 0;
		auto exp_it = expansions.insert( {symbol, exp}).first;
		scalar_expansion = true;
		return &exp_it->second.val;
	}
	else {
		mu::Parser::exception_type e(mu::ecINVALID_NAME,symbol);
		cout << "EvaluatorCache::registerSymbol(): Unknown symbol " << symbol << endl;
		cout << "Available is ";
		if (ns) cout << " in namespace \"" << ns->ns_name << "\" ";
		set<string > all_symbols = search_scope->getAllSymbolNames<double>(allow_partial_spec);
		copy(all_symbols.begin(), all_symbols.end(),ostream_iterator<string>(cout,","));
		for (const auto& sym : locals) cout << ", " << sym.first;
		if (scalar_expansion_permitted) {
			all_symbols = search_scope->getAllSymbolNames<VDOUBLE>(allow_partial_spec);
			copy(all_symbols.begin(), all_symbols.end(),ostream_iterator<string>(cout,","));
			for (const auto& sym : v_locals) cout << ", " << sym.first;
		}
		cout << endl;
		throw e;
	}
	return  &no_val;
}

/// Add a local symbol @p name to the cache and return it's cache position
int EvaluatorCache::addLocal(string name, double value) {
	if (initialized) throw string("Cannot add locals after initilization in EvaluatorCache!");
	if (locals.count(name)) throw string("Cannot add local in EvaluatorCache! Symbol ") + name + " already exists";
	int pos = locals_cache.size();
	locals_cache.push_back(value);
	locals[name] = pos;
	locals_table.push_back( { name, LocalSymbolDesc::DOUBLE } );
	return pos;
}

/// Add a local symbol @p name to the cache and return it's cache position
int EvaluatorCache::addLocal(string name, VDOUBLE value) {
	if (initialized) throw string("Cannot add locals after initilization in EvaluatorCache!");
	if (locals.count(name)) throw string("Cannot add local in EvaluatorCache! Symbol ") + name + " already exists";
	int pos = locals_cache.size();
	locals_cache.push_back(value.x);
	locals_cache.push_back(value.y);
	locals_cache.push_back(value.z);
	v_locals[name] = pos;
	locals_table.push_back( { name, LocalSymbolDesc::VECTOR } );
	locals[name+".x"] = pos;
	locals[name+".y"] = pos+1;
	locals[name+".z"] = pos+2;
	return pos;
}

void EvaluatorCache::setLocalsTable(const vector<EvaluatorCache::LocalSymbolDesc>& layout)
{

	if ( ! locals_table.empty() )
		throw string("EvaluatorCache::setLocalsTable()  Refuse to override locals. Locals already defined.");
	for (const auto& loc : layout) {
		if (loc.type == LocalSymbolDesc::VECTOR) {
			addLocal(loc.symbol, VDOUBLE(0,0,0));
		}
		else 
			addLocal(loc.symbol, 0);
	}
}

double EvaluatorCache::get(const string& name) {
	if (locals.count(name))
		return locals_cache[locals[name]];
	if (externals.count(name))
		return externals[name].valD;
	if (expansions.count(name))
		return expansions[name].val;
	return 0;
}


EvaluatorCache::ParserDesc EvaluatorCache::getDescriptor(mu::Parser *parser) {
	auto symbols = parser->GetUsedVar();
	ParserDesc desc;
	desc.requires_expansion = false;
	for (const auto & sym : symbols) {
		
		// look up scalar expansion of vector symbols
		if (scalar_expansion && expansions.find(sym.first) != expansions.end() ){
			desc.requires_expansion = true;
		}
		// look up local symbols
		auto loc = locals.find(sym.first);
		if ( loc != locals.end() ){
			desc.loc_symbols.insert(sym.first);
			continue;
		}
		// look up external symbols
		auto ext = externals.find(sym.first);
		if ( ext != externals.end() ){
			desc.ext_symbols.insert(ext->second.sym);
			continue;
		}
		if (parser_symbols.count(sym.first)) {
			desc.loc_symbols.insert(sym.first);
			continue;
		}

		throw string("EvaluatorCache::getDescriptor:  Symbol ") + sym.first + " not registered.";
	}
	initialized = true;   // No more registration of local symbols are allowed to keep the cache positions.
	return desc;
}

void EvaluatorCache::attach(mu::Parser *parser) {
	auto symbols = parser->GetUsedVar();
	for (const auto & sym : symbols) {
		// look up local symbols
		auto loc = locals.find(sym.first);
		if ( loc != locals.end() ){
			parser->DefineVar(sym.first, &locals_cache[loc->second]);
			continue;
		}
		// look up external symbols
		auto it_ext = externals.find(sym.first);
		if ( it_ext != externals.end() ){
			auto& ext = it_ext->second;
			// Inject constant values as values
			if (ext.type == SymbolDesc::DOUBLE) {
				if (ext.sym->flags().space_const && ext.sym->flags().time_const && ext.type) {
					auto accessor = static_pointer_cast<const SymbolAccessorBase<double> >(ext.sym);
					parser->DefineConst(sym.first, accessor->safe_get(SymbolFocus::global));
					continue;
				}
				parser->DefineVar(sym.first, &ext.valD);
				continue;
			}
			// Look up scalar expansion of vector symbols
			else if ( scalar_expansion && ext.type == SymbolDesc::VECTOR) {
				assert(expansions.count(sym.first));
				parser->DefineVar(sym.first, &expansions[sym.first].val);
				continue;
			}
		}
		
		if (parser_symbols.count(sym.first))
			continue;

		throw string("EvaluatorCache::attach:  Symbol ") + sym.first + " not registered.";
	}
	initialized = true;   // No more registration of local symbols are allowed to keep the cache positions.
}

/// Fill the cache with data wrt. @p focus
void EvaluatorCache::fetch(const SymbolFocus& focus, const bool safe) {
// 	if (current_focus == focus && current_time = SIM::getTime()) return;
	for (auto& sym : externals) {
		if (sym.second.source == SymbolDesc::Ext) {
			if (sym.second.type == SymbolDesc::DOUBLE) {
				auto accessor = static_pointer_cast<const SymbolAccessorBase<double> >(sym.second.sym);
				sym.second.valD = safe ? accessor->safe_get(focus) : accessor->get(focus);
			}
			else if (sym.second.type == SymbolDesc::VECTOR) {
				auto accessor = static_pointer_cast<const SymbolAccessorBase<VDOUBLE> >(sym.second.sym);
				sym.second.valV = safe ? accessor->safe_get(focus) : accessor->get(focus);
			}
		}
	}
// 	current_focus = focus;
}

/// A list of used external symbols.
std::set<Symbol> EvaluatorCache::getExternalSymbols() {
	std::set<Symbol> symbols;
	for ( const auto& sym : externals) {
		symbols.insert(symbols.end(), sym.second.sym);
	}
	return symbols;
}


string EvaluatorCache::toString() {
	stringstream s;
	s << "Locals: ";
	for ( const auto& sym : locals) {
		s << sym.first << "=" << locals_cache[sym.second] << "; ";
	}
	for ( const auto& sym : v_locals) {
		s << sym.first << "=" << VDOUBLE(locals_cache[sym.second], locals_cache[sym.second+1], locals_cache[sym.second+2])<< "; ";
	}
	s << "Externals: ";
	for ( const auto& sym : externals) {
		s << sym.first << "=";
		if (sym.second.type == SymbolDesc::DOUBLE)
			s << sym.second.valD;
		else
			s << sym.second.valV; 
		s << "; ";
	}
	return s.str();
}
