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

#include "muParser/muParser.h"
#include "evaluator_cache.h"
#include "random_functions.h"
#include "scope.h"
#include <mutex>

unique_ptr<mu::Parser> createMuParserInstance();

/**
\defgroup MuParser Evaluating math expressions
\ingroup Concepts

Mathematical expressions are evaluated at run-time using [MuParser](http://beltoforion.de/article.php?a=muparser), while all variables are resolved using Morpheus' \ref Symbols system.

**Operators**:
+, -, *, /, ^, =, >=, <=, !=, ==, <, >
and, or, xor, !

**Predefined Functions**:
if([condition], [then], [else]), and, or, xor, sin, cos, tan, asin, acos, atan, sinh, cosh, tanh, asinh, acosh, atanh, log2, log10, ln, exp, pow, sqrt, sign, rint, abs, min, max, sum, avg, mod

**SBML compatibility Functions**:
piecewise, lt, leq, eq, neq, geq, gt, arcsin, arccos, arctan, arcsinh, arccosh, arctanh

**Random number** generators:
rand_uni([min], [max])
rand_int([min], [max])
rand_norm([mean], [stdev])
rand_gamma([shape], [scale])
rand_bool()


**/

typedef EvaluatorCache::EvaluatorSymbol EvaluatorVariable;

/** @brief Run-time Expression Evaluation
 * 
 * Uses muParser to evaluate string defined expressions,
 * while variables are resolved with platform symbols
 * 
 * Compatible -- can handle Vector and Scalar expressions
 * Threadsafe -- nope
 */

template <class T>
class ExpressionEvaluator {
public:
	
	/// Evaluator constructor using a private internal data cache
	ExpressionEvaluator(string expression, const Scope* scope, bool partial_spec = false);
	/// Evaluator constructor using a preallocated (shared) data cache
	ExpressionEvaluator(string expression, shared_ptr<EvaluatorCache> cache); 
	
	ExpressionEvaluator(const ExpressionEvaluator<T> & other, shared_ptr<EvaluatorCache> cache = shared_ptr<EvaluatorCache>()); //!< copy constructor
	
	
	/**  \brief Set the list of local variables to be used for expression evaluation. 
	 *   These local variables must be set separately and override symbols from the scope.
	 *   The order in the vector is also decisive for the expected data order in setLocals().
	 */
	void setLocalsTable(const vector<EvaluatorVariable>& locals) { evaluator_cache->setLocalsTable(locals); };
	
	/** \brief Set the evaluator's local variables
	 *  @p data is a contiguous array of doubles values, where a VECTOR type parameter
	 *  is composed of three entries representing x, y and z component.
	 */
	void setLocals(const double* data) const { evaluator_cache->setLocals(data);/*(&symbol_values[l_sym_cache_offset], data, sizeof(double) * local_symbols.size()); */};
	
	/** \brief Initialize expression evaluator.
	 *  In particular, all dependent symbols, functions and parameters must be declared prior
	 *  to initalisation. Symbols are retrieved form the scope.
	 */
	
	///  \brief Number of local variables in terms of doubles (VDOUBLEs count 3 doubles)
	int getLocalsCount() { return evaluator_cache->getLocalsTable().size();  }
	
	/// Parse the expresion and bind to the external symbol via an \ref EvaluatorCache
	void init();
	
	/// \brief Mmetadata of the expression
	const SymbolBase::Flags& flags() const {
		if (!initialized )
			const_cast<ExpressionEvaluator*>(this)->init();
		return expr_flags;
		
	};
	/// \brief expressions spatial granularity
	Granularity getGranularity() const { return flags().granularity; }

	/// Description used for graphical visualization and reporting
	const string& getDescription() const;
	/// Raw expression
	string getExpression() const { return expression; }
	
	/// get the value for spatial element @p focus
	typename TypeInfo<T>::SReturn get(const SymbolFocus& focus, bool safe=false) const;
	/// get the value for spatial element @p focus and before ensure the expression to be initialized
	typename TypeInfo<T>::SReturn safe_get(const SymbolFocus& focus) const { return get(focus, true);}
	/// get without updating the associated cache (has been updated earlier).
	typename TypeInfo<T>::SReturn plain_get(const SymbolFocus& focus) const;
	/// set of symbols the expression depends on
	set<SymbolDependency> getDependSymbols() const;
	
private:
	
	int expectedNumResults() const;
	
	const Scope *scope;
	string expression;
	string clean_expression;
	bool allow_partial_spec;
	
	bool initialized = false;
	bool expr_is_const;
	T const_val;
	bool expr_is_symbol;
	SymbolAccessor<T> symbol_val;
	
	unique_ptr<mu::Parser> parser;
	SymbolBase::Flags expr_flags;
	bool expand_scalar_expr;
	// the value cache
	shared_ptr<EvaluatorCache> evaluator_cache;
	
	set< SymbolDependency > depend_symbols;
	
