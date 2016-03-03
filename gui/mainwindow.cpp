#include "mainwindow.h"

using namespace std;


//konstruktor der beim anlegen des hauptfenster aufgerufen wird
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)//, ui(new Ui::MainWindow)
{

// THIS CODE BLOCK NEEDS TO BE UNCOMMENTED ONLY WHEN BUILDING MORPHEUS A MAC BUNDLE
/*
#if defined(Q_OS_MAC)
 QDir dir (QApplication::applicationDirPath());
 dir.cdUp();
 dir.cdUp();
 dir.cd("plugins");
 QStringList libpaths_before = QApplication::libraryPaths();
 QApplication::setLibraryPaths( QStringList( dir.absolutePath() ) );

 QStringList libpaths = QApplication::libraryPaths();
 qDebug() << "(Mac only) Library Path (incl. plugins): " << libpaths;
#endif
*/
	QStringList libpaths = QApplication::libraryPaths();
	qDebug() << "Using library Path (should include Qt plugins dir): " << libpaths;

	QCoreApplication::setOrganizationName("Morpheus");
    QCoreApplication::setOrganizationDomain("morpheus.org");
    QCoreApplication::setApplicationName("Morpheus");
    QApplication::setWindowIcon(QIcon(":/morpheus.png") );
    QWidget::setWindowTitle(tr("Morpheus"));
    QWidget::setAcceptDrops(true);


    createMenuBar();
    createMainWidgets();
    initConfig();

    this->setStatusBar(new QStatusBar());
    permanentStatus = new QPushButton("Welcome to Morpheus");

    permanentStatus->setFlat(true);
    permanentStatus->setMaximumWidth(400);
    permanentStatus->setMinimumWidth(320);

    statusBar()->addPermanentWidget(permanentStatus);
    connect(config::getJobQueue(), SIGNAL(processChanged(int)), this, SLOT(jobStatusChanged(int)));
    connect(permanentStatus,SIGNAL(clicked()),this,SLOT(statusBarTriggered()));

    QStringList args = QApplication::arguments();
    model_index.model = -1;
    model_index.part = -1;

	for (int i=1; i<args.size(); i++) {
		if(args.at(i) == "--clear")
		{
			QSettings settings;
			settings.remove("jobs");
		}
		else
		{
			if (QFile::exists( args.at(i) )  ) {
				config::openModel( args.at(i));
			}
		}
	}
		

    if (config::getOpenModels().isEmpty()) {
		config::createModel();
	}

    connect(sweeper, SIGNAL(createSweep(SharedMorphModel, int)), config::getJobQueue(), SLOT(addSweep(SharedMorphModel,int)), Qt::QueuedConnection );
	connect(sweeper, SIGNAL(attributeDoubleClicked(AbstractAttribute*)), this, SLOT(selectAttribute(AbstractAttribute*)));
}

void MainWindow::handleMessage(const QString& message){
	QStringList arguments = message.split(' ');
	QString path=arguments.at(0);	
	for(uint i=1; i<arguments.size(); i++){
		if(!arguments.at(i).startsWith("--")){
			QString filename = path+"/"+arguments.at(i);
			if (QFile::exists( filename )  ) {
				config::openModel( filename );
			}
		}
	}
	

}

//------------------------------------------------------------------------------

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
	qDebug() << "Drag event, MIME TYPE = " << event->mimeData()->formats() << endl;
	
	if( event->mimeData()->hasFormat("text/plain") ||
		event->mimeData()->hasFormat("text/uri-list") ){
	
		foreach (QUrl url, event->mimeData()->urls()) {
			QString fileName = url.toLocalFile();
			QFileInfo file(fileName);
			qDebug() << "Filename = " << fileName << endl;
			if(file.exists()){
				// TODO: should actually check the MIME type, but not sure how
				QStringList tokens = fileName.split(".");
				qDebug() << "Tokens = " << tokens << endl;
				if( tokens[ tokens.size()-1 ] == "xml" ){
					qDebug() << "ACCEPTED!! = " << fileName << endl;
					event->acceptProposedAction();
					return;
				}
			}
		}
	}
}

void MainWindow::dropEvent(QDropEvent *event)
{
	foreach (QUrl url, event->mimeData()->urls()) {
		QString fileName = url.toLocalFile();
		QFile file(fileName);
		if(file.exists()){
			config::openModel( fileName );
		}
	}
	event->acceptProposedAction();
}

//------------------------------------------------------------------------------

void MainWindow::initConfig() {
    config* conf = config::getInstance();
    conf->setComputeResource(cb_resource->currentText());

    connect(cb_resource, SIGNAL(currentIndexChanged(QString)), config::getInstance(), SLOT(setComputeResource(QString)));
    connect(this, SIGNAL(startJob(SharedMorphModel)), config::getJobQueue(), SLOT(addProcess(SharedMorphModel)));
	connect(this, SIGNAL(stopJob(int)),               config::getJobQueue(), SLOT(stopProcess(int)));
	connect(job_controller, SIGNAL(stopJob(int)),      config::getJobQueue(), SLOT(stopProcess(int)));

    connect(conf, SIGNAL(modelSelectionChanged(int)), this,    SLOT(selectModel(int)));
    connect(conf, SIGNAL(modelAdded(int)),            this,    SLOT(addModel(int)));
    connect(conf, SIGNAL(modelClosing(int)),          this,    SLOT(removeModel(int)));
    connect(conf, SIGNAL(newRecentFile()),            this,    SLOT(updateRecentFileActions()));
    connect(conf, SIGNAL(modelSelectionChanged(int)), sweeper, SLOT(selectModel(int)));

}

