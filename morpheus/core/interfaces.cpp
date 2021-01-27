#include "interfaces.h"
#include "time_scheduler.h"
#include "plugin_parameter.h"
#include "time.h"


Plugin::Plugin() : plugin_name(""), local_scope(nullptr) {}

Plugin::~Plugin() {};

unique_ptr<Plugin::Factory> factory;

Plugin::Factory& Plugin::getFactory() { if (!factory) factory = make_unique<Factory>(); return *factory; }


XMLNode Plugin::saveToXML() const {
	return stored_node;
};

void Plugin::loadFromXML(const XMLNode xNode, Scope* scope) {
// 	assert( string(xNode.getName()) == XMLName());
	stored_node = xNode;
	getXMLAttribute(xNode, "name",plugin_name,false);
	
	string tags_string;
	getXMLAttribute(xNode, "tags", tags_string, false);
	auto tags_split = tokenize(tags_string," \t,", true);
	xml_tags.insert(tags_split.begin(), tags_split.end());
	if (xml_tags.empty()) xml_tags.insert("#untagged");

	for (uint i=0; i<plugin_parameters2.size(); i++) {
		plugin_parameters2[i]->loadFromXML(xNode, scope);
	}
	// Use the current scope as default scope;
	local_scope = scope;
};

bool Plugin::isTagged(const set< string >& tags) const
{
	auto it1 = tags.begin();
	auto it2 = xml_tags.begin();

	while (it1!=tags.end() && it2!=xml_tags.end()) {
		if (*it1<*it2) {
			it1++;
		}
		else if (*it2<*it1) {
			it2++;
		}
		else {
			return true;
		}
	}
	
	return false;
}

void Plugin::setInheritedTags(const set< string >& tags) { xml_tags.insert(tags.begin(),tags.end()); } 

void Plugin::registerPluginParameter(PluginParameterBase& parameter ) {
	plugin_parameters2.push_back(&parameter);
}

void Plugin::registerInputSymbol(SymbolDependency sym)
{
	input_symbols.insert(sym);
}

void Plugin::registerInputSymbols(set<SymbolDependency> in) {
	input_symbols.insert(in.begin(),in.end());
}

void Plugin::registerOutputSymbol(SymbolDependency sym)
{
	output_symbols.insert(sym);
}

void Plugin::registerOutputSymbols(set< SymbolDependency > out)
{
	output_symbols.insert(out.begin(),out.end());
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

bool Plugin::setParameter(const string& xml_path, string value)
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
// 		cout << this->XMLName() << ": Initializing parameter " << plugin_parameters2[i]->XMLPath()<< endl;
		plugin_parameters2[i]->init();
		
		auto in = plugin_parameters2[i]->getDependSymbols();
		input_symbols.insert(in.begin(), in.end());

		auto out = plugin_parameters2[i]->getOutputSymbols();
		output_symbols.insert(out.begin(), out.end());
	}
	
	if (! input_symbols.empty()) {
		cout << "Plugin " << XMLName() << ": Registered input symbol dependencies ";
		for (auto const& dep : input_symbols) {
			cout << dep->name() << " [" << dep->scope()->getName() << "] ,";
		}
		cout << endl;
	}
	if (! output_symbols.empty()) {
		cout << "Plugin" << XMLName() << ": Registered output symbol dependencies ";
		for (auto dep : output_symbols) {
			cout << dep->name() <<  " [" << dep->scope()->getName() << "] ,";
		}
		cout << endl;
	}
}

set< SymbolDependency > Plugin::getDependSymbols() const { return input_symbols; }

set< SymbolDependency > Plugin::getOutputSymbols() const { return output_symbols; }


class Annotation : public Plugin  { public: DECLARE_PLUGIN("Annotation"); };
REGISTER_PLUGIN(Annotation);

TimeStepListener::TimeStepListener(TimeStepListener::XMLSpec spec) : xml_spec(spec)
{
	valid_time = -1;
	time_step = -1;
	latest_time_step = -1;
	execute_systemtime = 0;
	execute_clocktime = 0;
}

set<SymbolDependency> TimeStepListener::getLeafDependSymbols() {
	if (leaf_input_symbols.empty()) {
		auto inputs = getDependSymbols();
		for (const auto& input : inputs) {
			auto leafs = input->leafDependencies();
			leaf_input_symbols.insert(leafs.begin(), leafs.end());
		}
	}
	return leaf_input_symbols;
}

set<SymbolDependency> TimeStepListener::getLeafOutputSymbols() {
	set<SymbolDependency> outputs;
	for (auto sym : getOutputSymbols()) {
		if ( !sym->dependencies().empty()) {
			auto leafs = sym->leafDependencies();
			outputs.insert(leafs.begin(), leafs.end());
		}
		else {
			outputs.insert(sym);
		}
		
	}
	return outputs;
}

