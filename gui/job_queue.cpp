#include "job_queue.h"

QStringList JobQueue::queueNames() {
#ifdef HAVE_LIBSSH
    return QStringList() << "local" << "interactive" << "remote";
#else
    return QStringList() << "local" << "interactive";
#endif
}

QMap<QString,JobQueue::QueueType> JobQueue::queueNamesMap() {
    QMap<QString,QueueType> map;
    map["local"] = local;
    map["interactive"] = interactive;
    map["remote"] = remote;
    return map;
}

JobQueue::JobQueue(QObject* parent) : QObject(parent)
{
    is_initialized = false;
}

//---------------------------------------------------------------------------------------------------

void JobQueue::run() {
    timer = new QTimer();
    connect(timer, SIGNAL(timeout()), this, SLOT(processQueue()));
	qDebug() << "JobQueue thread " << QThread::currentThreadId();
    timer->start(100);
}

//---------------------------------------------------------------------------------------------------

void JobQueue::restoreSavedJobs() {
	qDebug() << "JobQueue::restoreSavedJob()";

	QSqlQuery q(config::getDatabase());
	bool ok = q.exec("SELECT * FROM jobs;");
	if(!ok) qDebug() << "Error retrieving jobs from database: " << q.lastError();
	int field_resource = q.record().indexOf("processResource");
	int count = 0;
	while( q.next() ) {
// 		qDebug() << "Job record nr: " << ++count;
		QSqlRecord record = q.record();

		// create local or remote abstractProcess
		QSharedPointer<abstractProcess> job ;                
		if( q.value(field_resource)  == ProcessInfo::local )
			job = QSharedPointer<abstractProcess>(new localProcess( record ));
		else
			job = QSharedPointer<abstractProcess>(new remoteProcess( record ));

		// get job detail form abstractProcess and add to map that links jobid to job
		const ProcessInfo& info = job->info();
		jobs[info.job_id] = job;

		// update internal model of running and pending jobs
		connect(job.data(), SIGNAL(stateChanged(abstractProcess*)), this, SLOT(processStateChanged(abstractProcess*)));
		if (info.resource == ProcessInfo::local && info.state == ProcessInfo::PEND)
			pending_jobs.append(info.job_id);
		if (job->state() == ProcessInfo::RUN)
			running_jobs.append(info.job_id);
		if (job->state() == ProcessInfo::RUN || job->state() == ProcessInfo::PEND) {
			connect(job.data(), SIGNAL(stateChanged(abstractProcess*)), this, SLOT(processStateChanged(abstractProcess*)));
			connect(job.data(), SIGNAL(outputChanged(abstractProcess*)), this, SLOT(processStateChanged(abstractProcess*)));
			connect(job.data(), SIGNAL(criticalMessage(QString,int)), this, SLOT(processError(QString)));
		}

		emit processAdded(info.job_id);
	}	
	q.finish();
}

//---------------------------------------------------------------------------------------------------

int JobQueue::newJobID() {
	QSqlQuery q(config::getDatabase());
	bool ok = q.exec("SELECT MAX(id) FROM jobs;");
	if(!ok) qDebug() << "Error getting MAX(id) from jobs database: " << q.lastError();
	q.next();
	QSqlRecord r = q.record();
	if( !q.isValid() ) qDebug() << "Query not valid... :" << q.last() << ", error: "<< q.lastError();
	int index = r.indexOf("MAX(id)");
	int last_id = r.value( index ).toInt();
	int new_id = last_id + 1;

// 	qDebug() << "lastID " << r.value( index ).toInt() << ", newJobID = " << new_id << " | index = " << index ;
	q.finish();
	return new_id;
}


//---------------------------------------------------------------------------------------------------

void JobQueue::stopProcess(int id) {
    if ( ! jobs.contains(id) ) {
        qDebug() << "Given job id " << id << " is not queued !";
    }
    else {
        jobs[id]->stop();
    }
}

void JobQueue::removeProcess(int id, bool remove_data) {
    if ( ! jobs.contains(id) ) {
        qDebug() << "Given job id " << id << " is not queued !";
    }
    else {
        pending_jobs.removeAll(id);
        jobs[id]->remove(remove_data);
        jobs.remove(id);

        emit processRemoved(id);
    }
}


