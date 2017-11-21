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

enum SolverMethod { Discrete_Solver, Euler_Solver, Heun_Solver, Runge_Kutta_Solver };
/** Systemm Types
 *  - time continuous --> ode / pde  --> time intervals have to correspond to the connected systems.
 *    Can be shrinked modified to change that.
 *  - time discrete --> start time + regular interval
 *    + Synchronous Automat --> Discrete System with update frequ. 1
 *    + regular time interval events
 *    + single fire event systems
 *  - Event triggered systems --> i.e. upon cell division or whatsoever.
 *   
 **/

/** @brief Function evaluation object
 *
 * Is used as an intenal storage for all kinds of solvers.
 **/
class SystemFunc {
public:
	enum Type { ODE, EQU, FUN , VEQU};
	SystemFunc() : value(NULL), value_x(NULL), value_y(NULL),value_z(NULL) {};
	SystemFunc clone() {
		SystemFunc s(*this);
		s.parser=shared_ptr<mu::Parser>(new mu::Parser(*this->parser.get()));
		return s;
	}
		
	Type type;
	string symbol_name, expression;
	shared_ptr<mu::Parser> parser;
	set<string> dep_symbols;
	int cache_pos;
	double k0, k1, k2, k3, k4;
	double *value;
	double *value_x, *value_y, *value_z;
	
	SymbolRWAccessor<double> global_symbol;
	bool vec_spherical;
	SymbolRWAccessor<VDOUBLE> v_global_symbol;
};

/**
 * The SystemSolver Interface
 * 
 * 
 * There is a DATA CACHE with a particular data layout determined by the @c System class.
 * The following rule for the order applies:
 * 1. externaĺ symbols to be fetched
 * 1a. Dependend system variables
 * 1b. Equation/ODE variables
 * 2. local symbols
 * 2a. function variables
 * 2b. other internal variables, i.e. noise scaling and so on
 * 3. internal variables, the system may not be aware of.
 * This last point is optional to the solver.
 * 
 * !! Time symbol is always registered under 1a. to support local scaled time
 * 
 * Equations/Functions and ODEs are transfered as a vector of @c SystemFunc
 * 
*/
class SystemSolver{

	public:
		SystemSolver(vector<shared_ptr<SystemFunc> > f, map<string,int> cache_layout, SolverMethod method);
		void solve();
		valarray<double> cache;
		void setTimeStep(double ht);
		static const string noise_scaling_symbol;

	private:
		// remove copy constructor and assignment operator
		SystemSolver(const SystemSolver& p) {};
		const SystemSolver& operator=(const SystemSolver& p) { return *this;};
		// timer
		vector<struct timeval> sstart, send;
		vector<long> smtime, sseconds, suseconds, stotal;

		vector<SystemFunc> odes;
		vector<SystemFunc> equations;
		vector<SystemFunc> vec_equations;
		vector<SystemFunc> functions;

		map<string,int> cache_layout;
		double *local_time, *noise_scaling;
		double time_step;
		SolverMethod  solver_method;


		void RungeKutta(double ht);
		void Euler(double ht);
		void Heun(double ht);
		void Discrete();
};


/** @brief System class takes care to solve numerical interpolation of [ODE/Discret Equation/Triggered] Systems
 *
 *  We create a single class dealing with all the template instances of system that attach to the Plugin factory under different XMLNames, but are
 *  represented by the same class with different configuration
 */

template <SystemType type>
class System
{
public:
	void loadFromXML(const XMLNode node, Scope* scope);
	void init();

	// Set a symbol to be a local variable, that will not show up in the list of dependencies.
	void addLocalSymbol(string symbol, double value);
	const Scope* getLocalScope() { return local_scope; }; 
	set< SymbolDependency > getDependSymbols();
	set< SymbolDependency > getOutputSymbols();
	void setTimeStep(double ht);
	
protected:
	/// Compute Interface 
	void compute(const SymbolFocus& f);

	void computeContextToBuffer();
	void applyContextBuffer();

	void computeToBuffer(const SymbolFocus& f);
	void applyBuffer(const SymbolFocus& f);
	
	bool target_defined;
	const Scope *target_scope;
	Granularity target_granularity;
	VINT lattice_size;
	
	Scope *local_scope;
	CellType* celltype;
	
	struct timeval start, end;
	long mtime, seconds, useconds, total;
	SolverMethod solver_method;
	double time_scaling;
	
