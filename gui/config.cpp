#include "config.h"
#include "version.h"
#include "job_queue.h"


//	#include <QtPlugin>
	// Q_IMPORT_PLUGIN(qsqlite)
	// Q_IMPORT_PLUGIN(qtiff)

	
config* config::instance = 0;

//------------------------------------------------------------------------------
config::~config() {
	job_queue->deleteLater();
	job_queue_thread->deleteLater();
	if (db.isOpen())
		db.close();
    QSqlDatabase::removeDatabase("SQLITE");
}

config::config() : QObject(), helpEngine(NULL) {
    /* Restore Configuration setting from QSettings file*/
    QSettings settings;
    settings.beginGroup("simulation");
        QString oD_default = QDesktopServices::storageLocation(QDesktopServices::HomeLocation);
        oD_default +="/morpheus";
        app.general_outputDir       = settings.value("outputDir", oD_default).toString();
    settings.endGroup();


    settings.beginGroup("preferences");
        app.preference_stdout_limit = settings.value("stdout_limit", 10).toInt();
        app.preference_max_recent_files = settings.value("max_recent_files", 10).toInt();
        app.preference_jobqueue_interval = settings.value("jobqueue_interval", 2500).toInt();
        app.preference_jobqueue_interval_remote = settings.value("jobqueue_interval_remote", 10000).toInt();
    settings.endGroup();

#ifdef Q_OS_WIN32
        QString exec_default = "morpheus.exe";
        QString gnuplot_default = settings.value("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\gnuplot.exe", "").toString();
        QString ffmpeg_default =  settings.value("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\ffmpeg.exe", "").toString();
#else
		QString exec_default = "morpheus";
		QString gnuplot_default = "";
        QString ffmpeg_default = "";
#endif

    settings.beginGroup("local");

// Q_FOREACH ( QString path, morpheus_exec_paths ) { if (QFileInfo(path).exists()) { exec_default = path; break; } }
        app.local_executable        = settings.value("executable",exec_default).toString();
		app.local_GnuPlot_executable = settings.value("GnuPlotExecutable",gnuplot_default).toString();
        app.local_FFmpeg_executable = settings.value("FFmpegExecutable",ffmpeg_default).toString();
        app.local_maxConcurrentJobs = settings.value("maxConcurrentJobs",1).toInt();
        app.local_maxThreads        = settings.value("maxThreads",1).toInt();
        app.local_timeOut           = settings.value("timeOut",999).toInt();
    settings.endGroup();

    settings.beginGroup("remote");
        QString local_username = QString( getenv("USER") ); // best guess is that remote username is equal to local username
        app.remote_user             = settings.value("user",local_username).toString();
        app.remote_host             = settings.value("host","deimos.hrsk.tu-dresden.de").toString();
        app.remote_simDir           = settings.value("simDir","simulation").toString();
        app.remote_executable       = settings.value("executable","morpheus").toString();
		app.remote_GnuPlot_executable = settings.value("GnuPlotExecutable","").toString();
        app.remote_maxThreads       = settings.value("maxThreads",4).toInt();
        app.remote_dataSyncType     = settings.value("dataSyncType","continuous").toString();
    settings.endGroup();

    current_model = -1;
	qRegisterMetaType<SharedMorphModel>("SharedMorphModel");

/*
       Initialize a SQLite database for jobs and sweeps
*/
	try {
		db = QSqlDatabase::addDatabase("QSQLITE");
		if(!db.isValid() ) throw;
		
/*		QPluginLoader loader("qsqlite4.dll");
		QObject *plugin = loader.instance();
		if ( ! plugin) throw;
		QSqlDriverPlugin *sqlPlugin  = qobject_cast<QSqlDriverPlugin *>(plugin);
		if (!sqlPlugin ) throw;
		db = QSqlDatabase::addDatabase(sqlPlugin->create("QSQLITE"));*/
		
		QDir job_db_path(QDesktopServices::storageLocation(QDesktopServices::DataLocation));
		job_db_path.mkpath(job_db_path.path());
		db.setDatabaseName(job_db_path.filePath("morpheus.db.sqlite"));
		qDebug() << "SQLite database path: " << db.databaseName();
		
		if (!db.open()) throw;
	}
	catch (...) {
		QMessageBox::critical(0,
			qApp->tr("Invalid database"),
			qApp->tr("Invalid SQL database connection.\nClick Cancel to exit."),
			QMessageBox::Cancel );
		throw("Unable to create database connection");
		qApp->exit();
	}

/*		std::cout << "database NOT open" << std::endl;
		QMessageBox::critical(0, qApp->tr("Cannot open database"),
		qApp->tr("Unable to establish a database connection.\n"
		         "Morpheus needs SQLite support to store job information.\n\n"
		         "Click Cancel to exit."), QMessageBox::Cancel);
		qApp->quit();
	} */
	
	db.exec("PRAGMA synchronous=NORMAL");
	if ( ! db.tables().contains("jobs")) {
		qDebug() << "creating Job DataBase";
		QSqlQuery query;
		query.prepare(
			"CREATE TABLE IF NOT EXISTS jobs ("
			"id INTEGER PRIMARY KEY, "
			// simulation
			"processPid INTEGER DEFAULT -1,"
			"processThreads INTEGER DEFAULT 1, "
			"processState INTEGER DEFAULT -1, "
			"processResource INTEGER DEFAULT -1, "
			"simTitle VARCHAR(255) DEFAULT \"\" , "
			"simXMLname VARCHAR(255) DEFAULT \"\", "
			"simDirectory VARCHAR(255) DEFAULT \"\", "
			// time
			"timeStart INTEGER DEFAULT 0, "
			"timeStop INTEGER DEFAULT 1, "
			"timeCurrent INTEGER DEFAULT 0, "
			"timeExec INTEGER DEFAULT 0, "
			// remote 
			"remoteTask INTEGER DEFAULT 0 )"
		);
		bool ok = query.exec();
		if( !ok ){
			qDebug() << "Creating SQL table jobs failed: " << query.lastError();
		}
		
// 		query.prepare("CREATE INDEX jobsweeps ON jobs(sweepId) ");
// 		ok = query.exec();
// 		if( !ok ){
// 			qDebug() << "Creating SQL index jobs.sweepId failed: " << query.lastError();
// 		}
// 		
		query.prepare(
			"CREATE TABLE IF NOT EXISTS sweeps ("
				"id INTEGER PRIMARY KEY, "
				"name VARCHAR(255) DEFAULT \"\", "
				"header TEXT DEFAULT \"\","
				"subDir VARCHAR(255) DEFAULT \"\" ,"
				"paramData BLOB"
			")"
		 );
		ok = query.exec();
		if( !ok ){
			qDebug() << "Creating SQL table sweeps failed: " << query.lastError();
		}
		
		query.prepare(
			"CREATE TABLE IF NOT EXISTS sweep_jobs ("
				"id INTEGER DEFAULT NULL PRIMARY KEY , "
				"sweep REFERENCES sweeps(id), "
				"job REFERENCES jobs(id), "
				"paramSet TEXT DEFAULT \"\""
			")"
		);
		ok = query.exec();
		if( !ok ){
			qDebug() << "Creating SQL table sweep_jobs failed: " << query.lastError();
		}
		query.prepare("CREATE INDEX swjo1 ON sweep_jobs(sweep) ");
		ok = query.exec();
		if( !ok ){
			qDebug() << "Creating SQL index on sweep_jobs(sweep) failed: " << query.lastError();
		}
		query.prepare("CREATE UNIQUE INDEX swjo2 ON sweep_jobs(job) ");
		ok = query.exec();
		if( !ok ){
			qDebug() << "Creating SQL index on sweep_jobs(job) failed: " << query.lastError();
		}

		query.prepare("CREATE  TRIGGER remove_job AFTER DELETE ON jobs BEGIN DELETE FROM sweep_jobs WHERE job=OLD.id; END; ");
		ok = query.exec();
		if( !ok ){
			qDebug() << "Creating SQL trigger for removing jobs failed: " << query.lastError();
		}
		
		query.prepare(
			"CREATE TRIGGER remove_sweep_job AFTER DELETE ON sweep_jobs "
			"BEGIN "
				"DELETE FROM sweeps WHERE "
					"OLD.sweep=id "
					"AND NOT EXISTS (SELECT * FROM sweep_jobs WHERE sweep=OLD.sweep); "
			"END ");
				ok = query.exec();
		if( !ok ){
			qDebug() << "Creating SQL trigger for removing sweep_jobs failed: " << query.lastError();
		}
	}
	db.close();

	// Creating a Job Queue that runs in a separate thread ..
	qDebug() << "Main thread " << QThread::currentThreadId();
	job_queue_thread = new QThread();
	job_queue = new JobQueue();
	job_queue->moveToThread(job_queue_thread);
	connect( job_queue_thread, SIGNAL(started()), job_queue, SLOT(run()) );

	// 		connect( task, SIGNAL(workFinished()), thread, SLOT(quit()) );
	//automatically delete thread and task object when work is done:
	connect( job_queue_thread, SIGNAL(finished()), job_queue, SLOT(deleteLater()) );
	connect( job_queue_thread, SIGNAL(finished()), job_queue_thread, SLOT(deleteLater()) );

	job_queue_thread->start();
}