void JobQueue::debugProcess(int id) {
    if ( ! jobs.contains(id) ) {
        qDebug() << "Given job id " << id << " is not queued !";
    }
    else {
        jobs[id]->debug();
    }
}
//---------------------------------------------------------------------------------------------------

void JobQueue::addSweep(SharedMorphModel model, int sweep_id)
{
	QueueType queue = queueNamesMap()[config::getInstance()->getApplication().general_resource];
	// sweeps cannot be done interactively
	if ( queue == interactive ) queue = local;
	
	model->sweep_lock = true;
	emit statusMessage(QString("Starting Parameter Sweep %1").arg(sweep_id),0);
	// Create Parameter List and store Parameter values
	QList<AbstractAttribute*> flat_params;
	QList<QString> stored_parameters;
	QList<QStringList> param_values;

	model->param_sweep.createJobList(flat_params,param_values);
	
	for (uint i=0;i<flat_params.size();i++)
		stored_parameters.push_back(flat_params[i]->get());
	
	// Set output terminals to file ...
	QList<AbstractAttribute*> terminal_names = model->rootNodeContr->getModelDescr().terminal_names;
	QList<QString> stored_terminals;
	for (int i=0; i<terminal_names.size(); i++) {
		stored_terminals.push_back(terminal_names[i]->get());
		if (stored_terminals.back() == "wxt" || stored_terminals.back() == "aqua" ||stored_terminals.back() == "x11" ) {
				terminal_names[i]->set("png");
		}
	}
	
	// Fetching sweep information
	QSqlQuery sweep_jobs(config::getDatabase());
	if ( ! sweep_jobs.exec(QString("SELECT * FROM sweeps WHERE id=%1").arg(sweep_id)) ) {
		qDebug() << "Unable to retrieve sweep info from DB: " << sweep_jobs.lastError();
		emit criticalMessage(QString("Unable to retrieve sweep info from DB: %1").append(sweep_jobs.lastError().text()), true);
		return;
	}

	sweep_jobs.first();
	QString sweep_name = sweep_jobs.value(sweep_jobs.record().indexOf("name")).toString();
	QString sweep_dir_name = sweep_jobs.value(sweep_jobs.record().indexOf("subDir")).toString();
	QDir sweep_dir(config::getApplication().general_outputDir);
	sweep_dir.mkdir(sweep_dir_name);
	sweep_dir.cd(sweep_dir_name);
	QString sweep_header =  sweep_jobs.value(sweep_jobs.record().indexOf("header")).toString();
	
// 	writing the sweep summary
	QFile summary_file(sweep_dir.absolutePath() + "/sweep_summary.txt");
	summary_file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate);
	QTextStream summary(&summary_file);
	summary << sweep_header << endl;
	
	// Fetching all the jobs planned
	bool ok = sweep_jobs.exec(QString("SELECT * FROM sweep_jobs WHERE sweep=%1").arg(sweep_id));
	if (!ok) {
		qDebug() << "Unable to retrieve sweep jobs from DB: " << sweep_jobs.lastError();
		emit criticalMessage(QString("Unable to retrieve sweep info from DB: %1").append(sweep_jobs.lastError().text()), true);
		return;
	}
	int param_idx = sweep_jobs.record().indexOf("paramSet");
	int sweep_job_idx = sweep_jobs.record().indexOf("id");
	
	QList<QStringList> param_sets;
	QList<int> sweep_job_ids;
	while (sweep_jobs.next()) {
		param_sets.push_back(sweep_jobs.value(param_idx).toString().split(";"));
		sweep_job_ids.push_back(sweep_jobs.value(sweep_job_idx).toInt());
	}
