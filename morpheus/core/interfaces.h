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

#ifndef INTERFACES_H
#define INTERFACES_H

#include "simulation.h"
#include "ClassFactory.h"
#include <chrono>
#include <set>

class AbstractPluginParameter;
class PluginParameterBase;
template <class T> class ExpressionEvaluator;
template <class T> class ThreadedExpressionEvaluator;
class CellType;
class Cell;
class InteractionEnergy;
class PDE_Layer;




enum SystemType { DISCRETE_SYS, CONTINUOUS_SYS};

using namespace std;
/**
 * \page PluginSystem Plugin System
 * \brief The Plugin System allows for easy extension of the framework by self-crafted plugin descendants
 * 
 * \tableofcontents
 */

// we might have celltype plugins, cell plugins, and pde_layer plugins ?? In case it's more convenient we might also plug all to the a single Factory. However, you should keep in mind that all keys must finally be unique!
/** 
 * \page PluginSystem 
 * \section Introduction Introduction to plugin development

This page provides a quick intro on plugin development.

Derived from the base class Plugin several are provided to interface with the simulation process.
You may derive your plugin from a subset of these \ref PluginInterfaces classes.

\subsection PluginExample Example
*/
/*\verbatim

class MyPlugin : public Check_Update_Listener, public Update_Listener
{
	public:
		// necessary for platform integration and associating the plugin with a tag.
		DECLARE_PLUGIN("MySuperTag");

		MySuperPlugin();
		// load and save MySuperPlugin properties from/to XML
		void loadFromXML(const XMLNode);
		XMLNode saveToXML() const;

		// attach to a specific celltype
		virtual void init(CellType *);

		// prevent certain updates
		bool update_check(const Cell* cell, const & update, _TODO todo);

		// get notification of accepted updates
		void update_notify(const Cell* cell, const & update, _TODO todo);
};

------- .cpp


// Integrate the plugin in  the plugin system
REGISTER_PLUGIN(MySuperPlugin);

MySuperPlugin::MySuperPlugin() : Plugin() {
	addPluginParameter( new )
}

void MySuperPlugin::loadFromXML(const XMLNode node){
	Plugin::loadFromXML(node);
	...
};

XMLNode MySuperPlugin::saveToXML() const {
	XMLNode node = Plugin::saveToXML();
	...
	return node;
};

void init() {
	Plugin::init();
}

\endverbatim
*/




/**
 * \page PluginSystem
 * 
 * \section Development Plugin Development
 * \subsection PluginInterfaces Plugin Interfaces 
 * 
 * Several inteface classes to derive from allow a Plugin to interfere with the simulation
 * system.
 * 
 *  - CPM_Check_Update
 *  - CPM_Energy
 *  - CPM_Update_Listener
 *  - TimeStepListener
 *  - Analysis_Listener
 * 
 * Derive from this class, add plugin interfaces and develop your own plugins
 * 
 * 
 * \subsection Integration Plugin Integration
 *
 * Integration of plugins is largely automated.
 * 
 * Example
 */






/// Information about a SymbolDependency
struct SymbolDependency {string name; const Scope* scope;};

inline bool operator<(const SymbolDependency& lhs, const SymbolDependency& rhs) {
	if (lhs.name < rhs.name) return true;
	else if (lhs.name == rhs.name) {
		return lhs.scope<rhs.scope;
	}
	else
		return false;
}

inline bool operator==(const SymbolDependency& lhs, const SymbolDependency& rhs) {
	return (lhs.name == rhs.name && lhs.scope == rhs.scope);
}


/** \brief Abstract plugin base class
 * 
 * This class provides basic framework integration namely the
 *   - infrastructure for plugin factory registration
 *   - PluginParameter
 *   - Symbol dependency tracking
 * 
 * All interface classes inherit the Plugin class in a virtually and thus share the same Plugin instance.
 * There is no need to inherit directly from this class.
 * 
 * \remark
 *    Take care that you call the inherited methods first when overriding them in your plugin.
 *    Else, plugin integration will fail.
 */
class Plugin {
	private:
		vector<PluginParameterBase* > plugin_parameters2;
		set<SymbolDependency> input_symbols;
		// All writable symbols are solely registered as output Symbols
		set<SymbolDependency> output_symbols;
		
	protected:
		string plugin_name;
		XMLNode stored_node;
		const Scope* local_scope;
		
	public:
		Plugin() {  Plugin::plugins_alive++;}
		virtual  ~Plugin() { Plugin::plugins_alive--; };  // any plugin will have a virtual destructor though

// 		static multiset<string> plugins_alive;
		static int plugins_alive;
		/// Get an XMLNode containing the XML specification the plugin has loaded
		XMLNode saveToXML() const;
		