//------------------------------------------------------------------------------

void MainWindow::createMenuBar()
{
    menubar = new QMenuBar();
    QMenu *fileMenu = menubar->addMenu(tr("&File"));

    QAction *fileNew = new QAction(QThemedIcon("document-new", style()->standardIcon(QStyle::SP_FileDialogNewFolder)), tr("&New"), this);
    fileNew->setShortcut(QKeySequence::New);
    fileNew->setStatusTip(tr("Create a new model-file"));
    fileMenu->addAction(fileNew);

    fileMenu->addSeparator();

    QAction *fileOpen = new QAction( QThemedIcon("document-open", style()->standardIcon(QStyle::SP_DialogOpenButton)), tr("&Open..."), this);
    fileOpen->setShortcut(QKeySequence::Open);
    fileOpen->setStatusTip(tr("Open existing model from file"));
    fileMenu->addAction(fileOpen);

    QAction *fileReload= new QAction(QThemedIcon("document-revert",QIcon(":/document-revert.png")),tr("&Reload"), this);
    fileReload->setShortcut(QKeySequence(Qt::Key_F5));
    fileReload->setStatusTip(tr("Reload model from last file"));
    fileMenu->addAction(fileReload);

	if (SBMLImporter::supported) {
		QAction *importSBML= new QAction(QThemedIcon("document-import",QIcon(":/document-import.png")),tr("&Import SBML"), this);
		importSBML->setStatusTip(tr("Import an SBML model into a new Celltype of the current model"));
		fileMenu->addAction(importSBML);
	}

    fileMenu->addSeparator();

    QAction *fileSaveAs = new QAction(QThemedIcon("document-save-as",style()->standardIcon(QStyle::SP_DialogSaveButton) ), tr("&Save As..."), this);
    fileSaveAs->setShortcut(QKeySequence::SaveAs);
    fileSaveAs->setStatusTip(tr("Save model to file"));
    fileMenu->addAction(fileSaveAs);

    QAction *fileSave = new QAction(QThemedIcon("document-save",style()->standardIcon(QStyle::SP_DialogSaveButton) ), tr("&Save"), this);
    fileSave->setShortcut(QKeySequence::Save);
    fileSave->setStatusTip(tr("Save model to file"));
    fileMenu->addAction(fileSave);

    fileMenu->addSeparator();

    for(int i =0; i < 100/*config::getApplication().preference_max_recent_files*/; i++){
        recentFileActs[i] = new QAction(this);
        connect(recentFileActs[i], SIGNAL(triggered()), this, SLOT(openRecentFile()));
        fileMenu->addAction(recentFileActs[i]);
		recentFileActs[i]->setToolTip(recentFileActs[i]->data().toString());
        recentFileActs[i]->setVisible(false);
    }
    updateRecentFileActions();

    fileMenu->addSeparator();

    QAction *act_settings = fileMenu->addAction(QThemedIcon("document-properties", QIcon(":/settings.png")), tr("&Settings"));
#if QT_VERSION < 0x040600
    act_settings->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_K);
#else
    act_settings->setShortcut(QKeySequence::Preferences);
#endif
    act_settings->setStatusTip(tr("Open settings dialog"));

    fileMenu->addSeparator();
    
    QAction *fileClose = fileMenu->addAction(QThemedIcon("document-close", QIcon(":/document-close.png")), tr("&Close"));
    fileClose->setShortcut(QKeySequence::Close);
    fileClose->setStatusTip(tr("Close current model"));

    QAction *appQuit = fileMenu->addAction(QThemedIcon("application-exit", QIcon(":/application-exit.png")), tr("&Quit"));
#if QT_VERSION < 0x040600
    appQuit->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q);
#else
    appQuit->setShortcut(QKeySequence::Quit);
#endif
    appQuit->setStatusTip(tr("Quit Morpheus"));

    QMenu *examplesMenu = menubar->addMenu(tr("&Examples"));