// 	QDialog* sweep_dialog = new QDialog(0, Qt::Dialog);
// 	sweep_dialog->setModal(true);
// 	sweep_dialog->setWindowTitle("Morpheus Sweep");
// 	sweep_dialog->setLayout(new QVBoxLayout());
// 	sweep_dialog->layout()->setAlignment(Qt::AlignHCenter);
//
// 	QLabel* current_job_label = new QLabel();
// 	current_job_label->setText("Creating jobs");
// 	sweep_dialog->layout()->addWidget(current_job_label);
//
// 	QProgressBar* pBar = new QProgressBar( );
// 	pBar->setRange(0, sweep_job_ids.size());
//
// 	sweep_dialog->layout()->addWidget(pBar);
//
// 	sweep_dialog->open();
	
	QSqlQuery("BEGIN TRANSACTION",config::getDatabase()).exec();
	QSqlQuery update_sweep_job(config::getDatabase());
	update_sweep_job.prepare("UPDATE sweep_jobs SET job = :job_id WHERE id = :sweep_job_id");
	
	// Creating all the jobs
	for (uint job=0; job<sweep_job_ids.size();job++) {

		Q_ASSERT(param_sets[job].size() == flat_params.size());
		// Adjust the model parameters to the current parameter set
		for (int j=0; j<flat_params.size(); j++) {
// 			qDebug() << "j: " << j << ", job: " << job << ", param_set " << param_sets[job][j] ;
			flat_params[j]->set(param_sets[job][j]);
		}

		// Set Up the Job
		int job_id = this->newJobID();
		emit statusMessage(QString("Submitting job %1 (%2/%3)").arg(job_id).arg(job+1).arg(sweep_job_ids.size()),((100 * (job+1))/sweep_job_ids.size() ));

		QSharedPointer<abstractProcess> process;
		if (queue == local) {
			process = QSharedPointer<localProcess>(new localProcess(model, job_id, sweep_dir_name));
		}
		else {
			process = QSharedPointer<remoteProcess>(new remoteProcess(model, job_id, sweep_dir_name));
		}
		
		// Update DB information (link job to sweep)
		jobs[job_id] = process;
		update_sweep_job.bindValue(":job_id",job_id);
		update_sweep_job.bindValue(":sweep_job_id",sweep_job_ids[job]);
		if ( ! update_sweep_job.exec()) {
			qDebug() << "Unable to update job id in sweep_jobs " << update_sweep_job.lastError();
			emit criticalMessage(QString("Unable to update job %1 in sweep_jobs ").arg(job_id).append(update_sweep_job.lastError().text()),false);
		}
		
		// write summary information
		summary << process->info().sim_dir << "\t" << param_sets[job].join("\t")<< endl;
		
		// connetct queue notifiers
		connect(process.data(), SIGNAL(stateChanged(abstractProcess*)), this, SLOT(processStateChanged(abstractProcess*)));
		connect(process.data(), SIGNAL(outputChanged(abstractProcess*)), this, SLOT(processStateChanged(abstractProcess*)));
		connect(process.data(), SIGNAL(criticalMessage(QString)), this, SLOT(processError(QString)));
		emit processAdded(process->jobID());

		// process the queue ...
		if ( queue == remote)
			process->start();
		else if ( queue == local) {
			pending_jobs.append(job_id);
			processQueue();
		}
	}
	QSqlQuery("COMMIT TRANSACTION",config::getDatabase()).exec();
	summary_file.close();
	//revert changed terminals
	for (int i=0; i<stored_terminals.size(); i++) {
		terminal_names[i]->set(stored_terminals[i]);
	}
	// revert sweep parameters
	for (int i=0; i<stored_parameters.size(); i++){
		flat_params[i]->set(stored_parameters[i]);
	}
	model->sweep_lock = false;
	emit statusMessage(QString("Successfully started Parameter Sweep %1").arg(sweep_id));
}

