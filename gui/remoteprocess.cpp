#include "remoteprocess.h"

remoteProcess::remoteProcess(SharedMorphModel model, int pid, QString sub_dir) : abstractProcess(model, pid, sub_dir)
{
    init();
    storeJob();
}

//---------------------------------------------------------------------------------------------------

remoteProcess::remoteProcess(QSqlRecord& record) : abstractProcess(record)
{
    init();
//     qDebug() << "remoteProcess(QSqlRecord&)";
    run_task = remoteProcess::runTask(record.value( record.indexOf("remoteTask") ).toInt());

    readOutput();

	if(state() != ProcessInfo::DONE && state() != ProcessInfo::EXIT) {
		if (ssh.checkConnection()) {
			checkTimer->start(config::getApplication().preference_jobqueue_interval_remote);
		}
		else {
			emit criticalMessage("No ssh_session!\n" + ssh.getLastError());
		}
    }

}
//---------------------------------------------------------------------------------------------------

void remoteProcess::init() {

    _info.resource = ProcessInfo::remote;
    checkTimer = new QTimer(this);
    stopwatch = new QElapsedTimer();

    connect(checkTimer, SIGNAL(timeout()), SLOT(checkState()));
    run_task = NONE;

    remoteDir = config::getApplication().remote_simDir;
    numthreads = config::getApplication().remote_maxThreads;
}

//---------------------------------------------------------------------------------------------------

void remoteProcess::storeJob() {
    QString runstr;
    if (run_task == SIM)
        runstr= "SIM";
    else if (run_task == SYNC)
        runstr= "SYNC";
    else
        runstr= "";
    abstractProcess::storeJob();


    QSqlQuery q(config::getDatabase());
    q.prepare("UPDATE jobs "
                    "SET remoteTask = :remoteTask "
                    "WHERE id = :id ");
    q.bindValue(":id",                        _info.job_id);
    q.bindValue(":remoteTask",		run_task);

    bool ok = q.exec();
    if(!ok){
            qDebug() << "Error updating remoteTask to SQL Database: " << q.lastQuery() << ", error: "  << q.lastError();
    }
}

//---------------------------------------------------------------------------------------------------

void remoteProcess::start()
{
	QString dir(remoteDir + "/" + _info.sim_dir + "/");
	cout << "Starting remote job: " << (dir + model_file_name).toStdString() << endl;

	QString output;
	if ( ! ssh.push_directory(outputDir,dir)) {
		emit criticalMessage( "Unable to start Simulation!\n" + ssh.getLastError());
		this->changeState(ProcessInfo::EXIT);
		return;
	}

	QString executable = config::getApplication().remote_executable;
	if (!config::getApplication().remote_GnuPlot_executable.isEmpty()) {
		executable = QString("\"%1 --gnuplot-path %2\"").arg(executable,config::getApplication().remote_GnuPlot_executable);
	}
	
	QString command;
	command = QString("processProxy.sh -a start -m %1 -d %2 -n %5 -f %4 -e %3").arg(model_file_name,_info.sim_dir,executable,remoteDir).arg(numthreads);
	cout << command.toStdString() << endl;
	QString result;

	if ( ! ssh.exec(command, &result))
	{
		emit criticalMessage("Unable to start Simulation!\n" + ssh.getLastError());
		this->changeState(ProcessInfo::EXIT);
		return;
    }
    else
    {
        cout << "called processProxy to start simulation"<< endl;
        QRegExp re_pid("<(\\d+)>");
        if  (! re_pid.indexIn(result) ) {
            emit criticalMessage("Unable to get the Job ID of the remote batch system!\n Take care yourself to remove the job!");
            ID = -1;
        }
        else {
            ID = re_pid.capturedTexts()[1].toInt();
            run_task = SIM;
            cout << "LSF Job ID of remote job = " << ID << endl;
            checkTimer->start(5000);
            setRunTime(0);
        }
    }
}

//---------------------------------------------------------------------------------------------------

void remoteProcess::stop()
{
    if (_info.state == ProcessInfo::RUN || _info.state == ProcessInfo::PEND) {
        QString command = QString("processProxy.sh -a stop -i ") + QString::number(ID);

        if ( ! ssh.exec(command))
        {
			emit criticalMessage(QString("Unable to stop process: %1!\n").arg( _info.job_id) + ssh.getLastError());
        }
        else
        {
            checkState();
        }
    }
}

//---------------------------------------------------------------------------------------------------

void remoteProcess::remove(bool force)
{
    if(config::getApplication().remote_dataSyncType == "Continuous")
        Synchronizer::removeSyncTask(localDir + "/" + _info.sim_dir, remoteDir + "/" + _info.sim_dir);

    if (checkTimer)
        checkTimer->stop();

    if (_info.state == ProcessInfo::RUN || _info.state == ProcessInfo::PEND) {
        stop();
        QString command = QString("processProxy.sh -a delete -d %1 -f %2").arg(_info.sim_dir, remoteDir);

        if ( ! ssh.exec(command))
        {
			emit criticalMessage(QString("Unable to delete the simulation-output!\n") + ssh.getLastError());
        }
    }

    abstractProcess::remove(force);
}

