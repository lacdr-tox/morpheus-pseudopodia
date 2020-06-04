#include "system.h"
#include "property.h"
#include "delay.h"

// Time interval scaled Noise Functions for contiuous time Processes

double getRandomNormValueSDE(double mean, double stdev, double scaling) {
	return mean + getRandomGauss(stdev) * scaling;
}

template <>
void SystemFunc<double>::initFunction() {
	if (fun_params_names.size() != fun_params.size())
		fun_params.resize( fun_params_names.size() );
	for (uint i=0; i<fun_params_names.size(); i++) {
// 		cout << "Registering parameter " << fun_params_names[i]<< endl;
		evaluator->parser->DefineVar(fun_params_names[i], &fun_params[i]);
		cache->addParserLocal(fun_params_names[i]);
	}
	
	callback = make_shared<CallBack>(evaluator,  fun_params.empty() ? NULL : &fun_params[0], fun_params.size());
}

template<>
void SystemFunc<VDOUBLE>::initFunction() {}

const string SystemSolver::noise_scaling_symbol = "_noise_scaling";

System::System(SystemType type, SystemContext context_req) : context_requirement(context_req), system_type(type) {}


void System::loadFromXML(const XMLNode node, Scope* scope)
{
	target_scope = scope;
	local_scope = scope->createSubScope("System");
	
	for (auto sym : local_symbols) {
		local_scope->registerSymbol( SymbolRWAccessorBase<double>::createVariable(sym.first, sym.first, sym.second) );
	}

// 	addLocalSymbol(SystemSolver::noise_scaling_symbol, 1 );
	if (system_type == CONTINUOUS) {
		PluginParameter<SystemSolver::Method, XMLNamedValueReader, DefaultValPolicy> method;
		method.setXMLPath("solver");
		map<string,SystemSolver::Method> solver_map;
		solver_map["Euler [fixed, O(1)]"]   = SystemSolver::Method::Euler;
		solver_map["euler"]                 = SystemSolver::Method::Euler;
		solver_map["fixed1"]                = SystemSolver::Method::Euler;
		
		solver_map["Heun [fixed, O(2)]"]    = SystemSolver::Method::Heun;
		solver_map["heun"]                  = SystemSolver::Method::Heun;
		solver_map["fixed2"]                = SystemSolver::Method::Heun;
		
		solver_map["Runge-Kutta [fixed, O(4)]"] = SystemSolver::Method::Runge_Kutta4;
		solver_map["runge-kutta"]           = SystemSolver::Method::Runge_Kutta4;
		solver_map["fixed4"]                = SystemSolver::Method::Runge_Kutta4;
		
		solver_map["Dormand-Prince [adative, O(5)]"] = SystemSolver::Method::AdaptiveDP;
		solver_map["dormand-prince"]        = SystemSolver::Method::AdaptiveDP;
		solver_map["adaptive45"]            = SystemSolver::Method::AdaptiveDP;
		solver_map["adaptive45-dp"]         = SystemSolver::Method::AdaptiveDP;
		solver_map["runge-kutta-adaptive"]  = SystemSolver::Method::AdaptiveDP;
		solver_map["runge-kutta-adaptive-DP"] = SystemSolver::Method::AdaptiveDP;
		
		solver_map["Cash-Karp [adative, O(5)]"] = SystemSolver::Method::AdaptiveCK;
		solver_map["cash-karp"]             = SystemSolver::Method::AdaptiveCK;
		solver_map["adaptive45-ck"]         = SystemSolver::Method::AdaptiveCK;
		solver_map["runge-kutta-adaptive-CK"] = SystemSolver::Method::AdaptiveCK;
		
		solver_map["Bogacki-Shampine [adative, O(3)]"] = SystemSolver::Method::AdaptiveBS;
		solver_map["bogacki-shampine"]      = SystemSolver::Method::AdaptiveBS;
		solver_map["adaptive23"]            = SystemSolver::Method::AdaptiveBS;
		solver_map["runge-kutta-adaptive-BS"] = SystemSolver::Method::AdaptiveBS;

		solver_map["Euler-Maruyama [stochastic, O(1)]"] = SystemSolver::Method::Euler;
		solver_map["stochastic"]            = SystemSolver::Method::Euler;
		
		method.setValueMap(solver_map);
		method.setDefault("Dormand-Prince [adative, O(5)]");
		method.loadFromXML(node, scope);
		solver_spec.method = method();
	
		solver_spec.time_scaling = 1.0;
		getXMLAttribute(node,"time-scaling",solver_spec.time_scaling);
		solver_spec.epsilon = 1e-4;
		getXMLAttribute(node,"solver-eps",solver_spec.epsilon,true);
	}
	else {
		solver_spec.method = SystemSolver::Method::Discrete;
		solver_spec.time_scaling = 1.0;
	}
	
	int nPlugins = node.nChildNode();
	for(int i=0; i<nPlugins; i++) {
		XMLNode xNode = node.getChildNode(i);
// 		try {
			string xml_tag_name(xNode.getName());
			
			shared_ptr<Plugin> p;
			if (xml_tag_name == "Intermediate") {
				auto eqn = make_shared<SystemFunc<double> >();
				eqn->type = SystemFunc<double>::VAR_INIT;
				getXMLAttribute(xNode,"value", eqn->expression);
				getXMLAttribute(xNode,"symbol", eqn->symbol_name);
				evals.push_back(eqn);
			}
			else if (xml_tag_name == "IntermediateVector"){
				auto eqn = make_shared<SystemFunc<VDOUBLE> >();
				eqn->type = SystemFunc<VDOUBLE>::VAR_INIT;
				getXMLAttribute(xNode,"value", eqn->expression);
				getXMLAttribute(xNode,"symbol", eqn->symbol_name); 
				vec_evals.push_back(eqn);
			}
			else {
				p = PluginFactory::CreateInstance(xml_tag_name);			
				if (! p.get()) 
					throw(string("Unknown plugin " + xml_tag_name));
				
				p->loadFromXML(xNode, local_scope);
	// 			uint n_interfaces=0;

				if ( dynamic_pointer_cast<FunctionPlugin>(p) ) {
				// TODO We must inject the evaluator_cache of the proper SystemSolver (local scope)
				// The function is already registered in the scope through loadFromXML, but must be removed now ...
					auto eqn = make_shared<SystemFunc<double> >();
					auto function = static_pointer_cast<FunctionPlugin>(p);
					eqn->type = SystemFunc<double>::FUN;
					eqn->expression = function->getExpr();
					eqn->symbol_name = function->getSymbol();
					eqn->fun_params_names = function->getParams();
					eqn->fun_params.resize( eqn->fun_params_names.size() );
					evals.push_back(eqn);
				}
				else if (dynamic_pointer_cast<Container<double>>(p) && p->XMLName() == Container<double>::VariableXMLName()) {
					// TODO We should not call these temporary variables Variables, but what name? <Temp> 

				}
				else if ( dynamic_pointer_cast<DifferentialEqn>(p)) {
					auto eqn = make_shared<SystemFunc<double> >();
					eqn->type = SystemFunc<double>::ODE;
					eqn->expression = dynamic_pointer_cast<DifferentialEqn>(p)->getExpr();
					eqn->symbol_name = dynamic_pointer_cast<DifferentialEqn>(p)->getSymbol();
					evals.push_back(eqn);
					equations.push_back(eqn);
				}
				else if ( dynamic_pointer_cast<Equation>(p)) {
					auto eqn = make_shared<SystemFunc<double> >();
					eqn->type = SystemFunc<double>::RULE;
					eqn->expression = dynamic_pointer_cast<Equation>(p)->getExpr();
					eqn->symbol_name = dynamic_pointer_cast<Equation>(p)->getSymbol();
					evals.push_back(eqn);
					equations.push_back(eqn);
				}
				else if (dynamic_pointer_cast<Container<VDOUBLE>>(p) && p->XMLName() == Container<VDOUBLE>::VariableXMLName()) {
					auto eqn = make_shared<SystemFunc<VDOUBLE> >();
					eqn->type = SystemFunc<VDOUBLE>::VAR_INIT;
					eqn->expression = dynamic_pointer_cast<Container<VDOUBLE>>(p)->getInitExpression();
					eqn->symbol_name = dynamic_pointer_cast<Container<VDOUBLE>>(p)->getSymbol();
					vec_evals.push_back(eqn);
				}
				else if ( dynamic_pointer_cast<VectorEquation>(p)) {
					auto eqn = make_shared<SystemFunc<VDOUBLE> >();
					eqn->type = SystemFunc<VDOUBLE>::RULE;
					eqn->expression = dynamic_pointer_cast<VectorEquation>(p)->getExpr();
					eqn->symbol_name = dynamic_pointer_cast<VectorEquation>(p)->getSymbol();
					eqn->notation = dynamic_pointer_cast<VectorEquation>(p)->getNotation();
					vec_evals.push_back(eqn);
					vec_equations.push_back(eqn);
				}
				else 
					plugins.push_back(p);
			}
			
// 		} catch(...) {
// 			cerr << "Exception in System::loadFromXML "<< endl; exit(-1);
// 		}
	}
}


