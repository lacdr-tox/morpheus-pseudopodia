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

#ifndef EXPRESSION_EVALUATOR_H
#define EXPRESSION_EVALUATOR_H

// #include "interfaces.h"
#include "muParser/muParser.h"
#include "scope.h"
#include <mutex>

/** Expression Evaluation Wrapper
 * 
 * Uses muParser to evaluate string defined expressions,
 * while variables are resolved with platform symbols
 * 
 * Compatible -- can handle Vector and Scalar expressions
 * Threadsafe -- nope
 */

unique_ptr<mu::Parser> createMuParserInstance();

template <class T>
class ExpressionEvaluator {
public:
	ExpressionEvaluator(string expression, bool partial_spec = false);
	ExpressionEvaluator(const ExpressionEvaluator<T> & other);
	void setSymbolFactory(mu::facfun_type factory, void* internal);
	void init(const Scope* scope);
	
	const SymbolBase::Flags& flags() const { return expr_flags; };
	Granularity getGranularity() const { return flags().granularity; }

	const string& getDescription() const;
	string getExpression() const { return expression; }
	
	typename TypeInfo<T>::SReturn get(const SymbolFocus& focus, bool safe=false) const;
	typename TypeInfo<T>::SReturn safe_get(const SymbolFocus& focus) const { return get(focus, true);}
	
	set<SymbolDependency> getDependSymbols() const;
	
	
private:
	
	int expectedNumResults() const;
	
	T const_val;
	string expression;
	bool allow_partial_spec;
	bool expr_is_symbol;
	bool expr_is_const;
	bool have_factory;
	bool expand_scalar_expr;
	const Scope *scope;
	unique_ptr<mu::Parser> parser;
	SymbolBase::Flags expr_flags;
	vector< SymbolAccessor<double> > symbols;
	vector< SymbolAccessor<VDOUBLE> > v_symbols;
	mutable vector<double> symbol_values;
	uint v_sym_cache_offset;
	set<string> depend_symbols;
};

#ifdef HAVE_OPENMP
class OMPMutex
{
public:
	OMPMutex()             {omp_init_lock(&_lock);}
	~OMPMutex()            {omp_destroy_lock(&_lock);}
	void lock()         {omp_set_lock(&_lock);}
	void unlock()           {omp_unset_lock(&_lock);}
	bool try_to_lock()      {return omp_test_lock(&_lock);}
private:
	OMPMutex(const OMPMutex&);
	OMPMutex&operator=(const OMPMutex&);
	omp_lock_t _lock;
}; 

typedef OMPMutex GlobalMutex;

#else

typedef std::mutex GlobalMutex;

#endif

template <class T>
class ThreadedExpressionEvaluator {
public:
	ThreadedExpressionEvaluator(string expression, bool partial_spec = false) { evaluators.push_back( unique_ptr<ExpressionEvaluator<T> >(new ExpressionEvaluator<T>(expression,partial_spec)) );};
	void init(const Scope* scope) { for (auto& evaluator : evaluators) evaluator->init(scope); }
	const string& getDescription() const { return evaluators[0]->getDescription(); };
	const SymbolBase::Flags& flags() const { return evaluators[0]->flags(); }
	Granularity getGranularity() const { return evaluators[0]->getGranularity(); };
	string getExpression() const { return evaluators[0]->getExpression(); };

	typename TypeInfo<T>::SReturn get(const SymbolFocus& focus) const {
		uint t = omp_get_thread_num();
		if (evaluators.size()<=t || ! evaluators[t] ) {
			mutex.lock();
			if (evaluators.size()<=t) {
				evaluators.resize(t+1);
			}
			evaluators[t] = unique_ptr<ExpressionEvaluator<T> >( new ExpressionEvaluator<T>(*evaluators[0]) );
			mutex.unlock();
		}
		return evaluators[t]->get(focus);
	};
	typename TypeInfo<T>::SReturn safe_get(const SymbolFocus& focus) const {
		uint t = omp_get_thread_num();
		if (evaluators.size()<=t || ! evaluators[t] ) {
			mutex.lock();
			if (evaluators.size()<=t) {
				evaluators.resize(t+1);
			}
			evaluators[t] = unique_ptr<ExpressionEvaluator<T> >( new ExpressionEvaluator<T>(*evaluators[0]) );
			mutex.unlock();
		}
		return evaluators[t]->safe_get(focus);
	};
	set<SymbolDependency> getDependSymbols() const { return evaluators[0]->getDependSymbols(); };
private:
	mutable vector<unique_ptr<ExpressionEvaluator<T> > > evaluators;
	mutable GlobalMutex mutex;
};

// #include "symbol_accessor.h"

template <class T>
ExpressionEvaluator< T >::ExpressionEvaluator(string expression, bool partial_spec)
{
	this->expression = expression;
	if (expression.empty())
		throw string("Empty expression in ExpressionEvaluator");
	parser = createMuParserInstance(); 
	have_factory = false;
	expr_is_const = false;
	expr_is_symbol = false;
	allow_partial_spec = partial_spec;
}