	set<string> available_symbols;
	map<string,double> local_symbols;
	static double* registerVariable(const char*, void*);
	vector< shared_ptr<Plugin> > plugins;
	vector< shared_ptr<VectorEquation> > vec_equations;
	vector< shared_ptr<SystemFunc> > functionals, equations, functions;
	vector< SymbolAccessor<double> > external_symbols;
	map<string, int> cache_layout;
	vector< shared_ptr<SystemSolver> > solvers;
	
	
};


/** @brief ContinuousSystem is a Solver for time continuous ODE systems that is thightly coupled to the TimeScheduler
 */

class ContinuousSystem: public System<CONTINUOUS_SYS>, public ContinuousProcessPlugin {
public:
	DECLARE_PLUGIN("System");

    ContinuousSystem() : ContinuousProcessPlugin(ContinuousProcessPlugin::CONTI,TimeStepListener::XMLSpec::XML_REQUIRED) {};
	/// Compute and Apply the state after time step @p step_size.
	void loadFromXML(const XMLNode node, Scope* scope) override;
	void init(const Scope* scope) override;
	void prepareTimeStep() override { System<CONTINUOUS_SYS>::computeContextToBuffer(); };
	void executeTimeStep() override { System<CONTINUOUS_SYS>::applyContextBuffer(); };
	void setTimeStep(double t) override { ContinuousProcessPlugin::setTimeStep(t); System<CONTINUOUS_SYS>::setTimeStep(t * time_scaling); };
	const Scope* scope()  override{ return System<CONTINUOUS_SYS>::getLocalScope(); };
};

/** @brief DiscreteSystem regularly applies a System on each individual in a context.
 */
class DiscreteSystem: public System<DISCRETE_SYS>, public InstantaneousProcessPlugin {
public:
	DECLARE_PLUGIN("DiscreteSystem");
    DiscreteSystem() : InstantaneousProcessPlugin(TimeStepListener::XMLSpec::XML_REQUIRED) {};

	/// Compute and Apply the state after time step @p step_size.
	void loadFromXML(const XMLNode node, Scope* scope) override {  InstantaneousProcessPlugin::loadFromXML(node, scope); System<DISCRETE_SYS>::loadFromXML(node, scope); };
	void init(const Scope* scope) override;
	void executeTimeStep() override { System<DISCRETE_SYS>::computeContextToBuffer(); System<DISCRETE_SYS>::applyContextBuffer();  };
	const Scope* scope() override { return System<DISCRETE_SYS>::getLocalScope(); };
};


/** @brief TriggeredSystem can be used to apply a System to an individual in a context and applies a System if it holds.
 */
class TriggeredSystem: public System<DISCRETE_SYS> {
public:
	void trigger(const SymbolFocus& f) { System<DISCRETE_SYS>::compute(f); };
};


/** \defgroup ML_Event Event
\ingroup ML_Global
\ingroup ML_CellType
\ingroup InstantaneousProcessPlugins

\section Description
An Event is a conditionally executed set of assignemnts. A provided Condition
is tested in regular intervals (time-step) for all different contexts in the current scope.
If no time-step is provided, the minimal time-step of the input symbols is used. 

The set of assignments is executed when the conditions turns from false to true (trigger = "on change")
or whenever the condition is found true (trigger="when true").

\section Examples

Set symbol "candivide" (e.g. assume its a CellProperty) to 1 after 1000 simulation time units

\verbatim
	<Event trigger="on change" time-step="1000">
		<Condition>time >= 1000</Condition>
		<Rule symbol-ref="candivide">
			<Expression>1</Expression>
		</Rule>
	</Event>
\endverbatim
*/

/** @brief EnventSystem checks regularly a condition for each individual in a context and applies a System if the condition holds.
 */
class EventSystem: public System<DISCRETE_SYS>, public InstantaneousProcessPlugin {
public:
	DECLARE_PLUGIN("Event");
    EventSystem() : InstantaneousProcessPlugin( TimeStepListener::XMLSpec::XML_OPTIONAL ) {};
    void loadFromXML ( const XMLNode node, Scope* scope) override;
    void init (const Scope* scope) override;
	/// Compute and Apply the state after time step @p step_size.
	void executeTimeStep() override;
	CellType* celltype;
	const Scope* scope() override { return System<DISCRETE_SYS>::getLocalScope(); };

protected:
	// TODO We have to store the state of the event with respect to the context !! map<SymbolFocus, bool> old_value ?? that is a map lookup per context !!!
	// Alternatively, we can also create a hidden cell-property --> maybe a wse way to store the state
	shared_ptr<ExpressionEvaluator<double> > condition;
	map<SymbolFocus,double> condition_history;
	bool trigger_on_change;
	Granularity condition_granularity;
	vector<SymbolFocus> contexts_with_buffered_data;
};





#endif // ODE_SYSTEM_H