void remoteProcess::debug()
{
	if (_info.state != ProcessInfo::RUN || _info.state != ProcessInfo::PEND) {
		
		QString executable = config::getApplication().remote_executable;
		QString command = QString("processProxy.sh -a debug -m %1 -d %2 -e %3").arg(model_file_name,_info.sim_dir,executable);
		cout << command.toStdString() << endl;
		QString result;

		if ( ! ssh.exec(command, &result))
		{
			emit criticalMessage(QString("Unable to debug simulation:\n") + ssh.getLastError());
		}
    }
}
//---------------------------------------------------------------------------------------------------

void remoteProcess::checkState()
{
	// make sure only one checking instance is running per job ...
	if (ID > 0 && check_lock.tryLock(20) ) {

		QString command = QString("processProxy.sh -a check -i %1").arg(ID);
		QString result;
		QString sync_type;

		if ( state() == ProcessInfo::RUN || state() == ProcessInfo::PEND) {

			if (checkTimer->interval() < 30000)
				checkTimer->setInterval( min(checkTimer->interval()*2,30000));

			setRunTime(stopwatch->elapsed());
			sync_type = config::getApplication().remote_dataSyncType;

			if (sync_type == "Continuous") {
				Synchronizer::addSyncTask( localDir + "/" + _info.sim_dir, remoteDir + "/" + _info.sim_dir);
			}

			if ( ! ssh.exec(command, &result)) {
				emit criticalMessage(QString("Unable to check state of process: %1!\n").arg(_info.job_id) + ssh.getLastError());
			}
			else {
				if (result == "" or result=="No Jobs found") {
					result = "DONE";
				}
				int first_non_letter=0;
				while ( first_non_letter< result.size() && result.at(first_non_letter).isLetter()) first_non_letter++;
				result.truncate(first_non_letter);
			}

			if (result != translateState())
			{
				setState(result);
				if (state() == ProcessInfo::RUN) {
					stopwatch->start();
				}
				if (state() == ProcessInfo::DONE || state() == ProcessInfo::EXIT) {
					checkTimer->stop();
					setRunTime(stopwatch->elapsed());
				}
			}
		}

		if(state() != ProcessInfo::DONE && state() != ProcessInfo::EXIT && state() != ProcessInfo::PEND) {
			QString sync_type = config::getApplication().remote_dataSyncType;
			if (sync_type == "Afterwards") {
				if ( ! ssh.pull_file(remoteDir + "/" + _info.sim_dir + "/" +model_file_name + ".out", output_file) )
				{
					output =  QString("Error:") + " Can't read Simulation-Output-File!\n" + ssh.getLastError();
					return;
				}
			}
			readOutput();
		}

		if(state() == ProcessInfo::DONE || state() == ProcessInfo::EXIT)
		{
			if(run_task == SIM) {
				Synchronizer::addSyncTask( localDir + "/" + _info.sim_dir, remoteDir + "/" + _info.sim_dir);
				run_task = SYNC;
				checkTimer->start(5000);
			}
			else if(run_task == SYNC) {
				if(Synchronizer::isSyncTaskFinished(localDir + "/" + _info.sim_dir, remoteDir + "/" + _info.sim_dir)) {
					readOutput();
					checkTimer->stop();
				}
			}
		}
	}
	check_lock.unlock();
}


//---------------------------------------------------------------------------------------------------

QString remoteProcess::getResource() const
{return QString("remote");}

//---------------------------------------------------------------------------------------------------

int remoteProcess::setState(QString str)
{
	ProcessInfo::status state;
    cout << "setState(): '" << str.toStdString() << "'" << endl;
    if(str == "PEND")
        {state = ProcessInfo::PEND;}
    else if(str == "PSUSP")
        {state = ProcessInfo::PSUSP;}
    else if(str == "RUN")
        {state = ProcessInfo::RUN;}
    else if(str == "USUSP")
        {state = ProcessInfo::USUSP;}
    else if(str == "SSUSP")
        {state = ProcessInfo::SSUSP;}
    else if(str == "DONE")
        {state = ProcessInfo::DONE;}
    else if(str == "EXIT")
        {state = ProcessInfo::EXIT;}
    else if(str == "UNKWN")
        {state = ProcessInfo::UNKWN;}
    else if(str == "ZOMBI")
        {state = ProcessInfo::ZOMBI;}
    else
        {state = ProcessInfo::UNKWN;}
        
	abstractProcess::changeState(state);
	
    return 0;
}

//---------------------------------------------------------------------------------------------------

remoteProcess::~remoteProcess()
{
    if (checkTimer)
        delete checkTimer;
}