//------------------------------------------------------------------------------

config* config::getInstance() {
    if ( config::instance == 0 ) {
        config::instance = new config();
    }
    return config::instance;
}


//------------------------------------------------------------------------------

SharedMorphModel config::getModel() {
//    qDebug() << getInstance()->current_model << " " << getInstance()->openModels.size();
	if (getInstance()->openModels.empty())
		return SharedMorphModel();
    if (getInstance()->current_model > getInstance()->openModels.size() -1)
        getInstance()->current_model = getInstance()->openModels.size() -1;
    return getInstance()->openModels[getInstance()->current_model];
}

// ---------------------------------------------------

QString config::getPathToExecutable(QString exec_name) {
	
	exec_name = exec_name.trimmed();
#ifdef Q_OS_WIN32
	if (!exec_name.endsWith(".exe"))
		exec_name.append(".exe");
#endif
	QFileInfo info;
	info.setFile(exec_name);
	if (info.exists() && info.isExecutable() && info.isFile()) {
		qDebug() << "Found executable " << info.filePath();
		return info.canonicalFilePath();
	}
	
	QString app_name = exec_name.split("/").last();

	info.setFile(QCoreApplication::applicationDirPath() + "/" + app_name);
	if (info.exists() && info.isExecutable()  && info.isFile()) {
		qDebug() << "Found executable " << info.filePath();
		return info.canonicalFilePath();
	}
	
	char* env_paths_c = getenv("PATH");
#ifdef Q_OS_WIN32
	QStringList env_paths = QString(env_paths_c).split(";");
#else
	QStringList env_paths = QString(env_paths_c).split(":");
#endif
	
	Q_FOREACH (const QString& path, env_paths) {
		info.setFile(path + "/" + app_name);
		if (info.exists() && info.isExecutable()  && info.isFile()) {
			qDebug() << "Found executable " << info.filePath();
			return info.canonicalFilePath();
		}
	}
	
	return "";
	
}