// 	QMenu* examplesMenu = fileMenu->addMenu(QThemedIcon("applications-science",QIcon(":/applications-science.png")),tr("&Examples"));
	QDir ex_dir(":/examples");
	QStringList ex_categories_sl = ex_dir.entryList();
	QMap<int, QString> ex_categories;
	foreach( QString ex_cat, ex_categories_sl ) {
		if( ex_cat == "ODE" )
			ex_categories.insert(1,ex_cat);
		if( ex_cat == "PDE" )
			ex_categories.insert(2,ex_cat);
		if( ex_cat == "CPM" )
			ex_categories.insert(3,ex_cat);
		if( ex_cat == "Multiscale" )
			ex_categories.insert(4,ex_cat);
		if( ex_cat == "Miscellaneous" )
			ex_categories.insert(5,ex_cat);
        if( ex_cat == "Challenge" )
            ex_categories.insert(6,ex_cat);
    }
	
	//foreach( QString ex_cat, ex_categories ) {
	for( QMap<int,QString>::iterator it = ex_categories.begin(); it != ex_categories.end(); it++){
		QString ex_cat = it.value();
		QDir ex_cat_dir(ex_dir); ex_cat_dir.cd(ex_cat);
		QMenu * ex_cat_menu = examplesMenu->addMenu(ex_cat);
		QStringList examples = ex_cat_dir.entryList();
		foreach( QString example, examples ) {
			if (example.endsWith(".xml")) {
				QAction *openEx = ex_cat_menu->addAction(example);
				QString ex_path = QString(":/examples/")+ex_cat_dir.dirName() + "/" + example;
	// 			qDebug() << "added example " << ex_path;
				example_files.insert(openEx,ex_path);
			}
		}
	}
	QAction* exInfoAction = examplesMenu->addAction(tr("&Examples website"));
	exInfoAction->setStatusTip(tr("Open Morpheus examples website for documentation."));
	connect(exInfoAction,SIGNAL(triggered()), config::getInstance(),SLOT(openExamplesWebsite()));
	
    examplesMenu->setStatusTip(tr("Open Morpheus example model"));

    QMenu *aboutMenu = menubar->addMenu(tr("&About"));
	QAction* aboutModel = new QAction(tr("&Model"),menubar);
	aboutModel->setStatusTip(tr("Show information about current model."));
	connect(aboutModel,SIGNAL(triggered()), config::getInstance(),SLOT(aboutModel()));
    aboutMenu->addAction(aboutModel);
	
	aboutMenu->addSeparator();
	
	QAction* aboutMorpheus = new QAction(tr("&Morpheus"),menubar);
	aboutMorpheus->setStatusTip(tr("Show information about Morpheus."));
	connect(aboutMorpheus,SIGNAL(triggered()), config::getInstance(),SLOT(aboutPlatform()));
    aboutMenu->addAction(aboutMorpheus);

	QAction* aboutMorheusWebsite = aboutMenu->addAction(tr("&Morpheus website"));
	aboutMorheusWebsite ->setStatusTip(tr("Open Morpheus website."));
	connect(aboutMorheusWebsite,SIGNAL(triggered()), config::getInstance(),SLOT(openMorpheusWebsite()));
	
	aboutMenu->addSeparator();
	
	QAction* aboutQt = new QAction(tr("&Qt"),menubar);
	aboutQt->setStatusTip(tr("Show information about Qt."));
	connect(aboutQt,SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    aboutMenu->addAction(aboutQt);
	
	QAction* aboutHelp = new QAction(tr("&Help"),menubar);
	aboutQt->setStatusTip(tr("Show Morpheus help."));
	connect(aboutHelp,SIGNAL(triggered()), config::getInstance(), SLOT(aboutHelp()));
    aboutMenu->addAction(aboutHelp);
	

    QToolBar *toolbar = new QToolBar("Main Toolbar",this);
    toolbar->setObjectName("Main Toolbar");

    toolbar->addAction(fileOpen);
    QToolButton* tbutton = (QToolButton*) toolbar->widgetForAction(fileOpen);
    tbutton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    toolbar->addAction(fileSave);
    tbutton = (QToolButton*) toolbar->widgetForAction(fileSave);
    tbutton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    toolbar->addSeparator();

    cb_resource = new QComboBox(this);
    cb_resource->addItems(config::getJobQueue()->queueNames());
	cb_resource->setCurrentIndex(0); // Local by default
	cb_resource->setFocusPolicy(Qt::ClickFocus);

    toolbar->addWidget(cb_resource);

    QAction *simStart = new QAction(QThemedIcon("media-playback-start" ,style()->standardIcon(QStyle::SP_MediaPlay)), tr("&Start"), toolbar);
    simStart->setShortcut(QKeySequence(Qt::Key_F8));
    simStart->setStatusTip(tr("Start morpheus simulation with current model"));
    toolbar->addAction(simStart);
    tbutton = (QToolButton*) toolbar->widgetForAction(simStart);
    tbutton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    QAction *simStop = new QAction(QThemedIcon("media-playback-stop", style()->standardIcon(QStyle::SP_MediaStop) ), tr("&Stop"), toolbar);
    simStop->setShortcut(QKeySequence(Qt::Key_F9));
    simStop->setStatusTip(tr("Terminate current morpheus simulation"));
    toolbar->addAction(simStop);
    interactive_stop_button = (QToolButton*) toolbar->widgetForAction(simStop);
    interactive_stop_button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    interactive_stop_button->setEnabled(false);

    // toolbar->addSeparator();
    // QAction *simGraph = new QAction(QThemedIcon("distribute-graph-directed", QIcon(":/graph.svg")), tr("&Graph"), toolbar);
    // simGraph->setShortcut(QKeySequence(Qt::Key_F10));
    // simGraph->setStatusTip(tr("Generate symbol dependency graph for current model"));
    // simGraph->setToolTip(tr("Generate symbol dependency graph for current model"));
    // toolbar->addAction(simGraph);
    // graph_button = (QToolButton*) toolbar->widgetForAction(simGraph);
    // graph_button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    setMenuBar(menubar);
    addToolBar(toolbar);

	toolbar->setFocusPolicy(Qt::NoFocus);
    connect(menubar, SIGNAL(triggered(QAction*)), this, SLOT(menuBarTriggered(QAction*)));
    connect(toolbar, SIGNAL(actionTriggered(QAction*)), this, SLOT(toolBarTriggered(QAction*)));
}

//------------------------------------------------------------------------------