		/** \brief Load a plugin confuguration from XML
		 *  You just have to take care for the XML Tags not treated by the plugin interface or PluginParmeters
		 */
		virtual void loadFromXML(const XMLNode);
		
		/// XML Tag the Plugin corresponds to. Gets overridden by the DECLARE_PLUGIN macro.

		virtual string XMLName() const =0;
		
		/// \brief Register a PluginParameter for automatic treatment
		/// Loading from XML, initialisation and dependency tracking is done automatically
		/// **Note**  The platform takes a reference to the parameter, so don't move/copy the parameter after registration.
		void registerPluginParameter( PluginParameterBase& parameter );
/*		// Can be used to 
		template <class T>
		void registerUsedSymbol( const SymbolAccessor<T>& parameter );
		template <class T>
		void registerUsedSymbol( const SymbolRWAccessor<T>& parameter );
	*/	
		void registerInputSymbol(string name, const Scope* scope);
		void registerInputSymbols(const set<SymbolDependency>& in);
		void registerOutputSymbol(string name, const Scope* scope);
		void registerOutputSymbols(const set<SymbolDependency>& out);
		
		void registerCellPositionDependency();
		void registerCellPositionOutput();
		
		
		/// Fully qualified name of the plugin
		string getFullName() const { return plugin_name; };
		virtual const Scope* scope() { return local_scope; };
		bool setParameter(string xml_path, string value);
		/// init method is called by the framework as soon as all model all containers and symbols have been set up
		virtual void init(const Scope* scope);
		/// The set of symbols the Plugin depends on
		set<SymbolDependency> getDependSymbols() const;
		/// The set of symbols the Plugin writes to
		set<SymbolDependency> getOutputSymbols() const;
};

typedef CClassFactory<string, Plugin> PluginFactory;

/** This macro creates all the declaration (class header) needed for plugin system integration.
 *  The string @param xml_tag_name defines the tag used to identify the plugin.
 *  Use this macro alongside with REGISTER_PLUGIN.
 */
#define DECLARE_PLUGIN(xml_tag_name) static bool factory_registration; \
static Plugin* createInstance(); \
string XMLName() const { return string(xml_tag_name); }; \
static string FactoryName() { return string(xml_tag_name); };

template <class PluginClass>
bool registerPlugin() {
	bool ret=PluginFactory::RegisterCreatorFunction(PluginClass::FactoryName(), PluginClass::createInstance );
	return ret;
}

/** This macro creates all the definitions (class source) needed for plugin system integration.
 *  The string @param PClass denotes the class to be integrated.
 *  Use this macro alongside with DECLARE_PLUGIN.
 */
#define REGISTER_PLUGIN(PClass) Plugin* PClass::createInstance() { return new PClass(); } \
bool PClass::factory_registration = registerPlugin<PClass>(); /* PluginFactory::RegisterCreatorFunction(PClass().XMLName(),PClass::createInstance) */


/** \defgroup CPM_EnergyPlugins CPM Hamiltonian Plugins
 *  \ingroup CPM
 * 	
 */

/** Plugin interface for defining an energy term in the CPM hamiltonian.
 * The delta method has to provide the change in energy due to a potential update with respect to cell cell_id.
 * The hamiltonian is the total energy of a cell with respect to this energy term, but is currently not used at all.
 */

class CPM_Energy : virtual public Plugin {
	public:
		/** Compute the change in energy due to an update with respect to cell cell_id. todo contains information about 
		 * 
		 * Updated cell properties are availible in the Cell  via accessors prefaced with updated_ .
		 */
		virtual double delta(const SymbolFocus& cell_focus, const CPM::Update& update) const =0;
		virtual double hamiltonian(CPM::CELL_ID cell_id) const =0;            // Berechnung gesamte Energie
};

// class Cell_Interaction : virtual public CellType_Plugin {
// 	public:
// 	virtual double interaction(double base_interaction, const CPM::STATE& State_a, const CPM::STATE& State_b) const =0;
// };


/** \defgroup CPM_CheckUpdatePlugins CPM Check Update Plugins
 *  \ingroup CPM
 * 	\brief Plugin interface for defining a rule to check for the CPM before updates take place.
 */

/** Plugin interface for defining an update check rule for the CPM.
 * The update_check is called for any cell_id involved in an update. This way certain updates can be prevented, e.g
 * for creating a connectivity constraint or freezing certain cells.
 */
class CPM_Check_Update : virtual public Plugin
{
	public:
		