void System::addLocalSymbol(string symbol, double value) {
	
	local_symbols[symbol] = value;
	
	if (local_scope) {
		auto sym = SymbolRWAccessorBase<double>::createVariable(symbol,symbol,value);
		local_scope->registerSymbol(sym);
	}
}

bool less_fun_dep_level(shared_ptr<SystemFunc<double>> a, shared_ptr <SystemFunc<double>> b) { return a->cache_idx < b->cache_idx; }


void System::init() {
	assert(local_scope);
	
	for (auto p : sub_step_hooks) {
		if ( dynamic_cast<Equation*>(p)) {
			auto eqn = make_shared<SystemFunc<double> >();
			eqn->type = SystemFunc<double>::EQN;
			eqn->expression = static_cast<Equation*>(p)->getExpr();
			eqn->symbol_name = static_cast<Equation*>(p)->getSymbol();
			evals.push_back(eqn);
// 			equations.push_back(eqn);
			cout << "Equation hook registered " << eqn->symbol_name << "=" << eqn->expression << endl;
		}
		else if ( dynamic_cast<VectorEquation*>(p)) {
			auto eqn = make_shared<SystemFunc<VDOUBLE> >();
			eqn->type = SystemFunc<VDOUBLE>::EQN;
			eqn->expression = static_cast<VectorEquation*>(p)->getExpr();
			eqn->symbol_name = static_cast<VectorEquation*>(p)->getSymbol();
			eqn->notation = static_cast<VectorEquation*>(p)->getNotation();
			vec_evals.push_back(eqn);
// 			vec_equations.push_back(eqn);
		}
		else {
			throw string("Unknown sub-step hook type");
		}
	}
	
	// 	cout << "System init with " <<  omp_get_max_threads() << " threads for CellType " << (ct ? ct->getName() : " NULL " ) << endl;
	for (auto plugin : plugins) {
		plugin->init(local_scope);
	}

	// initialize assignment targets
	for (const auto& eval : evals) {
		if (eval->type == SystemFunc<double>::ODE || eval->type == SystemFunc<double>::RULE || eval->type == SystemFunc<double>::EQN ) {
			eval->global_symbol = local_scope->findRWSymbol<double>(eval->symbol_name);
		}
	}
	for (const auto& eval : vec_evals) {
		if (eval->type == SystemFunc<VDOUBLE>::RULE || eval->type == SystemFunc<VDOUBLE>::EQN) {
			eval->global_symbol = local_scope->findRWSymbol<VDOUBLE>(eval->symbol_name);
		}
	}
	
	
	//////////////////////////////////////////
	//          Checking accessibility of the contexts
	//////////////////////////////////////////
	target_defined = false;
	uint num_assignments = 0;
	for (uint i=0; i<evals.size(); i++) {
		// Check uniform output containers for non-contexted application
		if (evals[i]->type == SystemFunc<double>::ODE || evals[i]->type == SystemFunc<double>::RULE || evals[i]->type == SystemFunc<double>::EQN) {
			num_assignments++;
			if ( ! target_defined) {
				target_granularity = evals[i]->global_symbol->granularity();
				target_defined = true;
			}
			else if (evals[i]->global_symbol->granularity() != target_granularity) {
				throw string("System: Fatal Error! Output symbols in System of Equations have non-uniform granularity!");
			}
		}
	}
	for (uint i=0; i<vec_evals.size(); i++) {
		if (vec_evals[i]->type == SystemFunc<VDOUBLE>::RULE || vec_evals[i]->type == SystemFunc<VDOUBLE>::EQN) {
			num_assignments++;
			if ( ! target_defined) {
				target_granularity = vec_evals[i]->global_symbol->granularity();
				target_defined = true;
			}
			else if (vec_evals[i]->global_symbol->granularity() != target_granularity) {
				throw string("System: Fatal Error! Output symbols in System of Equations have non-uniform granularity!");
			}
		}
	}
	
	/////////////////////////////////////////
	//        Creating blueprint solver for thread 1
	/////////////////////////////////////////
	solvers.clear();
	if (!target_defined) {
		setTimeStep(-1);
// 		return;
	}
		
	solvers.push_back(make_shared<SystemSolver>(local_scope, evals, vec_evals, solver_spec));

}

bool System::adaptive() const { return solver_spec.method == SystemSolver::Method::AdaptiveCK || solver_spec.method == SystemSolver::Method::AdaptiveDP || solver_spec.method == SystemSolver::Method::AdaptiveBS;};

void System::computeToBuffer(const SymbolFocus& f)
{
	computeToTarget(f,true);
}

void System::applyBuffer(const SymbolFocus& f)
{
	for (uint i =0; i<equations.size(); i++) {
		equations[i]->global_symbol->applyBuffer(f);
	}
	for (uint i =0; i<vec_equations.size(); i++) {
		vec_equations[i]->global_symbol->applyBuffer(f);
	}
}

void System::applyBuffer(const SymbolFocus& f, const vector<double>& buffer)
{
	uint b=0;
	for (uint i =0; i<equations.size(); i++) {
		equations[i]->global_symbol->set(f, buffer[b++]);
	}
	for (uint i =0; i<vec_equations.size(); i++) {
		vec_equations[i]->global_symbol->set(f,VDOUBLE(buffer[b],buffer[b+1],buffer[b+2]));
		b+=3;
	}
}

void System::computeToTarget(const SymbolFocus& f, bool use_buffer, vector<double>* buffer)
{
	auto solv_num = omp_get_thread_num();

	if (solv_num>=solvers.size() || ! solvers[solv_num]) {
		mutex.lock();
		// Accomodate the solver
		if (solv_num>=solvers.size())
			solvers.resize(solv_num+1);
		try {
			// Create and place the solver
			if (! solvers[solv_num]) {
				if (!solvers[0]) return;
				solvers[solv_num] = make_shared<SystemSolver>(*solvers[0]);
			}
		}
		catch (const string& e){ cerr << "Could not clone solver!\n" << e << endl; exit(-1); }
		catch (...){ cerr << "Could not clone solver!\n"<< endl; exit(-1); }
		mutex.unlock();
	}
	solvers[solv_num]->solve(f, use_buffer, buffer);
}

void System::compute(const SymbolFocus& f)
{
	computeToTarget(f,false);
}

void System::computeContextToBuffer()
{
	FocusRange range(target_granularity, target_scope);
	if (range.size() > 50) {
#pragma omp parallel for schedule(static)
		for (auto focus = range.begin(); focus<range.end(); ++focus) {
			computeToBuffer(*focus);
		}
	}
	else {
		for (const auto& focus :range ) {
			computeToBuffer(focus);
		}
	}
}

void System::applyContextBuffer()
{
	for (uint i =0; i<equations.size(); i++) {
		equations[i]->global_symbol->applyBuffer();
	}
	for (uint i =0; i<vec_equations.size(); i++) {
		vec_equations[i]->global_symbol->applyBuffer();
	}
}

void System::setTimeStep ( double ht )
{
	solver_spec.time_step = ht * solver_spec.time_scaling;
	for (uint i=0; i<solvers.size(); i++) 
		solvers[i]->setTimeStep(solver_spec.time_step);
}

set< SymbolDependency > System::getDependSymbols()
{
	set<SymbolDependency> s = solvers[0]->getExternalDependencies();
	
	auto time = find_if(s.begin(), s.end(), [](const SymbolDependency& dep) { return dep->name() == SymbolBase::Time_symbol; });
	if (time != s.end())
		s.erase(time);
	
	return s;
}

set< SymbolDependency > System::getOutputSymbols()
{
	set<SymbolDependency> s;
	for (uint i=0; i<equations.size(); i++) {
		s.insert(equations[i]->global_symbol);
	}
	for (uint i=0; i<vec_equations.size(); i++) {
		s.insert(vec_equations[i]->global_symbol);
	}
	return s;
}


