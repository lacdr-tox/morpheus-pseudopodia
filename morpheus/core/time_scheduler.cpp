#include "time_scheduler.h"

TimeScheduler::TimeScheduler() : start_time("StartTime",0), save_interval("SaveInterval",-1), stop_time("StopTime",1000), is_state_valid(false) {};

TimeScheduler& TimeScheduler::getInstance() {
	static TimeScheduler sched;
	return sched;
}

void TimeScheduler::loadFromXML(XMLNode xTime)
{
	TimeScheduler& ts = getInstance();
	ts.xmlTime = xTime;
	
	ts.start_time.loadFromXML(xTime.getChildNode("StartTime"));
	ts.stop_time.loadFromXML(xTime.getChildNode("StopTime"));
		
	string expression;
	 if (getXMLAttribute(xTime,"StopCondition/Condition/text",expression)) {
		ts.stop_condition = shared_ptr<ExpressionEvaluator<double> >(new ExpressionEvaluator<double>(expression)) ;
	}
		
	if (xTime.nChildNode("SaveInterval")) { 
		ts.save_interval.loadFromXML(xTime.getChildNode("SaveInterval")); 
	}
	ts.current_time = ts.start_time();
	
}


void TimeScheduler::init()
{
	TimeScheduler& ts = getInstance();
	ts.current_time = ts.start_time();
	ts.progress_notify_interval = (ts.stop_time() - ts.current_time)/ 100;
	
	/// REgister symbol Readers and Writers in their respective Scopes
	for (auto tsl : ts.all_listeners) {
		cout << tsl->XMLName() << endl;
		const_cast<Scope*>(tsl->scope())->registerTimeStepListener(tsl);
		
		set<SymbolDependency> dep_sym = tsl->getDependSymbols();
		for (auto dep : dep_sym) {
			assert(dep.scope);
			const_cast<Scope*>(dep.scope)->registerSymbolReader(tsl,dep.name);
		}
		
		set<SymbolDependency> out_sym = tsl->getOutputSymbols();
		for (auto out : out_sym) {
			assert(out.scope);
			const_cast<Scope*>(out.scope)->registerSymbolWriter(tsl,out.name);
		}
	}
	
	/// Propagate the intrinsic TSL time steps through to scopes of their dependencies
	double minimal_time_step = 1.0;

	for (auto tsl : ts.all_listeners) {
		if (tsl->timeStep() > 0) {
			
			double time_step = tsl->timeStep();
			if (time_step < minimal_time_step)
				minimal_time_step = time_step;
			
			cout << "\n TimeStepListener \"" << tsl->XMLName() << "\" propagates its time step " << time_step << endl;
			cout << " Upstream: " << endl;
			set<SymbolDependency> dep_sym = tsl->getDependSymbols();
			for (auto dep : dep_sym) {
				cout << " -> " << dep.name  << endl;
				const_cast<Scope*>(dep.scope)->propagateSinkTimeStep(dep.name, time_step);
			}
			cout << " Downstream: " << endl;
			set<SymbolDependency> out_sym = tsl->getOutputSymbols();
			for (auto out : out_sym) {
				cout << "-> "<< out.name << endl;
				const_cast<Scope*>(out.scope)->propagateSourceTimeStep(out.name, time_step);
			}
		}
	}
	
	
	// Splitting up the listeners in the various subclasses
	for (uint i=0; i<ts.all_listeners.size(); i++) {
		if ( dynamic_cast<ContinuousProcessPlugin*>(ts.all_listeners[i]) ) {
			ts.continuous.push_back( dynamic_cast<ContinuousProcessPlugin*>(ts.all_listeners[i]) );
		}
		else if ( dynamic_cast<InstantaneousProcessPlugin*>(ts.all_listeners[i]) ) {
			ts.instantaneous.push_back(dynamic_cast<InstantaneousProcessPlugin*>(ts.all_listeners[i]));
			ts.all_phase2.push_back(ts.all_listeners[i]);
		}
		else if ( dynamic_cast<ReporterPlugin *>(ts.all_listeners[i])) {
			ts.reporter.push_back( dynamic_cast<ReporterPlugin *>(ts.all_listeners[i]));
			ts.all_phase2.push_back(ts.all_listeners[i]);
		}
		else if ( dynamic_cast<AnalysisPlugin *>(ts.all_listeners[i]) ) {
			ts.analysers.push_back( dynamic_cast<AnalysisPlugin *>(ts.all_listeners[i]) );
		}
		else {
			cout << "Something went wrong in the definition of Scheduling types " << endl;
			assert(0); exit(-1);
		}

	}

	// Sort Phase I listeners (continuous) using their rank 
	sort(ts.continuous.begin(),ts.continuous.end(),[] (ContinuousProcessPlugin* lhs, ContinuousProcessPlugin* rhs) -> bool {return lhs->getRank() < rhs->getRank();} );
	
	// Sort Phase II listeners using their interdependencies
	cout << "TimeScheduler::init: Ordering instantaneous processes for sequential update ..." << endl;
	// since Output symbols can be defined in multiple contexts,
	// symbol dependencies can be registered multiple times
	for (uint i=0; i<ts.all_phase2.size(); i++) {
		set<SymbolDependency> out_sym = ts.all_phase2[i]->getOutputSymbols();
		for (auto sym : out_sym) {
			if (!sym.scope->isSymbolDelayed(sym.name)) {
				const_cast<Scope*>(sym.scope)->addUnresolvedSymbol(sym.name);
			}
		}
	}
	
	vector<bool> scheduled_inst;
	vector<InstantaneousProcessPlugin*> ordered_instant;
	vector<ReporterPlugin*> ordered_reporters;
	vector<TimeStepListener*> ordered_phase2;
	scheduled_inst.resize(ts.all_phase2.size(),false);

	// Iteratively sort out the all_instant instances with no unresolved output symbols left
	// and remove their output symbols from the unresolved_symbols list
	for (uint i=0;i<ts.all_phase2.size();i++) {
		uint j;
		// Schedule ONE instance
		for (j=0; j<ts.all_phase2.size();j++) {
			if ( scheduled_inst[j] ) 
				continue;
			
			set<SymbolDependency> dep_syms = ts.all_phase2[j]->getDependSymbols();
			set<string> unresolved_symbols;
			for (auto dep: dep_syms ){
				if (const_cast<Scope*>(dep.scope)->isUnresolved(dep.name)) {
					unresolved_symbols.insert(dep.name);
				}
			}
			
			// Try to remove the ouput symbols of the Listener, in case they were wrongly registered also as input symbols
			if ( ! unresolved_symbols.empty()) {
				set<SymbolDependency> out_syms = ts.all_phase2[j]->getOutputSymbols();
				for (auto out: out_syms ){
					if (unresolved_symbols.count(out.name)) {
						unresolved_symbols.erase(out.name);
					}
				}
			}
			
			// 
			if (unresolved_symbols.empty()) {
				// Remove the output symbols from beeing unresolved
				set<SymbolDependency> out_syms = ts.all_phase2[j]->getOutputSymbols();
				for (auto out: out_syms ){
					const_cast<Scope*>(out.scope)->removeUnresolvedSymbol(out.name);
				}
				// Add the Listener to the ordered container
				ordered_phase2.push_back(ts.all_phase2[j]);
				scheduled_inst[j] = true;
				if ( dynamic_cast<InstantaneousProcessPlugin*>(ts.all_phase2[j]) ) {
					ordered_instant.push_back(dynamic_cast<InstantaneousProcessPlugin*>(ts.all_phase2[j]));
				}
				else {
					ordered_reporters.push_back(dynamic_cast<ReporterPlugin*>(ts.all_phase2[j]));
				}
				break;
			}
		}
		
		// Exit if no instance that can be scheduled is found ...
		if (j==ts.all_phase2.size()) {
			cerr << "TimeScheduler detected loop dependencies in Reporters / Instantaneous Processes :" << endl;
			for (j=0; j<ts.all_phase2.size();j++) {
				if ( ! scheduled_inst[j]) {
					cerr << ts.all_phase2[j]->XMLName() << endl;
// 					     << (ts.all_phase2[j]->getFullName().empty() ? string() : string(" [")+ts.all_phase2[j]->getFullName()+"]")
// 					     << " [" << join(ts.all_phase2[j]->getDependSymbols(),",") << "] -> [" << join(ts.all_phase2[j]->getOutputSymbols(),",") << "]" << endl;
				}
			}
// 			cerr << "Left unresolved " << join(unresolved_symbols,",") << endl;
			exit(-1);
		}
	}

	ts.all_phase2 = ordered_phase2;
	ts.reporter = ordered_reporters;
	ts.instantaneous = ordered_instant;
	
	
	cout << " \n";
	cout << "======================================================\n";
	cout << " Time Schedule\n";
	cout << "======================================================\n";
	cout << " Time precision patch is " << ts.time_precision_patch << "\n";
	cout << "\n";
	cout << "=====|    Phase I     |===============================\n";
	
	int current_rank=-1;
	vector<string> rank_names;
	rank_names.push_back("CPM");
	rank_names.push_back("Delays");
	rank_names.push_back("Reactions");
	rank_names.push_back("Diffusion");
	for (uint i=0; i<ts.continuous.size(); i++) {
		if (current_rank<ts.continuous[i]->getRank()) {
			current_rank = ts.continuous[i]->getRank();
			cout << "-----. "  << rank_names[current_rank] << " .------------------------------------\n";
		}
		cout << "  + " << ts.continuous[i]->timeStep() << " => " << ts.continuous[i]->XMLName();
		if ( ! ts.continuous[i]->getFullName().empty() )
		     cout << " [" << ts.continuous[i]->getFullName() << "]";
		set<string> dep;
		for (auto it : ts.continuous[i]->getDependSymbols() ) dep.insert(it.name);
		set<string> out;
		for (auto it : ts.continuous[i]->getOutputSymbols() ) out.insert(it.name);
		cout << " [" << join(dep,",") << "] -> [" << join(out,",") << "]\n";
	}

	cout << "\n";
	cout << "=====|    Phase II    |===============================\n";
	cout << "-----. Reporters and Instantaneous Processes.---------\n";
	for (uint i=0; i<ts.all_phase2.size(); i++) {
		cout << "  + " << ts.all_phase2[i]->timeStep() << " => " << ts.all_phase2[i]->XMLName();
		if ( ! ts.all_phase2[i]->getFullName().empty() )
		     cout << " [" << ts.all_phase2[i]->getFullName() << "]";
		set<string> dep;
		for (auto it : ts.all_phase2[i]->getDependSymbols() ) dep.insert(it.name);
		set<string> out;
		for (auto it : ts.all_phase2[i]->getOutputSymbols() ) out.insert(it.name);
		cout << " [" << join(dep,",") << "] -> [" << join(out,",") << "]\n";
	}

	cout << "\n";
	cout << "=====|    Phase III   |===============================\n";
	cout << "-----. Analysis .-------------------------------------\n";
	for (uint i=0; i<ts.analysers.size(); i++) {
		cout << "  + " << ts.analysers[i]-> timeStep() << " => " << ts.analysers[i]->XMLName();
		set<string> dep;
		for (auto it : ts.analysers[i]->getDependSymbols() ) dep.insert(it.name);
		cout << " [" << join(dep,",") << "] \n";
	}
	cout << "------------------------------------------------------\n";
	
	cout << "======================================================\n";
	cout << endl;
	
	ts.current_time = 0;
	
	if (ts.stop_condition) {
		ts.stop_condition->init(SIM::getGlobalScope());
	}
}


