#include "abstractprocess.h"

abstractProcess::abstractProcess(SharedMorphModel model, int job_id, QString sub_dir) :QObject(NULL)
{
	model_file_name = "model.xml";
	_info.job_id = job_id;
	_info.state = ProcessInfo::PEND;
	_info.run_time = -1;
	updateRunTimeString();
	_info.progress = 0;
	ID = 0;
	
	_info.title = model->rootNodeContr->getModelDescr().title;
// 	model->rootNodeContr->firstChild("Description")->firstChild("Title")->getText();
	_info.model_file = model->xml_file.path;
	if (_info.title.isEmpty())
		_info.title = "Simulation";
	
	if ( ! sub_dir.isEmpty())
		_info.sim_dir = sub_dir + "/";
	_info.sim_dir += _info.title.replace(" ","_") + "_" + QString::number(_info.job_id);

	// Reading simulation data ...
	nodeController* time = model->rootNodeContr->firstChild("Time");
    // read StartTime/StopTime as double, because they can be in scientific notation (e.g. "1.5+e3")
    _info.start_time = floor( time->firstChild("StartTime")->attribute("value")->get().toDouble() );
    _info.stop_time = ceil( time->firstChild("StopTime")->attribute("value")->get().toDouble() );
	_info.current_time = _info.start_time;

	
	localDir = config::getApplication().general_outputDir;
	
	QDir dir(localDir);
	dir.mkpath(_info.sim_dir);
	dir.cd(_info.sim_dir);
	outputDir = dir.absolutePath();
	
	// check whether output directory exists and is writable (fixes #34)
	QFileInfo od(outputDir);
	if(od.isDir() && od.isWritable()){
		cout << "Output directory = " << outputDir.toStdString() << endl;
	}
	else{
		_info.state = ProcessInfo::EXIT;
		cout << "Output directory = " << outputDir.toStdString() << " IS NOT FOUND OR NOT WRITABLE!!" << endl;
		emit criticalMessage("Output directory '("+QString(outputDir)+")' not found or not writable.");
		return;
	}
	
	// copying the supplementary file to the output folder
	const QList < AbstractAttribute* >&  system_files = model->rootNodeContr->getModelDescr().sys_file_refs;
	QList<QString> original_paths;

	QDir model_dir;
	if (model->xml_file.path.isEmpty()) {
		model_dir = QDir(".");
	}
	else {
		model_dir = QFileInfo(model->xml_file.path).absoluteDir();
	}

	for (int i=0; i<system_files.size(); i++ ) {
		original_paths.push_back(system_files[i]->get());
		if (system_files[i]->isActive() && ! system_files[i]->isDisabled()) {
			QFileInfo file(model_dir,system_files[i]->get().trimmed());
			if (file.exists()) {
				qDebug() << "Copying supplementary file " << system_files[i]->get() << " to output folder "
				<< outputDir + "/" + file.fileName();
				QFile::copy(file.absoluteFilePath(), outputDir + "/" + file.fileName());
			}
			else {
				qDebug() << "Unable to copy supplementary file " << system_files[i]->get() << " to output folder "
				<< outputDir + "/" + file.fileName();
				qDebug() << "Assumed path was " << file.filePath();
			}
			system_files[i]->set(file.fileName());
		}
	}

// writing the model file
	if  (! model->xml_file.save(outputDir + "/" + model_file_name) )
	{
		throw QString("Can't create xml-model: '%1'!\n; Unable to start simulation!\n").arg(outputDir + "/" + model_file_name);
	}

	// reset xml file paths
	for (int i=0; i<system_files.size(); i++ ) {
		system_files[i]->set(original_paths[i]);
	}

	output = "";
	output_file.setFileName(outputDir + "/" + model_file_name + ".out");
	output_read = 0;
	pending_read = false;
}

//---------------------------------------------------------------------------------------------------

ProcessInfo::status abstractProcess::stateMsgToState(QString stateMsg) {
	ProcessInfo::status state;
	if (stateMsg == "PEND")
		state = ProcessInfo::PEND;
	else if  (stateMsg == "RUN")
		state = ProcessInfo::RUN;
	else if  (stateMsg == "DONE")
		state = ProcessInfo::DONE;
	else if  (stateMsg == "EXIT")
		state = ProcessInfo::EXIT;
	else
		state = ProcessInfo::UNKWN;
	return state;
}

