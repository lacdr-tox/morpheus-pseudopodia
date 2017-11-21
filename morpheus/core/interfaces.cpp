#include "interfaces.h"
#include "time_scheduler.h"
#include "plugin_parameter.h"

int Plugin::plugins_alive =0;

XMLNode Plugin::saveToXML() const {
	return stored_node;
};

void Plugin::loadFromXML(const XMLNode xNode, Scope* scope) {
// 	assert( string(xNode.getName()) == XMLName());
	stored_node = xNode;
	getXMLAttribute(xNode, "name",plugin_name,false);

	for (uint i=0; i<plugin_parameters2.size(); i++) {
		plugin_parameters2[i]->loadFromXML(xNode);
	}
	// Use the current scope as default scope;
	local_scope = scope;
};

void Plugin::registerPluginParameter(PluginParameterBase& parameter ) {
	plugin_parameters2.push_back(&parameter);
}

void Plugin::registerInputSymbol(Symbol sym)
{
	registerInputSymbols( sym->dependencies() );
}

void Plugin::registerInputSymbol(const SymbolDependency& sym)
{
	input_symbols.insert(sym);
}


void Plugin::registerInputSymbols(const set< SymbolDependency >& in)
{
	input_symbols.insert(in.begin(), in.end());
}

void Plugin::registerOutputSymbol(Symbol sym)
{
	auto sd = sym->dependencies();
	output_symbols.insert(sd.begin(),sd.end());
}

void Plugin::registerOutputSymbols(const set< SymbolDependency >& out)
{
	for (auto sym : out) {
		output_symbols.insert(sym);
	}
}


void Plugin::registerCellPositionDependency()
{
	assert(local_scope);
	registerInputSymbol(local_scope->findSymbol<VDOUBLE>(SymbolBase::CellCenter_symbol));
}

void Plugin::registerCellPositionOutput()
{
	assert(local_scope);
	registerOutputSymbol(local_scope->findSymbol<VDOUBLE>(SymbolBase::CellCenter_symbol));
}

bool Plugin::setParameter(string xml_path, string value)
{
	for (uint i=0; i<plugin_parameters2.size(); i++) {
		if (plugin_parameters2[i]->XMLPath() == xml_path) {
			plugin_parameters2[i]->read(value);
			return true;
		}
	}
	return false;
}


void Plugin::init(const Scope* scope)
{
	// Use the current scope as default scope;
	if (!local_scope)
		local_scope = scope;
	
	for (uint i=0; i<plugin_parameters2.size(); i++) {
		cout << this->XMLName() << ": Initializing parameter " << plugin_parameters2[i]->XMLPath()<< endl;
		plugin_parameters2[i]->init(scope);
		
		auto in = plugin_parameters2[i]->getDependSymbols();
		input_symbols.insert(in.begin(), in.end());

		auto out = plugin_parameters2[i]->getOutputSymbols();
		output_symbols.insert(out.begin(), out.end());
	}
	
	if (! input_symbols.empty()) {
		cout << "Plugin " << XMLName() << ": Registered input symbol dependencies ";
		for (auto dep : input_symbols) {
			cout << dep.name << " [" << dep.scope->getName() << "] ,";
		}
		cout << endl;
	}
	if (! output_symbols.empty()) {
		cout << "Plugin" << XMLName() << ": Registered output symbol dependencies ";
		for (auto dep : output_symbols) {
			cout << dep.name <<  " [" << dep.scope->getName() << "] ,";
		}
		cout << endl;
	}
}

set< SymbolDependency > Plugin::getDependSymbols() const { return input_symbols; }

set< SymbolDependency > Plugin::getOutputSymbols() const { return output_symbols; }

TimeStepListener::TimeStepListener(TimeStepListener::XMLSpec spec) : xml_spec(spec)
{
	is_adjustable =  (xml_spec == XMLSpec::XML_NONE);
	 
	valid_time = -1;
	time_step = -1;
	latest_time_step = -1;
	execute_systemtime = 0;
}

void TimeStepListener::setTimeStep(double t)
{
// 	assert(t>0);
	time_step = t;
	if (time_step<0) {
		valid_time = numeric_limits< double >::max();
	}
	else 
		valid_time = SIM::getTime();
}


void TimeStepListener::updateSourceTS(double ts) {
	if (ts <=0)
		assert(0);
	
	// This is the case for the most processes, that run on the freq of their input
	if (is_adjustable) {
		if (time_step <= 0 || time_step > ts) {
			setTimeStep(ts);
			propagateSourceTS(ts);
			// This is notifying all dowstream processes
			propagateSinkTS(ts);
		}
	}

}


void TimeStepListener::updateSinkTS(double ts) {};

void TimeStepListener::propagateSinkTS(double ts)
{
	for (auto it: getDependSymbols()) {
		const_cast<Scope*>(it.scope)->propagateSinkTimeStep(it.name,ts);
	}
}

void TimeStepListener::propagateSourceTS(double ts)
{
	for (auto it: getOutputSymbols()) {
		const_cast<Scope*>(it.scope)->propagateSourceTimeStep(it.name,ts);
	}
}


void TimeStepListener::loadFromXML(const XMLNode node, Scope* scope)
{
	Plugin::loadFromXML(node, scope);
	
	if (xml_spec == XMLSpec::XML_REQUIRED) {
		is_adjustable = false;
		if ( ! getXMLAttribute(node, "time-step", time_step) ) {
			throw MorpheusException(string("Missing required time-step element in TSL \"") + XMLName() + "\"", node);
		}
		if (time_step<=0) {
			// disable scheduling the plugin
			time_step=-1;
		}
	}
	else if (xml_spec == XMLSpec::XML_OPTIONAL) {
		time_step = -1;
		if (getXMLAttribute(node, "time-step", time_step)) {
			if (time_step<=0) {
				time_step=-1;
			}
			is_adjustable = false;
			
		} else {
			is_adjustable = true;
		}
	}
	else {
		is_adjustable = true;
	}
	
}