template <class T>
ExpressionEvaluator<T>::ExpressionEvaluator(const ExpressionEvaluator<T> & other) 
{
	// at first, copy all configuration
	expression = other.expression;
	expr_is_symbol = other.expr_is_symbol;
	expr_is_const = other.expr_is_const;
	const_val = other.const_val;
	allow_partial_spec = other.allow_partial_spec;
	expand_scalar_expr = other.expand_scalar_expr;
	
	have_factory = false;
	scope = other.scope;
	symbols = other.symbols; 
	v_symbols = other.v_symbols;
	v_sym_cache_offset = other. v_sym_cache_offset;
	depend_symbols = other.depend_symbols;
	
	// explicit copies
	if (other.parser) {
		parser = unique_ptr<mu::Parser>( new mu::Parser(*other.parser));
	}
	// create a local symbol value cache and relink to the local parser
	symbol_values.resize(other.symbol_values.size());
	for( uint i_sym=0; i_sym<symbols.size(); i_sym++) {
		parser->DefineVar(symbols[i_sym]->name(), &symbol_values[i_sym]);
	}
	for (uint i=0; i<v_symbols.size(); i++) {
		parser->DefineVar(v_symbols[i]->name(),&symbol_values[v_sym_cache_offset+i] );
	}
}

template <class T>
void ExpressionEvaluator<T>::setSymbolFactory(mu::facfun_type factory, void* internal)
{
	have_factory = true;
	parser->SetVarFactory(factory, internal);
}

template <class T>
void ExpressionEvaluator<T>::init(const Scope* scope)
{
	// Binding symbol values to the mu_parser
	// Currently we register all available symbols and clean up afterwards.
	// Alternatively we could use a VariableFactory, but this may not be a member function and is
	// thus a bit more tricky.
	this->scope = scope;
	
	
	string clean_expression=expression;
	string remove_chars="\t\n\r";
	string::size_type pos=0;
	while ( (pos = clean_expression.find_first_of(remove_chars,pos)) != string::npos) {
		clean_expression[pos]=' ';
		pos++;
	}
	pos = clean_expression.find_first_not_of(" ");
	if (pos!=0) {
		if (pos==string::npos) pos = clean_expression.end() - clean_expression.begin();
		clean_expression.erase(clean_expression.begin(), clean_expression.begin()+pos);
	}
	pos = clean_expression.find_last_not_of(" ");
	if (pos!=clean_expression.size()-1 && pos!=string::npos) {
		clean_expression.erase(clean_expression.begin()+pos+1, clean_expression.end());
	}

	if (clean_expression.empty()) {
		throw string("Empty Expression in ExpressionEvaluator");
	}
	
	double buzz_value = 0.0;
	expr_is_symbol = false;
	
	if (!have_factory) {
		set<string> symbol_names  = scope->getAllSymbolNames<double>();
		vector< SymbolAccessor<double> >::const_iterator it;
	// 	cout << "Declaring Variables for Function " << this->clean_expression << ": ";
		for (auto symbol : symbol_names) {
			try{
	// 			cout <<  it->getName() << ", "; cout.flush();
				parser->DefineVar( symbol.c_str(), &buzz_value );
			}
			catch (mu::Parser::exception_type &e){
				string scopename = ( scope->getName() );
				throw  (string("Error in declaration of variable \"") +symbol+ ("\"  in ")+ scopename + ("."));
				//cerr << "Error in declaring variable '"<< symbol <<"'  for Expression: " << e.GetMsg() << ":\n\n" << endl;
				//assert(0); exit(-1);
			}
		}
		
		mu::varmap_type used_symbols;
		try{
			parser->SetExpr(clean_expression);
			used_symbols = parser->GetUsedVar();
			if (parser->GetNumResults() == 1 && expectedNumResults() > 1) {
				expand_scalar_expr = true;
				cout << "Scalar expansion for " << clean_expression << endl;
			}
			else {
				expand_scalar_expr = false;
			}
		}
		catch(mu::Parser::exception_type &e){
			string scopename = ( scope->getName() );
			throw  (string("Error \'") + e.GetMsg() + "\' in expression \""+ e.GetExpr() +("\" in ")+ scopename + ("."));
		}
		
		parser->ClearVar();
		symbols.clear();
		v_symbols.clear();
		symbol_values.resize(used_symbols.size(), 0.0);

		uint i_sym=0;
		for( auto symbol : used_symbols) {
			try {
				symbols.push_back( scope->findSymbol<double>(symbol.first,allow_partial_spec) );
				parser->DefineVar( symbol.first, &symbol_values[i_sym] );
				depend_symbols.insert(symbol.first);
				i_sym++;
			}
			catch (...) {
				if (expand_scalar_expr) {
					v_symbols.push_back(scope->findSymbol<VDOUBLE>(symbol.first,allow_partial_spec));
					depend_symbols.insert(symbol.first);
				} 
				else {
					throw ;
				}
			}
		}
		v_sym_cache_offset = symbols.size();
		for (uint i=0; i<v_symbols.size(); i++) {
			parser->DefineVar(v_symbols[i]->name(),&symbol_values[v_sym_cache_offset+i] );
		}
		
		if (expand_scalar_expr && v_symbols.size() == 0) {
			throw string("Refuse to expand scalar expression '") + expression + string("' to vector") ;
		}
	}
	else {
		try{
			parser->SetExpr(clean_expression);
		}
		catch (mu::Parser::exception_type &e){
			string scopename = ( scope->getName() );
			throw  (string("Error \'") + e.GetMsg() + "\' in expression \""+ e.GetExpr() +("\" in ")+ scopename + ("."));
		}
		mu::varmap_type used_symbols;
		used_symbols = parser->GetUsedVar();
		for( auto symbol : used_symbols) {
			depend_symbols.insert(symbol.first);
		}
	}
	
	// Initialize expression flags, i.e. for constness
	expr_flags.space_const = true;
	expr_flags.time_const = true;
	expr_flags.stochastic = false;
	expr_flags.integer = false;
	expr_flags.delayed = false;
	expr_flags.partially_defined = false;
	expr_flags.writable = false;
	expr_flags.granularity = Granularity::Global;
	
	for ( const auto& symb : symbols) {
		const auto& of = symb->flags();
		expr_flags.space_const &= of.space_const;
		expr_flags.time_const &= of.time_const;
		expr_flags.stochastic |= of.stochastic;
		expr_flags.partially_defined |= of.partially_defined;
		expr_flags.granularity += of.granularity;
	}
	for ( const auto& symb : v_symbols) {
		const auto& of = symb->flags();
		expr_flags.space_const &= of.space_const;
		expr_flags.time_const &= of.time_const;
		expr_flags.stochastic |= of.stochastic;
		expr_flags.partially_defined |= of.partially_defined;
		expr_flags.granularity += of.granularity;
	}
	expr_is_const = expr_flags.time_const && expr_flags.space_const;
	
	// random functions prevent an expression from beeing const
	set<string> volatile_functions;
	volatile_functions.insert(sym_RandomUni);
	volatile_functions.insert(sym_RandomInt);
	volatile_functions.insert(sym_RandomBool);
	volatile_functions.insert(sym_RandomGamma);
	volatile_functions.insert(sym_RandomNorm);
	for ( auto fun : parser->GetUsedFun()) {
		if (volatile_functions.count(fun))
			expr_is_const = false;
	}
	
	if (expr_is_const) {
		expr_is_const = false;
		const_val = safe_get(SymbolFocus::global);
		expr_is_const = true;
// 		cout << "Expression " << this->getExpression() << " is const" << endl;
	}
	
	// Check for direct symbol
	if (TypeInfo< VDOUBLE >::name == TypeInfo< T >::name) {
		if (v_symbols.size() == 1 && clean_expression == v_symbols[0]->name()){
			expr_is_symbol = true;
// 			cout << "Expression " << this->getExpression() << " is a symbol" << endl;
		}
		else {
			expr_is_symbol = false;
		}
	}
	else {
		if (symbols.size() == 1 && clean_expression == symbols[0]->name()){
			expr_is_symbol = true;
			expr_flags.integer = symbols[0]->flags().integer;
// 			cout << "Expression " << this->getExpression() << " is a symbol" << endl;
		}
		else {
			expr_is_symbol = false;
		}
	}
}

 
template <class T>
const string& ExpressionEvaluator<T>::getDescription() const
{
	if (expr_is_symbol)
		return symbols[0]->description();
	else 
		return expression;
}


