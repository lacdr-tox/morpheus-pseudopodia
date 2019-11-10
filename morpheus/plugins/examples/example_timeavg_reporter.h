#include "core/interfaces.h"
#include "core/celltype.h"
#include "core/focusrange.h"


 /* 
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
 
/** \defgroup ML_TimeAVG TimeAVG
\ingroup Symbol
\ingroup ML_CellTyope, ML_Global
\brief Reports a temporal average of \e input to \e output

  * \b duration - averaging duration
  * \b input  -  expression to be averaged
  * \b ouput  -  symbol to write the average
*/


class TimeAVGPlugin : public ReporterPlugin {
private:
	PluginParameter<double, XMLValueReader, RequiredPolicy> duration;
	PluginParameter<double, XMLEvaluator, RequiredPolicy> input;
	PluginParameter<double, XMLWritableSymbol, RequiredPolicy> output;
	
	struct  BufferData { double time; double val; };
	CellType::PropertyAccessor< boost::circular_buffer<BufferData> > history_property;
	
	boost::circular_buffer<BufferData> history_global;
	
	
public:
	DECLARE_PLUGIN("TimeAVG");
	TimeAVGPlugin(): ReporterPlugin() { 
		// Interface with the XML configuration
		duration.setXMLPath("duration");
		input.setXMLPath("input");
		output.setXMLPath("output");
		
		registerPluginParameter(duration);
		registerPluginParameter(input); 
		registerPluginParameter(output); 
	}

	void loadFromXML(const XMLNode node, Scope * scope) override {
		ReporterPlugin::loadFromXML(node,scope);
	}
	
	void init(const Scope * scope) override {
		ReporterPlugin::init(scope); // Also registers all symbol dependencies and symbol output from PluginParameter2
		
		if (input.granularity() != output.granularity()) {
			throw string("Granularity mismatch in input / output");
		}
		
		if (input.granularity() == Granularity::Cell) {
			history_property = scope->getCellType()->addProperty(string("time_cache_")+input.stringVal(), boost::circular_buffer<BufferData>());
		}
		else if (input.granularity() ==  Granularity::Global) {
			//
		}
		else {
			throw string("Granularity of the input currently not supported");
		}
	};
	
	// update time averages for all input contexts
	void report() override{
		double time = SIM::getTime();
		
		// The FocusRange allows to iterate over the local scope using the granularity of the output;
		// I.e. for cell granularity and celltype scope every cell selected by the focus once.
		FocusRange range(output.granularity(), local_scope);
		for (const auto& focus : range) {
			auto& history = history_global;
			if (output.granularity() == Granularity::Cell)
				history = history_property->getRef(focus);
			
			// clean up the history
			while (history.size()>0 && (history[0].time <= time -duration())) 
				history.pop_front();
			
			// adjust buffer capacity
			if (history.full())
				history.resize(history.size()+10);
			
			// store current value
			history.push_back( {time, input(focus)} );
			
			// average the history 
			double val = std::accumulate(history.begin(), history.end(),0) / history.size();
			output.set(focus, val);
		}
	}
};
