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

/** 
 * \page Scheduling Scheduling
 * \brief Time Scheduling takes care to safely join various model components into a coupled simulation.
 * 
 * Model components with different updating schemes, i.e sub-models, data mappers, events, .. ,
 * are safely joined into a coupled simulation by ordering their execution and adjusting their
 * update frequencies using the dependency graph.
 * 
 * Based on their rules for time step adjustment we discriminate time continuous, time discrete
 * and instantaneous processes, mappers and analysers (see TimeStepListener).
 * 
 * \section Aims
 *   - Determine a valid sequential order of the model components.
 *   - Determine an optimized update frequencies based on the model interdependencies \
 *     and numerical constraints. the constraints of the numerical schemes.
 * 
 *  \section DependencyTree Dependency tree
 * Morpheus uses Symbol
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
