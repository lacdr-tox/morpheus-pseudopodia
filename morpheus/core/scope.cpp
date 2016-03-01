#include "scope.h"
#include "interfaces.h"
#include "celltype.h"

Scope::Scope() : parent(nullptr) , ct_component(nullptr), name("root") { 
	scope_id = max_scope_id; 
	max_scope_id++; 

	// using a colorset according to color theory, generated with http://paletton.com
	graphstyle["type_double"] 	= "fillcolor=\"#d3d247\""; // green-ish
	graphstyle["type_vdouble"]	= "fillcolor=\"#b5b426\"";
	graphstyle["type_other"] 	= "fillcolor=\"#8f8eod\"";
	graphstyle["node"] 			= "style=filled,fillcolor=\"#fffea3\"";
	graphstyle["background"] 	= "bgcolor=\"#2341782f\"";  // blue-ish (with alpha value 47 (0-255))
	graphstyle["arrow_connect"]	= "dir=none, style=\"dashed\", penwidth=1, color=\"#38568c\"";
	graphstyle["arrow_read"] 	= "penwidth=2, color=\"#112c5f\"";
	graphstyle["arrow_write"] 	= "penwidth=3, color=\"#8f100d\"";
}
Scope::Scope(Scope* parent, string name, map<string, string> graphstyle, CellType * celltype) : parent(parent), ct_component(celltype), name(name), graphstyle(graphstyle) { 
	scope_id = max_scope_id; 
	max_scope_id++;
};


Scope* Scope::createSubScope(string name, CellType* ct)
{
	cout << "Creating subscope " << name << " in scope " << this->name << endl;
// 	Scope* scope = new Scope(this,name,ct);
	auto scope = shared_ptr<Scope>( new Scope(this,name,graphstyle,ct) );
	sub_scopes.push_back(scope);
	if(ct) {
		component_scopes.push_back(scope);
	}
	return scope.get();
}

CellType* Scope::getCellType() const {
	if (!ct_component) {
		if (!parent) {
			return nullptr;
			cout << "Warning: Reading an empty celltype from Scope;" << endl;
		}
		else
			return parent->getCellType();
	}
	return ct_component;
}


void Scope::registerSymbol(SymbolData data)
{
    cout << "Registering Symbol " << data.name << " with linktype " << SymbolData::getLinkTypeName(data.link) << " of type " << data.type_name << endl;
	assert(! data.name.empty() );
	assert(! data.type_name.empty() );
	assert( data.link != SymbolData::UnLinked );
	
	bool redefinition = false;
	auto it = local_symbols.find(data.name);
	if (it != local_symbols.end()) {
		if (it->second.link == SymbolData::CompositeSymbolLink) {
			if (data.type_name == it->second.type_name)
				data.component_subscopes = it->second.component_subscopes;
		}
		else {
			cerr << "Redefinition of a symbol "/*with different type [" << symbol.name << "]*/ <<endl;
			cerr << " type " << data.getLinkTypeName() << " ?= " << local_symbols[data.name].getLinkTypeName() << endl;
			cerr << " name " << data.name << " ?= " << local_symbols[data.name].name << endl;
			cerr << " fullname " << data.fullname << " ?= " << local_symbols[data.name].fullname << endl;
			exit(-1);
		}
	}

	// Assert correct meta-data 
	if (data.base_name.empty()) data.base_name = data.name;
	if (data.fullname.empty()) data.fullname = data.name;
	
	local_symbols[data.name] = data;
	
	if ( ct_component ) {
		assert(parent);
		if (data.name == data.base_name) {
			// is a real symbol, not derived like 'vec.x' 
			parent->registerSubScopeSymbol(this, data.name);
		}
	}
	
	// creating read-only derived symbol definitions for vector properties
	if (data.type_name == TypeInfo<VDOUBLE>::name()) {
        // register subelement accessors for sym.x ,sym .y , sym.z 
		data.type_name = TypeInfo< double >::name();
		data.writable = false;
		
		string base_name = data.name;

		data.link = SymbolData::VecXLink;
		data.name = base_name + ".x";
		this->registerSymbol(data);
		
		data.link = SymbolData::VecYLink;
		data.name = base_name + ".y";
		this->registerSymbol(data);
		
		data.link = SymbolData::VecZLink;
		data.name = base_name + ".z";
		this->registerSymbol(data);
		
		data.link = SymbolData::VecAbsLink;
		data.name = base_name + ".abs";
		this->registerSymbol(data);
		
		data.link = SymbolData::VecPhiLink;
		data.name = base_name + ".phi";
		this->registerSymbol(data);
		
		data.link = SymbolData::VecThetaLink;
		data.name = base_name + ".theta";
		this->registerSymbol(data);
	}
}