	friend class SystemSolver; // Allow the SystemSolver to rewire the parser's function definitions to thread-local instances
	template <class S>
	friend class SystemFunc;
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


/** Wrapper class to Evaluator to allow OpenMP thread-safe expression evaluation. 
 * 
 *  All methods just forward to a OpenMP thread specific evaluator.
 *  For interface documentation look up Evaluator.
 */
template <class T>
class ThreadedExpressionEvaluator {
public:
	ThreadedExpressionEvaluator(string expression, const Scope* scope, bool partial_spec = false) { evaluators.push_back( make_unique<ExpressionEvaluator<T> >(expression, scope, partial_spec) );};
	
	void setLocalsTable(const vector<EvaluatorVariable>& locals) { for (auto& evaluator : evaluators) evaluator->setLocalsTable(locals); }
	void setLocals(const double* data) const { getEvaluator()->setLocals(data); }
	int getLocalsCount() const { return evaluators[0]->getLocalsCount(); } 
	
	void init() { for (auto& evaluator : evaluators) evaluator->init(); }
	const string& getDescription() const { return evaluators[0]->getDescription(); };
	const SymbolBase::Flags& flags() const { return evaluators[0]->flags(); }
	Granularity getGranularity() const { return evaluators[0]->getGranularity(); };
	string getExpression() const { return evaluators[0]->getExpression(); };