void TimeScheduler::compute()
{
    std::chrono::high_resolution_clock highc;
    auto start = highc.now();
	Plugin* current_plugin = nullptr;
	TimeScheduler& ts = getInstance(); 
	try {
		if (! ts.is_state_valid ) {
			// Now run all Reporters, Equations.
			for (uint i=0; i<ts.reporter.size(); i++) {
				// if (ts.reporter[i]->currentTime() <= ts.current_time + ts.time_precision_patch ) {
					current_plugin = ts.reporter[i];
					ts.reporter[i]->executeTimeStep_internal();
				//}
			}

			// Now run all analysers
			for (uint i=0; i<ts.analysers.size(); i++) {
				if ( ! ts.analysers[i]->endState() ) {
					current_plugin = ts.analysers[i];
					ts.analysers[i]->executeTimeStep_internal();
				}
			}
			ts.is_state_valid = true;
			cout << setprecision(2) << setiosflags(ios::fixed)
			<< "Time: " << ts.current_time / ts.stop_time.getTimeScaleUnitFactor() << " "
			<< SIM::getTimeScaleUnit()
			<< endl;
			ts.last_progress_notification = ts.current_time;
			
		}
			
		double stop_time = ts.getStopTime();
		while (ts.current_time + ts.time_precision_patch < stop_time) {
			double min_current_time = stop_time;
			
			// We provide a time schedule for time CONTINUOUS processes, where X(t) just depends on X(t-1)
			// And a second schedule for INSTANTANEOUS events for which we use the synchronous X(t) = f (X(t-1))events
			// In case they are REPORTERS, they will be resolved consecutively in the order of their interdependency
			//
			
			///////////////////////////////////////////////////////////////
			// PHASE I -- TIME CONTINUOUS -- Synchronously updates schemes
			///////////////////////////////////////////////////////////////
			
			// Run the computations to a buffer for reactions, ...
			for (uint i=0; i<ts.continuous.size(); i++) {
				if (ts.continuous[i]->currentTime() <= ts.current_time + ts.time_precision_patch) {
					current_plugin = ts.continuous[i];
					ts.continuous[i]->prepareTimeStep_internal();
				}
			}
			
			// Now execute all required updates on continuous-time schemes. First will be CPM, then Delays, then Reactions, then Diffusion
			for (uint i=0; i<ts.continuous.size(); i++) {
				if (ts.continuous[i]->currentTime() <= ts.current_time + ts.time_precision_patch) {
					current_plugin = ts.continuous[i];
					ts.continuous[i]->executeTimeStep_internal();
				}
				min_current_time = min(min_current_time,ts.continuous[i]->currentTime() );
			}
			
			// Now also respect the instantaneous processes and look how far in the future they are valid
			for (uint i=0; i<ts.instantaneous.size(); i++) {
				min_current_time = min(min_current_time, ts.instantaneous[i]->currentTime());
			}
			
			// That's how far we are able to travel within one step, still keeping all processes in a valid state.
			ts.current_time = min_current_time;
			
			////////////////////////////////////////////////////////////////////////////////
			// PHASE II -- INSTANTANEOUS, sequentially sorted (Equations, Events, Reporters)
			////////////////////////////////////////////////////////////////////////////////
			
			for (uint i=0; i<ts.all_phase2.size(); i++) {
				if (ts.all_phase2[i]->currentTime() <= ts.current_time + ts.time_precision_patch) {
					current_plugin = ts.all_phase2[i];
					ts.all_phase2[i]->executeTimeStep_internal();
				}
			}


			////////////////////////////////////////////////////////////////////////////////
			// PHASE III -- ANALYSIS
			////////////////////////////////////////////////////////////////////////////////
			
			for (uint i=0; i<ts.analysers.size(); i++) {
				if (ts.analysers[i]->currentTime() <= ts.current_time + ts.time_precision_patch) {
					if( !ts.analysers[i]->endState() ) {
						current_plugin = ts.analysers[i];
						ts.analysers[i]->executeTimeStep_internal();
					}
				}
			}
			
			// Progress notification
			if (ts.last_progress_notification + ts.progress_notify_interval <= ts.current_time + ts.time_precision_patch) {
				cout << setprecision(2) << setiosflags(ios::fixed)
					<< "Time: " << ts.current_time / ts.stop_time.getTimeScaleUnitFactor() << " "
	// 				 << SIM::sim_stop_time.getTimeScaleUnit()
					<< endl;
				ts.last_progress_notification = ts.current_time;
			}
			
			// Checkpointing 
			if ( ts.save_interval.getSeconds()>0 && ts.last_save_time + ts.save_interval.getSeconds() <= ts.current_time + ts.time_precision_patch){
				SIM::saveToXML();
				ts.last_save_time = ts.current_time;
			}
			
			// StopCondition
			if( ts.stop_condition ){
				SymbolFocus sf;
				if( ts.stop_condition->get( sf ) ){
					cout << "Simulation terminated on StopCondition (" << ts.stop_condition->getExpression() << ")" << endl;
					break;
				}
			}
		}

		if (ts.last_progress_notification < ts.current_time)
			cout << setprecision(2) << setiosflags(ios::fixed)
				<< "Time: " << ts.current_time / ts.stop_time.getTimeScaleUnitFactor() << " "
	// 			 << SIM::sim_stop_time.getTimeScaleUnit()
				<< endl;
		// Now run all Reporters, Equations, that did not run during the last time step
		for (uint i=0; i<ts.reporter.size(); i++) {
			if (ts.reporter[i]->latestTimeStep() < ts.current_time - ts.time_precision_patch ) {
				current_plugin = ts.reporter[i];
				ts.reporter[i]->executeTimeStep_internal();
			}
		}
		// Now run analysers that did not run during the last time step
		for (uint i=0; i<ts.analysers.size(); i++) {
			if (ts.analysers[i]->latestTimeStep() < ts.current_time - ts.time_precision_patch ) {
				cout << "Now running " << ts.analysers[i]->XMLName() << " last run at " << ts.analysers[i]->latestTimeStep() << endl;
				current_plugin = ts.analysers[i];
				ts.analysers[i]->executeTimeStep_internal();
			}
		}
	} 
	catch (string e){
		if (current_plugin)
			throw MorpheusException(e,current_plugin->getXMLNode());
		else
			throw e;
	}
    ts.execTime = chrono::duration_cast<chrono::microseconds>(highc.now()-start).count() / 1000.0;
// 	cout << endl;
}