bool injectGaussionNoiseScaling(string& expression, SystemSolver::Method solver_method)
{
	// Setup proper random noise generators for gaussian noise
	size_t pos=0; bool modified_expr=false;
	while ( (pos=expression.find(sym_RandomNorm,pos)) != string::npos ) {
		// is a real function call
		pos += sym_RandomNorm.size();

		if (expression[pos] == '(') {
			if ( solver_method != SystemSolver::Method::Euler ) {
				cerr << "System: ODE contains stochastic term, but no Runge-Kutta solver for stochastic differential equations is available (choose stochastic method (Euler-Maruyama) instead)."<< endl;
				// TODO Should throw
				exit(-1);
			}
			int level = 1;
			size_t next = pos;
			while (level > 0) {
				next = expression.find_first_of("()",next+1);
				if (next == string::npos ) {
					// TODO Throw non-matching parentheses @pos
				}
				else if (expression[next] == '(') {
					level++;
				}
				else if (expression[next] == ')') {
					level--;
				}
			}
			expression.insert(next, string(",")+SystemSolver::noise_scaling_symbol);
			modified_expr=true; 
		}
	}
	
	return modified_expr;
}


SystemSolver::SystemSolver(Scope* scope, const std::vector< shared_ptr<SystemFunc< double >> >& fun, const std::vector< shared_ptr<SystemFunc< VDOUBLE >> >& vfun, SystemSolver::Spec spec)
{
	this->spec = spec;
// Situation: This constructor is called after loading all functionals and 
// optionally retrieval of the required external symbols
	this->cache = make_shared<EvaluatorCache>(scope);
	// override the global time with a solver local scaled time
	local_time_idx = cache->addLocal(SymbolBase::Time_symbol,0.0);
	// override the noise_scaling with a solver local
	noise_scaling_idx = cache->addLocal(SystemSolver::noise_scaling_symbol,0.0);
	
	for (uint i=0; i<fun.size(); i++) {
		auto& eval =fun[i];
		switch (eval->type) {
			case SystemFunc<double>::FUN :
				// also offer non-parametric functions like initialized locals
				if (!eval->fun_params_names.empty()) break;
			case SystemFunc<double>::VAR_INIT :
				eval->cache_idx = cache->getLocalIdx(eval->symbol_name);
				if (eval->cache_idx==-1)
					eval->cache_idx = cache->addLocal(eval->symbol_name,0.0);
				break;
			case SystemFunc<double>::ODE:
				eval->rate_cache_idx = cache->getLocalIdx(eval->symbol_name);
				if (eval->rate_cache_idx==-1) {
// 					cout << "Adding " << eval->symbol_name+".rate" << endl;
					eval->rate_cache_idx = cache->addLocal(eval->symbol_name+".rate",0.0);
				}
				break;
			default: break;
		}
	}
	
	// Copy the functionals and wire them to the local cache
	for (uint i=0; i<fun.size(); i++) {
		auto eval = fun[i]->clone(cache);
		evals.push_back(eval);
		eval->evaluator = make_shared<ExpressionEvaluator<double> >(eval->expression, cache);
		switch (eval->type) {
			case SystemFunc<double>::VAR_INIT :
				var_initializers.push_back(eval);
				break;
			case SystemFunc<double>::FUN :
				functions.push_back(eval);
				// register the Functions parameters
				eval->initFunction();
				if (eval->fun_params_names.empty()) var_initializers.push_back(eval);
				break;
			case SystemFunc<double>::RULE :
				rules.push_back(eval);
				if (!eval->global_symbol) eval->global_symbol = scope->findRWSymbol<double>(eval->symbol_name);
				eval->cache_idx = cache->addLocal(eval->symbol_name,0.0);
				break;
			case SystemFunc<double>::ODE:
				odes.push_back(eval);
				if (!eval->global_symbol) eval->global_symbol = scope->findRWSymbol<double>(eval->symbol_name);
				eval->cache_idx = cache->addLocal(eval->symbol_name,0.0);
				eval->evaluator->parser->DefineFun( sym_RandomNorm,  &getRandomNormValueSDE,  false);
				if (injectGaussionNoiseScaling(eval->expression, spec.method))
					cout << "System: Adding scaling to noise function in SystemSolver: " << eval->expression << endl;
				break;
			case SystemFunc<double>::EQN:
				cout << "SystemSolver::  Registering equation hook  " << eval->symbol_name << "=" << eval->expression << endl;
				equations.push_back(eval);
				if (!eval->global_symbol) eval->global_symbol = scope->findRWSymbol<double>(eval->symbol_name);
				eval->cache_idx = cache->addLocal(eval->symbol_name,0.0);
				break;
		}
	}
	// Copy the vector functionals and wire them to the local cache
	for (uint i=0; i<vfun.size(); i++) {
		auto eval = vfun[i]->clone(cache);
		vec_evals.push_back(eval);
		eval->evaluator = make_shared<ExpressionEvaluator<VDOUBLE> >(eval->expression, cache);
		
		switch (eval->type) {
			case SystemFunc<VDOUBLE>::VAR_INIT :
				vec_var_initializers.push_back(eval);
				eval->cache_idx = cache->addLocal(eval->symbol_name,VDOUBLE(0,0,0));
				break;
			case SystemFunc<VDOUBLE>::FUN :
				throw string("VectorFunctions not supported");
// 				vec_functions.push_back(eval);
				break;
			case SystemFunc<VDOUBLE>::RULE :
				vec_rules.push_back(eval);
				if (!eval->global_symbol) eval->global_symbol = scope->findRWSymbol<VDOUBLE>(eval->symbol_name);
				eval->cache_idx = cache->addLocal(eval->symbol_name,VDOUBLE(0,0,0));
				break;
			case SystemFunc<VDOUBLE>::ODE:
				throw string("Vector differential equations not supported");
				break;
			case SystemFunc<VDOUBLE>::EQN:
				vec_equations.push_back(eval);
				if (!eval->global_symbol) eval->global_symbol = scope->findRWSymbol<VDOUBLE>(eval->symbol_name);
				eval->cache_idx = cache->addLocal(eval->symbol_name,VDOUBLE(0,0,0));
				break;
				
		}
	}
	
	
	// All variables are declared, all evaluators created
	for (auto& eval : evals) {
		// register local functions
		for (const auto& fun : functions) {
			eval->evaluator->parser->DefineFun(fun->symbol_name, fun->getCallBack());
// 			cout << "Registered function " << fun->symbol_name << " with " << fun->getCallBack()->argc() << " parameters." << endl;
		}
		// Initialize the evaluator aka parsing the expression
		assert(eval->evaluator);
		// Init the evaluators, but do not attach to the cache yet since more evaluators are to come and modify the cache.
		eval->evaluator->init();
		// Grepping the dependencies
		auto deps = eval->evaluator->getDependSymbols();
		std::transform(deps.begin(), deps.end(), std::inserter(eval->dep_symbols,eval->dep_symbols.begin()), [](const Symbol& sym) { return sym->name();});
		
		// Fail on non-gaussion Noise in ODEs
		// Noise requires proper time scaling
		if ( eval->type == SystemFunc<double>::ODE) {
			auto used_fun = eval->evaluator->parser->GetUsedFun();
			if (used_fun.count(sym_RandomBool) || used_fun.count(sym_RandomGamma) || used_fun.count(sym_RandomUni) || used_fun.count(sym_RandomInt) ) {
				throw string("System: stochastic ODEs may only contain normal-distributed noise \"") + sym_RandomNorm + "()\".";
			}
		}
		
// 		cout << eval->symbol_name << " at cache pos " << eval->cache_idx << endl;
 	}
	for (auto& eval :vec_evals) {
		// register local functions
		for (const auto& fun : functions) {
			eval->evaluator->parser->DefineFun(fun->symbol_name, fun->getCallBack());
		}
		// Initialize the evaluator aka parsing the expression
		assert(eval->evaluator);
		// Init the evaluators, but do not attach to the cache yet since more evaluators are to come and modify the cache.
		eval->evaluator->init();
		// Grepping the dependencies
		auto deps = eval->evaluator->getDependSymbols();
		std::transform(deps.begin(), deps.end(), std::inserter(eval->dep_symbols,eval->dep_symbols.begin()), [](const Symbol& sym) { return sym->name();});
	}
// 	// Now attach to the cache, which will become fixed in layout
// 	for (auto& eval : evals) { eval->evaluator->attach(); }
// 	for (auto& eval : vec_evals) { eval->evaluator->attach(); }
	
	//////////////////////////////////////////
	//   Reorder var_initializers using their interdependency
	//////////////////////////////////////////
	
	cout << "System::init: Reordering Initializers ..." << endl;
	set<string> unresolved_init_symbols;
	struct Initializer { bool vec; shared_ptr<SystemFunc<double>> dfun; shared_ptr<SystemFunc<VDOUBLE>> vfun; set<string> dep_symbols; int order;};
	vector<Initializer> initializers;
	for (uint i=0; i<var_initializers.size();i++) {
		unresolved_init_symbols.insert(var_initializers[i]->symbol_name);
		initializers.push_back({false,var_initializers[i],nullptr,var_initializers[i]->dep_symbols,-1});
	}
	for (uint i=0; i<vec_var_initializers.size();i++) {
		unresolved_init_symbols.insert(vec_var_initializers[i]->symbol_name);
		unresolved_init_symbols.insert(vec_var_initializers[i]->symbol_name+".x");
		unresolved_init_symbols.insert(vec_var_initializers[i]->symbol_name+".y");
		unresolved_init_symbols.insert(vec_var_initializers[i]->symbol_name+".z");
		initializers.push_back({true,nullptr, vec_var_initializers[i],vec_var_initializers[i]->dep_symbols,-1});
	}

	vector<string> res(unresolved_init_symbols.size());
	
	for (uint i=0;i<initializers.size();i++) {
		uint j;
		for (j=0; j<initializers.size();j++) {
			if (initializers[j].order==-1) {
				
				if ( set_intersection(
						initializers[j].dep_symbols.begin(),initializers[j].dep_symbols.end(),
						unresolved_init_symbols.begin(),unresolved_init_symbols.end(), res.begin()) 
						== res.begin() ) 
				{
					if (! initializers[i].vec)
						unresolved_init_symbols.erase(initializers[i].dfun->symbol_name);
					else {
						unresolved_init_symbols.erase(initializers[i].vfun->symbol_name);
						unresolved_init_symbols.erase(initializers[i].vfun->symbol_name+".x");
						unresolved_init_symbols.erase(initializers[i].vfun->symbol_name+".y");
						unresolved_init_symbols.erase(initializers[i].vfun->symbol_name+".z");
					}
					initializers[j].order = i;
					break;
				}
			}
		}
		if (j==initializers.size()) {
			cerr << "System: Detected a dependency loop in Initializers:" << endl;
			for (j=0; j<initializers.size();j++) {
				if (initializers[j].order==-1) {
					if (initializers[i].vec)
						cerr << initializers[j].vfun->symbol_name  << " = " <<  initializers[j].vfun->expression << endl;
					else
						cerr << initializers[j].dfun->symbol_name  << " = " <<  initializers[j].dfun->expression << endl;
				}
			}
			exit(-1);
		}
	}
	sort(initializers.begin(),initializers.end(), [](const Initializer& l,const Initializer& r) { return l.order < r.order; });
	
	vec_var_initializers.clear();
	var_initializers.clear();
	for (const auto& init: initializers) {
		if (init.vec)
			vec_var_initializers.push_back(init.vfun);
		else
			var_initializers.push_back(init.dfun);
	}
};

