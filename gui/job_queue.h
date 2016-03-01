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

#ifndef JOB_QUEUE_H
#define JOB_QUEUE_H
#include <QThread>
#include <QSharedPointer>
#include <QList>
#include <QMap>
#include "localprocess.h"
#include "remoteprocess.h"

class parameterSweeper;

class JobQueue : public QObject
{
    Q_OBJECT
public:
    static QStringList queueNames();

    JobQueue(QObject* parent = 0);
    ~JobQueue();

    const QMap<int, QSharedPointer<abstractProcess> >& getJobs();
	QList<int> getInteractiveJobs() { QList<int> k(running_interactive_jobs); k.append(pending_interactive_jobs); return k; };

public slots:
	/*!< Start the job queue. */
	void run();
    int addProcess(SharedMorphModel model);
	void addSweep(SharedMorphModel model, int sweep_id);

	void stopProcess(int id);                     ///  Stops the job @p id.
    void removeProcess(int id, bool remove_data); /// Removes the job @p id.
    void debugProcess(int id);                    /// Removes the job @p id.

private:
    QMap<int, QSharedPointer<abstractProcess> > jobs;
    QList<int> pending_jobs;
    QList<int> pending_interactive_jobs;
    QList<int> running_jobs;
	QList<int> running_interactive_jobs;
	QList<int> finished_interactive_jobs;
    bool is_initialized;

    QTimer *timer; /*!< Timer which defines the intervals after those the list of startable jobs is checked. */

    void restoreSavedJobs(); /*!< Restore jobs from the QSettings. */
    enum QueueType { local, interactive, remote };
    QMap<QString,QueueType> queueNamesMap();

    int newJobID(); /*!< Returns a free Job ID */

private slots:
    void processStateChanged(abstractProcess *process);
    /*!< Update the informations to the given process, when its state changed. */
    void processQueue(); /*!< Checks the queue for jobs that can be started. */
	void processError(QString error);

signals:
	void statusMessage(QString message, int progress = -1);
	void criticalMessage(QString message, bool popup);
    void processAdded(int);
    void processChanged(int);
    void processRemoved(int);
};

#endif // JOB_QUEUE_H
