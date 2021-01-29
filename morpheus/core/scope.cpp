#include "scope.h"
#include "interfaces.h"
#include "celltype.h"
#include "cpm.h"

int Scope::max_scope_id = 0;

Scope::Scope() : parent(nullptr) , name("root"), ct_component(nullptr) { 
	scope_id = max_scope_id; 
	max_scope_id++; 
}

Scope::Scope(Scope* parent, string name, CellType * celltype) : parent(parent), ct_component(celltype), name(name) { 
	scope_id = max_scope_id; 
	max_scope_id++;
};

Scope::~Scope()  { 
// 	cout << "Deleting scope " << name << endl;
// 	for (auto& sym : symbols) {
// 		sym.second->setScope(nullptr);
// 	}
} 


Scope* Scope::createSubScope(string name, CellType* ct)
{
	cout << "Creating subscope " << name << " in scope " << this->name << endl;
// 	Scope* scope = new Scope(this,name,ct);
	auto scope = shared_ptr<Scope>( new Scope(this,name,ct) );
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
	// if symbol exists, it could be the default of a composite symbol
	if (it != symbols.end()) {
		// assert same type
		if (symbol->type() != it->second->type()) {
			stringstream s;
			s << "Redefinition of a symbol \"" << symbol->name() << "\" with different type." << endl;
			s << " type " << symbol->type() << " != " << it->second->type() << endl;
			throw SymbolError(SymbolError::Type::InvalidDefinition, s.str());
		}
		// assert composite symbol
		auto comp = dynamic_pointer_cast<CompositeSymbol_I>(it->second);
		if (! comp) {
			throw SymbolError(SymbolError::Type::InvalidDefinition, string("Redefinition of a symbol \"") + symbol->name() + "\" in scope \""  + this->name + "\"");
		}
		symbol->setScope(shared_from_this());
		comp->setDefaultValue(symbol);
		return;
	}

	// Register the symbol
	symbol->setScope(shared_from_this());
	symbols.insert( {symbol->name(), symbol} );
	
	// Forward symbols of spatial components to the parental scope
	if ( ct_component ) {
		assert(parent);
		// if it's a real symbol, not derived like 'vec.x' 
		if (! dynamic_pointer_cast<VectorComponentAccessor>(symbol) ) {
			parent->registerSubScopeSymbol(this, symbol);
		}
	}
	
	// Create read-only vector component symbols for vectors, i.e. VDOUBLE
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

void Scope::removeSymbol(Symbol sym) {
	auto it = symbols.find(sym->name());
	if (it != symbols.end())  {
		if (dynamic_pointer_cast<CompositeSymbol_I>(it->second) ) {
			dynamic_pointer_cast<CompositeSymbol_I>(it->second)->removeCellTypeAccessor(sym);
			
		}
		
		symbols.erase(it);
		if (dynamic_pointer_cast<const SymbolAccessorBase<VDOUBLE> >( sym)) {
			symbols.erase(sym->name() + ".x");
			symbols.erase(sym->name() + ".y");
			symbols.erase(sym->name() + ".z");
			symbols.erase(sym->name() + ".theta");
			symbols.erase(sym->name() + ".phi");
			symbols.erase(sym->name() + ".abs");
		}
		if ( ct_component ) {
			assert(parent);
			// if it's a real symbol, not derived like 'vec.x' 
			if (! dynamic_pointer_cast<const VectorComponentAccessor>(sym) ) {
				parent->removeSubScopeSymbol(sym);
			}
		}
	}
	else cout << "Unable to remove Symbol " << sym->name() << " of type " << sym->linkType() << endl;
	
};

void Scope::init()
{
	for (auto symbol : composite_symbols) {
		symbol.second->init(component_scopes.size());
	}
}

// Currently, this implementation is only available for CellType scopes, that may override the global scope symbol within the lattice part that they occupy
void Scope::registerSubScopeSymbol(Scope *sub_scope, Symbol symbol) {
	
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
			
			// if existing symbol is not a Composite yet, remove the old Symbol registration and replace it by a Composite Symbol with a default value
			if (! dynamic_pointer_cast<CompositeSymbol_I>(it->second)) {
				if (dynamic_pointer_cast<const SymbolAccessorBase<double> >(symbol)) {
					auto composite_sym = make_shared<CompositeSymbol<double> >(symbol->name(), dynamic_pointer_cast< const SymbolAccessorBase<double> >(it->second));
					composite_sym_i = composite_sym;
					composite_symbols[symbol->name()] = composite_sym;
					
					symbols.erase(it);
					registerSymbol(composite_sym);
				}
				else if (dynamic_pointer_cast<const SymbolAccessorBase<VDOUBLE> >(symbol)){
					auto composite_sym = make_shared<CompositeSymbol<VDOUBLE> >(symbol->name(), dynamic_pointer_cast<const SymbolAccessorBase<VDOUBLE> >(it->second) );
					composite_sym_i = composite_sym;
					composite_symbols[symbol->name()] = composite_sym;
					
					// Unregister derived symbols and the real one !!!
					symbols.erase(it->first+".x");
					symbols.erase(it->first+".y");
					symbols.erase(it->first+".z");
					symbols.erase(it->first+".phi");
					symbols.erase(it->first+".theta");
					symbols.erase(it->first+".abs");
					symbols.erase(it);
					registerSymbol(composite_sym);
				}
				else {
					throw string("Composity symbol type not implemented in Scope ") + symbol->type();
				}
			}
			else {
				composite_sym_i = dynamic_pointer_cast<CompositeSymbol_I>(it->second);
			}
			
			composite_sym_i->addCellTypeAccessor(sub_scope_id, symbol);
		}
	}
	else {
		// Create a composite symbol
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
			throw string("Symbol type not implemented in CompositeSymbol ") + "\n" + symbol->type() + "!=" + TypeInfo<VDOUBLE>::name() + "!=" + TypeInfo<double>::name();
		}
		composite_sym_base->setScope(shared_from_this());
		composite_sym_i->addCellTypeAccessor(sub_scope_id, symbol);
		composite_symbols[composite_sym_i->name()] = composite_sym_i;
		registerSymbol(composite_sym_base);
	}
}

void Scope::removeSubScopeSymbol(Symbol sym) {
	auto it = symbols.find(sym->name());
	if (it != symbols.end()) {
		dynamic_pointer_cast<CompositeSymbol_I>(it->second)->removeCellTypeAccessor(sym);
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
		throw SymbolError(SymbolError::Type::Undefined, string("Unknown symbol '") + name + string("' in findSymbol."));
	}
}


void Scope::registerTimeStepListener(TimeStepListener* tsl)
{
	local_tsl.insert(tsl);
}


void Scope::registerSymbolReader(TimeStepListener* tsl, string symbol)
{
	cout << "registering reader " << tsl->XMLName() << " on symbol " << symbol << " in Scope " << name << endl;
	symbol_readers.insert(pair<string, TimeStepListener*>(symbol, tsl));
}


void Scope::registerSymbolWriter(TimeStepListener* tsl, string symbol)
{
	cout << "registering writer " << tsl->XMLName() << " on symbol " << symbol << " in Scope " << name << endl;
	symbol_writers.insert(pair<string,TimeStepListener*>(symbol,tsl));
}


void Scope::propagateSinkTimeStep(string symbol, double time_step)
{
	auto range = symbol_writers.equal_range(symbol);
	for (auto it = range.first; it != range.second; it++) {
// 		cout << "Scope " << name << ": Propagate up to " << it->second->XMLName() << endl;
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
// 		cout << "Scope " << name << ": Propagate down to " << it->second->XMLName() << endl;
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