int JobQueue::addProcess(SharedMorphModel model)
{
    QSharedPointer<abstractProcess> process;

    int freeID =  newJobID();
	emit statusMessage(QString("Starting Job %1").arg(freeID));
    QList<QString> stored_terminals;
    // create a local process
    QueueType queue = queueNamesMap()[config::getInstance()->getApplication().general_resource];

    QList<AbstractAttribute*> terminal_names = model->rootNodeContr->getModelDescr().terminal_names;
    if (queue == interactive) {
        // set all gnuplotter terminals to screen terminal

        for (int i=0; i<terminal_names.size(); i++) {
            stored_terminals.push_back(terminal_names[i]->get());
            terminal_names[i]->set("screen");
        }
    }
    else if (queue == remote) {
		QVector<QString> screen_term = {"screen","qt","wxt","x11","windows","aqua"};
        // set all gnuplotter terminals to png
        QList<AbstractAttribute*> terminal_names = model->rootNodeContr->getModelDescr().terminal_names;
        for (int i=0; i<terminal_names.size(); i++) {
            stored_terminals.push_back(terminal_names[i]->get());
            if (screen_term.contains(stored_terminals.back())) {
                 terminal_names[i]->set("png");
            }
        }
    }

    if (queue == local || queue == interactive)
    {
        process = QSharedPointer<localProcess>(new localProcess(model, freeID));

    }
    // create a remote process
    else
    {
        process = QSharedPointer<remoteProcess>(new remoteProcess(model, freeID));
    }
    //revert changed terminals
    for (int i=0; i<stored_terminals.size(); i++) {
        terminal_names[i]->set(stored_terminals[i]);
    }

    jobs[freeID] = process;
    connect(process.data(), SIGNAL(stateChanged(abstractProcess*)), this, SLOT(processStateChanged(abstractProcess*)));
    connect(process.data(), SIGNAL(outputChanged(abstractProcess*)), this, SLOT(processStateChanged(abstractProcess*)));
	connect(process.data(), SIGNAL(criticalMessage(QString)), this, SLOT(processError(QString)));

    emit processAdded(process->jobID());

    if ( queue == remote)
    {
        process->start();
    }
    else if ( queue == local)
    {
        pending_jobs.append(freeID);
        processQueue();
    }
    else if ( queue == interactive)
    {
        pending_interactive_jobs.append(freeID);
        processQueue();
    }
    else Q_ASSERT(FALSE);

    Q_ASSERT(freeID == process->jobID());

    return freeID;
}

//---------------------------------------------------------------------------------------------------

void JobQueue::processStateChanged(abstractProcess *process) {
    if(process->state() == ProcessInfo::DONE || process->state() == ProcessInfo::EXIT)
    {
        disconnect(process,SIGNAL(outputChanged(abstractProcess*)));
        disconnect(process,SIGNAL(stateChanged(abstractProcess*)));
		disconnect(process,SIGNAL(criticalMessage(QString)));
        if(process->info().resource == ProcessInfo::local) {
            running_jobs.removeAll(process->jobID());
			if (running_interactive_jobs.contains(process->jobID())) {
				running_interactive_jobs.removeAll(process->jobID());
				finished_interactive_jobs.append(process->jobID());
			}
			processQueue();
        }
    }
    emit processChanged(process->jobID());
}

void JobQueue::processError ( QString error )
{
	bool popup = true;
	if (sender()) {
		abstractProcess* source = qobject_cast<abstractProcess*>(sender());
		if (source)
			popup = pending_interactive_jobs.contains(source->jobID()) || running_interactive_jobs.contains(source->jobID()) || finished_interactive_jobs.contains(source->jobID()) ;
	}
	emit criticalMessage(error, popup);
}


//---------------------------------------------------------------------------------------------------

void JobQueue::processQueue()
{
    if (! is_initialized) {
        timer->setInterval( config::getApplication().preference_jobqueue_interval );
        restoreSavedJobs();
        is_initialized = true;
    }

    int num_concurrent_jobs = config::getApplication().local_maxConcurrentJobs;

    while ( running_jobs.size() < num_concurrent_jobs && ! pending_interactive_jobs.empty())
    {
		int job_id = pending_interactive_jobs.front();
		pending_interactive_jobs.pop_front();
		running_jobs.push_back(job_id);
		running_interactive_jobs.push_back(job_id);
		
        jobs[job_id]->start();
    }

    while ( running_jobs.size() < num_concurrent_jobs && ! pending_jobs.empty() )
    {
		int job_id = pending_jobs.front();
		pending_jobs.pop_front();
		running_jobs.push_back(job_id);
        
		
        jobs[job_id]->start();

    }
}

//---------------------------------------------------------------------------------------------------


const QMap<int, QSharedPointer<abstractProcess> >& JobQueue::getJobs() {
    return jobs;
}

//---------------------------------------------------------------------------------------------------

JobQueue::~JobQueue() {

    // stop local processes befor killing them. this prevents event notifications during destruction
    pending_jobs.clear();
    for ( QMap<int, QSharedPointer<abstractProcess> >::iterator i=jobs.begin(); i != jobs.end(); i++) {
        if (i.value()->info().resource == ProcessInfo::local && i.value()->state() == ProcessInfo::RUN) {
            i.value()->stop();
        }
    }
}
