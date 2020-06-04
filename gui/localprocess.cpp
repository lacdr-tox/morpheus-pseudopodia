#include "localprocess.h"

localProcess::localProcess(SharedMorphModel model, int pid, QString sub_dir) : abstractProcess(model, pid, sub_dir)
{
    _info.resource = ProcessInfo::local;
    init();
    storeJob();
}

/* deprecated: read job from old QSettings to new SQL database (only for tranfering QSettings to SQL) */
// localProcess::localProcess(QSettings& settings) : abstractProcess(settings)
// {
//     _info.resource = ProcessInfo::local;
// }

/* restore jobs from SQL database */
localProcess::localProcess(QSqlRecord& record) : abstractProcess(record)
{
//     qDebug() << "localProcess(QSqlRecord&)";
    _info.resource = ProcessInfo::local;
    init();
    if ( !readOutput() ) // if output file cannot be found, remove entry from QSettings
        remove(false);

    // in case anything crashed, we just assume those jobs crashed too.
    if (_info.state == ProcessInfo::RUN || _info.state == ProcessInfo::PEND)
    {
        abstractProcess::changeState( ProcessInfo::EXIT );
    }
}
//---------------------------------------------------------------------------------------------------

void localProcess::init(){

    timer = new QTimer(this);
    stopwatch = new QTime();
    connect(timer, SIGNAL(timeout()), SLOT(checkRuntime()));

    numthreads = config::getApplication().local_maxThreads;


    QString err(outputDir + "/" + model_file_name + ".err");
    errFile = new QFile(err);


    process = new QProcess();

    connect(process, SIGNAL(stateChanged(QProcess::ProcessState)), SLOT(changeState(QProcess::ProcessState)));
    connect(process, SIGNAL(readyReadStandardError()), SLOT(readErrorMsg()));
}

//---------------------------------------------------------------------------------------------------

void localProcess::changeState(QProcess::ProcessState state)
{
    ProcessInfo::status new_state;
    if (state == QProcess::NotRunning)
    {
        timer->stop();
        setRunTime(stopwatch->elapsed());
		
		if (!error_msg.isEmpty()) {
			    emit criticalMessage(error_msg.left(500));
				new_state = ProcessInfo::EXIT;
		}
		else if (process->exitStatus() == QProcess::NormalExit)
			new_state = ProcessInfo::DONE;
		else
			new_state = ProcessInfo::EXIT;

    }
    else if (state == QProcess::Running)
    {
        new_state = ProcessInfo::RUN;
        stopwatch->start();
        setRunTime(0);
        timer->start(config::getApplication().preference_jobqueue_interval );
    }
    else /*if (state == QProcess::Starting )*/ {
        return;
    }

    abstractProcess::changeState(new_state);

    readOutput();
}

//---------------------------------------------------------------------------------------------------

void localProcess::start()
{
    // set number of openMP threads for simulation
#if QT_VERSION < 0x040600
    process->setEnvironment( QProcess::systemEnvironment() << QString("OMP_NUM_THREADS=%1").arg(numthreads));
#else
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("OMP_NUM_THREADS", QString::number(numthreads));
    process->setProcessEnvironment(env);
#endif

	QString command = config::getPathToExecutable( config::getApplication().local_executable );
	QStringList arguments;
    if (command.isEmpty()) {
		abstractProcess::changeState(ProcessInfo::EXIT);
		emit criticalMessage("Cannot find morpheus simulator.\nCheck the morpheus simulator path in the settings.");
		return;
	}
	
	if ( ! config::getApplication().local_GnuPlot_executable.isEmpty()) {
		arguments << "-gnuplot-path";
		arguments << config::getApplication().local_GnuPlot_executable;
	}
#ifdef Q_OS_WIN32
	else if ( ! config::getPathToExecutable("gnuplot").isEmpty())
		arguments << "-gnuplot-path" << config::getPathToExecutable("gnuplot");
#endif
		
	arguments << model_file_name;
	
    process->setWorkingDirectory(outputDir);
    process->setStandardOutputFile(output_file.fileName());

    // run morpheus
	process->start(command,arguments);

    // process->start(command);
    ID = (int)process->pid();
    qDebug() << process->errorString();
}

//---------------------------------------------------------------------------------------------------

void localProcess::stop()
{
    if(process->state() != QProcess::NotRunning )
    {
        process->kill();
        process->waitForFinished(1000);
    }
}

//---------------------------------------------------------------------------------------------------

void localProcess::debug()
{
    if(process->state() != QProcess::Running )
    {
		// set number of openMP threads for simulation
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert("OMP_NUM_THREADS", QString::number(1));
		process->setProcessEnvironment(env);
        QString filename= QDir::fromNativeSeparators(QDir::tempPath()) + "/gdb_cmd.txt";
        ofstream fout;
        fout.open( filename.toStdString().c_str() );
        if(fout.is_open()){
            //fout << "exec-file " + (config::getApplication().local_executable).toStdString() + "\n";
            fout << "set logging file " + outputDir.toStdString() +"/gdb.log" + "\n";
            fout << "set logging on\n";
            fout << "set logging overwrite\n";
            fout << "run " << model_file_name.toStdString() << "\n";
            fout << "backtrace full\n";
            fout << "backtrace \n";
            fout << "quit\n";
            fout.close();
        }
        else{
            qDebug() << " COULD NOT OPEN " << filename;
        }

        qDebug() << "GDB commands written to " << filename;
		
        QString command = config::getPathToExecutable("gdb");
		if (command.isEmpty()) {
			emit criticalMessage("Unable to locate \"gdb\" executable");
			abstractProcess::changeState(ProcessInfo::EXIT);
			return;
		}
		
        command = "gdb --command="+filename+ " --args " + config::getApplication().local_executable;        process->setWorkingDirectory(outputDir);
        process->setStandardOutputFile(output_file.fileName());
        process->setStandardErrorFile(errFile->fileName());
        qDebug() << "workingDir: " << outputDir << endl;
        qDebug() << "command: " << command << endl;

        // run morpheus
        process->start(command);
        ID = (int)process->pid();
        qDebug() << process->errorString();
    }
}

//---------------------------------------------------------------------------------------------------

void localProcess::remove(bool force)
{
    if( _info.state == ProcessInfo::RUN ){
        this->stop();
    }
    abstractProcess::remove(force);
}

//---------------------------------------------------------------------------------------------------

void localProcess::readErrorMsg()
{
	if (process->state() == QProcess::NotRunning)
		return;
	QByteArray buffer(process->readAllStandardError());
	QString tmp(buffer);
	error_msg += tmp;

	if(errFile->open(QFile::WriteOnly))
	{
		errFile->write(tmp.toStdString().c_str());
		errFile->close();
	}
	if (error_msg.size()>1000)
		stop();
}

//---------------------------------------------------------------------------------------------------

void localProcess::checkRuntime()
{
    readOutput();

    setRunTime(stopwatch->elapsed());

    int i = floor((double)stopwatch->elapsed() /1000/60);

    if (i > config::getApplication().local_timeOut)
    {
        output.append("Process was stopped! Max Runtime exceeded!");
        emit outputChanged(this);
        stop();
    }
}

//---------------------------------------------------------------------------------------------------

localProcess::~localProcess()
{
    if( _info.state == ProcessInfo::RUN)
        stop();
    delete process;
    if (timer)
        delete timer;
}