//------------------------------------------------------------------------------

QList< SharedMorphModel > config::getOpenModels() {
    return getInstance()->openModels;
}


JobQueue* config::getJobQueue() {
    Q_ASSERT(getInstance()->job_queue);
    return getInstance()->job_queue;
}

//------------------------------------------------------------------------------

int config::openModel(QString filepath) {
	if (filepath.isEmpty()) return -1;
    // if the model is already open, just switch to that model
    config* conf = getInstance();
	QString xmlFile = QFileInfo(filepath).absoluteFilePath();
    for (int i=0; i<conf->openModels.size(); i++) {
       if (xmlFile==conf->openModels[i]->xml_file.path) {
           return i;
       }
    }

    SharedMorphModel m;
    try {
        m = SharedMorphModel( new MorphModel(xmlFile,conf));
    }
    catch(ModelException e)
    {
        // TODO activate this message ...
        QMessageBox::critical(qApp->activeWindow(),"Error opening morpheus model",e.message);
		return -1;
    }
    // substitude the last model if it was created from scratch and is still unchanged
    if ( ! conf->openModels.isEmpty() && conf->openModels.back()->isEmpty()) {
        conf->openModels.back()->close();
        emit conf->modelClosing(conf->openModels.size()-1);
        conf->openModels.pop_back();
		  conf->current_model = -1;
    }
    conf->openModels.push_back(m);
    int new_index = conf->openModels.size()-1;
    addRecentFile(m->xml_file.path);
    emit conf->modelAdded(new_index);
//     switchModel(new_index);

    return new_index;
}