void MainWindow::createMainWidgets()
{
    setTabPosition(Qt::AllDockWidgetAreas,QTabWidget::North);

    modelList = new QTreeWidget();
    modelList->setColumnCount(1);
    modelList->setHeaderLabels(QStringList() << "");
    modelList->setHeaderHidden(true);
    modelList->setRootIsDecorated(false);
    modelList->setAlternatingRowColors(true);
    modelList->setIndentation(40);
    modelList->setContextMenuPolicy(Qt::CustomContextMenu);
    modelList->setAutoExpandDelay(200);

    connect(modelList, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showModelListMenu(QPoint)));
    connect(modelList, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(modelListChanged(QTreeWidgetItem*)));
	connect(modelList, SIGNAL(clicked(QModelIndex)), this, SLOT(showCurrentModel()) );

    modelMenu = new QMenu();
    showModelXMLAction = modelMenu->addAction(QThemedIcon("text-xml",QIcon(":/text-xml.png")),"Show XML");
    modelMenu->addSeparator();
    addModelPartMenu = modelMenu->addMenu(QThemedIcon("list-add",QIcon(":/list-add.png")),"Add");
    removeModelPartAction = modelMenu->addAction(QThemedIcon("list-remove",QIcon(":/list-remove.png")),"Remove");
    copyModelPartAction = modelMenu->addAction(QThemedIcon("edit-copy",QIcon(":/edit-copy.png")),"Copy");
    pasteModelPartAction = modelMenu->addAction(QThemedIcon("edit-paste",QIcon(":/edit-paste.png")),"Paste");
    modelMenu->addSeparator();
    closeModelAction =  modelMenu->addAction(QThemedIcon("dialog-close",style()->standardIcon(QStyle::SP_DialogCloseButton)),"Close");
    //modelMenu->addSeparator();
    //mailAttachAction =  modelMenu->addAction(QThemedIcon("mail-send",QIcon(":/mail-attach.png")),"Send by mail");
    connect(modelMenu, SIGNAL(triggered(QAction*)), this, SLOT( modelActionTriggerd (QAction*)));
 
    // Navigation & other Dock Widgets

    documentsDock = new QDockWidget("Documents",this);
    documentsDock->setWidget(modelList);
    documentsDock->setObjectName("DocumentsDock");
    addDockWidget(Qt::LeftDockWidgetArea,documentsDock,Qt::Vertical);
	documentsDock->setFocusProxy(modelList);
	documentsDock->setAcceptDrops(true);
	
	
//     clipBoard = new QListWidget();
//     clipBoard -> setDragEnabled(true);
// 	clipBoard -> setFocusPolicy( Qt::ClickFocus );
//     QDockWidget* dw1 = new QDockWidget("ClipBoard",this);
//     dw1->setObjectName("ClipBoardDock");
//     dw1->setWidget(clipBoard);
//     addDockWidget(Qt::RightDockWidgetArea,dw1,Qt::Vertical);
// 	dw1->setFocusProxy(clipBoard);


    fixBoard = new QListWidget();
    fixBoard->setAlternatingRowColors(true);
	fixBoard->setContextMenuPolicy( Qt::ActionsContextMenu );
	QAction* fixBoardCopyAction = new QAction(QThemedIcon("edit-copy",QIcon(":/edit-copy.png")),"Copy",fixBoard);
	connect(fixBoardCopyAction, SIGNAL(triggered(bool)), this, SLOT(fixBoardCopyNode()));
	fixBoard->addAction(fixBoardCopyAction);
    connect(fixBoard,SIGNAL(doubleClicked(QModelIndex)),this, SLOT(fixBoardClicked(QModelIndex)));
// 	connect(fixBoard,SIGNAL(customContextMenuRequested(QPoint)),this, SLOT(fixBoardContextMenu(QPoint)));
    dwid_fixBoard = new QDockWidget("FixBoard",this);
    dwid_fixBoard->setWidget(fixBoard);
    dwid_fixBoard->setObjectName("FixBoardDock");
    addDockWidget(Qt::LeftDockWidgetArea,dwid_fixBoard,Qt::Vertical);
	dwid_fixBoard->setFocusProxy(fixBoard);

	jobQueueDock = new QDockWidget("JobQueue",this);
	job_queue_view = new JobQueueView(this);
	connect(job_queue_view,SIGNAL(jobSelected(int)),this,SLOT(showJob(int)));
	connect(job_queue_view,SIGNAL(sweepSelected(QList<int>)),this,SLOT(showSweep(QList<int>)));
	connect(job_queue_view, SIGNAL(erronousXMLPath(QString)), this, SLOT(selectXMLPath(QString)));
	
    jobQueueDock->setWidget(job_queue_view);
    jobQueueDock->setObjectName("JobQueueDock");
    addDockWidget(Qt::LeftDockWidgetArea,jobQueueDock,Qt::Vertical);
	jobQueueDock->setFocusProxy(job_queue_view);
	
	docuDock = new DocuDock(this);
	docuDock->setObjectName("DocuDock");
	addDockWidget(Qt::BottomDockWidgetArea,docuDock,Qt::Horizontal);
	
    // The Core Editor Stack
    editorStack = new QStackedWidget();
    editorStack->setFrameStyle(QFrame::StyledPanel | QFrame::Panel | QFrame::Sunken);
	
    sweeper = new parameterSweeper();
    editorStack->addWidget(sweeper);

    job_controller = new JobView();
    editorStack->addWidget(job_controller);
	editorStack->setAcceptDrops(false);
	connect(job_controller,SIGNAL(selectSweep (int)),sweeper, SLOT(loadSweep(int)));
// 	QWidget::setTabOrder(modelList,editorStack);
	QWidget::setTabOrder(job_queue_view,job_controller);
	
    setCentralWidget(editorStack);
    editorStack->show();
}

//------------------------------------------------------------------------------

