#include "evaluator_cache.h"

mu::value_type* EvaluatorCache::registerSymbol(const mu::char_type* symbol, void* ptr) {
	auto that = reinterpret_cast<EvaluatorCache*>(ptr);
	return that->registerSymbol_internal(symbol);
}

mu::value_type* EvaluatorCache::registerSymbol_internal(const mu::char_type* symbol) {
	initialized = true; 
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
	if (externals.count(symbol)) {
		return &externals[symbol].val;
	}
	if (local_scope->getAllSymbolNames<double>(allow_partial_spec).count(symbol)) {
		SymbolDesc sd;
		sd.sym = local_scope->findSymbol<double>(symbol, allow_partial_spec);
		sd.val= 0;
		auto ext_it = externals.insert({ symbol, sd }).first;
		return &ext_it->second.val;
	}
	if (scalar_expansion_permitted && local_scope->getAllSymbolNames<VDOUBLE>(allow_partial_spec).count(symbol)) {
		/// create the scalar expansion wrapper
		auto ext_it = v_externals.find(symbol);
		// not registered yet
		if (ext_it == v_externals.end()) {
			VSymbolDesc ext;
			ext.sym = local_scope->findSymbol<VDOUBLE>(symbol, allow_partial_spec);
			ext.val = VDOUBLE(0,0,0);
			ext_it = v_externals.insert({symbol,ext}).first;
		}
		ExpansionDesc exp;
		exp.source = ExpansionDesc::EXTERNAL;
		exp.vexternal_source_it = ext_it;
		exp.val = 0;
		auto exp_it = expansions.insert( {symbol, exp}).first;
		scalar_expansion = true;
		return &exp_it->second.val;
	}
	else {
		mu::Parser::exception_type e(mu::ecINVALID_NAME,symbol);
		cout << "EvaluatorCache::registerSymbol(): Unknown symbol " << symbol << endl;
		cout << "Available is ";
		set<string > all_symbols = local_scope->getAllSymbolNames<double>(allow_partial_spec);
		copy(all_symbols.begin(), all_symbols.end(),ostream_iterator<string>(cout,","));
		for (const auto& sym : locals) cout << ", " << sym.first;
		if (scalar_expansion_permitted) {
			all_symbols = local_scope->getAllSymbolNames<VDOUBLE>(allow_partial_spec);
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
	uint pos = locals_cache.size();
	locals_cache.push_back(value);
	locals[name] = pos;
	locals_table.push_back( { name, EvaluatorSymbol::DOUBLE } );
	return pos;
}

/// Add a local symbol @p name to the cache and return it's cache position
int EvaluatorCache::addLocal(string name, VDOUBLE value) {
	if (initialized) throw string("Cannot add locals after initilization in EvaluatorCache!");
	if (locals.count(name)) throw string("Cannot add local in EvaluatorCache! Symbol ") + name + " already exists";
	uint pos = locals_cache.size();
	locals_cache.push_back(value.x);
	locals_cache.push_back(value.y);
	locals_cache.push_back(value.z);
	v_locals[name] = pos;
	locals_table.push_back( { name, EvaluatorSymbol::VECTOR } );
	locals[name+".x"] = pos;
	locals[name+".y"] = pos+1;
	locals[name+".z"] = pos+1;
	return pos;
}

void EvaluatorCache::setLocalsTable(const vector<EvaluatorCache::EvaluatorSymbol>& layout)
{

	if ( ! locals_table.empty() )
		throw string("EvaluatorCache::setLocalsTable()  Refuse to override locals. Locals already defined.");
	for (const auto& loc : layout) {
		if (loc.type == EvaluatorSymbol::VECTOR) {
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
		return externals[name].val;
	if (expansions.count(name))
		return expansions[name].val;
	return 0;
}


EvaluatorCache::ParserDesc EvaluatorCache::getDescriptor(mu::Parser *parser) {
	auto symbols = parser->GetUsedVar();
	ParserDesc desc;
	desc.requires_expansion = false;
	for (const auto & sym : symbols) {
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
		// look up scalar expansion of vector symbols
		if (scalar_expansion) {
			auto exp = expansions.find(sym.first);
			if ( exp != expansions.end() ){
				desc.requires_expansion = true;
				if (exp->second.source == ExpansionDesc::EXTERNAL)
					desc.ext_symbols.insert(v_externals[exp->first].sym);
				continue;
			}
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
		auto ext = externals.find(sym.first);
		if ( ext != externals.end() ){
			if (ext->second.sym->flags().space_const && ext->second.sym->flags().time_const ) {
				parser->DefineConst(sym.first, ext->second.sym->safe_get(SymbolFocus::global));
			}
			else {
				parser->DefineVar(sym.first, &ext->second.val);
			}

			continue;
		}
		// look up scalar expansion of vector symbols
		if (scalar_expansion) {
			auto exp = expansions.find(sym.first);
			if ( exp != expansions.end() ){
				parser->DefineVar(sym.first, &exp->second.val);
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
	if (safe) {
		for (auto& sym : externals) {
			sym.second.val = sym.second.sym->safe_get(focus);
		}
		for (auto& sym : v_externals) {
			sym.second.val = sym.second.sym->safe_get(focus);
		}
	}
	else {
		for (auto& sym : externals) {
			sym.second.val = sym.second.sym->get(focus);
		}
		for (auto& sym : v_externals) {
			sym.second.val = sym.second.sym->get(focus);
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
	for ( const auto& sym : v_externals) {
		symbols.insert(sym.second.sym);
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
		s << sym.first << "=" << sym.second.val << "; ";
	}
	for ( const auto& sym : v_externals) {
		s << sym.first << "=" << sym.second.val << "; ";
	}
	return s.str();
}