int config::importModel(QSharedPointer< MorphModel > model)
{
	config* conf = getInstance();
	// substitude the last model if it was created from scratch and is still unchanged
	if ( ! conf->openModels.isEmpty() && conf->openModels.back()->isEmpty()) {
		conf->openModels.back()->close();
		emit conf->modelClosing(conf->openModels.size()-1);
		conf->openModels.pop_back();
	}

	conf->openModels.push_back(model);
	int new_index = conf->openModels.size()-1;
	emit conf->modelAdded(new_index);
// 	switchModel(new_index);

	return new_index;
}


//------------------------------------------------------------------------------

int config::createModel(QString xml_path)
{
    config* conf = getInstance();

    if ( ! conf->openModels.isEmpty() &&  conf->openModels.back()->isEmpty()) {
        conf->openModels.back()->close();
        emit conf->modelClosing(conf->openModels.size()-1);
        conf->openModels.pop_back();
    }
    SharedMorphModel m;
    if (xml_path.isEmpty()) {
		try {
			m =  SharedMorphModel(new MorphModel(conf));
		}
		catch(ModelException e)
		{
			QMessageBox::critical(qApp->activeWindow(),"Error creating morpheus model",e.message ,QMessageBox::Ok,QMessageBox::NoButton);
			return -1;
		}
	}
	else {

		try {
			m = SharedMorphModel( new MorphModel(xml_path,conf));
		}
		catch(ModelException e)
		{
			QMessageBox::critical(qApp->activeWindow(),"Error creating morpheus model",e.message ,QMessageBox::Ok,QMessageBox::NoButton);
			return -1;
		}
        
        m->xml_file.path = "";
        m->xml_file.name = MorpheusXML::getNewModelName();
        m->rootNodeContr->saved();
    }
    int id = conf->openModels.size();
    conf->openModels.push_back(m);
    emit conf->modelAdded(id);
//     switchModel(id);

    return id;
}

//------------------------------------------------------------------------------

bool config::closeModel(int index, bool create_model)
{
    config* conf = getInstance();
    if (index == -1) index = conf->current_model;
    if (conf->openModels[index]->close()) {
        emit conf->modelClosing(index);
        conf->openModels.removeAt(index);
        if (conf->current_model < conf->openModels.size())
            conf->switchModel(conf->current_model);
        else if (conf->current_model>0)
            conf->switchModel(conf->current_model-1);
        else {
            if (create_model)
                createModel();
        }
    }
    else
        return false;
    return true;
}

//------------------------------------------------------------------------------

void config::switchModel(int index) {
    config* conf = config::getInstance();
    if (index == conf->current_model) return;
    conf->current_model = index;
    if (index>=0)
        emit conf->modelSelectionChanged(conf->current_model);
}


//------------------------------------------------------------------------------

QSqlDatabase& config::getDatabase()
{
	config* conf = getInstance();
	if ( ! conf->db.isOpen() )
		conf->db.open();
	return conf->db;
}



//------------------------------------------------------------------------------

const config::application& config::getApplication() {
    return getInstance()->app;
}

//------------------------------------------------------------------------------

const QList<QDomNode> config::getNodeCopies() {
    return getInstance()->xmlNodeCopies;
}

//------------------------------------------------------------------------------

void config::setApplication(application a) {

	config* conf = getInstance();
	if (a.remote_user != conf->app.remote_user || a.remote_host != conf->app.remote_host) {
		getInstance()->app = a;
		sshProxy().clearSessions();
	}
	else {
		getInstance()->app = a;
	}

    QSettings settings;
    settings.beginGroup("simulation");
        settings.setValue("outputDir", a.general_outputDir);
    settings.endGroup();

    settings.beginGroup("preferences");
        settings.setValue("stdout_limit", a.preference_stdout_limit);
        settings.setValue("max_recent_files", a.preference_max_recent_files);
        settings.setValue("jobqueue_interval", a.preference_jobqueue_interval);
        settings.setValue("jobqueue_interval_remote", a.preference_jobqueue_interval_remote);
    settings.endGroup();

    settings.beginGroup("local");
        settings.setValue("executable",         a.local_executable);
		settings.setValue("GnuPlotExecutable",  a.local_GnuPlot_executable);
        settings.setValue("FFmpegExecutable",  a.local_FFmpeg_executable);
        settings.setValue("maxConcurrentJobs",  a.local_maxConcurrentJobs);
        settings.setValue("maxThreads",         a.local_maxThreads);
        settings.setValue("timeOut",            a.local_timeOut);
    settings.endGroup();

    settings.beginGroup("remote");
        settings.setValue("user",               a.remote_user);
        settings.setValue("host",               a.remote_host);

        settings.setValue("executable",         a.remote_executable);
		settings.setValue("GnuPlotExecutable",  a.remote_GnuPlot_executable);
        settings.setValue("maxThreads",        a.remote_maxThreads);

        settings.setValue("dataSyncType",       a.remote_dataSyncType);
        settings.setValue("simDir",             a.remote_simDir);
    settings.endGroup();

}