void Scope::init()
{
	int n_subscopes = this->component_scopes.size();
	if (n_subscopes>0) {
		for (auto& sym :local_symbols) {
			if ( ! sym.second.component_subscopes.empty()) 
				sym.second.component_subscopes.resize(n_subscopes,NULL);
		}
	}
	/*
	for (auto scope : component_scopes) {
		scope->init();
	}
	*/

}


// Currently, this implementation is only available for CellType scopes, that may override the global scope symbol within the lattice part that they occupy
void Scope::registerSubScopeSymbol(Scope *sub_scope, string symbol_name) {
	if (sub_scope->ct_component == NULL) {
		throw (string("Scope:: Invalid registration of subscope symbol ") + symbol_name +(". Subscope is no spatial component."));
	}
	if (parent != NULL ) {
		throw (string("Scope:: Invalid registration of subscope symbol ") + symbol_name +(" in non-root scope [")+ name +"].");
	}
	
	int sub_scope_id = sub_scope->ct_component->getID();
	
	auto it = local_symbols.find(symbol_name);
	if (it != local_symbols.end()) {
		SymbolData sym = sub_scope->local_symbols[symbol_name];
		if (it->second.type_name != sym.type_name) {
			cerr << "Scope::registerSubScopeSymbol : Cannot register type incoherent sub-scope symbol \"" << symbol_name << "\"!" << endl; 
		}
		else {
			if (it->second.component_subscopes.size()<=sub_scope_id)
				it->second.component_subscopes.resize(sub_scope_id+1, NULL);
			it->second.component_subscopes[sub_scope->ct_component->getID()]= sub_scope;
			it->second.is_composite = true;
		}
	}
	else {
		SymbolData v_sym = sub_scope->local_symbols[symbol_name];
		
		v_sym.link = SymbolData::CompositeSymbolLink;
		v_sym.invariant = false;
		v_sym.time_invariant = false;
		v_sym.writable = false;
		v_sym.is_composite = true;
		v_sym.component_subscopes.resize(sub_scope_id+1, NULL);
		v_sym.component_subscopes[sub_scope->ct_component->getID()] = sub_scope;
		local_symbols[symbol_name] = v_sym;
		cout << "!!! SubScope Symbol \"" << symbol_name << "\" registered!" << endl;
		
			// creating read-only derived symbol definitions for vector properties
		if (v_sym.type_name == TypeInfo<VDOUBLE>::name()) {
			// register subelement accessors for sym.x ,sym .y , sym.z 
			v_sym.type_name = TypeInfo< double >::name();
			v_sym.writable = false;
			
			string base_name = v_sym.name;

			v_sym.link = SymbolData::VecXLink;
			v_sym.name = base_name + ".x";
			this->registerSymbol(v_sym);
			
			v_sym.link = SymbolData::VecYLink;
			v_sym.name = base_name + ".y";
			this->registerSymbol(v_sym);
			
			v_sym.link = SymbolData::VecZLink;
			v_sym.name = base_name + ".z";
			this->registerSymbol(v_sym);
			
			v_sym.link = SymbolData::VecAbsLink;
			v_sym.name = base_name + ".abs";
			this->registerSymbol(v_sym);
			
			v_sym.link = SymbolData::VecPhiLink;
			v_sym.name = base_name + ".phi";
			this->registerSymbol(v_sym);
			
			v_sym.link = SymbolData::VecThetaLink;
			v_sym.name = base_name + ".theta";
			this->registerSymbol(v_sym);
		}
	}
}

string Scope::getSymbolType(string name) const {
	auto it = local_symbols.find(name);
	if (it!=local_symbols.end()) {
		return it->second.type_name;
	}
	else if (parent) {
		return parent->getSymbolType(name);
	}
	else {
		throw string("Unknown symbol '") + name + string("' in getSymbolType.");
	}
}