	typename TypeInfo<T>::SReturn get(const SymbolFocus& focus) const { return getEvaluator()->get(focus); };
	typename TypeInfo<T>::SReturn safe_get(const SymbolFocus& focus) const { return getEvaluator()->safe_get(focus); };
	set<SymbolDependency> getDependSymbols() const { return evaluators[0]->getDependSymbols(); };
private:
	ExpressionEvaluator<T>* getEvaluator() const {
		uint t = omp_get_thread_num();
		if (evaluators.size()<=t || ! evaluators[t] ) {
			mutex.lock();
			if (evaluators.size()<=t) {
				evaluators.resize(t+1);
			}
			evaluators[t] = make_unique<ExpressionEvaluator<T> >( *evaluators[0] );
			mutex.unlock();
		}
		return evaluators[t].get();
	}
	mutable vector<unique_ptr<ExpressionEvaluator<T> > > evaluators;
	mutable GlobalMutex mutex;
};




/*************************
 *     Implementation    *
 * ***********************/

template <class T>
ExpressionEvaluator< T >::ExpressionEvaluator(string expression, const Scope* scope, bool partial_spec)
{
	this->expression = expression;
	this->scope = scope;
	if (expression.empty())
		throw string("Empty expression in ExpressionEvaluator");
	allow_partial_spec = partial_spec;
	parser = createMuParserInstance();
	evaluator_cache = make_shared<EvaluatorCache>(scope, partial_spec);
	expr_is_const = false;
	expr_is_symbol = false;
}

template <class T>
ExpressionEvaluator< T >::ExpressionEvaluator(string expression, shared_ptr<EvaluatorCache> cache)
{
	this->expression = expression;
	if (expression.empty())
		throw string("Empty expression in ExpressionEvaluator");
	allow_partial_spec = cache->getPartialSpec();
	parser = createMuParserInstance();
	evaluator_cache = cache;
	scope = cache->getScope();
	expr_is_const = false;
	expr_is_symbol = false;
}

// How can you duplicate a set of ExpressionEvaluators that share the same cache?? this is required by systems.

template <class T>
ExpressionEvaluator<T>::ExpressionEvaluator(const ExpressionEvaluator<T> & other, shared_ptr<EvaluatorCache> cache)
{
	// at first, copy all configuration
	scope = other.scope;
	expression = other.expression;
	clean_expression = other.clean_expression;
	allow_partial_spec = other.allow_partial_spec;
	
	initialized =  other.initialized;
	expr_is_const = other.expr_is_const;
	const_val = other.const_val;
	expr_is_symbol = other.expr_is_symbol;
	symbol_val = other.symbol_val;
	
	expr_flags = other.expr_flags;
	expand_scalar_expr = other.expand_scalar_expr;
	depend_symbols = other.depend_symbols;
	
	
	// explicit copies
	if (other.parser) {
		parser = make_unique<mu::Parser>( *other.parser );
	}
	if (other.evaluator_cache || cache) {
		if (cache)
			evaluator_cache = cache;
		else 
			evaluator_cache = make_unique<EvaluatorCache>(*other.evaluator_cache);
		parser->SetVarFactory(evaluator_cache->registerSymbol,(void*) evaluator_cache.get());
		evaluator_cache -> attach(parser.get());
	}
	
}

template <class T>
void ExpressionEvaluator<T>::init()
{
	// Binding symbol values to the mu_parser
	// We use the Evaluator cache and the provided Symbol factor registerSymbol()

	// Cleanup
	clean_expression=expression;
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

	// Prepare the Parser scope for parsing
	mu::facfun_type factory = EvaluatorCache::registerSymbol;
	parser->SetVarFactory(factory, (void*) evaluator_cache.get() );
	
	auto scope_symbols = scope->getAllSymbols<double>();
	for (const auto& symbol :scope_symbols) {
		if (symbol->flags().function) {
			// DefineFun with the generic callback interface
			auto fun_symbol = dynamic_pointer_cast<const FunctionAccessor<double> >(symbol);
			parser->DefineFun(
				symbol->name(),
				fun_symbol->getCallBack(),
				! symbol->flags().stochastic
			);
		}
	}
	
	// TODO We do not support VectorFunctions yet
// 	auto scope_vec_symbols = scope->getAllSymbols<double>();
// 	for (const auto& symbol :scope_vec_symbols) {
// 		if (symbol->flags().function) {
// 			// DefineFun with the generic callback interface
// 			auto fun_symbol = dynamic_pointer_cast<const FunctionAccessor<VDOUBLE> >(symbol);
// 			parser->DefineFun(
// 				symbol->name(),
// 				fun_symbol->getCallBack(),
// 				! symbol->flags().stochastic
// 			);
// 		}
// 	}
	
	EvaluatorCache::ParserDesc desc;
	try{
		
		if (expectedNumResults() > 1)  {
			evaluator_cache->permitScalarExpansion(true);
		}
		
		parser->SetExpr(clean_expression);
		desc = evaluator_cache->getDescriptor(this->parser.get());

	}
	catch(mu::Parser::exception_type &e){
		string scopename = ( scope->getName() );
		throw  (string("Error \'") + e.GetMsg() + "\' in expression \""+ expression +("\" in ")+ scopename + ("."));
	}
	
	// collapse the flags of the external symbols
	// check what to do with the local symbols --> they may change on whatever condition ???
	expr_is_symbol = false;
	
	// Initialize expression flags, i.e. for constness
	expr_flags.space_const = true;
	expr_flags.time_const = true;
	expr_flags.stochastic = false;
	expr_flags.integer = false;
	expr_flags.delayed = false;
	expr_flags.partially_defined = false;
	expr_flags.writable = false;
	expr_flags.granularity = Granularity::Global;
	
	depend_symbols = desc.ext_symbols;
	for ( const auto& symb : depend_symbols) {
		const auto& of = symb->flags();
		expr_flags.space_const &= of.space_const;
		expr_flags.time_const &= of.time_const;
		expr_flags.stochastic |= of.stochastic;
		expr_flags.partially_defined |= of.partially_defined;
		expr_flags.granularity += of.granularity;
	}
	
	// random functions prevent an expression to be constant
	set<string> volatile_functions;
	volatile_functions.insert(sym_RandomUni);
	volatile_functions.insert(sym_RandomInt);
	volatile_functions.insert(sym_RandomBool);
	volatile_functions.insert(sym_RandomGamma);
	volatile_functions.insert(sym_RandomNorm);
	set<string> const_functions;
	
// 	auto scope_symbols = scope->getAllSymbols<double>();
	for (const auto& symbol :scope_symbols) {
		if (symbol->flags().function) {
			if (symbol->flags().stochastic) {
				volatile_functions.insert(symbol->name());
			}
			else if (symbol->flags().time_const && symbol->flags().space_const) {
				const_functions.insert(symbol->name());
			}
		}
	}
	
	for ( auto fun : parser->GetUsedFun()) {
		if (volatile_functions.count(fun)) {
			expr_flags.space_const = expr_flags.time_const = false;
			expr_flags.stochastic = true;
		}
		else if (const_functions.count(fun)==0) {
			expr_flags.space_const = expr_flags.time_const = false;
		}
	}
	
	expr_is_const = expr_flags.time_const && expr_flags.space_const && desc.loc_symbols.empty();
	
	if (parser->GetNumResults() == 1 && expectedNumResults() > 1) {
		if ( !  desc.requires_expansion )
			throw string("Refuse to expand scalar expression ") + clean_expression + " to vector. Require at least one vector symbol";
		expand_scalar_expr = true;
		cout << "Scalar expansion for " << clean_expression << endl;
	}
	else {
		expand_scalar_expr = false;
	}
	
	initialized = true;
	
	// update and collect data
	if (expr_is_const) {
		expr_is_const = false;
		const_val = safe_get(SymbolFocus::global);
		expr_is_const = true;
		cout << "Expression " << this->getExpression() << " is const (";
		for (auto const& dep: depend_symbols ) { cout << dep->name() << ", " ; }
		cout << ")" << endl;
	}
	
	// Check for direct symbol
	if (depend_symbols.size() == 1 && (*depend_symbols.begin())->name() == clean_expression ) {
		expr_is_symbol = true;
		// cast the symbol into a typed SymbolAccessor
		symbol_val = dynamic_pointer_cast< const SymbolAccessorBase<T> >(*depend_symbols.begin());
		expr_flags.integer = symbol_val->flags().integer;
		cout << "Expression " << this->getExpression() << " is a symbol" << endl;

	}
	else 
		expr_is_symbol = false;
}

 
template <class T>
const string& ExpressionEvaluator<T>::getDescription() const
{
	if (expr_is_symbol)
		return symbol_val->description();
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

template <class T>
set< SymbolDependency > ExpressionEvaluator<T>::getDependSymbols() const
{
	return depend_symbols;
}




#endif