void TimeStepListener::setTimeStep(double t)
{
// 	assert(t>0);
	time_step = t;
	prepared_time_step = t;
	if (time_step<0) {
		valid_time = numeric_limits< double >::max();
		latest_time_step = numeric_limits< double >::max();
	}
	else if (time_step == 0 ) {
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
	for (auto it: getLeafDependSymbols()) {
		const_cast<Scope*>(it->scope())->propagateSinkTimeStep(it->name(),ts);
	}
}

void TimeStepListener::propagateSourceTS(double ts)
{
	for (auto it: getLeafOutputSymbols()) {
		const_cast<Scope*>(it->scope())->propagateSourceTimeStep(it->name(),ts);
	}
}


void TimeStepListener::loadFromXML(const XMLNode node, Scope* scope)
{
	switch (xml_spec) {
		case XMLSpec::XML_NONE: 
			is_adjustable = true;
			break;
		case XMLSpec::XML_OPTIONAL: 
			if (node.isAttributeSet("time-step")) {
				xml_time_step.setXMLPath("time-step");
				registerPluginParameter(xml_time_step);
				is_adjustable = false;
				xml_spec = XMLSpec::XML_REQUIRED;
			}
			else {
				xml_spec = XMLSpec::XML_NONE;
				is_adjustable = true;
				time_step = -1;
			}
			break;
		case XMLSpec::XML_REQUIRED:
			xml_time_step.setXMLPath("time-step");
			registerPluginParameter(xml_time_step);
			is_adjustable = false;
			break;
	}
		
	Plugin::loadFromXML(node, scope);
}


void TimeStepListener::init(const Scope* scope)
{
	Plugin::init(scope);
	
	valid_time = SIM::getTime();
	
	if (xml_spec == XMLSpec::XML_REQUIRED) {
		if (xml_time_step(SymbolFocus::global)>0) {
			setTimeStep(xml_time_step(SymbolFocus::global));
		}
		else {
			setTimeStep(-1);
		}
	}
	else if (xml_spec == XMLSpec::XML_OPTIONAL) {
		if (xml_time_step.isMissing() || xml_time_step(SymbolFocus::global)<=0) {
			is_adjustable = true;
			setTimeStep(time_step);
		}
		else {
			setTimeStep(xml_time_step(SymbolFocus::global));
			is_adjustable = false;
		}
	}
	else {
		is_adjustable = true;
		setTimeStep(time_step);
	}

	TimeScheduler::reg(this);
}

void TimeStepListener::prepareTimeStep_internal(double max_time)
{
	auto start = highc.now();
#ifdef _POSIX_CPUTIME
	timespec cstart;
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &cstart);
#else
	auto cstart = std::clock();
#endif
	
	prepared_time_step = min(time_step>0 ? time_step : SIM::getStopTime() - valid_time , max_time-valid_time);
	prepareTimeStep_impl(prepared_time_step);
	
#ifdef _POSIX_CPUTIME
	timespec cstop;
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &cstop);
	double dt = difftime(cstart.tv_sec, cstop.tv_sec) + (cstop.tv_nsec-cstart.tv_nsec)*1e-6;
	if (dt > 0) { execute_clocktime += dt; }
#else
	auto cstop = std::clock();
	if (cstop-cstart > 0) { execute_clocktime += (1000.0 * (cstop-cstart)) / CLOCKS_PER_SEC; }
#endif
	
	execute_systemtime += chrono::duration_cast<chrono::microseconds>(highc.now()-start).count()  * 1e-3;
}

void TimeStepListener::executeTimeStep_internal()
{
	auto start = highc.now();
#ifdef _POSIX_CPUTIME
	timespec cstart;
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &cstart);
#else
	auto cstart = std::clock();
#endif
	
	executeTimeStep_impl();

#ifdef _POSIX_CPUTIME
	timespec cstop;
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &cstop);
	double dt = difftime(cstart.tv_sec, cstop.tv_sec) + (cstop.tv_nsec-cstart.tv_nsec)*1e-6;
	if (dt > 0) { execute_clocktime += dt; }
#else
	auto cstop = std::clock();
	if (cstop-cstart > 0) {
		execute_clocktime += (1000.0 * (cstop-cstart)) / CLOCKS_PER_SEC;
	}
#endif
	execute_systemtime += chrono::duration_cast<chrono::microseconds>(highc.now()-start).count() * 1e-3;
	
	latest_time_step = SIM::getTime();
	if (prepared_time_step>=0) {
		valid_time += prepared_time_step;
	}
	else 
		valid_time = SIM::getStopTime();
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
	valid_time = SIM::getTime()/* + timeStep()*/;
	
}

ReporterPlugin::ReporterPlugin(TimeStepListener::XMLSpec spec) : TimeStepListener(spec) 
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