void TimeScheduler::finish()
{
	TimeScheduler& ts = getInstance(); 
	for (uint i=0; i<ts.analysers.size(); i++) {
		ts.analysers[i]->finish();
	}
	
    cout << "======================================================\n";
	cout << " Time Schedule Performance Table\n";
	cout << "======================================================\n";
	cout << "\n";
	
	std::sort(ts.all_listeners.begin(),ts.all_listeners.end(),[](TimeStepListener* a, TimeStepListener* b) { return a->execSysTime()>b->execSysTime(); } );
	for (uint i=0; i<ts.all_listeners.size(); i++) {

		cout << "  + " << setw(6) << (ts.all_listeners[i]->execSysTime()/ts.execTime)*100.0 <<"%" << " = " << setw(8) << ts.all_listeners[i]->execSysTime() << "[ms] | ";
		
		cout << ts.all_listeners[i]->XMLName();
		if ( ! ts.all_listeners[i]->getFullName().empty() )
		     cout << " [" << ts.all_listeners[i]->getFullName() << "]";
		set<string> dep;
		for (auto it : ts.all_listeners[i]->getDependSymbols() ) dep.insert(it.name);
		set<string> out;
		for (auto it : ts.all_listeners[i]->getOutputSymbols() ) out.insert(it.name);
		cout << " [" << join(dep,",") << "] -> [" << join(out,",") << "]"; 
		cout << endl;
	}	
	cout << "======================================================\n";
	cout << endl;
	
	
	if (ts.save_interval() >= 0) SIM::saveToXML();
	
	ts.all_listeners.clear();
	ts.continuous.clear();
	ts.reporter.clear();
	ts.instantaneous.clear();
	ts.all_phase2.clear();
	ts.analysers.clear();
	
	ts.stop_condition.reset();
}


void TimeScheduler::reg(TimeStepListener* tsl)
{
	TimeScheduler& ts = TimeScheduler::getInstance();
    ts.all_listeners.push_back(tsl);
}
