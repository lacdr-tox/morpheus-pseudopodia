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

#include "interfaces.h"
#include "muParser/muParser.h"

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
	
	///  Expression is constant in time, not necessarily in space ...
	bool isConst() const;
	bool isInteger() const;

	const string& getDescription() const;
	Granularity getGranularity() const;
	string getExpression() const { return expression; }
	
	
	typename TypeInfo<T>::SReturn get(const SymbolFocus& focus) const;
	
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
	vector< SymbolAccessor<double> > symbols;
	vector< SymbolAccessor<VDOUBLE> > v_symbols;
	mutable vector<double> symbol_values;
	uint v_sym_cache_offset;
	set<string> depend_symbols;
};


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


template <class T>
class ThreadedExpressionEvaluator {
public:
	ThreadedExpressionEvaluator(string expression, bool partial_spec = false) { evaluators.push_back( unique_ptr<ExpressionEvaluator<T> >(new ExpressionEvaluator<T>(expression,partial_spec)) );};
	void init(const Scope* scope) { evaluators[0]->init(scope); }
	bool isConst() const { return evaluators[0]->isConst(); };
	const string& getDescription() const { return evaluators[0]->getDescription(); };
	Granularity getGranularity() const { return evaluators[0]->getGranularity(); };
	string getExpression() const { return evaluators[0]->getExpression(); };
	bool isInteger() const { return evaluators[0]->isInteger(); };
	typename TypeInfo<T>::SReturn get(const SymbolFocus& focus) const {
		int t = omp_get_thread_num();
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
	set<SymbolDependency> getDependSymbols() const { return evaluators[0]->getDependSymbols(); };
private:
	mutable vector<unique_ptr<ExpressionEvaluator<T> > > evaluators;
	mutable OMPMutex mutex;
};

#include "symbol_accessor.h"

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
	// explicit copies
	if (other.parser) {
		parser = unique_ptr<mu::Parser>( new mu::Parser(*other.parser));
	}
	expression = other.expression;
	expr_is_symbol = other.expr_is_symbol;
	expr_is_const = other.expr_is_const;
	const_val = other.const_val;
	allow_partial_spec = other.allow_partial_spec;
	
	have_factory = false;
	scope = other.scope;
	symbols = other.symbols; 
	v_symbols = other.v_symbols;
	depend_symbols = other.depend_symbols;
	
	// relink the symbol_values cache to the parser
	symbol_values.resize(symbols.size());
	for( int i_sym=0; i_sym<symbols.size(); i_sym++) {
		parser->DefineVar(symbols[i_sym].getName(), &symbol_values[i_sym]);
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
			}
			else {
				expand_scalar_expr = false;
			}
		}
		catch(mu::Parser::exception_type &e){
			string scopename = ( scope->getName() );
			throw  (string("Error in expression \"")+ e.GetExpr() +("\" in ")+ scopename + ("."));
			//cerr << "Error in Expression: " << e.GetMsg() << ":\n\n" << e.GetExpr() << endl;   

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
			parser->DefineVar(v_symbols[i].getName(),&symbol_values[v_sym_cache_offset+i] );
		}
		
		if (expand_scalar_expr && v_symbols.size() == 0) {
			throw string("Refuse to expand scalar expression to vector");
		}
	}
	else {
		try{
			parser->SetExpr(clean_expression);
		}
		catch (mu::Parser::exception_type &e){
			cerr << "Error in Expression: " << e.GetMsg() << ":\n\n" << e.GetExpr() << endl;
			exit(-1);
		}
		mu::varmap_type used_symbols;
		used_symbols = parser->GetUsedVar();
		for( auto symbol : used_symbols) {
			depend_symbols.insert(symbol.first);
		}
	}
	
	// Check for constness
	expr_is_const = true;
	for ( const auto& symb : symbols) {
		expr_is_const = expr_is_const && symb.isConst();
	}
	for ( const auto& symb : v_symbols) {
		expr_is_const = expr_is_const && symb.isConst();
	}
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
		const_val = get(SymbolFocus::global);
		expr_is_const = true;
// 		cout << "Expression " << this->getExpression() << " is const" << endl;
	}
	
	// Check for direct symbol
	if (TypeInfo< VDOUBLE >::name() == TypeInfo< T >::name()) {
		if (v_symbols.size() == 1 && clean_expression == v_symbols[0].getName()){
			expr_is_symbol = true;
// 			cout << "Expression " << this->getExpression() << " is a symbol" << endl;
		}
		else {
			expr_is_symbol = false;
		}
	}
	else {
		if (symbols.size() == 1 && clean_expression == symbols[0].getName()){
			expr_is_symbol = true;
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
		return symbols[0].getFullName();
	else 
		return expression;
}

template <class T>
bool ExpressionEvaluator<T>::isConst() const
{
	return expr_is_const;
}

template <class T>
bool ExpressionEvaluator<T>::isInteger() const
{
	if (expr_is_symbol)
		return symbols.front().isInteger();
	else
		return false;
}

template <class T>
Granularity ExpressionEvaluator<T>::getGranularity() const
{
	Granularity granularity = Granularity::Global;
	for (auto&& sym : symbols) {
		granularity+= sym.getGranularity();
	}
	return granularity;
}


// template <class T>
// typename TypeInfo<T>::SReturn  ExpressionEvaluator<T>::get(const SymbolFocus& focus) const { 
// 	static_assert(false,"Expression Evaluators are only available for types double and VDOUBLE");
// };


//Fully specified template methods
template <>
typename TypeInfo<double>::SReturn  ExpressionEvaluator<double>::get(const SymbolFocus& focus) const;

template <>
typename TypeInfo<float>::SReturn  ExpressionEvaluator<float>::get(const SymbolFocus& focus) const;

template <>
typename TypeInfo<VDOUBLE>::SReturn  ExpressionEvaluator<VDOUBLE>::get(const SymbolFocus& focus) const;

template <>
const string&  ExpressionEvaluator<VDOUBLE>::getDescription() const;


template <class T>
set< SymbolDependency > ExpressionEvaluator<T>::getDependSymbols() const
{
	set<SymbolDependency> sym_dep;
	for (auto& s : symbols ) {
// 		cout << "d: " << s.getName() << "["<< s.getScope()->getName() << "]";
		SymbolDependency sd = { s.getBaseName(), s.getScope()};
		sym_dep.insert(sd);
	}
	for (auto& s : v_symbols ) {
// 		cout << "v: " << s.getName() << "["<< s.getScope()->getName() << "]";
		SymbolDependency sd = { s.getBaseName(), s.getScope()};
		sym_dep.insert(sd);
	}
	return sym_dep;
}




#endif