//---------------------------------------------------------------------------------------------------


/// Constructor used for restoring saved jobs
abstractProcess::abstractProcess(QSqlRecord& r) : QObject(NULL) {
	_info.job_id      = r.value( r.indexOf("id") ).toInt();
	ID                = r.value( r.indexOf("processPid") ).toInt();
	numthreads        = r.value( r.indexOf("processThreads") ).toInt();
	_info.state       = ProcessInfo::status(r.value( r.indexOf("processState") ).toInt());
	
	model_file_name   = r.value( r.indexOf("simXMLName") ).toString();
	_info.title       = r.value( r.indexOf("simTitle") ).toString();
	_info.sim_dir     = r.value( r.indexOf("simDirectory") ).toString();
	
    _info.start_time   = r.value( r.indexOf("timeStart") ).toInt();
    _info.stop_time    = r.value( r.indexOf("timeStop") ).toInt();
    _info.current_time = r.value( r.indexOf("timeCurrent") ).toInt();
	_info.run_time    = r.value( r.indexOf("timeExec") ).toInt();
	updateRunTimeString();

	localDir = config::getApplication().general_outputDir;
	outputDir = localDir + "/" + _info.sim_dir;
	output = "";
	QString out(outputDir + "/" + model_file_name + ".out");
	output_file.setFileName(out);
	output_read = 0;
	
	// If the Job was running while the application closed / crashed ...
	if (_info.state == ProcessInfo::RUN)
		pending_read = true;

	if ( (_info.stop_time-_info.start_time) != 0)
		_info.progress = ((_info.current_time-_info.start_time) * 100) / (_info.stop_time-_info.start_time);

	output = QString("...Attempting to retreive job status and simulation output...");
	
}

//---------------------------------------------------------------------------------------------------

bool abstractProcess::readOutput()
{
    pending_read = true;
    if (output_file.exists()) {
        if ( output_file.size() > config::getApplication().preference_stdout_limit *1000000 && _info.state == ProcessInfo::RUN) {
            stop();
			emit criticalMessage(QString("Output of job %1 exceeded the maximum size of 10Mb! Stopping job!").arg(_info.job_id ));
        }
        if ( output_read == output_file.size() ) {
            pending_read = false;
        }
        else {
            emit outputChanged(this);
        }
        return true;
    }
    else{
        qDebug() << output_file.fileName() << " does not exist! ";
    }
    return false;
}
//---------------------------------------------------------------------------------------------------

bool abstractProcess::internal_readOutput() {
	
	if (output_file.size() == output_read)
		return true;
	
	if (!output_file.isOpen()) {
		output_file.open(QFile::ReadOnly | QIODevice::Text);
		if ( ! output_file.isOpen() ) {
			qDebug() << "Unable to open output file " << output_file.fileName();
			output = QString("Error: Can't open file: '%1'!\nReading the output of simulation not possible!\n").arg(output_file.fileName());
			return false;
		}
		output.clear();
	}

	pending_read = false;
	// just read new data ...
	if (output_file.bytesAvailable()>0) {

		QTextStream in(&output_file);
		while (!in.atEnd() && output.size() < max_output_size) {
			output += in.readLine() + "\n"; 
			if (output.size() >= max_output_size) {
				output+= "Output exceeded maximum. Skipping whatever remains of the file\n";
			}
		}
		output_read = output_file.pos();
		QRegExp reg("Time: (\\d+(\\.\\d+)?)");
		reg.lastIndexIn(output);
		if (reg.matchedLength() > 0) {
			_info.current_time = reg.cap(1).toDouble();
			if (_info.stop_time - _info.start_time<=0)
				_info.progress = 100;
			else
				_info.progress = ((_info.current_time-_info.start_time) * 100) / (_info.stop_time-_info.start_time);
// 				qDebug() << "Read time " << _info.current_time << " of " << _info.stop_time;
			//QSettings().setValue(QString("jobs/%1/currentMCS").arg(_info.job_id), _info.current_mcs);
		}
	}
	
	if (this->state() == ProcessInfo::RUN)
		output_file.close();
	
	return true;
}

