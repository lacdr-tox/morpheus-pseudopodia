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


#ifndef ODE_SYSTEM_H
#define ODE_SYSTEM_H

// #include "symbol_accessor.h"
#include "interfaces.h"
#include "focusrange.h"
#include "function.h"
#include "equation.h"
#include "vector_equation.h"
#include "diff_eqn.h"

// performance timer
#include <sys/time.h>

/** Systemm Types
 *  - time continuous --> ode / pde  
 *    --> time intervals have to correspond to the connected systems.
 *    --> time intercals can be shrinked modified to change that.
 *  - time discrete --> start time + regular interval
 *    + Synchronous Automaton --> Discrete System with update frequ. 1
 *    + regular time interval events
 *    + single fire event systems
 *  - Event triggered systems --> i.e. upon cell division or whatsoever.
 *   
 **/

/** 
 * @brief System's expression evaluation unit
 *
 * Used as an intenal functional storage for the solvers.
 **/
template <class T>
class SystemFunc {
public:
	enum Type { ODE, RULE, FUN, VAR_INIT, EQN};

	shared_ptr<SystemFunc<T>> clone(shared_ptr<EvaluatorCache> cache) const {
		auto s = make_shared<SystemFunc<T>>(*this);
		s->cache = cache;
		
		if (evaluator) {
			if (s->cache)
				s->evaluator = make_shared<ExpressionEvaluator<T>>(*this->evaluator.get(), cache);
			else {
				s->evaluator = make_shared<ExpressionEvaluator<T>>(*this->evaluator.get());
				s->cache = s->evaluator->evaluator_cache; 
			}
			
			if (s->type == FUN) {
				s->initFunction();
			}
		}
		return s;
	}
	mu::fun_class_generic* getCallBack() { 
		if (!callback) initFunction();
		return callback.get();
	}
	
	void initFunction();
	
		
	Type type;
	
	SymbolRWAccessor<T> global_symbol;
	int cache_idx;
	int rate_cache_idx;
	
	string symbol_name, expression;
	bool vec_spherical;
	shared_ptr< ExpressionEvaluator<T> > evaluator;
	shared_ptr<EvaluatorCache> cache;
	vector<double> fun_params;
	vector<string> fun_params_names;
	set<string> dep_symbols;
	
	T k0, k1, k2, k3, k4, k5, k6, k7;
	T err, dy;

private:
	/// @brief Private CallBack class for registration of the internal evaluator in mu_parser as a generic lambda object
	class CallBack : public mu::fun_class_generic {
		public:
			CallBack(shared_ptr<ExpressionEvaluator<double> > evaluator, double *data, uint size) : evaluator(evaluator), params(data), nparams(size) {};
			
			/// Evaluate the lambda with arguments @p args in a spatial context given by the anonymous @p data pointer, which points in fact to a (const) SymbolFocus.
			mu::value_type operator()(const double* args, void * data) const override {
				if (params) memcpy( params, args, sizeof(double)*nparams );
				return evaluator->get(*reinterpret_cast<const SymbolFocus*>(data));
			};
			/// Number of arguments the lambda expects
			int argc() const override {
				return nparams;
			};
			
		private:
			shared_ptr<ExpressionEvaluator<double> > evaluator;
			double* params;
			uint nparams;
	};
	
	shared_ptr<CallBack> callback;
};

template<>
void SystemFunc<double>::initFunction();

template<>
void SystemFunc<VDOUBLE>::initFunction();

/**
 * The SystemSolver class 
 * 
 * All expressions in a SystemSolver are represented by SystemFunc<> instances and evaluated by ExpressionEvaluators that share the same EvaluatorCache,
 * thus their variable state is tightly coupled.
*/
class SystemSolver{