void TimeStepListener::init(const Scope* scope)
{
	Plugin::init(scope);
	valid_time = SIM::getTime();
	latest_time_step = -1;
	if (time_step>0)
		setTimeStep(time_step);
	else {
		valid_time = numeric_limits< double >::max();
		if (time_step<0)
			latest_time_step = numeric_limits< double >::max();
	}

	TimeScheduler::reg(this);
}

void TimeStepListener::prepareTimeStep_internal()
{
	auto start = highc.now();
	prepareTimeStep_impl();
	execute_systemtime += chrono::duration_cast<chrono::microseconds>(highc.now()-start).count();
}

void TimeStepListener::executeTimeStep_internal()
{
	auto start = highc.now();
	executeTimeStep_impl();
	execute_systemtime += chrono::duration_cast<chrono::microseconds>(highc.now()-start).count();
	
	latest_time_step = SIM::getTime();
	if (time_step>0) {
		valid_time += time_step;
	}
}


// void TimeStepListener::doTimeStep()
// {
// 	if (time_step>0) {
// 		latest_time_step = SIM::getTime();
// 		valid_time += time_step;
// 	}
// 	else  {
// 		cout << "Disabling TimeStepListener " << XMLName() << " with time-step=0." << endl;
// 		valid_time = SIM::getStopTime();
// 	}
// 	
// }


void ContinuousProcessPlugin::updateSinkTS(double ts)
{
	if (ts <=0)
		assert(0);
	
	// This is the case for the most processes, that run on the freq of their input
	if (is_adjustable) {
		if (timeStep() == -1 || timeStep() > ts) {
			setTimeStep(ts);
			// Notification all upstream processes
			propagateSourceTS(ts);
			// Notification all dowstream processes
			propagateSinkTS(ts);
		}
	}
}


InstantaneousProcessPlugin::InstantaneousProcessPlugin(TimeStepListener::XMLSpec xml_spec) : TimeStepListener(xml_spec) { };

void InstantaneousProcessPlugin::setTimeStep(double ts)
{
	TimeStepListener::setTimeStep(ts);
	valid_time = SIM::getTime() + timeStep();
	
}

ReporterPlugin::ReporterPlugin() : TimeStepListener(TimeStepListener::XMLSpec::XML_NONE) 
{
	min_source_timestep = -1;
	min_sink_timestep = -1;
};

void ReporterPlugin::updateSourceTS(double ts)
{
	if (ts <=0)
		assert(0);
	
	// This is the case for the most processes, that run on the freq of their input
	if (is_adjustable) {
		if (min_source_timestep == -1 || (min_source_timestep>ts) ) {
			min_source_timestep = ts;
			propagateSourceTS(ts);
			if (min_sink_timestep>0 && ( timeStep() == -1 || max(min_sink_timestep, min_source_timestep)<timeStep()) ) {
				setTimeStep(max(min_sink_timestep, min_source_timestep));
				propagateSinkTS(timeStep());
			}
		}
	}
}


void ReporterPlugin::updateSinkTS(double ts)
{
	if (ts <=0)
		assert(0);
	
	if (is_adjustable) {
		if (min_sink_timestep == -1 || (min_sink_timestep>ts) ) {
			min_sink_timestep = ts;
			propagateSinkTS(ts);
			if (min_source_timestep>0 && ( timeStep() == -1 || max(min_sink_timestep, min_source_timestep)<timeStep()) ) {
				setTimeStep( max(min_sink_timestep, min_source_timestep) );
				propagateSourceTS(timeStep());
			}
		}
	}
}


int AnalysisPlugin::max_time_precision =0;

void AnalysisPlugin::init(const Scope* scope)
{
	TimeStepListener::init(scope);
	if (!is_adjustable && timeStep() <= 0) {
		setTimeStep(SIM::getStopTime()-SIM::getStartTime());
		valid_time=SIM::getStopTime();
	} 
}

void AnalysisPlugin::setTimeStep(double ts){
	TimeStepListener::setTimeStep(ts); 

	stringstream sstr;
	sstr << ts;
	if (sstr.str().find_first_of(".") != string::npos) {
		int prec = sstr.str().size() - sstr.str().find_first_of(".") -1;
		if (prec > max_time_precision)
			max_time_precision = prec;
	}
};

// void Analysis_Listener::loadFromXML(const XMLNode xNode) {
// 	Plugin::loadFromXML(xNode);
// 	interval = 100;
// 	schedule_flags = TimeStepFlags::NO_FLAG;
// 	endstate = false;
// 	getXMLAttribute(xNode,"interval",interval);
// 	
// 	if (interval<=0) {
// 		schedule_flags = TimeStepFlags::CAN_BE_ADJUSTED;
// 	}
// 	
// 	getXMLAttribute(xNode,"endstate",endstate);
// }
// 
// int Analysis_Listener::max_time_precision =0;
// 
// void Analysis_Listener::init(double time){
// 	Plugin::init(); 
// 	current_time = time;
// 	if (endstate) 
// 		interval = SIM::getStopTime();
// 	stringstream sstr;
// 	sstr << interval;
// 	if (sstr.str().find_first_of(".") != string::npos) {
// 		int prec = sstr.str().size() - sstr.str().find_first_of(".") -1;
// 		if (prec > Analysis_Listener::max_time_precision)
// 		Analysis_Listener::max_time_precision = prec;
// 	}
// 	
// 	TimeScheduler::reg(this);
// };
// 
// void Analysis_Listener::notify(double time){
// 	current_time = time + interval;
// }