		/** \brief Check whether an update with respect to cell cell_id is permittable.
		 *  
		 *  Returning false prevents the update. 
		 *  Post-Updated cell properties are availible in the Cell instance via accessors prefaced with updated_ .
		 */
		virtual bool update_check(CPM::CELL_ID  cell_id, const CPM::Update& update) =0;
};

/** \defgroup CPM_UpdateListenerPlugins CPM Update Listener Plugins
 *  \ingroup CPM
 * 	\brief Plugin interface for getting notifications of cell updates check rule for the CPM.
 */

/** Plugin interface for getting notifications of cell updates check rule for the CPM.
 * 
 * In addition, when a cell is selected for update, the set_update_notify method is called.
 */

class CPM_Update_Listener : virtual public Plugin
{
	public:
		virtual void set_update_notify(CPM::CELL_ID cell_id, const CPM::Update& update) {};
		virtual void update_notify(CPM::CELL_ID cell_id, const CPM::Update& update) =0;
};


/** \brief Plugin interface for getting included into the Frameworks TimeScheduler.
 * 
 *  Various predefined scheduling types are available and have to be in the set during plugin constructor.
 *  Also see \ref Scheduling.
 */
class TimeStepListener : virtual public Plugin
{
	public:
		
		/** Scheduling types
		 * 
		 *  - CONTINUOUS schemes have XML-define time-stepings and are PHASE I processes
		 *  - INSTANTANEOUS schemes are PHASE II and have XML-defined time-steppings
		 *  - REPORTERS are scheduled on demand such that they are run when data is required but not more often than the input changes
		 *  - MCSListeners are scheduled fix to the time discrete sampler (e.g. Monte Carlo Step for CPM)
		*/
		
// 		enum class ScheduleType { CONTINUOUS, DELAY, MONTECARLOSAMPLER, INSTANTANEOUS, REPORTER, ANALYSER };

		
		enum class XMLSpec { XML_REQUIRED, XML_OPTIONAL, XML_NONE };
		
		TimeStepListener( XMLSpec spec);
		// Load data from XML (time-step, if defined )
		void loadFromXML(const XMLNode ) override;
		// Init the Plugin for the scoe is was defined in
		void init(const Scope* scope) override;
		
		/// Actual time step size as selected by the scheduler
		double timeStep() const { return time_step; }
		/// The current time of the process
		double currentTime() const { return valid_time;} 
		/// System time spent processing this plugin [ms]
		double execSysTime() { return execute_systemtime / 1000; }

		/// Time step adjustable time step
		bool isAdjustable() { return is_adjustable; }
		
		/// Update time stepping of the symbols the Listener depends on
		/// Returns whether the actual time step of the Listener changed
		virtual void updateSourceTS(double t);
		/// Update the demanded update frequency of the symbols the Listener writes to
		/// Returns whether the actual time step of the Listener changed
		virtual void updateSinkTS(double ts);
		
	protected:

		bool is_adjustable;
		virtual void setTimeStep(double t);
		double latestTimeStep() { return latest_time_step; };
		
		/// time until which the TSL is valid
		double valid_time;
		double latest_time_step;

		
		friend class TimeScheduler;
		friend Scope;
		
		XMLSpec xml_spec;

		/// update the time step depending on input and output timesteps and the Listener type and flags
		/// Override callbacks for actually performing some operation, exclusively used for buffered synchronous processes
		virtual void prepareTimeStep_impl() {};
		/// Override callbacks for actually performing some operation
		virtual void executeTimeStep_impl() =0;
		
		/// Slot to be called by the Time Scheduler
		void prepareTimeStep_internal();
		/// Slot to be called by the Time Scheduler
		void executeTimeStep_internal();
		
// 		void doTimeStep();
// 		virtual void phase2_executeTS() {};
		
		void propagateSourceTS(double ts);
		void propagateSinkTS(double ts);
		
private:
		/// time step duration
		double time_step;
		/// time needed for execution (measured in milliseconds)
		double execute_systemtime;
		std::chrono::high_resolution_clock highc;
};

/// \defgroup ContinuousProcessPlugins Continuous Process Plugins

/** \brief Interface providing basic functionality and methods to develop plugins for time continuous processes
 * 
 *  Scheduling and integration into the TimeScheduler is automatically accomplished.
 *  
 */

class ContinuousProcessPlugin : public TimeStepListener {
public:
	/** 
	 * Ranks are meant to provide means to classify the Phase 1 processes (time continuous processes), such that they can be ordered in their execution accordingly.
	 * Rank 1 -- Monte Carlo Sampler like CPM, that do not store intermediates via prepareTimeStep
	 * Rank 2 -- Delays have to be progressed **before** new data can be entered
	 * Rank 3 -- Continuous time processes (ODEs), that do buffer their solutions in prepareTimeStep
	 * Rank 4 -- Post hoc processes without interdependencies (e.g. plain Advection and Diffusion after operator splitting)
	 */
	enum Rank { MCS = 0, DELAY = 1, CONTI=2, INDEPEND=3 };
	