	public:
		enum class Method {
			Discrete,
			Euler,
			Heun,
			Runge_Kutta4,
			Runge_Kutta_AdaptiveCK,
			Runge_Kutta_AdaptiveDP
		};
		struct Spec {
			SystemSolver::Method method;
			double epsilon;
			double time_scaling;
			double time_step;
		};
		// remove copy constructor and assignment operator
		SystemSolver(Scope* scope, const std::vector< shared_ptr<SystemFunc< double >> >& fun, const std::vector< shared_ptr<SystemFunc< VDOUBLE >> >& vfun, SystemSolver::Spec spec);
		SystemSolver(const SystemSolver& p);
		const SystemSolver& operator=(const SystemSolver& p)= delete;
		void solve(const SymbolFocus& f, bool use_buffer, vector<double>* ext_buffer=nullptr);
// 		valarray<double> cache;
		void setTimeStep(double ht);
		set<Symbol> getExternalDependencies() { return cache->getExternalSymbols(); };
		static const string noise_scaling_symbol;

	private:
		// timer
// 		vector<struct timeval> sstart, send;
// 		vector<long> smtime, sseconds, suseconds, stotal;

		vector<shared_ptr<SystemFunc<double>>> evals, odes, rules, functions, var_initializers, equations;
		vector<shared_ptr<SystemFunc<VDOUBLE>>> vec_evals, vec_rules, vec_var_initializers, vec_equations;

		shared_ptr<EvaluatorCache> cache;
		uint local_time_idx, noise_scaling_idx;
		
		Spec spec;

		void fetchSymbols(const SymbolFocus& f);
		void writeSymbols(const SymbolFocus& f);
		void writeSymbolsToBuffer(const SymbolFocus& f);
		void writeSymbolsToExtBuffer(vector<double>* ext_buffer);
		void updateLocalVars(const SymbolFocus& f);
		
		void check_result(double value , const SystemFunc<double>& e) const;
		void check_result(const VDOUBLE& value , const SystemFunc<VDOUBLE>& e) const;
		
		void RungeKutta(const SymbolFocus& f, double ht);
		void RungeKutta_adaptive(const SymbolFocus& f, double ht);
		void RungeKutta_45CashKarp(const SymbolFocus& f, double ht);
		void RungeKutta_45DormandPrince(const SymbolFocus& f, double ht);
		void Euler(const SymbolFocus& f, double ht);
		void Heun(const SymbolFocus& f, double ht);
		void Discrete(const SymbolFocus& f);
		void EquationHooks(const SymbolFocus& f);
}; 

/** @brief System class takes care to solve numerical interpolation of [ODE/Discret Equation/Triggered] Systems
 *
 *  We create a single class dealing with all the template instances of system that attach to the Plugin factory under different XMLNames, but are
 *  represented by the same class with different configuration
 */

class System
{
public:
	System(SystemType type);
	void loadFromXML(const XMLNode node, Scope* scope);
	void init();

	// Set a symbol to be a local variable, that will not show up in the list of dependencies.
	void addLocalSymbol(string symbol, double value);
	const Scope* getLocalScope() { return local_scope; }; 
	set< SymbolDependency > getDependSymbols();
	set< SymbolDependency > getOutputSymbols();
	bool adaptive() const;
	void setTimeStep(double ht);
	void setSubStepHooks(const vector<ReporterPlugin*> hooks) { assert(sub_step_hooks.empty()); sub_step_hooks = hooks; this->init(); };
	
protected:
	/// Compute Interface 
	void computeToTarget(const SymbolFocus& f, bool use_buffer, vector<double>* buffer=nullptr);
	void compute(const SymbolFocus& f);

	void computeContextToBuffer();
	void applyContextBuffer();

	void computeToBuffer(const SymbolFocus& f);
	void applyBuffer(const SymbolFocus& f);
	void applyBuffer(const SymbolFocus& f, const vector<double>& buffer);
	
	bool target_defined;
	const Scope *target_scope;
	Granularity target_granularity;
	VINT lattice_size;
	
	Scope *local_scope;
	CellType* celltype;
	mutable GlobalMutex mutex;
	