//---------------------------------------------------------------------------------------------------

void abstractProcess::remove(bool force) {
//     qDebug() << "abstractProcess::removing job " <<_info.job_id ;

     // remove job from SQL database
    QSqlQuery q(config::getDatabase());
    q.prepare(" DELETE FROM jobs WHERE id == :id ;");
    q.bindValue(":id", _info.job_id);
    bool ok = q.exec();
    if(!ok) qDebug() << "Error deleting from SQL table: " << q.lastError();
    q.finish();
    // remove simulation data
    if (force)
        removeDir(outputDir);

}


//---------------------------------------------------------------------------------------------------

bool abstractProcess::changeState(ProcessInfo::status state){
	QString old_state = getStateString();
	_info.state = state;
// 	qDebug() << "abstractProcess[" << _info.job_id << "]::changeState " << old_state << " -> " << getStateString();
	if (state  == ProcessInfo::DONE || state  == ProcessInfo::EXIT || state  == ProcessInfo::RUN)
		storeJob();
	
	 emit stateChanged(this);
	
	return true;
	
}
//---------------------------------------------------------------------------------------------------

void abstractProcess::storeJob() {
// 	qDebug() << "abstractProcess::storeJob() " << jobID() ;

	if (_info.state != ProcessInfo::PEND) {
		// Assume the Job was already stored initially ...
		QSqlQuery q(config::getDatabase());
		q.prepare(
			"UPDATE jobs "
			"SET processState = :new_state , processPid = :pid, timeCurrent = :timeCurrent,  timeExec =  :timeExec "
			"WHERE id = :id "
		);
		q.bindValue(":id",        _info.job_id);
		q.bindValue(":new_state", _info.state);
		q.bindValue(":pid",       ID);
		q.bindValue(":timeCurrent", _info.current_time);
		q.bindValue(":timeExec", _info.run_time);
		
		
		bool ok = q.exec();
		if(!ok){
			qDebug() << "Error updating job state to SQL Database: " << q.lastQuery() << ", error: "  << q.lastError();
			return;
		}
	}
	else {
		QSqlQuery q(config::getDatabase());
		bool ok;
		q.prepare("REPLACE INTO jobs(  id,  processPid,  processThreads,  processState,  processResource,  simTitle, simXMLname, simDirectory, timeStart,  timeStop,  timeCurrent,  timeExec, remoteTask)"
				"VALUES(  :id,  :processPid,  :processThreads,  :processState,  :processResource,  :simTitle, :simXMLname, :simDirectory, :timeStart,  :timeStop,  :timeCurrent,  :timeExec,  :remoteTask )"
				);

		q.bindValue(":id",              _info.job_id);
		// process
		q.bindValue(":processPid",      ID);
		q.bindValue(":processThreads",  numthreads);
		q.bindValue(":processState",    _info.state);
		q.bindValue(":processResource", _info.resource);
		//simulation
		q.bindValue(":simTitle",        _info.title);
		q.bindValue(":simXMLname",      model_file_name);
		q.bindValue(":simDirectory",    _info.sim_dir);
		// time
		q.bindValue(":timeStart",       _info.start_time);
		q.bindValue(":timeStop",        _info.stop_time);
		q.bindValue(":timeCurrent",     _info.current_time);
		q.bindValue(":timeExec",        _info.run_time);
		// remote task
		q.bindValue(":remoteTask",      0);

		ok = q.exec();
		if(!ok){
			qDebug() << "Error in storing job to database: " << q.lastError();
		}
	}
}

//---------------------------------------------------------------------------------------------------

const QString& abstractProcess::getOutput()
{
    if (pending_read)
        internal_readOutput();
    return output;
}

//---------------------------------------------------------------------------------------------------

const ProcessInfo&  abstractProcess::info() const
{
    if (_info.state == ProcessInfo::RUN && pending_read) {
        const_cast<abstractProcess*>(this)->internal_readOutput();
    }
    return _info;
}