//------------------------------------------------------------------------------

void config::setComputeResource(QString resource) {
    getInstance()->app.general_resource = resource;
}

//------------------------------------------------------------------------------

void config::receiveNodeCopy(QDomNode nodeCopy) {
    xmlNodeCopies.push_front(nodeCopy);
    while(xmlNodeCopies.size() > MaxNodeCopies) {
        xmlNodeCopies.pop_back();
    }
}

//------------------------------------------------------------------------------
void config::openExamplesWebsite(){
	QDesktopServices::openUrl(QUrl("http://imc.zih.tu-dresden.de/wiki/morpheus/doku.php?id=examples:examples"));
}
//------------------------------------------------------------------------------
void config::openMorpheusWebsite(){
	QDesktopServices::openUrl(QUrl("http://imc.zih.tu-dresden.de/wiki/morpheus"));
}

//------------------------------------------------------------------------------
void config::aboutModel()
{
    QString about;
	if (!getInstance()->getModel())
		return;
    QString title = getInstance()->getModel()->rootNodeContr->getModelDescr().title;
    QString details = getInstance()->getModel()->rootNodeContr->getModelDescr().details;

//    nodeController* title = getInstance()->getModel()->rootNodeContr->firstChild("Simulation");
//    if (title) title = title->firstChild("Title");
//    if (title)
//        about_message += title->getText() + "\"\n";
//    else
//        about_message +" \"unnamed\" \n";


    about += "Model:\n " + (title.size()>0?title:"Unknown title") + (details.size()==0?"":"\n\nDetails:\n " + details) +"\n\nFile:\n " + getModel()->xml_file.path;
//    about_message += QString("Number of edit operations %1 \n").arg(getModel().rootNodeContr->getModelDescr().edits);

//    QMessageBox::information(qApp->activeWindow(), QString("About model"), about, QMessageBox::Ok);

    QMessageBox msgBox(QMessageBox::Information, "About model",about,QMessageBox::Ok);
    msgBox.setIconPixmap(QPixmap(":/logo.png"));
    //msgBox.setParent(qApp->activeWindow());
    msgBox.exec();
}

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------

void config::aboutPlatform()
{
//#ifndef MORPHEUS_SVN_REVISION
//    #define MORPHEUS_SVN_REVISION "MORPHEUS_SVN_REVISION not defined"
//#endif
	
	QDate date = QDate::currentDate();
	
	QString header = "<p align='center'>Morpheus<br>";


	QString info  = "<p align='center'>Modeling and simulation environment for multi-scale and multicellular systems biology</p>"
					"<p align='center'>Version "+QString(MORPHEUS_VERSION_STRING)+", revision " + QString(MORPHEUS_REVISION_STRING) + "<br><br>"
					"Developed by Jörn Starruß and Walter de Back<br>"
					"Copyright 2009-"+ QString::number( date.year() )+", Technische Universität Dresden.<br><br>"
					"More information:<br><a href=\"http://imc.zih.tu-dresden.de/wiki/morpheus\">http://imc.zih.tu-dresden.de/wiki/morpheus</a></p><br>"
					"<p align='center'>Disclaimer:<br><font size=\"2\">Non-commercial use: Morpheus (the Software) is distributed for academic use and cannot be used for commercial gain without explicitly written agreement by the Developers. No warranty: The Software is provided \"as is\" without warranty of any kind, either express or implied, including without limitation any implied warranties of condition, uninterrupted use, merchantability, fitness for a particular purpose, or non-infringement. No liability: The Developers do not accept any liability for any direct, indirect, incidential, special, exemplary or consequential damages arising in any way out of the use of the Software.</font></p>";

//     QString title = "Morpheus: Modeling environment for multiscale and multicellular systems biology";
// 
//     QString copyright = "Copyright 2009-"+QDate::year()+", Technische Universität Dresden.";
// 
//     QString developers = "Developers: Jörn Starruß and Walter de Back";
// 
// //    QString contributors = "Contributors:\n Martin Lunze\n Peggy Thiemt\n Fabian Rost\n Robert Müller";
// 
//     QString version = "Version: 1.0.0 \n Revision: " + QString(MORPHEUS_SVN_REVISION);
// 
// 	QString website = "Website: <a href=\"http://imc.zih.tu-dresden.de/wiki/morpheus\">http://imc.zih.tu-dresden.de/wiki/morpheus<\a>";
// 
//     QString about = title +"\n\n" + copyright + "\n\n" + developers + "\n\n" + version + "\n\n" + version;

    QMessageBox msgBox; //(QMessageBox::Information, "About Morpheus",about,QMessageBox::Ok);
    
    msgBox.setText(header);
    msgBox.setInformativeText(info);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);    
	msgBox.setTextFormat(Qt::RichText);
	msgBox.setIconPixmap(QPixmap(":/logo.png"));
    //msgBox.setParent(qApp->activeWindow());
    msgBox.exec();

}


