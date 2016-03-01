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

#ifndef REMOTEPROCESS_H
#define REMOTEPROCESS_H

#include "abstractprocess.h"
#include "sshproxy.h"
#include "synchronizer.h"

using namespace std;

/*!
The remoteProcess represents a simulation of a cpm-model on a external machine.<br>
Informations about the process were stored and rejected in/from the QSettings.<br>
For more detailed informations please look at the abstractProcess.
*/


class remoteProcess : public abstractProcess
{
Q_OBJECT

public:
    remoteProcess(QSharedPointer<MorphModel> model, int pid, QString sub_dir="");
    /*!<
      Creates a simulationprocess which will be started on an external machine.
      \param xml xml-files that describes the cpm-model which will be simulated
      \param pid global id of the process which makes the process unique
      \param group name of the group in which the process will be started
      */
//     remoteProcess(QSettings&); /*!< Restores an unfinished process from the QSettings. */
    remoteProcess(QSqlRecord&); /*!< Restores an unfinished process from the SQL database. */
    ~remoteProcess();

    void start(); /*!< Starts the simulationprocess. */
    void stop(); /*!< Stops the simulationprocess. */
    void contin() {};
    void remove(bool force=false); /*!< Removes all data the simulationprocess has produced and saved on the filesystem. */
    void debug();

    virtual void storeJob(); /*!< Stores information to the unfinished process in QSettings. */

    QString getResource() const; /*!< Returns the used resource (in this case 'remote'). */

private:
    void init(); /*!< Initiates the process, sets output-files, create checktimers, ... */

    QString remoteDir; /*!< Directory on the external machine, where the simulationdata should be stored. */
    QTimer *checkTimer; /*!< Timer with whom the state of process will be checked. */
    QTime *stopwatch;
    sshProxy ssh; /*!< sshProxy which is used to connect ot the external machine. */
    QMutex check_lock;

    void readSimOutput(); /*!< Reads the new simulation-output and stores it in member variable. */
    void startZip(); /*!< Starts the process to zip all simulation-data. */
    void finishZip(); /*!< Finishes the ipping of simulation-data for following copying of the data to the local machine. */
    int setState(QString); /*!< Sets the new state of process. */

    enum runTask {NONE, SIM, ZIP, SYNC} run_task; /*!< Diffent running-tasks which will be passed during simulation. */

private slots:
    void checkState(); /*!< Checks the current state of simulation-process. */
};

#endif // REMOTEPROCESS_H
