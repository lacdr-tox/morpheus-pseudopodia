#include "scope.h"
#include "interfaces.h"
#include "celltype.h"
#include "cpm.h"


Scope::Scope() : parent(nullptr) , name("root"), ct_component(nullptr) { 
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


void Scope::registerSymbol(Symbol const_symbol)
{
	auto symbol = const_pointer_cast<SymbolBase>(const_symbol);
    cout << "Registering Symbol " << symbol->name() << " of linktype " << symbol->linkType() << " in Scope " << this->getName() << endl;
	
	auto it = symbols.find(symbol->name());
	if (it != symbols.end()) {
		if (symbol->type() != it->second->type()) {
			stringstream s;
			s << "Redefinition of a symbol \"" << symbol->name() << "\" with different type." << endl;
			s << " type " << symbol->type() << " != " << it->second->type() << endl;
			throw SymbolError(SymbolError::Type::InvalidDefinition, s.str());
		}
		auto comp = dynamic_pointer_cast<CompositeSymbol_I>(it->second);
		if (! comp) {
			throw SymbolError(SymbolError::Type::InvalidDefinition, string("Redefinition of a symbol \"") + symbol->name() + "\" in scope \""  + this->name + "\"");
		}
		symbol->setScope(this);
		comp->setDefaultValue(symbol);
		return;
	}

	symbol->setScope(this);
	symbols[symbol->name()] = symbol;
	
	if ( ct_component ) {
		assert(parent);
		if (! dynamic_pointer_cast<VectorComponentAccessor>(symbol) ) {
// 			// is a real symbol, not derived like 'vec.x' 
			parent->registerSubScopeSymbol(this, symbol);
		}
	}
	
	// creating read-only derived symbol definitions for vector properties, but not for subscope definitions
	if (dynamic_pointer_cast<SymbolAccessorBase<VDOUBLE> >( symbol)) {
        // register subelement accessors for sym.x ,sym .y , sym.z 
		auto v_sym = dynamic_pointer_cast<SymbolAccessorBase<VDOUBLE> >( symbol);
		auto derived = make_shared<VectorComponentAccessor>(v_sym,VectorComponentAccessor::Component::X);
		registerSymbol(derived);
		derived = make_shared<VectorComponentAccessor>(v_sym,VectorComponentAccessor::Component::Y);
		registerSymbol(derived);
		derived = make_shared<VectorComponentAccessor>(v_sym,VectorComponentAccessor::Component::Z);
		registerSymbol(derived);
		derived = make_shared<VectorComponentAccessor>(v_sym,VectorComponentAccessor::Component::PHI);
		registerSymbol(derived);
		derived = make_shared<VectorComponentAccessor>(v_sym,VectorComponentAccessor::Component::THETA);
		registerSymbol(derived);
		derived = make_shared<VectorComponentAccessor>(v_sym,VectorComponentAccessor::Component::R);
		registerSymbol(derived);
	}
}

void Scope::init()
{
	for (auto symbol : composite_symbols) {
		symbol.second->init(component_scopes.size());
	}
}

// Currently, this implementation is only available for CellType scopes, that may override the global scope symbol within the lattice part that they occupy
void Scope::registerSubScopeSymbol(Scope *sub_scope, Symbol const_symbol) {
	auto symbol = const_pointer_cast<SymbolBase>(const_symbol);
	if (sub_scope->ct_component == NULL) {
		throw SymbolError(SymbolError::Type::InvalidDefinition, string("Scope:: Invalid registration of subscope symbol ") + symbol->name() +(". Subscope is no spatial component."));
	}
	if (parent != NULL ) {
		throw  SymbolError(SymbolError::Type::InvalidDefinition, string("Scope:: Invalid registration of subscope symbol ") + symbol->name() +(" in non-root scope [")+ name +"].");
	}
	
	int sub_scope_id = sub_scope->ct_component->getID();
	
	auto it = symbols.find(symbol->name());
	if (it != symbols.end()) {
		if (it->second->type() != symbol->type()) {
			throw SymbolError(SymbolError::Type::InvalidDefinition,string("Scope::registerSubScopeSymbol : Cannot register type incoherent sub-scope symbol \"")  + symbol->name() + "\"!"); 
		}
		else {
			shared_ptr<CompositeSymbol_I> composite_sym_i;
			if (! dynamic_pointer_cast<CompositeSymbol_I>(it->second))
				if (dynamic_pointer_cast<SymbolAccessorBase<double> >(symbol)) {
					auto composite_sym = make_shared<CompositeSymbol<double> >(symbol->name(), dynamic_pointer_cast< SymbolAccessorBase<double> >(it->second));
					composite_sym_i = composite_sym;
					composite_symbols[symbol->name()] = composite_sym;
					symbols.erase(it);
					registerSymbol(composite_sym);
				}
				else if (dynamic_pointer_cast<SymbolAccessorBase<VDOUBLE> >(symbol)){
					auto composite_sym = make_shared<CompositeSymbol<VDOUBLE> >(symbol->name(), dynamic_pointer_cast<SymbolAccessorBase<VDOUBLE> >(it->second) );
					composite_sym_i = composite_sym;
					composite_symbols[symbol->name()] = composite_sym;
					symbols.erase(it);
					// Also manually erase derived symbols !!!
					symbols.erase(it->first+".x");
					symbols.erase(it->first+".y");
					symbols.erase(it->first+".z");
					symbols.erase(it->first+".phi");
					symbols.erase(it->first+".theta");
					symbols.erase(it->first+".abs");
					registerSymbol(composite_sym);
				}
				else {
					throw string("Composity symbol type not implemented in Scope ") + symbol->type();
				}
			else {
				composite_sym_i = dynamic_pointer_cast<CompositeSymbol_I>(it->second);
			}
			
			composite_sym_i->addCellTypeAccessor(sub_scope_id, symbol);
		}
	}
	else {
		shared_ptr<CompositeSymbol_I> composite_sym_i;
		shared_ptr<SymbolBase> composite_sym_base;
		if (symbol->type() == TypeInfo<double>::name()) {
			auto composite_sym = make_shared<CompositeSymbol<double> >(symbol->name());
			composite_sym_i = composite_sym;
			composite_sym_base = composite_sym;
		}
		else if (symbol->type() == TypeInfo<VDOUBLE>::name()){
			auto composite_sym = make_shared<CompositeSymbol<VDOUBLE> >(symbol->name());
			composite_sym_i = composite_sym;
			composite_sym_base = composite_sym;
		}
		else {
			throw string("Composite symbol type not implemented in Scope ") + symbol->type();
		}
		composite_sym_base->setScope(this);
		composite_sym_i->addCellTypeAccessor(sub_scope_id, symbol);
		composite_symbols[composite_sym_i->name()] = composite_sym_i;
		registerSymbol(composite_sym_base);
	}
}

Symbol Scope::findSymbol(string name) const {
	auto it = symbols.find(name);
	if (it!=symbols.end()) {
		return it->second;
	}
	else if (parent) {
		return parent->findSymbol(name);
	}
	else {
		throw string("Unknown symbol '") + name + string("' in findSymbol.");
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
		cout << " Propagate up to " << it->second->XMLName() << endl;
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
		cout << " Propagate down to " << it->second->XMLName() << endl;
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

void Scope::write_graph(ostream& out, const DepGraphConf& config) const
{
	out << "digraph {\n";
	out << "compound=true;\n";
	
	out << "subgraph cluster{\n";
	out << "labelloc=\"t\";";
	out << "label=\"Global\";";
	out << ""<< graphstyle.at("background") << "\n";
	out << "node["<< graphstyle.at("node") <<"]\n";
	stringstream links;
	this->write_graph_local_variables(out, links, config);
	out << "}" << endl;
	vector<string> link_lines = tokenize(links.str(),"\n");
	set<string> filter;
	for (const auto& line : link_lines )  {
		if ( filter.count(line) == 0 ) {
			filter.insert(line);
			out << line << "\n";
		}
	}
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

string Scope::pluginDotName(Plugin* p) const {
	if (dynamic_cast<TimeStepListener*>(p)) {
		return tslDotName(dynamic_cast<TimeStepListener*>(p));
	}
	else {
		return p->XMLName() + "_" + to_str(p->scope()->scope_id);
	}
}

string Scope::tslDotName(TimeStepListener* tsl) const
{
	string ts;
	if (tsl->timeStep()>0)
		ts =  to_str(tsl->timeStep(),4);
	else 
		ts = "0";
	
	std::replace( ts.begin(), ts.end(), '.', '_');
	std::replace( ts.begin(), ts.end(), '-', '_');
	
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

void Scope::write_graph_local_variables(ostream& definitions, ostream& links, const DepGraphConf& config ) const
{
	filtered_symbol_readers.clear();
	for (const auto& reader : symbol_readers) {
		if (config.exclude_symbols.count(reader.first))
			continue;
		if (config.exclude_plugins.count(reader.second->XMLName()))
			continue;
		filtered_symbol_readers.insert(reader);
	}
	
	filtered_symbol_writers.clear();
	for (const auto& writer : symbol_writers) {
		if (config.exclude_symbols.count(writer.first))
			continue;
		if (config.exclude_plugins.count(writer.second->XMLName()))
			continue;
		filtered_symbol_writers.insert(writer);
	}
	
	// Write time step listeners
	for (auto tsl : local_tsl) {
		if ( config.exclude_plugins.count( tsl->XMLName() ) )
			continue;
		
		if (tsl->XMLName() == "CPM") {
			string cpm_name = tslDotName(tsl);
			set<SymbolDependency> inter_dep = dynamic_cast<CPMSampler*>(tsl)->getInteractionDependencies();
			
			definitions << cpm_name << " [shape=record, label=\"{ "<< tsl->XMLName() << (tsl->getFullName().empty() ? string("") : string("\\n\\\"") + tsl->getFullName() + "\\\"")<< " | " <<  tsl->timeStep() <<" } \" ]\n" ;
			
			for (auto ct_scope : component_scopes) {
				auto dep = ct_scope->ct_component->cpmDependSymbols();
				if ( ! dep.empty() ) {
					auto it = dep.begin();
					while (config.exclude_plugins.count(it->first->XMLName()) && it != dep.end())
						it++;

					if (it != dep.end()) {
						string first_plugin = pluginDotName( it->first );
						links << cpm_name << " -> " << first_plugin << " [" << graphstyle.at("arrow_connect") << ",lhead=" << string("cluster_cpm") << ct_scope->scope_id << "] \n";
					}
				}
			}
			
			for (auto dep : inter_dep) {
				if ( config.exclude_symbols.count(dep.name) )
					continue;
				links << clean(dep.name) << "_" << dep.scope->scope_id <<" -> " << cpm_name  << " ["<< graphstyle.at("arrow_read") <<"]\n";
			}
		}
		else if (tsl->XMLName() == "Function" || tsl->XMLName() == "VectorFunction") { }
		else {
			definitions << tslDotName(tsl) << " [shape=record, label=\"{ "<< tsl->XMLName() << (tsl->getFullName().empty() ? string("") : string("\\n\\\"") + tsl->getFullName() + "\\\"")<< " | " <<  tsl->timeStep() <<" } \" ]\n" ;
		}
	}
	
	// Write local symbols, if they are used
	for (auto sym : symbols) {
		// Skip excluded symbols 
		if (config.exclude_symbols.count(sym.first))
			continue;
		
		// Skip unused local symbols
		if (filtered_symbol_readers.count(sym.first)==0 && filtered_symbol_writers.count(sym.first)==0  ) {
			if (ct_component) {
				
				if (parent->filtered_symbol_readers.count(sym.first) == 0)
					continue;
			}
			else
				continue;
		}
		
		bool virtual_composite  = false; // parent && virtual_cell_propery_decl.count(sym.first);
		
		if (virtual_composite) {
			definitions <<  clean(sym.first) << "_" << scope_id << "[peripheries=2,label=" << "\""<< sym.first<<"\",";
			definitions << dotStyleForType(sym.second->type());
			definitions << "];\n";
			virtual_cell_properies.push_back({sym.first, sym.second->type()});

			// Write composite links for virtual celltype symbols
			for (uint i = 0; i< sub_scopes.size(); i++ ) {
				if ( sub_scopes[i]->getCellType() && !sub_scopes[i]->getCellType()->isMedium()) {
					links <<  clean(sym.first) << "_" << sub_scopes[i]->scope_id << " -> " << clean(sym.first) << "_" << this->scope_id << " ["<< graphstyle.at("arrow_connect") <<"]\n";
				}
			}
		}
		else if (sym.second->linkType() == "CompositeLink") {
			definitions <<  clean(sym.first) << "_" << scope_id << "[peripheries=2,label=";
			definitions << "\""<< sym.first<<"\"";
			
			definitions << dotStyleForType(sym.second->type());
			definitions << "];\n";
			//  Write links to the subscope symbols 
			auto dependencies = sym.second->dependencies();
			for (auto dep : dependencies ) {
				if (dep.scope->scope_id == this->scope_id && dep.name == sym.first) continue;
				links <<  clean(dep.name) << "_" << dep.scope->scope_id << " -> " << clean(sym.first) << "_" << this->scope_id << " ["<< graphstyle.at("arrow_connect") <<"]\n";
			}
		}
		else if (dynamic_pointer_cast<VectorComponentAccessor>(sym.second)) {
		}
		else {
			definitions <<  clean(sym.first) << "_" << scope_id << "[label=\""<< sym.first<<"\",";
			definitions << dotStyleForType(sym.second->type());
			definitions << "];\n";
		}
	}
	
	// Write all the declarations of the subscopes
	for (auto sub_scope : sub_scopes) {
		definitions << "subgraph cluster_" << sub_scope->scope_id <<" {\n";
		definitions << "label=\"" << sub_scope->getName() << "\";\n";
		sub_scope->write_graph_local_variables(definitions, links, config);
		definitions << "}\n";
	}
	
	// Write virtual symbols for celltype scopes
	if (ct_component) {
		if (!ct_component->isMedium())
			for (const auto& sym : virtual_cell_properies) {
				if (config.exclude_symbols.count(sym.name))
					continue;
				definitions <<  clean(sym.name) << "_" << scope_id << "[label=\"" << sym.name <<  "\"," << dotStyleForType(sym.type) <<"];\n";
			}
		
		auto cpm_dep = ct_component->cpmDependSymbols();
		bool found_valid_cpm_plugin = false;
		for (auto dep : cpm_dep) {
			if ( config.exclude_plugins.count( dep.first->XMLName() ) == 0) {
				found_valid_cpm_plugin = true;
				break;
			}
		}
		if ( found_valid_cpm_plugin ) {
			string cpm_blob_name = string("cluster_cpm") + to_str(scope_id);
			definitions << "subgraph " << cpm_blob_name << " {\n";
			definitions << "label=\"CPM plugins\";\n";
			string current_plugin = "";
			string plugin_node_name;
			string last_dep = "";
			for (auto dep : cpm_dep) {
				if ( config.exclude_plugins.count( dep.first->XMLName() ))
					continue;
				
				if (dep.first->XMLName() != current_plugin) {
					current_plugin = dep.first->XMLName();
					TimeStepListener* tsl = dynamic_cast<TimeStepListener*>(dep.first);
					if (tsl) {
						plugin_node_name = tslDotName(tsl);
						definitions << plugin_node_name << "[shape=record, label=\"{" << current_plugin  << "|" << tsl->timeStep() << "}\"];\n";
						
					}
					else {
						plugin_node_name = clean(current_plugin) + "_" + to_str(this->scope_id);
						definitions << plugin_node_name << "[shape=record, label=\"" << current_plugin  << "\"];\n";
					}
					last_dep = "";
					
				}
				string dependency = clean(dep.second.name) + "_" + to_str(dep.second.scope->scope_id);
				if (last_dep != dependency) {
					if (! config.exclude_symbols.count(dep.second.name))
						links << dependency << " -> " << plugin_node_name  << " ["<< graphstyle.at("arrow_read") <<"]\n";
				}
				last_dep = dependency;
			}
			definitions << "}\n";
		}
		
		
	}
	
	// Write dependencies of TSLs
	for (auto reader : filtered_symbol_readers) {
		if (reader.second->XMLName() == "CPM" )
			continue;
		else if (reader.second->XMLName() == "Function" || reader.second->XMLName() == "VectorFunction" ) {
			set<SymbolDependency> fun_out = reader.second->getOutputSymbols();
			links << clean(reader.first) << "_" << this->scope_id << " -> " << fun_out.begin()->name << "_" << fun_out.begin()-> scope -> scope_id << "\n";
		}
		else {
			links << clean(reader.first) << "_" << this->scope_id << " -> " << tslDotName(reader.second) << " [" << graphstyle.at("arrow_read") << "] \n";
		}
	}
	
	// Write outputs of TSLs
	for (auto writer : filtered_symbol_writers) {
		if (writer.second->XMLName() == "Function"  || writer.second->XMLName() == "VectorFunction" )
			continue;
		links << tslDotName(writer.second) << " -> " << clean(writer.first) << "_" << this->scope_id << " [" << graphstyle.at("arrow_write") << "] \n";
	}
	
}