    ContinuousProcessPlugin(Rank phase1_rank, TimeStepListener::XMLSpec xml_spec = TimeStepListener::XMLSpec::XML_REQUIRED) : TimeStepListener(xml_spec), phase1_rank(phase1_rank) {};
	virtual void prepareTimeStep() = 0;
	virtual void executeTimeStep() = 0;
	
    virtual void updateSinkTS(double ts);
	Rank getRank() {return phase1_rank;};

private:
	void prepareTimeStep_impl() final { prepareTimeStep(); };
	void executeTimeStep_impl() final { executeTimeStep(); };

	Rank phase1_rank;

};

/// \defgroup InstantaneousProcessPlugins Instantaneous Process Plugins

/** \brief Interface providing basic functionality and methods to develop plugins for instantaneous processes
 * 
 *  Scheduling and integration into the TimeScheduler is automatically accomplished.
 */

class InstantaneousProcessPlugin : public TimeStepListener {
public:
    InstantaneousProcessPlugin(TimeStepListener::XMLSpec xml_spec);
    void setTimeStep(double ts) override;
	virtual void executeTimeStep() = 0;
private:
	void executeTimeStep_impl() final { executeTimeStep(); };
	// forwarding the generalized Phase 2 interface
};

/** \defgroup ReporterPlugins Reporter Plugins
\ingroup Plugins
**/

/** \brief Interface providing basic functionality and methods to develop Reporter plugins
 * 
 *  Scheduling and integration into the TimeScheduler is automatically accomplished.
 *  The mapping of data provided by the reporter is executed as often as data changes, but not more often than the output data is required
 */

class ReporterPlugin : public TimeStepListener {
public:
    ReporterPlugin();
	virtual void report() = 0;
protected:
	void updateSourceTS(double ts) override;
	void updateSinkTS(double ts) override;
private:
	void executeTimeStep_impl() final { report(); };
	// forwarding the generalized Phase 2 interface
	
	double min_source_timestep;
	double min_sink_timestep;
};


/** \defgroup AnalysisPlugins Analysis Plugins
\ingroup Plugins
**/

/** \brief Interface providing basic functionality and methods to develop Analysis/Output generating plugins
 * 
 *  Scheduling and integration into the TimeScheduler is automatically accomplished.
 *  The time-step attribute in the XML configuration is optionally and interpreted in the following way:
 *   - missing - Schedule to process as often as the input changes
 *   - time-step=0 - No intermediate notifications, only execute analysis at the end of the simulation
 *   - time-step>0 - Use the value as a fixed stepping, given in simulation time
 */

class AnalysisPlugin : public TimeStepListener {
public :
	AnalysisPlugin() : TimeStepListener(TimeStepListener::XMLSpec::XML_OPTIONAL) {};
	
	void init(const Scope* scope) { TimeStepListener::init(scope); end_state = (timeStep() <= 0);}
	bool endState() { return end_state; };
	void setTimeStep(double ts) override;
	/** Callback for scheduled notifications. **/
	virtual void analyse(double time) = 0;
	/** \brief Optional callback for notifications at the end of simulation
	 * 
	 *  Can be used to close files or evaluate data collected over the simulation period.
	 */
	virtual void finish() {};
	

	static int max_time_precision;
private:
	void executeTimeStep_impl() final { analyse(SIM::getTime()); };
	bool end_state;
};


/** \defgroup InitializerPlugins Population Initializer Plugins
\ingroup Plugins
**/
class Population_Initializer : virtual public Plugin
{
	public:
		virtual bool run(CellType* ct) =0;
};

class Field_Initializer : virtual public Plugin
{
	public:
		virtual bool run(PDE_Layer* pde) =0;
};

/**
 * \defgroup CPM_InteractionPlugins CPM Interaction Plugins
 *  \ingroup CPM
 *  \ingroup Plugins
 */

/// Interface to override the interaction energies between CPM cells computed by the CPM logic
class Interaction_Overrider : virtual public Plugin {
	public:
		virtual double interaction(CPM::STATE s1, CPM::STATE s2, double base_interaction) =0;
};

/// Interface to provide additional interaction energies between CPM cells
class Interaction_Addon : virtual public Plugin {
	public:
		/// Compute additional interaction energies between CPM cell **s1** and **s2**, that may depend on the individual cells.
		virtual double interaction(CPM::STATE s1, CPM::STATE s2) =0;
};


#endif // INTERFACES_H