void MainWindow::selectModel(int index, int part)
{
    config::modelIndex selected = modelListIndex(modelList->currentItem());

    if (model_index.model != index) {
        model_index.model = index;
        current_model = config::getOpenModels()[model_index.model];
        if (selected.model == model_index.model)
            part = selected.part;

        model_index.part = part >= 0 ? part : 0;

        setWindowTitle(tr("Morpheus - %1").arg(  current_model->xml_file.name ) );
        //setWindowIcon( QIcon(":/logo.png") );
		modelViewer[current_model]->updateConfig();
//         editorStack->setCurrentWidget(modelViewer[current_model]);
// 		QWidget::setTabOrder(modelList,modelViewer[current_model]);
        modelList->topLevelItem(index)->setExpanded(true);

        modelList->setCurrentItem(modelList->topLevelItem(index)->child(model_index.part));
        fixBoard->clear();
        QList<MorphModelEdit> fixes = current_model->rootNodeContr->getModelDescr().auto_fixes;
        for (int i=0; i<fixes.size(); i++) {
            QString info = fixes[i].info;
            // introduce line breaks
            int space = 0;
            int j=40;
            while (info.size()>j)
            {
                int next_space = info.indexOf(" ",space+1);
                while (next_space>0 && next_space<=j ) {
                    space = next_space;
                    next_space = info.indexOf(" ",space+1);
                }
                info.replace(space,1,"\n");
                j = max(j,space) + 40;

            }
            fixBoard->addItem(info);
            if ( ! fixes[i].value.isEmpty()) {
				QListWidgetItem* item = fixBoard->item(fixBoard->count()-1);
				item->setToolTip(fixes[i].value);
			}
        }
        modelList->setCurrentItem(modelList->topLevelItem(index)->child(model_index.part));
		
// 		showCurrentModel();
    }
    else {
        model_index.part = part >=0 ? part : selected.part>=0 ? selected.part : 0;
		showCurrentModel();
    }

	
    //qDebug() << current_model->xml_file.name << " " << current_model->parts[model_index.part].label;

	

}

//------------------------------------------------------------------------------

void MainWindow::menuBarTriggered(QAction* act)
{
    if(act->text() == "&New")
    {
        int index = config::createModel();
// 		if (index)
// 			selectModel(index);
        return;
    }
    if(act->text() == "&Open...")
    {
        loadXMLFile();
        return;
    }
    if (act->text() == "&Close") {
        config::closeModel(model_index.model);
		return;
    }
    if(act->text() == "&Reload")
    {
        if ( current_model &&  ! current_model->xml_file.path.isEmpty() ) {
            QString path = current_model->xml_file.path;
            if (config::closeModel(model_index.model,false)) {
                config::openModel( path );
            }
        }
        return;
    }
    if(act->text() == "&Save As...")
    {
		if ( current_model->xml_file.saveAsDialog() ) {
			config::addRecentFile(current_model->xml_file.path);
			current_model->rootNodeContr->saved();
			modelList->topLevelItem(model_index.model)->setText(0, current_model->xml_file.name);
			qDebug() << "Save As: " << current_model->xml_file.name << endl;
			this->setWindowTitle(tr("Morpheus - %1").arg(  current_model->xml_file.name ) );
		}
		else {
			//QMessageBox::critical(this,"Error", "Failed to save model.");
		}
        return;
    }
    if(act->text() == "&Save")
    {
		if (current_model) {
			if (current_model &&  current_model->xml_file.path.isEmpty()) {
				if ( current_model->xml_file.saveAsDialog() ) {
					config::addRecentFile(current_model->xml_file.path);
					current_model->rootNodeContr->saved();
					modelList->topLevelItem(model_index.model)->setText(0, current_model->xml_file.name);
				}
				else {
					QMessageBox::critical(this,"Error", "Failed to save the model.");
				}
			}
			else {
				if ( current_model->xml_file.save(current_model->xml_file.path) ) {
					current_model->rootNodeContr->saved();
					modelList->topLevelItem(model_index.model)->setText(0, current_model->xml_file.name);
				}
				else {
					QMessageBox::critical(this,"Error", "Failed to save model to " + current_model->xml_file.path );
				}
			}
		}
        return;
    }
    if (act->text() == "&Import SBML") {
		if (current_model) {
			QSharedPointer<MorphModel> sbml_import = SBMLImporter::importSBML();
			if (sbml_import)
				config::importModel(sbml_import);
		}
	}
    if(act->text() == "&Quit")
    {
        config::getDatabase().close();
        this->close();
        return;
    }
    if(act->text() == "&Settings")
    {
            settingsDialog settingsDia;
            settingsDia.exec();
    }
    if (example_files.contains(act)) {
		config::openModel(example_files[act]);
	}
}

//------------------------------------------------------------------------------

void MainWindow::toolBarTriggered(QAction* act)
{
    if(act->text() == "&Start")
    {
        startSimulation();
        return;
    }
    if(act->text() == "&Stop")
    {
        stopSimulation();
        return;
    }
}

//------------------------------------------------------------------------------

void MainWindow::setPermanentStatus(QString message) {
    statusMsgSource = sender();
    permanentStatus->setText(message);
}

//------------------------------------------------------------------------------

void MainWindow::statusBarTriggered() {
//     if (statusMsgSource == jobQueueView) {
// //        myJobController->selectMsgSource();
// //        tabW_Main->setCurrentWidget(myJobController);
//     }
//     else if (statusMsgSource == editorStack) {
//
//     }
//     else
//         cout << "status msg from unknown source " << endl;
}

