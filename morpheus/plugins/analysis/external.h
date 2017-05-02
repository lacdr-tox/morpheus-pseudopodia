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

/** \defgroup External
\ingroup AnalysisPlugins
\brief Execute external shell script

\section Description

Executes shell command periodically or at the end of simulation, e.g. to perform analysis using external command line tools. 

Commands are executed via shell, which means that .bashrc or .profile are read.

To customize the environment, one can set (override) environmental variables (e.g. $PATH).

One can run the shell command in a separate thread in which case the command is simply appended with '&' to fork the process.

Use "%" (percentage) to provide global symbols as arguments in the script. For instance the substring "%time" will be replaced with the current time.

Stdout and sdterr are written to the file "external_output.txt" and "external_error.txt"

- \b separate-thread: Execute process as a fork (appends '&').
\brief Execute external shell script

\section Description

Executes shell command periodically or at the end of simulation, e.g. to perform analysis using external command line tools. 

Commands are executed via shell, which means that .bashrc or .profile are read.

To customize the environment, one can set (override) environmental variables (e.g. $PATH).

One can run the shell command in a separate thread in which case the command is simply appended with '&' to fork the process.

Use "%" (percentage) to provide global symbols as arguments in the script. For instance the substring "%time" will be replaced with the current time.

Stdout and sdterr are written to the file "external_output.txt" and "external_error.txt"

- \b separate-thread: Execute process as a fork (appends '&').
- \b Command: Executable shell command, e.g. "tail -n 1 logger.txt" or "python analysis.py"
- \b Environment: variable/value, e.g. PATH=\usr\local\bin

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

Execute script in a separate thread. This is identical to appending the shell fork command '&'.
\verbatim
<External time-step="100" separate-thread="true">
	<Command string="python /home/USER/scripts/analysis.py">
</External>
\endverbatim
*/

/**
    @author Walter de Back
*/

#ifndef EXTERNAL_H
#define EXTERNAL_H

#include <core/interfaces.h>
#include <core/simulation.h>
#include <core/plugin_parameter.h>
#include <plugins/analysis/subprocess.hpp>

class External : public AnalysisPlugin
{

private:
	string command_orig;
	map<string, string> environvars; // environmental variables
	PluginParameter2<bool, XMLValueReader, DefaultValPolicy> fork; // run shell command in separate thread

	bool replace_symbols; // to replace global symbols indicated with %, e.g. %time
	string update_command(string command);
public:
    DECLARE_PLUGIN("External");
    External();

    virtual void loadFromXML(const XMLNode );
    virtual void init(const Scope* scope);
    /// record cell positions
    virtual void analyse(double time);
    /// write cell tracks to XML file
    virtual void finish();
	void execute();

};

#endif // CELL_TRACKER_H
