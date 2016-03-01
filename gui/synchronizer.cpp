#include "synchronizer.h"

Synchronizer* Synchronizer::instanz = 0;

//---------------------------------------------------------------------------------------------------

Synchronizer* Synchronizer::getSync()
{
    if(instanz == 0)
    {
        instanz = new Synchronizer();
    }
    return instanz;
}

//---------------------------------------------------------------------------------------------------

void Synchronizer::addSyncTask(QString local, QString remote)
{
   directory_pair d = { local, remote };

   Synchronizer *s = Synchronizer::getSync();

   if ( ! s->queue.contains(d) ) {
       s->queue.push_back(d);
   }

   if ( ! s->isRunning())
       s->start();
}

//---------------------------------------------------------------------------------------------------

void Synchronizer::removeSyncTask(QString local, QString remote)
{
    directory_pair d = { local, remote };

    Synchronizer *s = Synchronizer::getSync();

    if ( s->queue.contains(d) ) {
        s->queue.removeAll(d);
    }
}

//---------------------------------------------------------------------------------------------------

bool Synchronizer::isSyncTaskFinished(QString local, QString remote) {
    directory_pair d = { local, remote };

    Synchronizer *s = Synchronizer::getSync();

    if ( s->queue.contains(d) ) {
        return false;
    }
    else {
        return true;
    }
}

//---------------------------------------------------------------------------------------------------

void Synchronizer::run()
{
    // cout << "Found "<< queue.size() << " jobs in the queue" << endl;
    int no_succ_count = 0;
    while ( ! queue.empty() )
    {
        directory_pair p_pair = queue.front();
        if (ssh.pull_directory(p_pair.remote, p_pair.local)) {
          queue.pop_front();
          no_succ_count = 0;
        }
        else {
            no_succ_count++;
            cout << ssh.getLastError().toStdString() << endl;
            cout << "Synchronizer::run: unable to sync remote dir " << p_pair.remote.toStdString() << endl;
            if (no_succ_count == 10) {
               cout << "Dropping sync task " << p_pair.remote.toStdString() <<  " , " << p_pair.local.toStdString() << endl;
               queue.pop_front();
               no_succ_count = 0;
            }
            else {
                msleep( 5000 );
            }
        }
    }
}