//------------------------------------------------------------------------------

void MainWindow::openRecentFile(){

    QAction *action = qobject_cast<QAction*>(sender());
    QString filename = action->data().toString();
    QFile f( filename );
    if( !f.exists() )
    {
      QMessageBox::warning( this, QString("Error"), QString("The file \"" +filename+ "\" does not exist (anymore)." ), QMessageBox::Ok );
      return;
    }

    cout << "openRecentFile: " << qPrintable(filename) << endl;
    if(action){
        config::openModel( filename );
    }
}

//------------------------------------------------------------------------------

void MainWindow::loadXMLFile()
{
    QString directory = ".";
    if ( QSettings().contains("FileDialog/path") ) {
        directory = QSettings().value("FileDialog/path").toString();
    }

    QString fileName = QFileDialog::getOpenFileName(this, tr("Open model configuration"), directory, tr("Configuration Files (*.xml)"));
    if(fileName != "")
    {
        QString path = QFileInfo(fileName).dir().path();
        QSettings().setValue("FileDialog/path", path);
        QString xmlFile = fileName;
        config::openModel(xmlFile);
    }
    else
    {
        cout << "Loading has been cancelled" << endl;
    }
}

//------------------------------------------------------------------------------

void MainWindow::startSimulation()
{
//	documentsDock->setFocus();
 	jobQueueDock->setFocus();

	if (current_model) {
		if (editorStack->currentWidget()==sweeper){
			job_queue_view->setGroupBy( JobViewModel::SWEEP );
			jobQueueDock->raise();
			sweeper->submitSweep();
		}
		else{
			if( job_queue_view->getGroupBy() == JobViewModel::SWEEP )
				job_queue_view->setGroupBy( JobViewModel::MODEL );
			jobQueueDock->raise();
			emit startJob(current_model);
		}
	}
}

//------------------------------------------------------------------------------

void MainWindow::stopSimulation()
{
    if (  ! interactive_jobs.empty() )
    {
        emit stopJob(interactive_jobs.first());
    }
}

//------------------------------------------------------------------------------

config::modelIndex MainWindow::modelListIndex(QTreeWidgetItem* item) {
    config::modelIndex index;
    if (item) {
        if (modelList->indexOfTopLevelItem(item)==-1) {
            index.model = modelList->indexOfTopLevelItem(item->parent());
            index.part = item->parent()->indexOfChild(item);
        }
        else {
            index.model = modelList->indexOfTopLevelItem(item);
            index.part = -1;
        }
    }
    else {
        index.model = -1;
        index.part = -1;
    }
    return index;
}

//------------------------------------------------------------------------------

void MainWindow::showModelListMenu(QPoint p)
{
    QTreeWidgetItem* item = modelList->itemAt(p);
    if (item) {
        model_popup_index = modelListIndex(item);
        SharedMorphModel m = config::getOpenModels()[model_popup_index.model];

        QList<QString> addableChilds = m->rootNodeContr->getAddableChilds();
		if (addableChilds.empty()) 
			addModelPartMenu->setDisabled(true);
		else
			addModelPartMenu->setDisabled(false);
        addModelPartMenu->clear();
        for ( int i=0; i< addableChilds.size(); i++ ) {
            addModelPartMenu->addAction(addableChilds[i]);
        }
        if ( ! config::getNodeCopies().empty())
            pasteModelPartAction->setEnabled(addableChilds.contains(config::getNodeCopies().front().nodeName()));
        else
            pasteModelPartAction->setEnabled(false);
        if (model_popup_index.part < 0 || m->parts[model_popup_index.part].label == "ParamSweep"){
            copyModelPartAction->setVisible(false);
            removeModelPartAction->setVisible(false);
        }
        else {
            copyModelPartAction->setVisible(true);
            removeModelPartAction->setVisible(true);
            // It's a model part
            copyModelPartAction->setEnabled(true); // Copy
            removeModelPartAction->setEnabled( m->parts[model_popup_index.part].element->isDeletable() );
        }
        
        modelMenu->exec(modelList->mapToGlobal(p));
    }
}

//------------------------------------------------------------------------------

void MainWindow::modelActionTriggerd (QAction *act)
{
    SharedMorphModel popup_model = config::getOpenModels()[model_popup_index.model];
//    qDebug() << "MainWindow::modelListMenuTriggerd: " << act->text();
    if (act == copyModelPartAction) {
        copyNodeAction(popup_model->parts[model_popup_index.part].element->cloneXML());
    }

    else if (act == closeModelAction) {
        config::closeModel(model_popup_index.model);
    }

/*    else if (act == mailAttachAction) {
	    
	    // Save model to temporary directory
	    QString tempfilepath = QDesktopServices::storageLocation( QDesktopServices::TempLocation )+"/"+QString( current_model->xml_file.name )+".xml";
	    current_model->xml_file.save( tempfilepath );
	    qDebug() << "Saved model to temporary file: " << tempfilepath << endl;
	    
	    // open email client
	    QDesktopServices::openUrl(QUrl("mailto:walter@deback.net?subject=Morpheus model&body=See attachment.&attachment=\""+tempfilepath+"\""));
	    
    }
*/
    else if (act == showModelXMLAction) {
        XMLTextDialog* dia = new XMLTextDialog( popup_model->xml_file.domDocToText(), this);
        dia->exec();
        delete dia;
    }

    else if (act == removeModelPartAction) {
        QMessageBox msgBox;
        msgBox.setInformativeText(QString("Do you want to delete %1 ?").arg(popup_model->parts[model_popup_index.part].label));
        msgBox.setStandardButtons( QMessageBox::Ok | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Cancel);
        msgBox.move(this->cursor().pos());
        int i = msgBox.exec();

        if(i == QMessageBox::Ok)
        {
            popup_model->removePart(model_popup_index.part);
        }
    }

    else if (act == pasteModelPartAction)
    {
        popup_model->addPart(config::getNodeCopies().first().cloneNode(true));
    }
    else
    {
        popup_model->addPart(act->text());
    }
}

