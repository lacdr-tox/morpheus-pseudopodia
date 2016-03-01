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

#ifndef SYNCHRONIZER_H
#define SYNCHRONIZER_H

#include <QThread>
#include <QtGui>
#include "sshproxy.h"

using namespace std;

/*!
This class syncronizes a local directory and a remote directory via ssh.<br>
If there are differences between both directorys, the synchronizer pulls all files from the remote-machine, <br>
which doesn't exist in the local directory or which are different from the equivalent files in the local folder.<br>
The syncing-tasks are stored in a list and will be completed in a separate QThread.
*/
class Synchronizer : public QThread
{
public:
    static void addSyncTask(QString local, QString remote);
    /*!<
      Adds a syncing-task to the list, which syncs a local and a remote directory.
      \param local Path to the local directory, which will be synced.
      \param remote Path to the remote directory, which will be synced
      */
    static void removeSyncTask(QString local, QString remote);
    /*!<
      Removes a syncing-task from the list.
      \param local Path to the local directory which shouldn't be synced anymore
      \param remote Path to the remote directory which shouldn't be synced anymore
      */
    static bool isSyncTaskFinished(QString local, QString remote);
    /*!<
      Returns if the syncing of the given local and remote directory is finished or not.
      \param local Path to the local directory
      \param remote Path to the remote directory
      */

private:
    Synchronizer(){} /*!< Creates a syncronier. */

    /*! Container which describes a pair of directorys which should be synced. */
    struct directory_pair {
        QString local; /*!< Path to the local directory which should be synced */
        QString remote; /*!< Path to the remote directory which should be synced */
        bool operator==(const Synchronizer::directory_pair& r) { return local==r.local && remote == r.remote; } /*!< Returns true if the given directory-pair is equal to the current one. */
    };
    QList<directory_pair> queue; /*!< List of all syncing-tasks, which will be executed soon. */

    static Synchronizer* getSync(); /*!< Returns a syncronizer. */
    static Synchronizer *instanz; /*!< Global static object of the syncronizer. */
    void run(); /*!< Starts the syncing-process. */

    sshProxy ssh; /*!< sshProxy which is used to sync the directorys. */
};

#endif // SYNCHRONIZER_H
