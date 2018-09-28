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

/** 
\defgroup ML_External External
\ingroup ML_Analysis
\ingroup AnalysisPlugins
\brief Execute external shell script

\section Description

Executes shell command periodically or at the end of simulation, e.g. to perform analysis using external command line tools. 

Commands are executed via shell, which means that .bashrc or .profile are read.

To customize the environment, one can set (override) environmental variables (e.g. $PATH).

Use "%" (percentage) to provide global symbols as arguments in the script. For instance the substring "%time" will be replaced with the current time.

Stdout and sdterr are written to the file "external_output.txt" and "external_error.txt"

- \b detach: Run the process in the background while continuing simulation. Note that **timeout** is the maximum time the command may run after the simulation finished.
- \b timeout: Timeout for running the external process. Defaults to 30 seconds. (Only applicable with \b detach enabled)
- \b Command: Executable shell command, e.g. "tail -n 1 logger.txt" or "python analysis.py"
- \b Environment: variable/value, e.g. PATH='\\usr\\local\\bin'

\section Example
Say hi at the end of simulation
\verbatim
<External time-step="0">
	<Command string="echo 'Hello World'">
</External>
\endverbatim

Periodically execute python script using simulation output folder as cwd
\verbatim
<External time-step="100">
	<Command string="python /home/USER/scripts/analysis.py">
	<Environment variable="PYTHONPATH" value="/usr/lib/python2.7/">
</External>
\endverbatim

Provide the current time as argument to a python script
\verbatim
<External time-step="100">
	<Command string="python /home/USER/scripts/analysis.py --time=%time">
</External>
\endverbatim

Execute script along the simulation in a background process.
\verbatim
<External time-step="100" separate-thread="true">
	<Command string="python /home/USER/scripts/analysis.py">
</External>
\endverbatim
*/

/**
    @author Walter de Back, Jörn Starruß
*/

#ifndef EXTERNAL_H
#define EXTERNAL_H

#include <core/interfaces.h>
#include <core/simulation.h>
#include <core/plugin_parameter.h>
#include "tiny-process/process.hpp"
#include <thread>
#include <cstdlib>

struct DetachedProcess {
	enum { RUNNING, KILLED, FINISHED } state;
	int return_code;
	shared_ptr<TinyProcessLib::Process> process;
};

class External : public AnalysisPlugin
{

private:
	map<string, string> environvars; // environmental variables
	PluginParameter2<bool, XMLValueReader, DefaultValPolicy> detach; // run shell command in separate thread
	PluginParameter2<int, XMLValueReader, DefaultValPolicy> timeout; // run shell command in separate thread
	PluginParameter2<string, XMLValueReader, RequiredPolicy> command_orig;

	bool replace_symbols; // to replace global symbols indicated with %, e.g. %time
	string update_command(string command);
	static int instance_counter;
	int instance_id;
	list< shared_ptr<DetachedProcess> > detached_processes;
public:
    DECLARE_PLUGIN("External");
    External();
	~External();

    virtual void loadFromXML(const XMLNode xNode, Scope* scope) override;
    virtual void init(const Scope* scope) override;
    /// record cell positions
    virtual void analyse(double time) override;
    /// write cell tracks to XML file
    virtual void finish() override;
	void execute();

};

#endif // CELL_TRACKER_H