void MainWindow::reloadModelParts(int m) {
    if (m==-1) {
        const QList<SharedMorphModel >&  models = config::getOpenModels();
        foreach ( SharedMorphModel model, models) {
            if (sender() == model.data()) {
                m = models.indexOf(model);
            }
        }
    }
    qDebug() << "Reload model parts for idx " << m;
    modelList->topLevelItem(m)->takeChildren();
    SharedMorphModel model = config::getOpenModels()[m];

    // sort ModelParts (Elements) according to label (see compare_labels() in morpheus_model.h)
    std::sort( model->parts.begin(), model->parts.end(), MorphModelPart::compare_labels );

    for (int part=0; part < model->parts.size(); part++) {
        QTreeWidgetItem* part_item = new QTreeWidgetItem(QStringList(model->parts[part].label));
        modelList->topLevelItem(m)->insertChild(part, part_item);
    }
    if (model_index.model == m) {
        if (model_index.part>=model->parts.size())
            model_index.part = model->parts.size()-1;
        selectModel(m, model_index.part);
    }
}

//------------------------------------------------------------------------------

void MainWindow::updateRecentFileActions() {
    QSettings settings;
    QStringList files = settings.value("recentFileList").toStringList();
    files.removeAll("");

    int numRecentFiles = qMin(files.size(), (int)config::getApplication().preference_max_recent_files);

//    // if actual files list smaller than max recent files, add empty ones
//    if( files.size() < config::getApplication().preference_max_recent_files )
//        for(int i = numRecentFiles; i < 100/* config::getApplication().preference_max_recent_files*/; i++){
//            recentFileActs[i] = new QAction(this);
//            connect(recentFileActs[i], SIGNAL(triggered()), this, SLOT(openRecentFile()));
//            fileMenu->addAction(recentFileActs[i]);
//        }

    for(int i=0; i < numRecentFiles; i++){
        QString text = tr("&%1 %2").arg(i+1).arg(QFileInfo(files[i]).fileName());
        //qDebug() << "Recent files: " << i << ", text: "  << text;
        recentFileActs[i]->setText(text);
        recentFileActs[i]->setData(files[i]);
		recentFileActs[i]->setStatusTip(files[i]);
		recentFileActs[i]->setVisible(true);
    }

    // if actual file list larger than max recent files, set the remaining ones invisible
    if( files.size() > config::getApplication().preference_max_recent_files )
        for(int i=numRecentFiles; i < config::getApplication().preference_max_recent_files; i++){
			
            recentFileActs[i]->setVisible(false);
		}


}

//------------------------------------------------------------------------------

void MainWindow::closeEvent(QCloseEvent *ce)
{
    while ( ! config::getOpenModels().isEmpty() )
    if ( ! config::closeModel(config::getOpenModels().size()-1, false)) {
        ce->ignore();
        return;
    }

    ce->accept();
    storeSettings();
}

//------------------------------------------------------

void MainWindow::readSettings(){
    // restore window properties from qsettings
    QSettings settings;

    restoreGeometry(settings.value("MainWindow/geometry").toByteArray());
    restoreState(settings.value("MainWindow/windowState").toByteArray());
}

//------------------------------------------------------

void MainWindow::storeSettings(){
    // store window property to qsettings
    QSettings settings;

    settings.setValue("MainWindow/geometry", saveGeometry());
    settings.setValue("MainWindow/windowState", saveState());
}

//------------------------------------------------------

void MainWindow::addModel(int index) {
    SharedMorphModel model = config::getOpenModels()[index];

    QTreeWidgetItem* c = new QTreeWidgetItem(QStringList(model->xml_file.name));
    c->setIcon(0,QThemedIcon("text-x-generic",QIcon(":/text-generic.png")));
    c->setIcon(1,QThemedIcon("edit-delete",QIcon(":/edit-delete.png")));
    QFont f (c->font(0));
    f.setBold(true);
    c->setFont(0,f);
    modelList->insertTopLevelItem(index,c);

    connect(model.data(),SIGNAL(modelPartAdded()),this,SLOT(reloadModelParts()));
    connect(model.data(),SIGNAL(modelPartRemoved()),this,SLOT(reloadModelParts()));

    domNodeViewer *viewer = new domNodeViewer(this);
    viewer->setModel(model,0);
    connect(viewer,SIGNAL(xmlElementCopied(QDomNode)),this,SLOT(copyNodeAction(QDomNode)));
	connect(viewer,SIGNAL(nodeSelected(nodeController*)),docuDock,SLOT(setCurrentNode(nodeController*)));
	connect(viewer,SIGNAL(xmlElementSelected(QString)),docuDock,SLOT(setCurrentElement(QString)));
	
    editorStack->addWidget(viewer);
    modelViewer[model] = viewer;
	modelAbout[model] =  new AboutModel(model);
	editorStack->addWidget(modelAbout[model]);

    reloadModelParts(index);

    documentsDock->raise();

    setPermanentStatus("Model loaded successfully");
    if ( ! model->rootNodeContr->getModelDescr().auto_fixes.isEmpty()) {
        dwid_fixBoard->raise();
    }

    statusMsgSource = editorStack;
    updateRecentFileActions();
}

