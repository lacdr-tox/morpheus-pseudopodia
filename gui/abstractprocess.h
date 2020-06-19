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

#ifndef ABSTRACTPROCESS_H
#define ABSTRACTPROCESS_H

#include <QtGui>
#include <QFile>
#include <QDomDocument>
//#include <iomanip>
//#include <sstream>
#include "config.h"
#include "morpheus_model.h"

using namespace std;

struct ProcessInfo {
    enum Resource{ local, remote };
    enum status {PEND, PSUSP, RUN, USUSP, SSUSP, DONE, EXIT, UNKWN, ZOMBI};
    int job_id;
    QString title; /*!< Title of the cpm-model. */
    QString model_file;
    status state;
    double start_time; double stop_time; int snapshot_time; double current_time;
    int run_time; int progress;
    QString run_time_string;
    QString sim_dir;
//    QString sim_sub_dir;
    Resource resource; /*!< Resource on which the simulation is executed (local or remote). */
};

struct ProcessMessage {
	enum Type {Notification, Error} type;
	int job_id;
	int sweep_id;
	int progress;
	QString message;
};

/*!
This class represents a simulation-process of a cpm-model.<br>
It describes the state of the simulation, for example if its pending, running, done or anything else.<br>
For every type of simulation exists a special sub-class.<br>
Local simulation-processes are running on the local machine and remote processes are running on an external machine.<br>
The abstract class holds the number of processes/simulations, the path where the data should be stored,<br>
the output of the simulation, the time how long its running, ...<br>
*/
class abstractProcess : public QObject
{
Q_OBJECT

public:
    static const int max_output_size = 1e7;
	static const bool zip_models = false;

    abstractProcess(SharedMorphModel model, int job_id, QString sub_dir);
    /*!<
      Creates a new simulation-process.
      \param file xml-configuration-file for the cpm-modell
      \param pid process identification digit, via it is unique
      \param group name of the simulation-group (used for parameter-sweeps)
      */
//     abstractProcess(QSettings& settings);

    abstractProcess( QSqlRecord& r );
    /*!<
      Restores an old unfinished process from SQL database
      \param settings Record from SQL database, which describes the old unfinished process
      */
    ~abstractProcess();

//    int time_elapsed; /*!< Duration of the process since its start */

    virtual void start() = 0; /*!< Virtual function to start a process (should be overwritten by subclasses) */
    virtual void stop() = 0; /*!< Virtual function to stop a process (should be overwritten by subclasses) */
    virtual void contin() = 0;
    virtual void remove(bool force=false); /*!< Virtual function to remove a process (should be overwritten by subclasses) */
    virtual void debug() = 0;

    /*!<
      Status of the simulation-process.
      PEND means the process is pending (it has not yet been started).
      PSUSP means the process was suspended while pending.
      RUN means the process is currently running.
      USUSP means the process was suspended while running.
      SSUSP means the process was suspended by the LSF.
      DONE means the process has terminated with status of 0.
      EXIT means the process has terminated with a non-zero-status.
      UNKWN means the state of process is unknown.
      ZOMBI means ??? .
      */

    ProcessInfo::status state() const; /*!< Returns the state of process. */
    int jobID() const { return _info.job_id; };
    QString translateState() const; /*!< Returns a QString-version of the state of process. */
    QString getStateString() const;
    QString getStateMessage() const; /*!< Returns a understandable declaration what the current state means. */
    static ProcessInfo::status stateMsgToState(QString stateMsg);

    const ProcessInfo& info() const;
    const QString& getOutput(); /*!< Returns the output produced while simulation is running. */
    int getNumThreads() const; /*!< Returns the number of threads used by the process. */

    void setRunTime(int); /*!< Returns the time left since starting the process. */
    int getLastSnapshot();

    virtual void storeJob(); /*!< Virtual function to store the job-settings and -state in the QSettings. */
	bool changeState(ProcessInfo::status state); /*!< Updates the job- state the SQL database. */

protected:
    qint64 ID; /*!< Internal id of the process (id from the underlying system) */
    QString model_file_name; /*!< name of the xml-file, which describes the cpm-model. */
    QString model_source_file;  /*!< name of the xml-file as it was opened in the editor */
    mutable ProcessInfo _info;

    bool pending_read;
    QString output; /*!< Output produced while simulation is running. */
    QFile output_file; /*!< Output-file, where all the output of simulation is logged. */
    qint64 output_read; /*!< Variable indicates if output can be read. */
    bool readOutput();
	void updateRunTimeString();

    int numthreads; /*!< Number of threads, which can be used by the simulation-process. */
    QString localDir; /*!< The root directory for all simulations data. */
    //QString simDir; /*!< The subdirectory to store the simulation data. */
    QString outputDir; /*!< Absolut directory-path, in which the data is stored. */
signals:
	void criticalMessage(QString );

private:
    void setUp(SharedMorphModel);
    /*!<
      Sets the output-directory for simulation-data in cpm-model.
      \param file xml-file in which the directory-path have to set
      */
    bool removeDir(const QString &dirName);
    /*!<
      Deletes a directory on the filesystem including all sub-directorys.
      \param dirName Name of the directoy.
      */
    bool internal_readOutput();
    /*!<
      Does the real job reading the output from file.
      */
signals:
    void stateChanged(abstractProcess*); /*!< Signal will be emited when the state of simulation-process changed! */
    void outputChanged(abstractProcess*); /*!< Signal will be emited when the output of simualtion changes! */
};

#endif // ABSTRACTPROCESS_H