	struct timeval start, end;
	long mtime, seconds, useconds, total;
	SystemSolver::Spec solver_spec;
	SystemType system_type;
	
// 	set<string> available_symbols;
	map<string,double> local_symbols;
// 	static double* registerVariable(const char*, void*);
	vector< shared_ptr<Plugin> > plugins;
	vector< shared_ptr<SystemFunc<VDOUBLE>> > vec_evals, vec_equations;
	vector< shared_ptr<SystemFunc<double>> > evals, equations;
	shared_ptr<EvaluatorCache> cache;
	vector< shared_ptr<SystemSolver> > solvers;
	vector<ReporterPlugin*> sub_step_hooks;
	
};


/** @brief ContinuousSystem is a Solver for time continuous ODE systems that is thightly coupled to the TimeScheduler
 */

class ContinuousSystem: public System, public ContinuousProcessPlugin {
public:
	DECLARE_PLUGIN("System");

    ContinuousSystem() : System(CONTINUOUS_SYS), ContinuousProcessPlugin(ContinuousProcessPlugin::CONTI,TimeStepListener::XMLSpec::XML_REQUIRED) {};
	/// Compute and Apply the state after time step @p step_size.
	void loadFromXML(const XMLNode node, Scope* scope) override;
	void init(const Scope* scope) override;
	void prepareTimeStep() override { System::computeContextToBuffer(); };
	void executeTimeStep() override { System::applyContextBuffer(); };
	void setTimeStep(double t) override { ContinuousProcessPlugin::setTimeStep(t); System::setTimeStep(t); };
	void setSubStepHooks(const vector<ReporterPlugin*> hooks) { System::setSubStepHooks(hooks); }
	const Scope* scope()  override{ return System::getLocalScope(); };
};

/** @brief DiscreteSystem regularly applies a System on each individual in a context.
 */
class DiscreteSystem: public System, public InstantaneousProcessPlugin {
public:
	DECLARE_PLUGIN("DiscreteSystem");
    DiscreteSystem() : System(DISCRETE_SYS), InstantaneousProcessPlugin(TimeStepListener::XMLSpec::XML_REQUIRED) {};

	/// Compute and Apply the state after time step @p step_size.
	void loadFromXML(const XMLNode node, Scope* scope) override {  InstantaneousProcessPlugin::loadFromXML(node, scope); System::loadFromXML(node, scope); };
	void init(const Scope* scope) override;
	void executeTimeStep() override { System::computeContextToBuffer(); System::applyContextBuffer();  };
	const Scope* scope() override { return System::getLocalScope(); };
};


/** @brief TriggeredSystem can be used to apply a System to an individual in a context and applies a System if it holds.
 */
class TriggeredSystem: public System {
public:
	TriggeredSystem() : System(DISCRETE_SYS) {}
	void trigger(const SymbolFocus& f) { System::compute(f); };
};


/** @brief EventSystem checks regularly a condition for each individual in a context and applies a System if the condition holds.
 */
class EventSystem: public System, public InstantaneousProcessPlugin {
public:
	DECLARE_PLUGIN("Event");
    EventSystem();
    void loadFromXML ( const XMLNode node, Scope* scope) override;
    void init (const Scope* scope) override;
	/// Compute and Apply the state after time step @p step_size.
	void executeTimeStep() override;
	CellType* celltype;
	const Scope* scope() override { return System::getLocalScope(); };

protected:
	// TODO We have to store the state of the event with respect to the context !! map<SymbolFocus, bool> old_value ?? that is a map lookup per context !!!
	// Alternatively, we can also create a hidden cell-property --> maybe a wise way to store the state
	shared_ptr<ExpressionEvaluator<double> > condition;
	PluginParameter<bool,XMLNamedValueReader,DefaultValPolicy> delay_compute;  /// Also delay the computation of the assignments
	PluginParameter<double,XMLEvaluator, DefaultValPolicy> delay;  /// Duration of the delay
	PluginParameter<bool,XMLNamedValueReader,DefaultValPolicy> trigger_on_change;
	map<SymbolFocus,double> condition_history;
	struct DelayedAssignment {
		const SymbolFocus focus;
		vector<double> value_cache;
	};
	multimap<double, DelayedAssignment> delayed_assignments;
	
	Granularity condition_granularity;
	vector<SymbolFocus> contexts_with_buffered_data;
};





#endif // ODE_SYSTEM_H