// template <class T>
// typename TypeInfo<T>::SReturn  ExpressionEvaluator<T>::get(const SymbolFocus& focus) const { 
// 	static_assert(false,"Expression Evaluators are only available for types double and VDOUBLE");
// };


//Fully specified template methods
template <>
typename TypeInfo<double>::SReturn  ExpressionEvaluator<double>::get(const SymbolFocus& focus, bool safe) const;

template <>
typename TypeInfo<float>::SReturn  ExpressionEvaluator<float>::get(const SymbolFocus& focus, bool safe) const;

template <>
typename TypeInfo<VDOUBLE>::SReturn  ExpressionEvaluator<VDOUBLE>::get(const SymbolFocus& focus, bool safe) const;

template <>
const string&  ExpressionEvaluator<VDOUBLE>::getDescription() const;


template <class T>
set< SymbolDependency > ExpressionEvaluator<T>::getDependSymbols() const
{
	set<SymbolDependency> sym_dep;
	sym_dep.insert(symbols.begin(),symbols.end());
	sym_dep.insert(v_symbols.begin(),v_symbols.end());
// 	for (auto& s : symbols ) {
// // 		cout << "d: " << s.getName() << "["<< s.getScope()->getName() << "]";
// 		sym_dep.insert(s);
// 	}
// 	for (auto& s : v_symbols ) {
// // 		cout << "v: " << s.getName() << "["<< s.getScope()->getName() << "]";
// 		sym_dep.insert(s);
// 	}
	return sym_dep;
}




#endif
