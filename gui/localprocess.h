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

#ifndef LOCALPROCESS_H
#define LOCALPROCESS_H

#include "abstractprocess.h"
#include "config.h"
#include <fstream>
#include <QProcess>

using namespace std;

/*!
The localProcess represents a simulation of a cpm-model on the local-user-machine.<br>
For more detailed informations please look at the abstractProcess.
*/
class localProcess : public abstractProcess
{
Q_OBJECT

public:
    localProcess(SharedMorphModel model, int pid, QString sub_dir="");
    /*!<
      Creates a on the local machine running simulaton process of a given cpm-model.
      \param file xml-file ehich describes the cpm-model, that shall be simulated
      \param pid the global id via which the process is unique
      \param group name of the group in which the process will be started
      */
//     localProcess(QSettings& settings);
    localProcess(QSqlRecord& record);
    /*!<
      Recreates a on the local machine running unfinished simulation process from SQL database
      \param settings SQL database table storing jobs
      */
    ~localProcess();

    void start(); /*!< Starts the simulation process on the local machine. */
    void stop(); /*!< Stops the simulation process. */
    void contin() {};
    void remove(bool force=false); /*!< Removes the simulation process and all its data from the filesystem. */
    void debug(); /*!< Debug */

private:
    void init(); /*!< Initiate the process. Sets the output- and error-file, the maximal number of threads, ... */

    QProcess *process; /*!< QProcess which is used to call and simulate the given model. */

    QTimer *timer; /*!< Timer which is used to check the runtime of process. */
    QTime *stopwatch; /*!<  */

    QFile *errFile; /*!< Error-file, where all errors during the simulation are logged. */

private slots:
    void changeState(QProcess::ProcessState state);
    /*!<
      Changes the current state of process.
      \param state new state of process
      */

    void readErrorMsg(); /*!< Reads the error messages of simualtion and shows them in a messagebox. */
    void checkRuntime(); /*!< Checks the current runtime of process and stops it if it exceed the maxRuntime. */
};

#endif // LOCALPROCESS_H