SystemSolver::SystemSolver(const SystemSolver& other)
{
	cache = make_shared<EvaluatorCache>(*other.cache);
	local_time_idx = other.local_time_idx;
	noise_scaling_idx = other.noise_scaling_idx;
	spec = other.spec;
	
	// Copy the functionals and wire them to the local cache
	for (uint i=0; i<other.evals.size(); i++) {
		
		auto e = other.evals[i]->clone(cache);
		evals.push_back(e);
		switch (e->type) {
			case SystemFunc<double>::VAR_INIT :
				var_initializers.push_back(e);
				break;
			case SystemFunc<double>::FUN :
				e->initFunction();
				functions.push_back(e);
				if (e->fun_params_names.empty()) var_initializers.push_back(e);
				break;
			case SystemFunc<double>::RULE :
				rules.push_back(e);
				break;
			case SystemFunc<double>::ODE:
				odes.push_back(e);
				break;
			case SystemFunc<double>::EQN:
				equations.push_back(e);
				break;
		}
	}
	// Copy the vector functionals and wire them to the local cache
	for (uint i=0; i<other.vec_evals.size(); i++) {
		
		auto e = other.vec_evals[i]->clone(cache);
		vec_evals.push_back(e);
		switch (e->type) {
			case SystemFunc<VDOUBLE>::VAR_INIT :
				vec_var_initializers.push_back(e);
				break;
			case SystemFunc<VDOUBLE>::FUN :
// 				vec_functions.push_back(e);
				break;
			case SystemFunc<VDOUBLE>::RULE :
				vec_rules.push_back(e);
				break;
			case SystemFunc<VDOUBLE>::ODE:
				throw string("Vector differential equations not supported");
				break;
			case SystemFunc<VDOUBLE>::EQN:
				vec_equations.push_back(e);
				break;
		}
	}
	// TODO Update all local function declarations in the solvers to use this solvers function handlers,
	// connencted to the solver local cache.
	for (auto eval : evals) {
		for (auto fun_def : functions) {
			eval->evaluator->parser->UpdateFun(fun_def->symbol_name, fun_def->getCallBack());
		}
	}
	for (auto eval : vec_evals) {
		for (auto fun_def : functions) {
			eval->evaluator->parser->UpdateFun(fun_def->symbol_name, fun_def->getCallBack());
		}
	}
// 	cout << "+"; cout.flush();
};

void SystemSolver::setTimeStep(double ht) {
	spec.time_step = ht;
}

void SystemSolver::solve(const SymbolFocus& f, bool use_buffer, vector<double>* ext_buffer) {
	// Fetch data
	fetchSymbols(f);

	switch (spec.method) {
		case Method::Euler: Euler(f, spec.time_step); break;
		case Method::Heun : Heun(f, spec.time_step); break;
		case Method::Runge_Kutta4 : RungeKutta(f, spec.time_step); break;
		case Method::Discrete : Discrete(f); break;
		case Method::AdaptiveCK :
		case Method::AdaptiveDP :
		case Method::AdaptiveBS :
			RungeKutta_adaptive(f,spec.time_step); break;
		default:
			throw MorpheusException("Solver method not implemented in solve().");
	}
	// Write back
	if (use_buffer) {
		if (ext_buffer)
			writeSymbolsToExtBuffer(ext_buffer);
		else
			writeSymbolsToBuffer(f);
	}
	else
		writeSymbols(f);
}

void SystemSolver::fetchSymbols(const SymbolFocus& f)
{
	cache->fetch(f);
	
	for (const auto& eval : evals) {
		if (eval->type == SystemFunc<double>::ODE || eval->type == SystemFunc<double>::RULE || eval->type == SystemFunc<double>::EQN )  {
			cache->setLocal(eval->cache_idx, eval->global_symbol->get(f));
		}
	}
	for (const auto& eval : vec_rules) {
		if (eval->type == SystemFunc<VDOUBLE>::RULE || eval->type == SystemFunc<VDOUBLE>::EQN)
			cache->setLocal(eval->cache_idx, eval->global_symbol->get(f));
	}
	
	// adjust time for time scaling
	cache->setLocal(local_time_idx, SIM::getTime() * spec.time_scaling);
	
	updateLocalVars(f);
// 	cout << cache->toString() << endl;
}

void SystemSolver::writeSymbols(const SymbolFocus& f)
{
	for (const auto& eval : evals) {
		if (eval->type == SystemFunc<double>::ODE || eval->type == SystemFunc<double>::RULE /*|| eval->type == SystemFunc<double>::EQN*/) 
			eval->global_symbol->set(f,cache->getLocalD(eval->cache_idx ));
	}
	for (const auto& eval : vec_rules) {
		eval->global_symbol->set(f,cache->getLocalV(eval->cache_idx ));
	}
}

void SystemSolver::writeSymbolsToBuffer(const SymbolFocus& f)
{
// 	cout << "writeSymbolsToBuffer " << cache->toString() << endl;
	for (const auto& eval : evals) {
		if (eval->type == SystemFunc<double>::ODE || eval->type == SystemFunc<double>::RULE /*|| eval->type == SystemFunc<double>::EQN */) {
			eval->global_symbol->setBuffer(f,cache->getLocalD(eval->cache_idx ));
		}
	}
	for (const auto& eval : vec_rules) {
		if (eval->type == SystemFunc<VDOUBLE>::RULE)
			eval->global_symbol->setBuffer(f,cache->getLocalV(eval->cache_idx ));
	}
}

