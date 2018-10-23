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

#ifndef CONFIG_H
#define CONFIG_H

#include <QtGui>
#include <QDomDocument>
#include <QDesktopServices>
#include <QtHelp/QHelpContentWidget>
#include <QtHelp/QHelpEngine>
#include <QtHelp/QtHelp>
#include <QtWebKit/QWebView>
#include <QTextBrowser>

#include <QtSql/QSqlDatabase>
#include <QSqlDriverPlugin>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlTableModel>


#if QT_VERSION < 0x040600
    inline QIcon QThemedIcon(QString a, QIcon b) { return b; };
#else
    inline QIcon QThemedIcon(QString a, QIcon b) { return QIcon::fromTheme(a,b); };
#endif

#include "morpheus_model.h"
#include "network_access.h"
// #include "configuration.h"


// Enable the Morpheus Usage Feedback System
// #define MORPHEUS_FEEDBACK

class JobView;
class JobQueue;

/*!
  Using this class creates an static object, which holds several informations about the loaded cpm-model,<br>
  the application-window (morpheus-gui) itself and about the simulations respectivly the settings,<br>
  which are necessary to start simulations.<br>
  The main-task of it is providing interfaces to validate, load and save xml-configurations of cpm-models.<br>
  Additionally it is possible to manipulate the informations hold in this class via some functions.
  */

class config : public QObject
{
Q_OBJECT

public:
	// Singleton
	config(const config&) = delete;
	const config& operator=(const config&) = delete;
	
    /*!
      This struct holds informations necessary to start morpheus-simulations.
      */
    struct application {
        QString general_resource; /*!< Location where morpheus-simulations should be executed. (local or remote) */
        QString general_outputDir; /*!< Local directory in which the data of simulations should be saved. */

		bool preference_allow_feedback;
        int preference_stdout_limit; /*!< Limit to the size of stdout in model.xml.out (simulation terminates when exceeded) */
        int preference_max_recent_files; /*!< Number of the maximal shown recent files in the menubar of morpheus-gui. */
        int preference_jobqueue_interval; /*!< Interval between updates of jobs in job queue */
        int preference_jobqueue_interval_remote; /*!< Interval between updates of jobs in job queue, for remote jobs*/

        QString local_executable; /*!< Path to the local executable of Morpheus. */
        QString local_GnuPlot_executable; /*!< Path to the local executable of GnuPlot. */
        QString local_FFmpeg_executable; /*!< Path to the local executable of FFmpeg. */
        int     local_maxConcurrentJobs; /*!< Number of maximal running local jobs. */
        int     local_maxThreads; /*!< Number of maximal useable threads on local machine. */
        int     local_timeOut; /*!< Maximal running-time for local simulations, before they will be stopped. */

        QString remote_user; /*!< Login-name of the user-account on the external machine. */
        QString remote_host; /*!< External machine, which will be used for remote simulations. */
        QString remote_executable; /*!< Path to the external executable of Morpheus. */
        QString remote_GnuPlot_executable; /*!< Path to the remote executable of GnuPlot. */
        QString remote_simDir; /*!< Directory on the external machine, in which the simulations will be started. */
        int     remote_maxThreads; /*!< Number of maximal useable threads on external machine. */
        QString remote_dataSyncType; /*!< Type of syncronizing the simulation-data of external jobs. (Continously or post hoc) */
    };

    struct modelIndex {
        int model; int part;
    };


    //static const int MaxRecentFiles = config::app.preference_max_recent_files; /*!< Number of the maximal shown recent files in the menubar of morpheus-gui. */
    static const int MaxNodeCopies = 5; /*!< Number of the maximal copied nodes stored in config. */

private:
    config();
    ~config();
    static config *instance;

    application app;
	QHelpEngine* helpEngine = NULL;
	ExtendedNetworkAccessManager* network = NULL;

    QList< SharedMorphModel > openModels;
    int current_model;
	

    JobQueue* job_queue = NULL;
	QThread* job_queue_thread = NULL;
	QSqlDatabase db;
	QMutex change_lock;
	/*!< SQLite database that stores job information*/


	QList<QDomNode> xmlNodeCopies; /*!< Temporarily stored copies of xml-nodes. */
	QDomDocument clipBoard_Document;

public:
	static QSqlDatabase& getDatabase();
	/*!< Return SQLite database that stores job information*/
	
    static config* getInstance();
    /*!< Returns an instance of class config. */
	
	static QString getVersion();
	/*!< Returns the Morpheus version string. */

    static const application& getApplication();
    /*!< Returns informations about the application-settings to start simulations. */

    static SharedMorphModel getModel();
    ///< Returns the currently selected model
	
	static QHelpEngine* getHelpEngine(bool lock=true);
	static ExtendedNetworkAccessManager* getNetwork();

    static void switchModel(int index);

    static int createModel(QString path = "");
	static int importModel(QSharedPointer<MorphModel> model);
    static int openModel(QString path = "");
    ///< Open the model file @param path. Returns the index of the new model or -1

    static bool closeModel(int index = -1, bool create_model = true);
    /*!< Close the model with index @param index, by default the current file.
      A saveOrReject-dialog is opened in case the current xml-model was modified.
      Returns false if the user canceled the operation or the operation failed. */

    static QList<SharedMorphModel> getOpenModels();
    ///< Returns a list of all open models.

    static void addRecentFile(QString filePath);
	static QString getPathToExecutable(QString exec_name);

    static JobQueue* getJobQueue();

    static const QList<QDomNode> getNodeCopies();

    static void setApplication(application a);
    /*!< Sets the application-informations to @param a. */

private slots:
	void ClipBoardChanged();

public slots:
    void setComputeResource(QString resource);
    /*!< Sets the location where cpm-simulations should be executed to 'add' (local or remote). */
    void receiveNodeCopy(QDomNode nodeCopy);
    /*!< Sets the temporarily stored copy of xml-node to 'nodeCopy'. */
	
	static void aboutModel(); /*!< Opens an about-dialog, which gives informations about the current loaded xml-model. */
    static void aboutPlatform(); /*!< Pop-up message box with information about the editor-plattform. */
	static void aboutHelp();
	static void openExamplesWebsite();
	static void openMorpheusWebsite();
signals:
    void modelAdded(int index);
    void modelClosing(int index);
    void modelSelectionChanged(int index);
    /*!< Signal is send when a new xml-model is loaded successfully. */
    void newRecentFile();
    /*!< Signal is send when the current filename changes. */
};

#endif // CONFIG_H