QHelpEngine* config::getHelpEngine()
{
	config* conf = getInstance();
	if (!conf->helpEngine) {
		QApplication::applicationDirPath();
		
		QStringList doc_path;
		doc_path <<  QApplication::applicationDirPath() + "/"
					<< QApplication::applicationDirPath() + "/doc/"
					<< QApplication::applicationDirPath() + "/../share/morpheus/"
					<< QApplication::applicationDirPath() + "/../../Resources/doc/"; // for Mac app bundle
		QString path;
		foreach(const QString& p, doc_path) {
			qDebug() << "Testing "  << p + "morpheus.qhc";
			if (QFile::exists(p+"morpheus.qhc"))
				path = p;
		}
		
		if (path.isEmpty()) {
			qDebug() << "Help engine setup failed. Unable to locate 'morpheus.qhc'.";
			conf->helpEngine = new QHelpEngine("");
		}
		else {
			conf->helpEngine = new QHelpEngine(path+"morpheus.qhc");
			if (conf->helpEngine->setupData() == false)
			{
				qDebug() << "Help engine setup failed";
				qDebug() << conf->helpEngine->error();
			}
			else 
				qDebug() << conf->helpEngine->namespaceName("morpheus.qhc");
		}
	}
	return conf->helpEngine;
}


void config::aboutHelp() {
	
	QDialog* help_box = new QDialog(0,Qt::Dialog );

	QHelpEngine* help = getHelpEngine();
	qDebug() << help->linksForIdentifier ( "Chemotaxis" );

	// "org.doxygen.Project"
	help_box->setModal(true);
	QBoxLayout* help_layout = new QBoxLayout(QBoxLayout::TopToBottom);
	help_box->setLayout(help_layout);
// 	HelpBrowser * help_view = new HelpBrowser(help);
// 	help_view->load(help.linksForIdentifier ( "Chemotaxis" ).begin().value());
// 	help_view->setSource(QUrl("qthelp://org.doxygen.project/doc/group__Chemotaxis.html"));
// 	help_view->load(QUrl("http://google.com"));
	
	QSplitter *helpPanel = new QSplitter(Qt::Vertical);
	QHelpContentWidget* contentBrowser = help->contentWidget();
	helpPanel->insertWidget(0, contentBrowser);
// 	connect(contentBrowser, SIGNAL(linkActivated(const QUrl&)) , help_view, SLOT(setSource(const QUrl&)) );
// 	helpPanel->insertWidget(1,help_view);
	help_layout->addWidget(helpPanel);
	
	help_box->exec();
}


//------------------------------------------------------------------------------

void config::addRecentFile(QString filePath)
{
    QStringList files = QSettings().value("recentFileList").toStringList();
    files.removeAll(filePath);
    files.prepend(filePath);
    while(files.size() > config::getApplication().preference_max_recent_files)
        files.removeLast();
    QSettings().setValue("recentFileList", files);

    emit config::getInstance()->newRecentFile();
}