string Scope::getSymbolBaseName(string name) const
{
	auto sym = local_symbols.find(name);
	if (sym != local_symbols.end()) {
		return sym->second.base_name;
	}
	else {
		throw string("Unknown symbol '") + name + string("' in getSymbolBaseName.");
	}
}

void Scope::registerTimeStepListener(TimeStepListener* tsl)
{
	local_tsl.insert(tsl);
}


void Scope::registerSymbolReader(TimeStepListener* tsl, string symbol)
{
	symbol_readers.insert(pair<string, TimeStepListener*>(symbol, tsl));
}


void Scope::registerSymbolWriter(TimeStepListener* tsl, string symbol)
{
	symbol_writers.insert(pair<string,TimeStepListener*>(symbol,tsl));
}


void Scope::propagateSinkTimeStep(string symbol, double time_step)
{
	auto range = symbol_writers.equal_range(symbol);
	for (auto it = range.first; it != range.second; it++) {
		cout << "   propagate " << it->second->XMLName() << endl;
		it->second->updateSinkTS(time_step);
	}
	if ( !component_scopes.empty()) {
		for (auto & sub_scope : component_scopes) {
			sub_scope->propagateSinkTimeStep(symbol,time_step);
		}
	}
}

void Scope::propagateSourceTimeStep(string symbol, double time_step)
{
	auto range = symbol_readers.equal_range(symbol);
	for (auto it = range.first; it != range.second; it++) {
		cout << "   propagate " << it->second->XMLName() << endl;
		it->second->updateSourceTS(time_step);
	}
	if (ct_component)
		parent->propagateSourceTimeStep(symbol,time_step);
}

void Scope::addUnresolvedSymbol(string symbol)
{
	unresolved_symbols.insert(symbol); 
	if (ct_component)
		parent->addUnresolvedSymbol(symbol);
}


void Scope::removeUnresolvedSymbol(string symbol)
{
	auto it = unresolved_symbols.find(symbol); 
	if (it == unresolved_symbols.end()) {
		cout << "Trying to remove unregistered symbol \"" << symbol << "\" from unresolved_symbols.";
		return;
	}
	unresolved_symbols.erase(it);
	if (ct_component) 
		parent->removeUnresolvedSymbol(symbol);
}

void Scope::write_graph(ostream& out) const
{
	out << "digraph {\n";
    out << "subgraph cluster{\n";
    out << "labelloc=\"t\"";
    out << "label=\"Global\"";
    out << ""<< graphstyle.at("background") << "\n";
	out << "node["<< graphstyle.at("node") <<"]\n";
	this->write_graph_local_variables(out);
    out << "}" << endl;
    out << "}" << endl;

}

int Scope::max_scope_id = 0;

string clean(const string& a ) {
	string b(a);
	
	for (auto& ch : b) {
		if (ch=='.') ch = '_';
	}
	return b;
}

set<string> virtual_cell_propery_decl {"cell.type", "cell.center", "cell.id", "cell.volume", "cell.length", "cell.surface"};
struct VCP_desc {string name; string type; };
vector<VCP_desc> virtual_cell_properies;

string Scope::tslDotName(TimeStepListener* tsl) const
{
	string ts;
	if (tsl->timeStep()>0)
		ts =  to_str(tsl->timeStep(),4);
	else 
		ts = "0";
	
	std::replace( ts.begin(), ts.end(), '.', '_');
	
	return tsl->XMLName() + "_" + to_str(tsl->scope()->scope_id) + "_" + ts;
}

string Scope::dotStyleForType(const string& type) const {
	if (type == TypeInfo<double>::name())
		return graphstyle.at("type_double");
	else if (type == TypeInfo<VDOUBLE>::name())
		return graphstyle.at("type_vdouble");
	else 
		return graphstyle.at("type_other");
}