void SystemSolver::writeSymbolsToExtBuffer(vector<double>* ext_buffer) {
	ext_buffer->clear();
	for (const auto& eval : evals) {
		if (eval->type == SystemFunc<double>::ODE || eval->type == SystemFunc<double>::RULE/* || eval->type == SystemFunc<double>::EQN*/) 
			ext_buffer->push_back(cache->getLocalD(eval->cache_idx ));
	}
	for (const auto& eval : vec_rules) {
		auto val = cache->getLocalV(eval->cache_idx);
		ext_buffer->push_back(val.x);
		ext_buffer->push_back(val.y);
		ext_buffer->push_back(val.z);
	}
}

void SystemSolver::updateLocalVars(const SymbolFocus& f) {
	for (const auto& ini : var_initializers) {
		cache->setLocal(ini->cache_idx, ini->evaluator->get(f));
	}
	for (const auto& ini : vec_var_initializers) {
		cache->setLocal(ini->cache_idx, ini->evaluator->get(f));
	}
	EquationHooks(f,false);
}


void SystemSolver::check_result(double value , const SystemFunc<double>& e) const {
	if ( std::isnan( value ) || std::isinf( value ) ){
		stringstream s;
		s << "SystemSolver returned " << (std::isnan( value ) ? "NaN" : "Inf") << " for expression '" << e.expression << "'." << endl;
		const mu::varmap_type& symbols = e.evaluator->parser->GetUsedVar();
		for (auto ii  : symbols) s << ii.first << "=" << *(ii.second) << "; "; 
		s << endl;
		if (std::isinf( value ))
			throw ExpressionException(s.str(), ExpressionException::ErrorType::INF);
		else
			throw ExpressionException(s.str(), ExpressionException::ErrorType::NaN);
	}
}

void SystemSolver::check_result(const VDOUBLE& value , const SystemFunc<VDOUBLE>& e) const {
	if ( std::isnan( value.x ) || std::isinf( value.x ) ||  std::isnan( value.y ) || std::isinf( value.y ) ||  std::isnan( value.z ) || std::isinf( value.z ) ){
		stringstream s;
		s << "SystemSolver returned NaN/Inf for expression '" << e.expression << "'." << endl;
		const mu::varmap_type& symbols = e.evaluator->parser->GetUsedVar();
		for (auto ii  : symbols) s << ii.first << "=" << *(ii.second) << "; "; 
		s << endl;
		if (std::isinf( value.x ) || std::isinf( value.y ) || std::isinf( value.z ) )
			throw ExpressionException(s.str(), ExpressionException::ErrorType::INF);
		else
			throw ExpressionException(s.str(), ExpressionException::ErrorType::NaN);
	}
}

void SystemSolver::EquationHooks(const SymbolFocus& f, bool write_extern) {
	// equations is not what it appears to be ....
	if (equations.empty() && vec_equations.empty())
		return;
	
	// Sequential processing of Equation hooks
	for (uint i=0; i < equations.size(); i++ ) {
		SystemFunc<double> &e = *equations[i];
		e.k0 = e.evaluator->get(f); check_result(e.k0,e);
		if (e.global_symbol->flags().delayed) {
			if (e.global_symbol->granularity() == Granularity::Cell) {
				auto delay = static_pointer_cast<const DelayPropertySymbol>(e.global_symbol);
				auto global_time = cache->getLocalD(local_time_idx) / spec.time_scaling;
				if (write_extern) delay->set(f, global_time, e.k0);
				this->cache->setLocal(e.cache_idx, delay->get(f, global_time));
// 				cout << "Setting " << delay->name() << " = {"<< global_time << "->" << e.k0<< "}" << endl;
			}
			else if (e.global_symbol->granularity() == Granularity::Global) {
				auto delay = static_pointer_cast<const DelayVariableSymbol>(e.global_symbol);
				auto global_time = cache->getLocalD(local_time_idx) / spec.time_scaling;
				if (write_extern) delay->set(f, global_time, e.k0);
				this->cache->setLocal(e.cache_idx, delay->get(f, global_time));
// 				cout << "Setting " << delay->name() << " = {"<< global_time << "->" << e.k0<< "}" << endl;
			}
			else 
				throw string("Unsupported granularity for delayed Symbol ") + e.global_symbol->name() + " of type " + e.global_symbol->linkType();
		}
		else {
// 			cout << "Setting " << e.symbol_name << " = {"<< cache->getLocalD(local_time_idx) << "->" << e.k0<< "}" << endl;
			this->cache->setLocal(e.cache_idx, e.k0);
		}
	}
	for (uint i=0; i < vec_equations.size(); i++) {
		SystemFunc<VDOUBLE> &e = *vec_equations[i];
		e.k0 = e.evaluator->get(f); check_result(e.k0,e);
		this->cache->setLocal(e.cache_idx, e.k0);
	}
}

void SystemSolver::Discrete(const SymbolFocus& f) {
	
	if (rules.empty() && vec_rules.empty())
		return;
	
	for (uint i=0; i < rules.size(); i++){
		SystemFunc<double> &e = *rules[i];
		e.k1 = e.evaluator->get(f); check_result(e.k1,e);
	}
	for (uint i=0; i < vec_rules.size(); i++) {
		SystemFunc<VDOUBLE> &e = *vec_rules[i];
		e.k1 = e.evaluator->get(f); check_result(e.k1,e);
	}
	// Assign locals
	for (uint i=0; i < rules.size(); i++){
		this->cache->setLocal(rules[i]->cache_idx, rules[i]->k1);
	}
	for (uint i=0; i < vec_rules.size(); i++) {
		auto& e = *vec_rules[i];
		this->cache->setLocal(e.cache_idx, VDOUBLE::from(e.k1, e.notation));
	}
}

void SystemSolver::Euler(const SymbolFocus& f, double ht) {
	
	cache->setLocal(noise_scaling_idx, sqrt(1.0/ht));

	for (uint i=0; i < odes.size(); i++){
		SystemFunc<double> &e = *odes[i];
		e.k0 = cache->getLocalD(e.cache_idx);
		e.k1 = e.evaluator->plain_get(f); check_result(e.k1,e);
	}
	for (uint i=0; i < odes.size(); i++){
		SystemFunc<double> &e = *odes[i];
		e.dy = e.k1;
		cache->setLocal(e.rate_cache_idx, e.dy);
		cache->setLocal(e.cache_idx, e.k0 + ht * e.dy);
	}
	cache->setLocal(local_time_idx, cache->getLocalD(local_time_idx) + ht);
	this->Discrete(f);
}

void SystemSolver::Heun(const SymbolFocus& f, double ht) {
	// constants of the butcher tableau of the classical 2nd order scheme
// 	const double a21=1;
// 	const double b1=0.5, b2=0.5;
// 	const double c2=1.0;
// 	// constants of the butcher tableau of the Heun method (Runge-Kutta 2th order scheme)
	const double a21=2.0/3;
	const double b1=0.25, b2=0.75;
	const double c2=2.0/3;
	
	cache->setLocal(noise_scaling_idx, sqrt(1.0/ht));

	// First Step Interpolation
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc<double> &e = *odes[i];
		e.k0 = cache->getLocalD(e.cache_idx);
		e.k1 = e.evaluator->plain_get(f); check_result(e.k1,e);
	}

	// Second Step interpolation
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc<double> &e = *odes[i];
		cache->setLocal(e.cache_idx, e.k0 + ht * a21 * e.k1);
	}
	cache->setLocal(local_time_idx, cache->getLocalD(local_time_idx) + c2 * ht);
	updateLocalVars(f);
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc<double> &e = *odes[i];
		e.k2 = e.evaluator->plain_get(f); check_result(e.k2,e);
	}
	
	// Apply the step to the local cache
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc<double> &e = *odes[i];
		e.dy = b1*e.k1 + b2*e.k2;
		cache->setLocal(e.rate_cache_idx, e.dy);
		cache->setLocal(e.cache_idx, e.k0 + ht * e.dy );
	}
	
	this->Discrete(f);
}