//
void abstractProcess::updateRunTimeString() {
	if( _info.run_time >= 0) {

        int msec = _info.run_time;
        int millisec=(msec)%1000;
        int seconds=(msec/1000)%60;
        int minutes=(msec/(1000*60))%60;
        int hours=(msec/(1000*60*60))%24;

        if(hours > 0)
            _info.run_time_string = QString("%1h %2m").arg(hours).arg(minutes,2,10,QChar('0'));
        else if( minutes > 0 )
            _info.run_time_string = QString("%1m %2s").arg(minutes).arg(seconds,2,10,QChar('0'));
        else
            _info.run_time_string = QString("%1s %2ms").arg(seconds).arg(millisec,3,10,QChar('0'));
    }
    else
        _info.run_time_string = " -- ";
}

//---------------------------------------------------------------------------------------------------

void abstractProcess::setRunTime(int elapsed)
{
    _info.run_time = elapsed;
	updateRunTimeString();

//     QSqlQuery q;
//     q.prepare("UPDATE jobs "
//                     "SET timeExec = :exectime "
//                     "WHERE id = :id ");
//     q.bindValue(":id",       _info.job_id);
//     q.bindValue(":exectime", _info.run_time);

//     bool ok = q.exec();
//     if(!ok){
//             qDebug() << "Error updating timeExec to SQL Database: " << q.lastQuery() << ", error: "  << q.lastError();
//     }


}

//---------------------------------------------------------------------------------------------------

int abstractProcess::getNumThreads() const
{
    return numthreads;
}

//---------------------------------------------------------------------------------------------------

abstractProcess::~abstractProcess()
{}

//---------------------------------------------------------------------------------------------------

QString abstractProcess::translateState() const
{
    switch (_info.state)
    {
        case ProcessInfo::PEND:
            {return ("PEND");}
        case ProcessInfo::PSUSP:
            {return ("PSUSP");}
        case ProcessInfo::RUN:
            {return ("RUN");}
        case ProcessInfo::USUSP:
            {return ("USUSP");}
        case ProcessInfo::SSUSP:
            {return ("SSUSP");}
        case ProcessInfo::DONE:
            {return ("DONE");}
        case ProcessInfo::EXIT:
            {return ("EXIT");}
        case ProcessInfo::ZOMBI:
            {return ("ZOMBIE");}
        case ProcessInfo::UNKWN:
        default:
            {return ("UNKWN");}
    }
}

//---------------------------------------------------------------------------------------------------

ProcessInfo::status abstractProcess::state() const
{
    return _info.state;
}

//---------------------------------------------------------------------------------------------------

QString abstractProcess::getStateString() const {
    switch(_info.state)
    {
        case ProcessInfo::PEND:
        {return ("Pending");}
        case ProcessInfo::PSUSP:
        {return ("Suspended");}
        case ProcessInfo::RUN:
        {return ("Running");}
        case ProcessInfo::USUSP:
        {return ("Suspended");}
        case ProcessInfo::SSUSP:
        {return ("Suspended");}
        case ProcessInfo::DONE:
        {return ("Done");}
        case ProcessInfo::EXIT:
        {return ("Exited");}
        default:
        {return ("Unknown");}
    }
}

//---------------------------------------------------------------------------------------------------

QString abstractProcess::getStateMessage() const
{
    switch(_info.state)
    {
        case ProcessInfo::PEND:
        {return ("Job is pending (not yet started).");}
        case ProcessInfo::PSUSP:
        {return ("Job has been suspended by the user while pending.");}
        case ProcessInfo::RUN:
        {return ("Job is currently running.");}
        case ProcessInfo::USUSP:
        {return ("Job has been suspended by the user while running.");}
        case ProcessInfo::SSUSP:
        {return ("Job has been suspended by the system while running.");}
        case ProcessInfo::DONE:
        {return ("Job has exited normally (exit value 0).");}
        case ProcessInfo::EXIT:
        {return ("Job has exited abnormally (exit value non-zero).");}
        default:
        {return ("Indicates some system problem. Please contact cosmos_sys.");}
    }
}

//---------------------------------------------------------------------------------------------------

bool abstractProcess::removeDir(const QString &dirName)
{
    bool result = true;
    QDir dir(dirName);

    if (dir.exists(dirName)) {
        Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
            if (info.isDir()) {
                result = removeDir(info.absoluteFilePath());
            }
            else {
                result = QFile::remove(info.absoluteFilePath());
            }

            if (!result) {
                return result;
            }
        }
        result = dir.rmdir(dirName);
    }

    return result;
}