//------------------------------------------------------

void MainWindow::removeModel(int index) {
    SharedMorphModel model = config::getOpenModels()[index];
    disconnect(model.data(),SIGNAL(modelPartAdded()),this,SLOT(reloadModelParts()));
    disconnect(model.data(),SIGNAL(modelPartRemoved()),this,SLOT(reloadModelParts()));
    editorStack->removeWidget(modelViewer[model]);
    modelViewer.remove(model);

    modelList->takeTopLevelItem(index);
    if (model_index.model == index) {
        model_index.model = -1;
    }
}

void MainWindow::showCurrentModel() {
	if (current_model->parts[model_index.part].label=="ParamSweep") {
        editorStack->setCurrentWidget(sweeper);
		QWidget::setTabOrder(modelList,sweeper);
		docuDock->setCurrentElement( "ParameterSweep" );
	}
	else if (model_index.part==0) {
		editorStack->setCurrentWidget(modelAbout[current_model]);
		modelAbout[current_model]->update();
		docuDock->setCurrentNode( current_model->parts[model_index.part].element);
		QWidget::setTabOrder(modelList,modelAbout[current_model]);
	}
    else {
        modelViewer[current_model]->setModelPart(model_index.part);
        editorStack->setCurrentWidget(modelViewer[current_model]);
// 		docuDock->setCurrentNode( current_model->parts[model_index.part].element);
		QWidget::setTabOrder(modelList,modelViewer[current_model]);
    }
}
//------------------------------------------------------

void MainWindow::modelListChanged(QTreeWidgetItem * item) {

    config::modelIndex selection = modelListIndex(item);
    if (model_index.model != selection.model)
        config::switchModel(selection.model);
    else {
        selectModel(selection.model, selection.part);
	}
 }


//------------------------------------------------------

void MainWindow::copyNodeAction(QDomNode node) {

    config::getInstance()->receiveNodeCopy(node);
//     clipBoard->clear();
    QList<QDomNode> nodes = config::getNodeCopies();
    QStringList entries;
    for (int i=0; i<nodes.size(); i++) {
        QDomDocument doc("");
        doc.appendChild(nodes[i]);

        QStringList xml = doc.toString(4).split("\n",QString::SkipEmptyParts);

        if (xml.size()>5) {
            while (xml.size()>4)
                xml.removeAt(2);
            xml.insert(xml.begin()+2,"      ...");
        }
//         QListWidgetItem * item = new QListWidgetItem(xml.join("\n"), clipBoard);
//         item->setData(Qt::EditRole, doc.toString(4));
//         clipBoard->addItem(item);

    }
}

//------------------------------------------------------

void MainWindow::selectXMLPath(QString path)
{
	QStringList xml_path = path.split("/", QString::SkipEmptyParts);
	// The 2nd element is the name of the part to be shown;
	if (xml_path.size()<2) return; 
	QString part_name = xml_path[1];
	int part_id = -1;
	for (int p=0; p<current_model->parts.size(); p++ ){
		if (current_model->parts[p].label == part_name) {
			part_id=p;
			break;
		}
	}
	if (part_id>=0) {
		selectModel(model_index.model, part_id);
		modelViewer[current_model]->selectNode(path);
	}
}


//------------------------------------------------------

void MainWindow::selectAttribute(AbstractAttribute* attr)
{
	editorStack->setCurrentWidget(modelViewer[current_model]);
	modelViewer[current_model]->selectNode(attr->getParentNode());
	
}

//------------------------------------------------------


void MainWindow::fixBoardClicked(QModelIndex item) {
    int row = item.row();
    const MorphModelEdit& e = current_model->rootNodeContr->getModelDescr().auto_fixes[row];

	QDomNode node = e.xml_parent;
	if (e.edit_type == NodeAdd &&  ! e.xml_parent.firstChildElement(e.name).isNull()) {
		 node = node.firstChildElement(e.name);
	}
	editorStack->setCurrentWidget(modelViewer[current_model]);
	modelViewer[current_model]->selectNode(node);
}

//------------------------------------------------------

void MainWindow::fixBoardCopyNode() {
	if (fixBoard->currentIndex().isValid()) {
		int row = fixBoard->currentIndex().row();
		QString fixBoard_copy_node = current_model->rootNodeContr->getModelDescr().auto_fixes[row].value;
		QApplication::clipboard()->setText(fixBoard_copy_node);
	}
}

//------------------------------------------------------

void MainWindow::jobStatusChanged(int id) {

	interactive_jobs = config::getJobQueue()->getInteractiveJobs();
	interactive_stop_button->setEnabled( ! interactive_jobs.isEmpty() );
    // Add a status message
    // update the job view, if necessary ...
}

//------------------------------------------------------

void MainWindow::showJob(int job_id)
{
	job_controller->setJob(job_id);
	editorStack->setCurrentWidget(job_controller);
}

void MainWindow::showSweep(QList<int> job_ids)
{
	job_controller->setSweep(job_ids);
	editorStack->setCurrentWidget(job_controller);
}

//------------------------------------------------------

//destruktor des hauptfensters
MainWindow::~MainWindow()
{
    delete modelMenu;
    delete menubar;
    delete sweeper;
	config::getInstance()->deleteLater();
}


