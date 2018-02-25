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

#ifndef TIME_SCHEDULER_H
#define TIME_SCHEDULER_H

#include "xml_functions.h"
#include "scales.h"
#include "expression_evaluator.h"
#include "interfaces.h"
// #include "function.h"

/** \page Scheduling Time Scheduling
frequencies to dependent processes.  
 *  \brief Time Scheduling takes care to safely join models with different updating schemes into a coupled simulation system,
 *  and to propagate update frequencies to dependent processes.
 * 
 *  Time continuous vs. time discrete vs. instantaneous vs. reporters vs. analysers ..
 * 
 *  Here shall be a lot of information about the scheduling ....
 *  \section Aims
 *   - Determine the order of updating schemes and plugins
 *   - Determine the update frequencies based on the model interdependencies and numerical constraints.
 * 
 *  \section DependencyProTree Dependency tree
 * 
 *  \section UpdatePhase Update phases
 * 
 *  Conservative time propagation.
 */

class TimeScheduler {
	
private:
	TimeScheduler();
	static TimeScheduler& getInstance();
	vector<TimeStepListener *> all_listeners;
	vector<ContinuousProcessPlugin *> continuous;
	vector<ReporterPlugin *> reporter;
	vector<InstantaneousProcessPlugin *> instantaneous;
	vector<TimeStepListener *> all_phase2;
	vector<AnalysisPlugin *> analysers;
	
	Scope* global_scope;
	double current_time;
	double last_save_time;
	double minimal_time_step;
	double time_precision_patch;
	double progress_notify_interval;
	double last_progress_notification;
	XMLNode xmlTime;
	
    double execTime;
	bool is_state_valid;
	
	Time_Scale start_time, save_interval, stop_time;
	shared_ptr <ExpressionEvaluator<double> > stop_condition;

public:
	/// Register a TimeStepListener based plugin
	static void reg(TimeStepListener *tsl);
	static void loadFromXML(XMLNode xTime, Scope* scope);
	static XMLNode saveToXML();

	static void init();
	/// compute until time 
	static void compute();
	
	static void finish();
	
	static double getTime() { return getInstance().current_time; };
	static string getTimeScaleUnit() { return getInstance().stop_time.getTimeScaleUnit(); };
// 	static double getTimeScaleValue() ;
	static double getStartTime() { return getInstance().start_time(); };
	static double getStopTime() { return getInstance().stop_time(); };
};

	// TODO: This shall become getTimeString
// 	string getTimeName();

#endif