void SystemSolver::RungeKutta(const SymbolFocus& f, double ht) {
	// constants of the butcher tableau of the classical Runge-Kutta 4th order scheme
// 	const double a21=0.5, a31=0.0, a32=0.5, a41=0.0, a42=0.0, a43=1.0;
// 	const double b1=1.0/6, b2=1.0/3, b3=1.0/3, b4=1.0/6;
// 	const double c2=0.5, c3=0.5, c4=1.0;
	
	// constants of the butcher tableau of the 3/8 Runge-Kutta 4th order scheme
	const double a21 = 1.0/3, a31=-1.0/3, a32=1.0, a41=1.0, a42=-1.0, a43=1.0;
	const double b1=1.0/8, b2=3.0/8, b3=3.0/8, b4=1.0/8;
	const double c2=1.0/3, c3=2.0/3, c4=1.0;

	
	// First Step interpolation

	for(uint i=0; i < odes.size(); i++) {
		SystemFunc<double> &e = *odes[i];
		e.k0 = cache->getLocalD(e.cache_idx);
		e.k1 = e.evaluator->plain_get(f); check_result(e.k1,e);
	}

	// Second Step interpolation
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc<double> &e = *odes[i];
		cache->setLocal(e.cache_idx, e.k0 + ht * a21 * e.k1);
	}
	cache->setLocal(local_time_idx, cache->getLocalD(local_time_idx) + c2*ht);
	updateLocalVars(f);
	
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc<double> &e = *odes[i];
		e.k2 = e.evaluator->plain_get(f); check_result(e.k2,e);
	}
	
	// Third Step interpolation
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc<double> &e = *odes[i];
		cache->setLocal(e.cache_idx, e.k0 + ht * (a31 * e.k1 + a32 * e.k2));
	}
	cache->setLocal(local_time_idx, cache->getLocalD(local_time_idx) + (c3-c2)*ht);
	updateLocalVars(f);
	
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc<double> &e = *odes[i];
		e.k3 = e.evaluator->plain_get(f); check_result(e.k3,e);
	}

	// Fourth Step interpolation
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc<double> &e = *odes[i];
		cache->setLocal(e.cache_idx,  e.k0 +  ht * (a41 * e.k1 + a42 * e.k2 + a43 * e.k3));
	}
	cache->setLocal(local_time_idx, cache->getLocalD(local_time_idx) + (c4-c3)*ht);
	updateLocalVars(f);
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc<double> &e = *odes[i];
		e.k4 = e.evaluator->plain_get(f);check_result(e.k4,e);
	}
	
	// Apply the step to the local cache
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc<double> &e = *odes[i];
		e.dy = b1*e.k1 + b2*e.k2 + b3*e.k3 + b4*e.k4;
		cache->setLocal(e.rate_cache_idx, e.dy);
		cache->setLocal(e.cache_idx, e.k0 + ht * e.dy );
	}
	this->Discrete(f);
}

void SystemSolver::RungeKutta_adaptive(const SymbolFocus& f, double ht) {
	double local_ht = ht;
	double total_ht = 0;
	const double tiny = 1e-15;
	const double safety = 0.9;
	const double epsilon = spec.epsilon;
	const double min_ht = 1e-30;
	const double starttime = cache->getLocalD(local_time_idx);
	
	for (;;){
		double max_err;
		max_err=0;
		try {
			if (spec.method==Method::AdaptiveCK)
				RungeKutta_45CashKarp(f,local_ht);
			else if (spec.method==Method::AdaptiveDP)
				RungeKutta_45DormandPrince(f,local_ht);
			else 
				RungeKutta_23BogackiShampine(f,local_ht);
			// compute maximum scaled error
			
			for (uint i=0; i < odes.size(); i++) {
				SystemFunc<double> &e = *odes[i];
// 				cout << "err " << to_str(e.err) << " y " << to_str(e.k0) << " dy " << to_str(e.dy) << " rel " << to_str(abs(e.err/(e.k0 + local_ht*e.dy + tiny))) << endl;
				max_err = max(max_err,abs(e.err/(e.k0 + local_ht*e.dy + tiny)));
			}
			max_err /= epsilon;
			if (max_err>1.0) {
				if ( local_ht <= min_ht) {
					throw string("Unable to integrate system. Minimal time step") + to_string(min_ht) + "reached.";
				}
// 				cout << "Error for ts " << to_str(local_ht) << " too big " << to_str(max_err) << ">" << "1.0" <<endl; 
				double new_ht = safety * local_ht * pow(max_err, -0.25);
				local_ht = max(0.1*local_ht, new_ht); // cap at 1/10 of the current time step.
// 				cout << "Downscaling to time step " << to_str(local_ht) << endl;
				continue;
			}
		}
		catch (const ExpressionException& e) {
			if (local_ht > min_ht) { // Retry
				// Reset the dirty cache
				for(uint i=0; i < odes.size(); i++) {
					SystemFunc<double> &e = *odes[i];
					cache->setLocal(e.cache_idx,e.k0);
				}
				cache->setLocal(local_time_idx, starttime+total_ht);
				// Rescale
				local_ht *= 0.1;
// 				cout << "Downscaling to time step " << to_str(local_ht) << endl;
				continue;
			}
			else 
				throw string("Unable to integrate system. Minimal time step") + to_string(min_ht) + "reached.\n" + e.what();
		}
		
		// Apply step to the cache
		for (uint i=0; i < odes.size(); i++) {
			SystemFunc<double> &e = *odes[i];
// 			if (e.k0 + e.dy < 0) {
// 				cout << "Got negative " << to_str(e.k0) << "+" << to_str(e.dy) << " ("<< to_str(e.err) << ")" <<endl;
// 			}
			cache->setLocal(e.cache_idx, e.k0 + local_ht * e.dy );
		}
		total_ht += local_ht;
		cache->setLocal(local_time_idx, starttime + total_ht);
		EquationHooks(f,true);
		
		if (total_ht >= ht) break;

		
		if (max_err < 0.75) {
			local_ht = safety * local_ht * pow(max_err, -0.20);
// 			cout << "Upscaling to time step " << to_str(local_ht) << endl;
		}
		
		// Adjust next step
		local_ht = min(local_ht, ht-total_ht);
	}
// 	cout << "Time " << starttime << " dt " << to_str(total_ht)<< endl;
	this->Discrete(f); // Execute rule based paradigms with the fixed external time step.
}

void SystemSolver::RungeKutta_23BogackiShampine(const SymbolFocus& f, double ht) {
	// constants of the butcher tableau of Runge-Kutta-45, the Cash-Karp parameterization
	// time steps
	const double c1=0.0, c2=0.5, c3=0.75, c4=1;
	// interpolation coefficients
	const double a21=0.5;
	const double a31=0, a32=0.75;
	const double a41=2.0/9, a42=1.0/3, a43=4.0/9;
	// solution coeffitients
	const double b3_1=2.0/9, b3_2=1.0/3, b3_3=4.0/9, b3_4=0;
	const double b2_1=7.0/24, b2_2=1.0/4, b2_3=1.0/3, b2_4=1.0/8;
	// solution error coefficients
	const double berr_1=b3_1-b2_1, berr_2=b3_2-b2_2, berr_3=b3_3-b2_3, berr_4=b3_4-b2_4;


	const double starttime = cache->getLocalD(local_time_idx);
	// First Step interpolation
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc<double> &e = *odes[i];
		e.k0 = cache->getLocalD(e.cache_idx);
		e.k1 = e.evaluator->plain_get(f); check_result(e.k1,e);
	}

	// Second Step interpolation
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc<double> &e = *odes[i];
		cache->setLocal(e.cache_idx, e.k0 + a21 * ht * e.k1);
	}
	cache->setLocal(local_time_idx, starttime + c2*ht);
	updateLocalVars(f);
	
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc<double> &e = *odes[i];
		e.k2 = e.evaluator->plain_get(f); check_result(e.k2,e);
	}
	
	// Third Step interpolation
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc<double> &e = *odes[i];
		cache->setLocal(e.cache_idx, e.k0 + a31 * ht * e.k1 + a32 * ht * e.k2);
	}
	cache->setLocal(local_time_idx, starttime + c3*ht);
	updateLocalVars(f);
	
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc<double> &e = *odes[i];
		e.k3 = e.evaluator->plain_get(f); check_result(e.k3,e);
	}

	// Fourth Step interpolation
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc<double> &e = *odes[i];
		e.dy = a41*e.k1 + a42*e.k2 + a43*e.k3;
		cache->setLocal(e.cache_idx,  e.k0 + ht*e.dy);
	}
	cache->setLocal(local_time_idx,  starttime + c4*ht);
	updateLocalVars(f);
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc<double> &e = *odes[i];
		e.k4 = e.evaluator->plain_get(f); check_result(e.k4,e);
	}
	
	// Estimate dy and err and reset the cache
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc<double> &e = *odes[i];
		e.err = abs(ht *( berr_1*e.k1 + berr_2*e.k2 + berr_3*e.k3 + berr_4*e.k4));
		//  e.dy = ( b3_1*e.k1 + b3_2*e.k2 + b3_3*e.k3 + b3_4*e.k4 );
		cache->setLocal(e.rate_cache_idx, e.dy);
		cache->setLocal(e.cache_idx, e.k0); // + ht * e.dy is added by the adaptive gouverner 
	}
	cache->setLocal(local_time_idx, starttime);
}