void Scope::write_graph_local_variables(ostream& out) const
{
	// Write local symbols, if they are used
	for (auto sym : local_symbols) {
		if (symbol_readers.count(sym.first)==0 && symbol_writers.count(sym.first)==0  ) {
			if (ct_component) {
				if (parent->symbol_readers.count(sym.first) == 0)
					continue;
			}
			else
				continue;
		}
		
		bool virtual_composite  = !parent && virtual_cell_propery_decl.count(sym.first);
		
		if (virtual_composite) {
			out <<  clean(sym.first) << "_" << scope_id << "[peripheries=2,label=" << "\""<< sym.first<<"\",";
			out << dotStyleForType(sym.second.type_name);
			out << "];\n";
			virtual_cell_properies.push_back({sym.first, sym.second.type_name});
		}
		else if (sym.second.is_composite) {
			out <<  clean(sym.first) << "_" << scope_id << "[peripheries=2,label=";
			if (sym.second.link == SymbolData::CompositeSymbolLink) {
				out << "\""<< sym.first<<"\"";
			}
			else {
				out << "\""<< sym.first<<"\",";
			}
			out << dotStyleForType(sym.second.type_name);
			out << "];\n";
		}
		else if (sym.second.link == SymbolData::VecAbsLink || sym.second.link == SymbolData::SymbolData::VecPhiLink ||
			sym.second.link == SymbolData::VecThetaLink || sym.second.link == SymbolData::SymbolData::VecXLink ||
			sym.second.link == SymbolData::VecYLink || sym.second.link == SymbolData::SymbolData::VecZLink) {
		}
		else {
			out <<  clean(sym.first) << "_" << scope_id << "[label=\""<< sym.first<<"\",";
			out << dotStyleForType(sym.second.type_name);
			out << "];\n";
		}
	}
	
	// Write virtual symbols for celltype scopes
	if (ct_component) {
		if (!ct_component->isMedium())
			for (const auto& sym : virtual_cell_properies) {
				out <<  clean(sym.name) << "_" << scope_id << "[label=\"" << sym.name <<  "\"," << dotStyleForType(sym.type) <<"];\n";
			}
	}
	
	// Write time step listeners
	for (auto tsl : local_tsl) {
		if (tsl->XMLName() != "DependencyGraph")
			out << tslDotName(tsl) << " [shape=record, label=\"{ "<< tsl->XMLName() << (tsl->getFullName().empty() ? string("") : string("\\n\\\"") + tsl->getFullName() + "\\\"")<< " | " <<  tsl->timeStep() <<" } \" ]\n" ;
	}
	
	// Now write all the declarations of the subscopes
	for (auto sub_scope : sub_scopes) {
		out << "subgraph cluster_" << sub_scope->scope_id <<" {\n";
		out << "label=\"" << sub_scope->getName() << "\";\n";
		sub_scope->write_graph_local_variables(out);
		out << "}\n";
	}
	
	// Write links for composite symbols to the subscope symbols
	for (auto sym : local_symbols) {
		if (sym.second.is_composite) {
			if (symbol_readers.count(sym.first)==0) 
				continue;
			
			for (uint i = 0; i< sym.second.component_subscopes.size(); i++ ) {
				if (sym.second.component_subscopes[i])
					out <<  clean(sym.first) << "_" << sym.second.component_subscopes[i]->scope_id << " -> " << clean(sym.first) << "_" << this->scope_id << " ["<< graphstyle.at("arrow_connect") <<"]\n";
			}
		}
	}
	// Write composite links for virtual celltype symbols
	if (!parent) {
		for (auto sym : virtual_cell_properies) {
			for (uint i = 0; i< sub_scopes.size(); i++ ) {
				if ( sub_scopes[i]->getCellType() && !sub_scopes[i]->getCellType()->isMedium()) {
					out <<  clean(sym.name) << "_" << sub_scopes[i]->scope_id << " -> " << clean(sym.name) << "_" << this->scope_id << " ["<< graphstyle.at("arrow_connect") <<"]\n";
				}
			}
		}
	}
	
	// Write dependencies of TSLs
	for (auto reader : symbol_readers) {
		out << clean(reader.first) << "_" << this->scope_id << " -> " << tslDotName(reader.second) << " [" << graphstyle.at("arrow_read") << "] \n";
	}
	
	// Write outputs of TSLs
	for (auto writer : symbol_writers) {
		out << tslDotName(writer.second) << " -> " << clean(writer.first) << "_" << this->scope_id << " [" << graphstyle.at("arrow_write") << "] \n";
	}
	
	
}


