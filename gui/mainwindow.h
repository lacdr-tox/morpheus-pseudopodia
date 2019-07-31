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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui>
#include <QSharedPointer>
#include <QProcess>
#include <iostream>
#include <sstream>
#include <algorithm>

// #include "xsd.h"
#include "config.h"
#include "job_queue.h"
#include "parametersweeper.h"


#include "domnodeviewer.h"
#include "about_model.h"
#include "jobview.h"
#include "jobcontroller.h"
#include "docu_dock.h"
#include "xmlpreview.h"
#include "settingsdialog.h"
#include "sbml_import.h"
#include "announcement.h"

using namespace std;

/*!
This class presents the main-window of the GUI for a cpm-simulation-editor. <br>
With the menubar you can create, load and save any valide cpm-model.<br>
It presents different tabs for the single parts of the loaded simulation-description. <br>
(for example: 'Simulation', 'Cellpopulation', 'Interaction', 'Analysis',...) <br>
These tabs can be used to manipulate whole xml-nodes or only specific xml-attributes.<br>
As well it provides other tabs for viewing the xml-model-description,<br>
for starting and managing many simulations of a cpm-model and for doing parametersweeps.
*/
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0); /*!< This constructor creates the whole GUI, with all widgets and objects. */
    ~MainWindow();

    void readSettings(); /*!< Restores the window-geometry from QSettings. */
    void closeEvent(QCloseEvent*); /*!< Closes the window and saves the geometry of window in QSettings. */

private:
    SharedMorphModel current_model; /// Holds a pointer to the active model
    config::modelIndex model_popup_index;
    config::modelIndex model_index;
	const QString param_sweep_name = "ParamSweep";

    QMenu *addModelPartMenu; /*!< Menu, which appears when clicking on the '+'-button in the tabbar, to add a new tab. */
    QMenu *modelMenu; /*!< Menu, which appears when requesting a menu for a tab in the tabbar (to remove or copy the tab). */
    QAction *showModelXMLAction, *closeModelAction, *mailAttachAction;
    QAction *copyModelPartAction, *removeModelPartAction,*addModelPartAction, *pasteModelPartAction;
//     QMenu* jobQueueMenu;
//     QMenu* jobQueueGroupingMenu;
    QMenuBar *menubar; /*!< Menubar of the mainwindow (load or save a xml-file, or open settings). */
    QComboBox *cb_resource; /*!< Combobox to choose the resource, where simulations shall be started. */
    QPushButton* permanentStatus; /*!< Statusbar at the bottom of window, which shows the latest state-news. */
    QObject* statusMsgSource;
    QToolButton* interactive_stop_button;

    QStackedWidget *editorStack;
    QDockWidget* documentsDock;
    QTreeWidget *modelList;
    QListWidget *clipBoard;
    QListWidget *fixBoard;
    QDockWidget* dwid_fixBoard;
	DocuDock* docuDock;
	AnnouncementDialog* announcer;
//     QTreeView* jobQueueView;
// 	QListWidget* jobQueueStatusText;
// 	QProgressBar* jobQueueStatusPorgress;
    QMap<SharedMorphModel, domNodeViewer* > modelViewer;
	QMap<SharedMorphModel, AboutModel* > modelAbout;
    QDockWidget* jobQueueDock;
	JobQueueView* job_queue_view;
//     JobViewModel* job_view_model;
    JobView* job_controller;
    QModelIndex current_job_index;
    QList<int> interactive_jobs;
	QMap<QAction*,QString> example_files;
	QString fixBoard_copy_node;

    XMLTextDialog *tabXMLPreview; /*!< Widget that is used to show the xml-structure of the current state of model. */
    // jobController *myJobController; /*!< Widget that is used to start, stop and remove simulation-jobs. */
    parameterSweeper *sweeper; /*!< Widget that is used to do parametersweeps. */

    void initConfig();
    /*!<
      Creates and initiates the config.
      Connects all interfaces to use the config during runtime as a global object.
      */

    config::modelIndex modelListIndex(QTreeWidgetItem* item);

    void loadXMLFile(); /*!< Opens an open-file-dialog to load another cpm-model. */

    void startSimulation(); /*!< Executes the cpm-simulator with the opened model as parameter. (Starts simulation) */
    void stopSimulation(); /*!< Stops the last started cpm-simulation. */

    void createMenuBar(); /*!< Creates the menubar, which is used to load, reload and save a model, to open recent files, ... */
    void createMainWidgets(); /*!< Creates the tabwidget which is used to hold all other widgets like xmlpreview, jobcontroller, ... as sub-tabs. */

    QAction *recentFileActs[100]; /*!< Actions to load the recent opened files. */

    void storeSettings(); /*!< Saves the window-geometry to QSettings. */

protected:
	void dragEnterEvent(QDragEnterEvent *event);
	void dropEvent(QDropEvent *event);
	
public slots:
    void selectModel(int index, int part = -1);
	void selectAttribute(AbstractAttribute* attr);
	void selectXMLPath(QString path, int model_id);
    void addModel(int index);
    void removeModel(int index);
	void handleMessage(const QString& message);

private slots:
    void copyNodeAction(QDomNode);

    void setPermanentStatus(QString message); /*!< Sets the given message as permanent status. */
    void jobStatusChanged(int);
//     void jobAdded(const QModelIndex&, int);

    void modelListChanged(QTreeWidgetItem*);
	void activatePart(QModelIndex);
	void showCurrentModel();
    void showModelListMenu(QPoint p); /*!< Opens the menu for copying or deleting the existing tab at position 'p'. */
	void modelActionTriggerd (QAction*); /*!< Calls the right function to transform the given Action from tabMenu. */

// 	void jobQueueChanged(const QModelIndex & index);
//     void showJobQueueMenu(QPoint p);
// 	void jobQueueMenuTriggered(QAction*);
	void showJob(int job_id);
	void showSweep(QList<int> job_ids);
    
    
	void syncModelList(int model=-1);

    void fixBoardClicked(QModelIndex item);
	void fixBoardCopyNode();

    void openRecentFile(); /*!< Opens the recently loaded file. */
    void updateRecentFileActions(); /*!< Updatest the list of recently opened files. */

signals:
	int startJob(SharedMorphModel model);
	void stopJob(int job_id);

    void emitXMLNodeCopy(QDomNode node); /*!< Signal is send when a xml-node is copied. */
};

#endif // MAINWINDOW_H