void SystemSolver::RungeKutta_45CashKarp(const SymbolFocus& f, double ht) {
	// constants of the butcher tableau of Runge-Kutta-45, the Cash-Karp parameterization
	// time steps
	const double c1=0.0, c2=0.2, c3=0.3, c4=0.6, c5=1.0, c6=7.0/8;
	// interpolation coefficients
	const double a21=0.2;
	const double a31=3.0/40, a32=9.0/40;
	const double a41=0.3, a42=-0.9, a43=1.2;
	const double a51=-11.0/54, a52=2.5, a53=-70.0/27, a54=35.0/27;
	const double a61=1631.0/55296, a62=175.0/512, a63=575.0/13824, a64=44275.0/110592, a65=253.0/4096;
	// solution coeffitients
	const double b5_1=37.0/378, b5_2=0, b5_3=250.0/621, b5_4=125.0/594, b5_5=0, b5_6=512.0/1771;
	const double b4_1=2825.0/27648, b4_2=0, b4_3=18575.0/48384, b4_4=13525.0/55296, b4_5=277.0/14336 , b4_6=1.0/4;
	// solution error coefficients
	const double berr_1=b5_1-b4_1, berr_2=b5_2-b4_2, berr_3=b5_3-b4_3, berr_4=b5_4-b4_4, berr_5=b5_5-b4_5, berr_6=b5_6-b4_6;


	const double starttime = cache->getLocalD(local_time_idx);
	// First Step interpolation
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc<double> &e = *odes[i];
		e.k0 = cache->getLocalD(e.cache_idx);
		e.k1 = e.evaluator->plain_get(f); check_result(e.k1,e);
	}

	// Second Step interpolation
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc<double> &e = *odes[i];
		cache->setLocal(e.cache_idx, e.k0 + a21 * ht * e.k1);
	}
	cache->setLocal(local_time_idx, starttime + c2*ht);
	updateLocalVars(f);
	
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc<double> &e = *odes[i];
		e.k2 = e.evaluator->plain_get(f); check_result(e.k2,e);
	}
	
	// Third Step interpolation
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc<double> &e = *odes[i];
		cache->setLocal(e.cache_idx, e.k0 + a31 * ht * e.k1 + a32 * ht * e.k2);
	}
	cache->setLocal(local_time_idx, starttime + c3*ht);
	updateLocalVars(f);
	
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc<double> &e = *odes[i];
		e.k3 = e.evaluator->plain_get(f); check_result(e.k3,e);
	}

	// Fourth Step interpolation
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc<double> &e = *odes[i];
		cache->setLocal(e.cache_idx,  e.k0 + ht* (a41*e.k1 + a42*e.k2 + a43*e.k3));
	}
	cache->setLocal(local_time_idx,  starttime + c4*ht);
	updateLocalVars(f);
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc<double> &e = *odes[i];
		e.k4 = e.evaluator->plain_get(f); check_result(e.k4,e);
	}
	
	// Fifth Step interpolation
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc<double> &e = *odes[i];
		cache->setLocal(e.cache_idx,  e.k0 + ht* (a51*e.k1 + a52*e.k2 + a53*e.k3 + a54*e.k4));
	}
	cache->setLocal(local_time_idx,  starttime + c5*ht);
	updateLocalVars(f);
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc<double> &e = *odes[i];
		e.k5 = e.evaluator->plain_get(f); check_result(e.k5,e);
	}
	
	// Sixth Step interpolation
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc<double> &e = *odes[i];
		cache->setLocal(e.cache_idx,  e.k0 + ht* (a61*e.k1 + a62*e.k2 + a63*e.k3 + a64*e.k4 + a65*e.k5));
	}
	cache->setLocal(local_time_idx,  starttime + c6*ht);
	updateLocalVars(f);
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc<double> &e = *odes[i];
		e.k6 = e.evaluator->plain_get(f); check_result(e.k6,e);
	}
	
	// Estimate dy and err and reset the cache
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc<double> &e = *odes[i];
		e.err = abs(ht *( berr_1*e.k1 + berr_2*e.k2 + berr_3*e.k3 + berr_4*e.k4 + berr_5*e.k5 + berr_6*e.k6));
		e.dy = ( b5_1*e.k1 + b5_2*e.k2 + b5_3*e.k3 + b5_4*e.k4 +  b5_5*e.k5 + b5_6*e.k6);
		cache->setLocal(e.rate_cache_idx, e.dy);
		cache->setLocal(e.cache_idx, e.k0); // + ht * e.dy is added by the adaptive gouverner 
	}
	cache->setLocal(local_time_idx, starttime);
}

void SystemSolver::RungeKutta_45DormandPrince(const SymbolFocus& f, double ht) {
	// constants of the butcher tableau of Runge-Kutta-45, the Dormand-Prince parameterization
	// time steps
	const double c1=0.0, c2=0.2, c3=0.3, c4=0.8, c5=8.0/9, c6=1, c7=1;
	// interpolation coefficients
	const double a21=0.2;
	const double a31=3.0/40, a32=9.0/40;
	const double a41=44.0/45, a42=-56.0/15, a43=32.0/9;
	const double a51=19372.0/6561, a52= -25360.0/2187, a53=64448.0/6561, a54= -212.0/729;
	const double a61=9017.0/3168, a62= -355.0/33, a63=46732.0/5247, a64=49.0/176, a65=-5103.0/18656;
	const double a71=35.0/384, a72=0, a73=500.0/1113, a74=125.0/192, a75=-2187.0/6784,a76=11.0/84;
	// solution coeffitients
	const double b5_1=35.0/384, b5_2=0, b5_3=500.0/1113, b5_4=125.0/192, b5_5=-2187.0/6784, b5_6=11.0/84, b5_7=0;
	const double b4_1=5179.0/57600, b4_2=0, b4_3=7571.0/16695, b4_4=393.0/640, b4_5=-92097.0/339200 , b4_6=187.0/2100, b4_7=1.0/40;
	// solution error coefficients
	const double berr_1=b5_1-b4_1, berr_2=b5_2-b4_2, berr_3=b5_3-b4_3, berr_4=b5_4-b4_4, berr_5=b5_5-b4_5, berr_6=b5_6-b4_6, berr_7=b5_7-b4_7;

	const double starttime = cache->getLocalD(local_time_idx);

	// First Step interpolation
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc<double> &e = *odes[i];
		e.k0 = cache->getLocalD(e.cache_idx);
		e.k1 = e.evaluator->plain_get(f); check_result(e.k1,e);
	}

	// Second Step interpolation
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc<double> &e = *odes[i];
		cache->setLocal(e.cache_idx, e.k0 + a21 * ht * e.k1);
	}
	cache->setLocal(local_time_idx, starttime + c2*ht);
	updateLocalVars(f);
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc<double> &e = *odes[i];
		e.k2 = e.evaluator->plain_get(f); check_result(e.k2,e);
	}
	
	// Third Step interpolation
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc<double> &e = *odes[i];
		cache->setLocal(e.cache_idx, e.k0 + ht *(a31*e.k1 + a32*e.k2));
	}
	cache->setLocal(local_time_idx, starttime + c3*ht);
	updateLocalVars(f);
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc<double> &e = *odes[i];
		e.k3 = e.evaluator->plain_get(f); check_result(e.k3,e);
	}

	// Fourth Step interpolation
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc<double> &e = *odes[i];
		cache->setLocal(e.cache_idx,  e.k0 + ht* (a41*e.k1 + a42*e.k2 + a43*e.k3));
	}
	cache->setLocal(local_time_idx, starttime + c4*ht);
	updateLocalVars(f);
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc<double> &e = *odes[i];
		e.k4 = e.evaluator->plain_get(f); check_result(e.k4,e);
	}
	
	// Fifth Step interpolation
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc<double> &e = *odes[i];
		cache->setLocal(e.cache_idx,  e.k0 + ht* (a51*e.k1 + a52*e.k2 + a53*e.k3 + a54*e.k4));
	}
	cache->setLocal(local_time_idx, starttime + c5*ht);
	updateLocalVars(f);
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc<double> &e = *odes[i];
		e.k5 = e.evaluator->plain_get(f); check_result(e.k5,e);
	}
	
	// Sixth Step interpolation
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc<double> &e = *odes[i];
		cache->setLocal(e.cache_idx,  e.k0 + ht* (a61*e.k1 + a62*e.k2 + a63*e.k3 + a64*e.k4 + a65*e.k5));
	}
	cache->setLocal(local_time_idx, starttime + c6*ht);
	updateLocalVars(f);
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc<double> &e = *odes[i];
		e.k6 = e.evaluator->plain_get(f); check_result(e.k6,e);
	}
	
	// Seventh Step interpolation
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc<double> &e = *odes[i];
		e.dy = (a71*e.k1 + a72*e.k2 + a73*e.k3 + a74*e.k4 + a75*e.k5 + a76*e.k6);
		cache->setLocal(e.cache_idx,  e.k0 + ht*e.dy);
	}
	cache->setLocal(local_time_idx, starttime + c7*ht);
	updateLocalVars(f);
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc<double> &e = *odes[i];
		e.k7 = e.evaluator->plain_get(f); check_result(e.k7,e);
	}
	
	// Estimate dy and err and reset the cache
	for(uint i=0; i < odes.size(); i++) {
		SystemFunc<double> &e = *odes[i];
		e.err = abs(ht *( berr_1*e.k1 + berr_2*e.k2 + berr_3*e.k3 + berr_4*e.k4 +  berr_5*e.k5 + berr_6*e.k6+ berr_7*e.k7));
		// e.dy = ( b5_1*e.k1 + b5_2*e.k2 + b5_3*e.k3 + b5_4*e.k4 +  b5_5*e.k5 + b5_6*e.k6 + b5_7*e.k7);
		cache->setLocal(e.rate_cache_idx, e.dy);
		cache->setLocal(e.cache_idx,e.k0); // reset the cache value, ht * e.dy is added by the adaptive gouverner 
	}
	cache->setLocal(local_time_idx, starttime);
}

