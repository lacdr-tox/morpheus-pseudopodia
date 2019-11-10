#include "core/interfaces.h"
#include "core/cell.h"

/* *********   User Documentation for the GUI  *************
 * 
 * This documentation contains the documentation of the plugin's XML language specification
 * It is parsed via doxygen, compiled for the QtHelp system and then compiled into the GUI.
 * 
 * (A) Create a documentation group based on the XML tag name. Good praxis is to define the documentation group as
 *  \defgroup ML_[PluginName] [PluginName] 
 *  The first name is the tag used to refer to the Plugin within the documentation, and the 
 *  second is required to be the XMLTag/XMLName of the Plugin
 *  
 * 
 * (B) Refer to the parental XML elements that shall allow the specification of your plugin (See XSD Group specification)
 *  *    XSD group             Docu
 *  *  GlobalPlugins   ->  \ingroup ML_Global
 *  *  CellTypePlugin  ->  \ingroup ML_CellType
 *  *  AnalysisPlugin  ->  \ingroup ML_Analysis
 *  *  SystemPlugins   ->  \ingroup ML_System
 *  *  ...
 * 
 * (C) Document the usage of your plugin.
*/


/** \defgroup ML_ExtSymbol ExtSymbol
\ingroup ML_Global

Simulates a given ExtSymbol model using the external library XXX.

**/


/* *********  Developer Documentation  *************
 *
 * parsed by doxygen but not integrated into the GUI
 */

/** \brief ExtSymbol integration plugin
 * 
 * The ExtSymbol plugin integrates an external ExtSymbol simulation with the symbol interface of Morpheus
 **/

/* *********  Plugin Declaration *************
 * 
 * Just for convenience, the implementation has been put inline here. 
 * Obviously should finally end up in the .cpp file.
 */

class ExtSymbolPlugin : public ContinuousProcessPlugin {
public:
	// This macro defines the XMLName of the plugin, and takes care for the plugin registration requirements.
	DECLARE_PLUGIN("ExtSymbol");
	
	// The constructor must call and specify the inherited ContinuousProcessPlugin
	// In addition, the PluginParameters to be read from XML are configured and registered.
	ExtSymbolPlugin() : ContinuousProcessPlugin(CONTI, XMLSpec::XML_REQUIRED), initialized(false) {
		symbol_name.setXMLPath("symbol");
		registerPluginParameter(symbol_name);
		any_value.setXMLPath("any-xml-attribute");
		registerPluginParameter(any_value);
	}
	
	// Load Parameters from XML and register symbols in the local scope 
	void loadFromXML(const XMLNode node, Scope * scope) override {
		ContinuousProcessPlugin::loadFromXML(node,scope);
		
		VINT lattice_size = SIM::lattice().size();  // --> Simulation space given by the lattice object in morpheus/core/lattice.h
		
		// Create an symbol accessor for the external data
		ext_symbol = make_shared<Symbol>(symbol_name(), this);
		// register to the local scope
		scope->registerSymbol(ext_symbol);
		// And specify that the Plugin will change that value during simulation (with it's time stepping).
		registerOutputSymbol(ext_symbol);
	}
	
	void init(const Scope * scope) override { 
		ContinuousProcessPlugin::init(scope);
		// Plugin parameters are initialized now
		
		/* Create your exsternal simulator here */
		/* Initialize here your symbolic dependencies */ 
	};
	
	void init() {
		if (!initialized)
			init(local_scope);
	}
	
	void prepareTimeStep(double step_size) override { 
		// Prepare the time step here
		// All symbols required  are guarenteed not to be updated yet, i.e. read values of other symbols here
		// You may also compute the time-step, the data, however, shall remain in state currentTime()
		
		// currentTime() -> currentTime() + timeStep()  <-- loaded from XML time-step attribute

		double t = this->currentTime();   // time of the plugin;
		double dt = step_size;     // step width as specified via XML
	};
	
	void executeTimeStep() override {
		// Apply time step changes previously prepared
		// currentTime is updated after this method run by the ContinuousProcessPlugin
		// the currentTime of other processes may or may not be updated to currentTime + dt  yet.
		
		cout << currentTime()<< " and any-value is " << any_value() << endl;
		};
		
	
	double get(const VINT& pos) { 
		// return your external data here ....
		return 0.0;
	}

	// A wrapper SymbolAccessor to allow read-only access the ExtSymbol
	class Symbol : public SymbolAccessorBase<double> {
	public:
		Symbol(string name, ExtSymbolPlugin* ext_plugin): SymbolAccessorBase<double>(name), descr(name), ext_plugin(ext_plugin) {
			// Carefully set flags in order to describe the symbols properties (also see SymbolBase::Flags)
			this->flags().granularity = Granularity::Node;
		};
		// Provide a description for output generation
		const string&  description() const override { return descr; }
		// For logging info creation
		std::string linkType() const override { return "ExtSymbolLink"; }
		// The standard access method for the symbol
		TypeInfo<double>::SReturn get(const SymbolFocus & f) const override { return ext_plugin->get(f.pos()); }
		
		// During initialisation phase, this access method is used.
		// You have to make sure that the data referred by the symbol is initialized, because the plugins init method may not have been called yet.
		TypeInfo<double>::SReturn safe_get(const SymbolFocus & f) const override {
			ext_plugin->init(); // assert it's initialized
			return ext_plugin->get(f.pos());
		};
		
		// For writable symbols, i.e. derived from SymbolRWAccessorBase<T>, the following methods also have to be implemented
		
		// void set(const SymbolFocus & f, typename TypeInfo<double>::Parameter value) const override { };
		// void setBuffer(const SymbolFocus & f, TypeInfo<double>::Parameter value) const override { }
		// void applyBuffer() const override { };
		// void applyBuffer(const SymbolFocus & f) const override { }
		
	private:
		ExtSymbolPlugin *ext_plugin;
		string descr;
	};
	
private:
	// PluginParameter2 is an adaptor to read, convert and evaluate a value from the XML
	PluginParameter2<string, XMLValueReader, RequiredPolicy> symbol_name;
	PluginParameter2<double, XMLValueReader, OptionalPolicy> any_value;
	bool initialized;
	shared_ptr<Symbol> ext_symbol;
};