void DiscreteSystem::init(const Scope* scope)
{
	InstantaneousProcessPlugin::init(scope);
	System::init();
	registerInputSymbols(System::getDependSymbols());
	registerOutputSymbols(System::getOutputSymbols());
}

EventSystem::EventSystem() : System(DISCRETE), InstantaneousProcessPlugin( TimeStepListener::XMLSpec::XML_OPTIONAL ) {
	delay.setDefault("0");
	delay.setXMLPath("delay");
	registerPluginParameter(delay);
	delay_compute.setDefault("on-trigger");
	delay_compute.setXMLPath("compute-time");
	delay_compute.setConversionMap({{"on-trigger",false},{"on-execution",true}});
	registerPluginParameter(delay_compute);
	trigger_on_change.setDefault("on change");
	trigger_on_change.setXMLPath("trigger");
	trigger_on_change.setConversionMap({{"when true",false},{"on change",true}});
	registerPluginParameter(trigger_on_change);
	xml_condition_history.setXMLPath("Condition/history");
	xml_condition_history.setDefault("false");
	registerPluginParameter(xml_condition_history);
	persistent.setXMLPath("persistent");
	persistent.setDefault("true");
	registerPluginParameter(persistent);
};

void EventSystem::loadFromXML ( const XMLNode node, Scope* scope )
{
    TimeStepListener::loadFromXML ( node, scope );
	registerInputSymbol(SIM::findGlobalSymbol<double>(SymbolBase::Time_symbol));

	// Allow manual adjustment that cannot be overridden
	if (InstantaneousProcessPlugin::timeStep()>0)
		this->is_adjustable = false;

	if (node.nChildNode("Condition")) {
		string expression;
		if (getXMLAttribute(node,"Condition/text",expression))
			condition = make_shared<ThreadedExpressionEvaluator<double> >(expression,scope);

		XMLNode xSystem = node.deepCopy();
		XMLNode xCondition = XMLNode::createXMLTopNode("Trash");
		while (xSystem.nChildNode("Condition")) {
			xCondition.addChild(xSystem.getChildNode("Condition"));
		}
		System::loadFromXML(xSystem, scope);
	}
	else {
		System::loadFromXML(node, scope);
	}
	
}

void EventSystem::init ( const Scope* scope)
{
	InstantaneousProcessPlugin::init (scope);
	System::init();
	
	if (condition)
		condition->init();
	
	if (condition) {
		registerInputSymbols(condition->getDependSymbols());
	}
	
	registerInputSymbols(System::getDependSymbols()); // TODO Should be a non-scheduling dependency
	registerOutputSymbols(System::getOutputSymbols());
}

void EventSystem::executeTimeStep()
{
	auto time = SIM::getTime();
	FocusRange range(target_granularity, target_scope);
	if (trigger_on_change()) {
#pragma omp parallel for schedule(static)
		for (auto focus = range.begin(); focus<range.end(); ++focus) {
			
			double cond = condition->get(*focus);
			auto history_val = condition_history.find(*focus);
			
			double trigger = ( history_val != condition_history.end()? history_val->second <= 0.0 :  ! xml_condition_history()) && cond;
			
			if (trigger) {
#pragma omp critical
				{
					double d = delay(*focus);
	// 				cout << "Delay " << d << " at " << to_str(time) << endl;
					if (d == 0) {
						compute(*focus);
					}
					else if (delay_compute()) {
						DelayedAssignment assignment {*focus,{}};
						delayed_assignments.insert({time + d,assignment});
					}
					else {
						DelayedAssignment assignment {*focus,{}};
						computeToTarget(*focus,true, &assignment.value_cache);
						delayed_assignments.insert({time + d,assignment});
					}
					if (history_val != condition_history.end()) {
						history_val->second = cond;
					}
					else 
						condition_history[*focus] = cond;
				}
			}
		}
	}
	else {
// 		if (range.size()>50) {
#pragma omp parallel for schedule(static)
			for (auto focus = range.begin(); focus<range.end(); ++focus) {
				if (condition->get(*focus))
					compute(*focus);
			}
// 		}
// 		else {
// 			for (auto focus : range) {
// 				if (condition->get(focus))
// 					compute(focus);
// 			}
// 		}
	}
	
	if (!delayed_assignments.empty()) {
		auto last = delayed_assignments.upper_bound(time);
		if (last == delayed_assignments.begin())  {
			return;
		}
		
		for (auto assignment = delayed_assignments.begin(); assignment!=last;) {
			if (persistent() || (condition->get(assignment->second.focus) > 0) ) {
// 				cout << "Delay exec at " << to_str(time) << endl;
				if (delay_compute()) {
					compute(assignment->second.focus);
				}
				else {
					applyBuffer(assignment->second.focus, assignment->second.value_cache);
				}
			}
			delayed_assignments.erase(assignment++);
		}
	}
};

void ContinuousSystem::init(const Scope* scope) {
	ContinuousProcessPlugin::init(scope);
	System::init();
	// The TimeStepListener was reading the timeStep in system time, thus has to be readjusted to the global time !!
	if (ContinuousProcessPlugin::timeStep()>0) {
	ContinuousProcessPlugin::setTimeStep(ContinuousProcessPlugin::timeStep() / solver_spec.time_scaling);
	}
	else {
		if ( ! System::adaptive() ) {
			throw MorpheusException(string("Selected Solver requires a time-step definition ") + to_str(ContinuousProcessPlugin::timeStep()),stored_node);
		}
		ContinuousProcessPlugin::setTimeStep(SIM::getStopTime());
	}
	
	
	
	if (System::adaptive()) {
		is_adjustable = true;
	}
	System::setTimeStep(TimeStepListener::timeStep());
	registerInputSymbols(System::getDependSymbols());
	registerOutputSymbols(System::getOutputSymbols());
	Plugin::local_scope = System::local_scope;
};


void ContinuousSystem::loadFromXML(const XMLNode node, Scope* scope) {
	ContinuousProcessPlugin::loadFromXML(node, scope);
	System::loadFromXML(node, scope);
};



// Register the class instances with the respective XML Tags!
REGISTER_PLUGIN(ContinuousSystem);
REGISTER_PLUGIN(DiscreteSystem);
REGISTER_PLUGIN(EventSystem);
