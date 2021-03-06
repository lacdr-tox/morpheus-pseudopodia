#include "mainwindow.h"
#include "uri_handler.h"
#include <QtConcurrent/QtConcurrentRun>
#ifdef USE_QWebEngine
	#include "network_schemes.h"
	#include <QWebEngineProfile>
	#if QTCORE_VERSION >= 0x051200
		#include <QWebEngineUrlScheme>
	#endif
// 	#define QWebEngineDebugger
#endif

// using namespace std;


MainWindow::MainWindow(const QCommandLineParser& cmd_line) : QMainWindow(nullptr)//, ui(new Ui::MainWindow)
{
    QWidget::setWindowTitle(tr("Morpheus"));
    QWidget::setAcceptDrops(true);


    createMainWidgets();
    createMenuBar();
    initConfig();

    this->setStatusBar(new QStatusBar());
    permanentStatus = new QPushButton("Welcome to Morpheus");

    permanentStatus->setFlat(true);
    permanentStatus->setMaximumWidth(400);
    permanentStatus->setMinimumWidth(320);

    statusBar()->addPermanentWidget(permanentStatus);
    connect(config::getJobQueue(), SIGNAL(processChanged(int)), this, SLOT(jobStatusChanged(int)));
//     connect(permanentStatus,SIGNAL(clicked()),this,SLOT(statusBarTriggered()));

    model_index.model = -1;
    model_index.part = -1;

	// uri_handler may process the arg urls here
	QTimer::singleShot(500,[&cmd_line,this]() {
		uri_handler = new uriOpenHandler(this);
		handleCmdLine(cmd_line,false);
	});

    if (config::getOpenModels().isEmpty()) {
		config::createModel();
	}

    connect(sweeper, SIGNAL(createSweep(SharedMorphModel, int)), config::getJobQueue(), SLOT(addSweep(SharedMorphModel,int)), Qt::QueuedConnection );
	connect(sweeper, SIGNAL(attributeDoubleClicked(AbstractAttribute*)), this, SLOT(selectAttribute(AbstractAttribute*)));
}

void MainWindow::handleMessage(const QString& message) {
	QStringList arguments = message.split("@@");
	qDebug() << arguments;
	QCommandLineParser cmd_line;
	parseCmdLine(cmd_line, arguments);
	if (cmd_line.isSet("convert"))
		handleSBMLConvert(cmd_line.value("convert"));
	else 
		handleCmdLine(cmd_line);
}


void MainWindow::handleCmdLine(const QCommandLineParser& cmd_line, bool tasks_only) {
	QDir current_path(".");
	if (cmd_line.isSet("model-path")) {
		current_path.setPath( cmd_line.value("model-path") );
	}
	QStringList files_urls = cmd_line.values("url");
	files_urls.append( cmd_line.positionalArguments() );
	for (int i=0; i<files_urls.size(); i++) {
		auto url = files_urls[i];
		if (url.contains(":")) {
			uri_handler->processUri(QUrl(url));
		}
		else {
			config::openModel( current_path.filePath(url) );
		}
	}
	
	QStringList imports = cmd_line.values("import");
	for (int i=0; i<imports.size(); i++) {
		
		auto url = imports[i];
		uriOpenHandler::URITask task;
		task.method = uriOpenHandler::URITask::Import;
		QRegularExpression scheme("^\\w{2,}:");
		if (scheme.match(url).hasMatch()) {
			task.m_model_url = url;
		}
		else {
			task.m_model_url = QUrl::fromLocalFile( url );
		}
		uri_handler->processTask(task, true);
	}
	
	if (!tasks_only) {
		// also allow manipulation of the morpheus GUI configuration
	}

}
//------------------------------------------------------------------------------

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
	qDebug() << "Drag event, MIME TYPE = " << event->mimeData()->formats() << endl;
	
	if( event->mimeData()->hasFormat("text/plain") ||
		event->mimeData()->hasFormat("text/uri-list") ){
		// Check for valid urls
		for (const QUrl& url: event->mimeData()->urls()) {
			if ( ! uriOpenHandler::isValidUrl(url)) return;
		}
		event->acceptProposedAction();
	}
}

void MainWindow::dropEvent(QDropEvent *event)
{
	if( event->mimeData()->hasFormat("text/plain") ||
		event->mimeData()->hasFormat("text/uri-list") ){
		for (const QUrl& url: event->mimeData()->urls()) {
			uri_handler->processUri(url);
		}
		event->acceptProposedAction();
	}
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

    QAction *fileNew = new QAction(QIcon::fromTheme("document-new", style()->standardIcon(QStyle::SP_FileDialogNewFolder)), tr("New"), this);
    fileNew->setShortcut(QKeySequence::New);
    fileNew->setStatusTip(tr("Create a new model-file"));
	connect(fileNew, &QAction::triggered, [](){config::createModel();} );
    fileMenu->addAction(fileNew);

    fileMenu->addSeparator();

    QAction *fileOpen = new QAction( QIcon::fromTheme("document-open", style()->standardIcon(QStyle::SP_DialogOpenButton)), tr("Open..."), this);
    fileOpen->setShortcut(QKeySequence::Open);
    fileOpen->setStatusTip(tr("Open existing model from file"));
	connect(fileOpen, &QAction::triggered, [=](){loadXMLFile();} );
    fileMenu->addAction(fileOpen);

    QAction *fileReload= new QAction(QIcon::fromTheme("document-revert",QIcon(":/icons/document-revert.png")),tr("Reload"), this);
    fileReload->setShortcut(QKeySequence(Qt::Key_F5));
    fileReload->setStatusTip(tr("Reload model from last file"));
	connect(fileReload, &QAction::triggered, [=](){
		 if ( current_model &&  ! current_model->xml_file.path.isEmpty() ) {
			QString path = current_model->xml_file.path;
			if (config::closeModel(model_index.model,false)) {
				config::openModel( path );
			}
		}
	});
    fileMenu->addAction(fileReload);

	if (SBMLImporter::supported) {
		QAction *importSBML= new QAction(QIcon::fromTheme("document-import",QIcon(":/document-import.png")),tr("Import SBML"), this);
		importSBML->setStatusTip(tr("Import an SBML model into a new Celltype of the current model"));
		connect(importSBML, &QAction::triggered, [=](){
			if (current_model) {
				SharedMorphModel sbml_import = SBMLImporter::importSBML();
				if (sbml_import)
					config::importModel(sbml_import);
				modelViewer[current_model]->setModelPart("CellTypes");
				showCurrentModel();
			}
		});
		fileMenu->addAction(importSBML);
	}

    fileMenu->addSeparator();

    QAction *fileSaveAs = new QAction(QIcon::fromTheme("document-save-as",style()->standardIcon(QStyle::SP_DialogSaveButton) ), tr("Save As..."), this);
    fileSaveAs->setShortcut(QKeySequence::SaveAs);
    fileSaveAs->setStatusTip(tr("Save model to file"));
	connect(fileSaveAs, &QAction::triggered, [=](){
		if (current_model) {
			current_model->getRoot()->synchDOM();
			if ( current_model->xml_file.saveAsDialog() ) {
				config::addRecentFile(current_model->xml_file.path);
				current_model->rootNodeContr->saved();
				modelList->topLevelItem(model_index.model)->setText(0, current_model->xml_file.name);
				qDebug() << "Saved as " << current_model->xml_file.name << endl;
				this->setWindowTitle(tr("Morpheus - %1").arg(  current_model->xml_file.name ) );
			}
		}
	});
    fileMenu->addAction(fileSaveAs);

    QAction *fileSave = new QAction(QIcon::fromTheme("document-save",style()->standardIcon(QStyle::SP_DialogSaveButton) ), tr("Save"), this);
    fileSave->setShortcut(QKeySequence::Save);
    fileSave->setStatusTip(tr("Save model to file"));
	connect(fileSave, &QAction::triggered, [=](){
		if (current_model) {
			current_model->getRoot()->synchDOM();
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
				if ( current_model->xml_file.save() ) {
					current_model->rootNodeContr->saved();
					modelList->topLevelItem(model_index.model)->setText(0, current_model->xml_file.name);
				}
				else {
					QMessageBox::critical(this,"Error", "Failed to save model to " + current_model->xml_file.path );
				}
			}
		}
	});
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

    QAction *act_settings = fileMenu->addAction(QIcon::hasThemeIcon("preferences-other") ? QIcon::fromTheme("preferences-other") : QIcon::fromTheme("configure", QIcon(":/settings.png")), "Settings");
    act_settings->setShortcut(QKeySequence::Preferences);
    act_settings->setStatusTip(tr("Open settings dialog"));
	connect(act_settings, &QAction::triggered, [&](){settingsDialog settingsDia; settingsDia.exec();} );

    fileMenu->addSeparator();
    
    QAction *fileClose = fileMenu->addAction( QIcon::fromTheme("document-close", QIcon(":/document-close.png")), tr("Close"));
    fileClose->setShortcut(QKeySequence::Close);
    fileClose->setStatusTip(tr("Close current model"));
	connect(fileClose, &QAction::triggered, [=](){ config::closeModel(model_index.model,true); });


    QAction *appQuit = fileMenu->addAction( QIcon::fromTheme("application-exit", QIcon(":/application-exit.png")), tr("Quit"));
    appQuit->setShortcut(QKeySequence::Quit);
    appQuit->setStatusTip(tr("Quit Morpheus"));
	connect(appQuit, &QAction::triggered, [=](){ config::getDatabase().close(); this->close(); });

    QMenu *examplesMenu = menubar->addMenu(tr("Examples"));

// 	QMenu* examplesMenu = fileMenu->addMenu( QIcon::fromTheme("applications-science",QIcon(":/applications-science.png")),tr("&Examples"));
	QDir ex_dir(":/examples");
	QStringList ex_categories_sl = ex_dir.entryList();
	QMap<int, QString> ex_categories;
	for(const QString& ex_cat: ex_categories_sl ) {
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
	
	for( const QString& ex_cat: ex_categories ) {
		QDir ex_cat_dir(ex_dir); ex_cat_dir.cd(ex_cat);
		QMenu * ex_cat_menu = examplesMenu->addMenu(ex_cat);
		QStringList examples = ex_cat_dir.entryList();
		for(const QString& example: examples ) {
			if (example.endsWith(".xml")) {
				QAction *openEx = ex_cat_menu->addAction(example);
				QString ex_path = QString(":/examples/")+ex_cat_dir.dirName() + "/" + example;
				connect(openEx, &QAction::triggered, [=](){ config::openModel(ex_path); });
			}
		}
	}
	QAction* exInfoAction = examplesMenu->addAction(tr("&Examples website"));
	exInfoAction->setStatusTip(tr("Open Morpheus examples website for documentation."));
	connect(exInfoAction, SIGNAL(triggered()), config::getInstance(),SLOT(openExamplesWebsite()));
	
    examplesMenu->setStatusTip(tr("Open Morpheus example model"));

	QMenu *windowMenu = menubar->addMenu(tr("&Window"));
	QAction* aDocsDock = documentsDock->toggleViewAction();
	aDocsDock->setText(tr("Show Documents"));
	windowMenu->addAction(aDocsDock);
	
	QAction* aJobsDock = jobQueueDock->toggleViewAction();
	aJobsDock->setText(tr("Show Job Queue"));
	windowMenu->addAction(aJobsDock);
	
	QAction* aDocuDock = docuDock->toggleViewAction();
	aDocuDock->setText(tr("Show Documentation"));
	windowMenu->addAction(aDocuDock);
	
	QAction* aFixesDock = dwid_fixBoard->toggleViewAction();
	aFixesDock->setText(tr("Show FixBoard"));
	windowMenu->addAction(aFixesDock);
	
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
	
	QAction* aboutAnnounce = new QAction(tr("Announcements"),menubar);
	aboutAnnounce->setStatusTip("Show all latest Announcementes");
	connect(aboutAnnounce,SIGNAL(triggered()), announcer, SLOT(showAllAnnouncements()));
	aboutMenu->addAction(aboutAnnounce);
	
	aboutMenu->addSeparator();
	
	QAction* aboutQt = new QAction(tr("&Qt"),menubar);
	aboutQt->setStatusTip(tr("Show information about Qt."));
	connect(aboutQt,SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    aboutMenu->addAction(aboutQt);
	
// 	QAction* aboutHelp = new QAction(tr("&Help"),menubar);
// 	aboutQt->setStatusTip(tr("Show Morpheus help."));
// 	connect(aboutHelp,SIGNAL(triggered()), config::getInstance(), SLOT(aboutHelp()));
//     aboutMenu->addAction(aboutHelp);

    QToolBar *toolbar = new QToolBar("Main Toolbar",this);
    toolbar->setObjectName("Main Toolbar");
    toolbar->setIconSize(QSize(24,24));
    toolbar->addAction(fileOpen);
    QToolButton* tbutton = (QToolButton*) toolbar->widgetForAction(fileOpen);
    tbutton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
//	if ((QSysInfo::productType() == "osx" || QSysInfo::productType() == "windows") && QIcon::hasThemeIcon("document-open-symbolic"))
//		tbutton->setIcon(QIcon::fromTheme("document-open-symbolic"));

    toolbar->addAction(fileSave);
    tbutton = (QToolButton*) toolbar->widgetForAction(fileSave);
    tbutton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
//	if ((QSysInfo::productType() == "osx" || QSysInfo::productType() == "windows") && QIcon::hasThemeIcon("document-save-symbolic"))
//		tbutton->setIcon(QIcon::fromTheme("document-save-symbolic"));

    toolbar->addSeparator();

    cb_resource = new QComboBox(this);
    cb_resource->addItems(config::getJobQueue()->queueNames());
	cb_resource->setCurrentIndex(0); // Local by default
	cb_resource->setFocusPolicy(Qt::ClickFocus);

    toolbar->addWidget(cb_resource);
	
	
    QAction *simStart = new QAction( QIcon::fromTheme("media-playback-start"), tr("&Start"), toolbar);
	if ((QSysInfo::productType() == "osx" || QSysInfo::productType() == "windows") && QIcon::hasThemeIcon("media-playback-start-symbolic"))
		simStart->setIcon(QIcon::fromTheme("media-playback-start-symbolic"));
    simStart->setShortcut(QKeySequence(Qt::Key_F8));
    simStart->setStatusTip(tr("Start morpheus simulation with current model"));
	connect(simStart, &QAction::triggered, [=](){startSimulation();} );
    toolbar->addAction(simStart);
    tbutton = (QToolButton*) toolbar->widgetForAction(simStart);
    tbutton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    QAction *simStop = new QAction(QIcon::fromTheme("media-playback-stop"), tr("&Stop"), toolbar);
	if ((QSysInfo::productType() == "osx" || QSysInfo::productType() == "windows") && QIcon::hasThemeIcon("media-playback-stop-symbolic"))
		simStop->setIcon(QIcon::fromTheme("media-playback-stop-symbolic"));
    simStop->setShortcut(QKeySequence(Qt::Key_F9));
    simStop->setStatusTip(tr("Terminate current morpheus simulation"));
	connect(simStop, &QAction::triggered, [=](){stopSimulation();} );
    toolbar->addAction(simStop);
    interactive_stop_button = (QToolButton*) toolbar->widgetForAction(simStop);
    interactive_stop_button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    interactive_stop_button->setEnabled(false);

    setMenuBar(menubar);
    addToolBar(toolbar);

	toolbar->setFocusPolicy(Qt::NoFocus);
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
//     connect(modelList, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(modelListChanged(QTreeWidgetItem*)));
	connect(modelList, &QTreeWidget::clicked, [=](const QModelIndex& /*index*/) { modelListChanged(modelList->currentItem());});
	connect(modelList, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(activatePart(QModelIndex)) );

	// Model popup menu
    modelMenu = new QMenu();
	auto show_xml_action = new QAction(QIcon::fromTheme("text-xml",QIcon(":/icons/text-xml.png")),"Show XML", this);
	connect(show_xml_action, &QAction::triggered, 
		[this](){
			XMLTextDialog dia( config::getOpenModels()[model_popup_index.model]->getXMLText() , this);
			dia.exec();
	});
	modelMenu->addAction(show_xml_action);
	
    modelMenu->addSeparator();

	removeModelPartAction = modelMenu->addAction(QIcon::fromTheme("list-remove",QIcon(":/icons/list-remove.png")),"Remove");
	connect(removeModelPartAction, &QAction::triggered,[this](){
		auto popup_model = config::getOpenModels()[model_popup_index.model];
		QMessageBox msgBox;
		msgBox.setInformativeText(QString("Do you want to delete %1 ?").arg(popup_model->parts[model_popup_index.part].label));
		msgBox.setStandardButtons( QMessageBox::Ok | QMessageBox::Cancel);
		msgBox.setDefaultButton(QMessageBox::Cancel);
		msgBox.move(this->cursor().pos());
		if (msgBox.exec() == QMessageBox::Ok) {
			popup_model->removePart(model_popup_index.part);
		}
	});
	
    copyModelPartAction = modelMenu->addAction(QIcon::fromTheme("edit-copy",QIcon(":/icons/edit-copy.png")),"Copy");
	connect(copyModelPartAction, &QAction::triggered,[this](){
		copyNodeAction(config::getOpenModels()[model_popup_index.model]->parts[model_popup_index.part].element->cloneXML());
	});
	
    pasteModelPartAction = modelMenu->addAction(QIcon::fromTheme("edit-paste",QIcon(":/icons/edit-paste.png")),"Paste");
	connect(pasteModelPartAction, &QAction::triggered,[this](){
		config::getOpenModels()[model_popup_index.model]->addPart(config::getNodeCopies().first().cloneNode(true));
	});
    modelMenu->addSeparator();
	
    closeModelAction =  modelMenu->addAction(QIcon::fromTheme("dialog-close",style()->standardIcon(QStyle::SP_DialogCloseButton)),"Close");
	connect(closeModelAction, &QAction::triggered, [this](){
		config::closeModel(model_popup_index.model, true);
	});
    //modelMenu->addSeparator();
    //mailAttachAction =  modelMenu->addAction(QThemedIcon("mail-send",QIcon(":/mail-attach.png")),"Send by mail");
 
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
	QAction* fixBoardCopyAction = new QAction(QIcon::fromTheme("edit-copy",QIcon(":/edit-copy.png")),"Copy",fixBoard);
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
	connect(job_queue_view, SIGNAL(erronousXMLPath(QString,int)), this, SLOT(selectXMLPath(QString,int)));
	
    jobQueueDock->setWidget(job_queue_view);
    jobQueueDock->setObjectName("JobQueueDock");
    addDockWidget(Qt::LeftDockWidgetArea,jobQueueDock,Qt::Vertical);
	jobQueueDock->setFocusProxy(job_queue_view);
	
	docuDock = new DocuDock(this);
	docuDock->setObjectName("DocuDock");
	addDockWidget(Qt::RightDockWidgetArea,docuDock,Qt::Vertical);

#ifdef QWebEngineDebugger
	auto debugger_dock = new QDockWidget("BrowserDebugger");
	debugger_dock->setObjectName("BrowserDebuggerDock");
	auto debugger = new WebViewer(this);
	pageList = new QListWidget();
#ifdef USE_QWebEngine
	auto doku_page = new QListWidgetItem("Doku");
	doku_page->setData(Qt::UserRole, QVariant::fromValue(qobject_cast<QWebEngineView*>(docuDock->getHelpView())->page()) ) ;
	connect(pageList,&QListWidget::itemClicked,[debugger](QListWidgetItem* item) { qobject_cast<QWebEngineView*>(debugger)->page()->setInspectedPage( item->data(Qt::UserRole).value<QWebEnginePage*>()) ; }) ;
	pageList->addItem(doku_page);
#endif
	auto debugger_widget = new QWidget(debugger_dock);
	auto debugger_layout = new QVBoxLayout();
	debugger_widget->setLayout(debugger_layout);
	debugger_layout->addWidget(pageList,0);
	debugger_layout->addWidget(debugger,4);
	debugger_dock->setWidget(debugger_widget);
	addDockWidget(Qt::LeftDockWidgetArea,debugger_dock,Qt::Vertical);	
#endif
	
    // The Core Editor Stack
    editorStack = new QStackedWidget();
    editorStack->setFrameStyle(QFrame::StyledPanel | QFrame::Panel | QFrame::Sunken);
	
	no_model_widget = new QWidget();
	no_model_widget->setLayout(new QVBoxLayout());
	no_model_widget->layout()->addWidget(new QLabel("no model selected!"));
	editorStack->addWidget(no_model_widget);
	
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
	
	announcer = new AnnouncementDialog(this);
	QTimer::singleShot(500, announcer, SLOT(showAnnouncements()));
	QTimer* announcementTimer = new QTimer(this);
	announcementTimer->setInterval(24*60*60*1000);
	connect(announcementTimer, SIGNAL(timeout()), announcer, SLOT(showAnnouncements()));
	announcementTimer->start();
	
#ifdef MORPHEUS_FEEDBACK
	auto feedback = new FeedbackRequestWindow(this);
	QTimer::singleShot(1000, feedback, SLOT(sendFeedBack()));

	QTimer* feedbackTimer = new QTimer(this);
	feedbackTimer->setInterval(24*60*60*1000);
	connect(feedbackTimer, SIGNAL(timeout()), feedback, SLOT(sendFeedBack()));
	feedbackTimer->start();
#endif
}

//------------------------------------------------------------------------------

void MainWindow::selectModel(int index, int part)
{
// 	qDebug() << "Selecting model " << index << "of" << config::getOpenModels().size() << ". Current is" << model_index.model;
	if (index<0) {
		model_index.model = -1;
		current_model = SharedMorphModel();
		modelList->blockSignals(true);
		modelList->setCurrentItem(modelList->topLevelItem(-1));
		modelList->blockSignals(false);
		editorStack->setCurrentWidget(no_model_widget);
		return;
	}
	if (model_index.model == index && (model_index.part==part || part == -1)) {
		showCurrentModel();
		return;
	}
	
	config::modelIndex selected = modelListIndex(modelList->currentItem());
	if (model_index.model != index) {
// 		qDebug() << "Switching model";
		model_index.model = index;
		current_model = config::getOpenModels()[model_index.model];
		if (selected.model == model_index.model)
			part = selected.part;
		model_index.part = part>=0 ? part : 0;

		modelViewer[current_model]->updateConfig();
		modelList->topLevelItem(index)->setExpanded(true);

		setWindowTitle(tr("Morpheus - %1").arg(  current_model->xml_file.name ) );

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
		config::switchModel(model_index.model);
    }
    else {
// 		qDebug() << "Switching model part";
		if (part<0 || part>=current_model->parts.size()) {
			model_index.part = selected.part;
			return;
		}
		else if (current_model->parts[part].enabled) {
			model_index.part = part;
		}
		else {
			// Select a new part to show ...
			while (model_index.part && !current_model->parts[model_index.part].enabled) {
				model_index.part--;
			}
		}
    }
    modelList->blockSignals(true);
	modelList->setCurrentItem(modelList->topLevelItem(index)->child(model_index.part));
	modelList->blockSignals(false);
	showCurrentModel();
	
}

//------------------------------------------------------------------------------

void MainWindow::setPermanentStatus(QString message) {
    statusMsgSource = sender();
    permanentStatus->setText(message);
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

    QString fileName = QFileDialog::getOpenFileName(this, tr("Open model"), directory, tr("Morphes Model Files (*.xml *.xml.gz)"));
    if(fileName != "")
    {
        QString path = QFileInfo(fileName).dir().path();
        QSettings().setValue("FileDialog/path", path);
        QString xmlFile = fileName;
// 		QtConcurrent::run(&config::openModel,xmlFile);
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
		if (item->parent()) {
			index.model = modelList->indexOfTopLevelItem(item->parent());
			index.part = item->parent()->indexOfChild(item);
		}
		else {
			index.model = modelList->indexOfTopLevelItem(item);
			index.part = 0;
		}
	}
    else {
		index.model = -1;
		index.part = 0;
	}
	return index;
}

//------------------------------------------------------------------------------

void MainWindow::showModelListMenu(QPoint p)
{
    QTreeWidgetItem* item = modelList->itemAt(p);
    if (item && ! item->isDisabled()) {
        model_popup_index = modelListIndex(item);
        SharedMorphModel m = config::getOpenModels()[model_popup_index.model];

        QList<QString> addableChilds = m->rootNodeContr->getAddableChilds();
// 		if (addableChilds.empty()) 
// 			addModelPartMenu->setDisabled(true);
// 		else
// 			addModelPartMenu->setDisabled(false);
//         addModelPartMenu->clear();
//         for ( int i=0; i< addableChilds.size(); i++ ) {
//             addModelPartMenu->addAction(addableChilds[i]);
//         }
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

void MainWindow::syncModelList (int m) {
	if (m==-1) {
		const QList<SharedMorphModel >&  models = config::getOpenModels();
		for(const SharedMorphModel& model: models) {
			if (sender() == model) {
				m = models.indexOf(model);
			}
		}
	}
// 	qDebug() << "Sync model list for model " << m;
	// In fact we just need to enable/disable on the basis of parts data.
// 	modelList->topLevelItem(m)->takeChildren();
	SharedMorphModel model = config::getOpenModels()[m];

	QTreeWidgetItem* modelItem = modelList->topLevelItem(m);
	for (int part=0; part < model->parts.size(); part++) {
		QTreeWidgetItem* part_item = modelItem->child(part);
		if ( ! part_item ) {
			part_item = new QTreeWidgetItem(QStringList(model->parts[part].label));
			modelItem->addChild(part_item );
		}
		if ( model->parts[part].enabled) {
			part_item->setDisabled(false);
			part_item->setForeground(0,style()->standardPalette().brush(QPalette::WindowText));
		}
		else {
			if (m==model_index.model && part == model_index.part) {
				// current part was deactivated
				int new_part = 0;
				for (int i=part; i<model->parts.size(); i++) {
					if (model->parts[i].enabled) {
						new_part = i;
						break;
					}
				}
				if (new_part==0) {
					for (int i=part; i>=0; i--) {
						if (model->parts[i].enabled) {
							new_part = i;
							break;
						}
					}
				}
				selectModel(model_index.model, new_part);
			}
			part_item->setDisabled(true);
			part_item->setForeground(0,style()->standardPalette().brush(QPalette::Disabled,QPalette::WindowText));
		}
	}

// 	if (model_index.model == m) {
// 		selectModel(m, model_index.part);
// 	}
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

// 	qDebug() << "Adding model " << index;
	QTreeWidgetItem* c = new QTreeWidgetItem(QStringList(model->xml_file.name));
	c->setIcon(0,QIcon::fromTheme("text-x-generic",QIcon(":/icons/text-generic.png")));
	c->setIcon(1,QIcon::fromTheme("edit-delete",QIcon(":/icons/edit-delete.png")));
	QFont f (c->font(0));
	f.setBold(true);
	c->setFont(0,f);
	modelList->insertTopLevelItem(index,c);

	connect(model,SIGNAL(modelPartAdded(int)),this,SLOT(syncModelList()));
	connect(model,SIGNAL(modelPartRemoved(int)),this,SLOT(syncModelList()));

	domNodeViewer *viewer = new domNodeViewer(this);
	viewer->setModel(model,0);
	connect(viewer,SIGNAL(xmlElementCopied(QDomNode)),this,SLOT(copyNodeAction(QDomNode)));
// 	connect(viewer,SIGNAL(nodeSelected(nodeController*)),docuDock,SLOT(setCurrentNode(nodeController*)));
	connect(viewer,SIGNAL(xmlElementSelected(QStringList)),docuDock,SLOT(setCurrentElement(QStringList)));
	
	editorStack->addWidget(viewer);
	modelViewer[model] = viewer;
	modelAbout[model] =  new AboutModel(model);
	editorStack->addWidget(modelAbout[model]);
	connect(modelAbout[model], &AboutModel::nodeSelected, this, [this](QString path) { selectXMLPath(path, this->model_index.model); });
	
#ifdef QWebEngineDebugger
	if (pageList) {
		auto doku_page = new QListWidgetItem("Page");
		doku_page->setData(Qt::UserRole, QVariant::fromValue(qobject_cast<QWebEngineView*>(modelAbout[model]->getView())->page()) ) ;
		pageList->addItem(doku_page);
	}
#endif

	syncModelList(index);

	documentsDock->raise();
	config::switchModel(index);

	setPermanentStatus("Model loaded successfully");
	if ( ! model->rootNodeContr->getModelDescr().auto_fixes.isEmpty()) {
		dwid_fixBoard->raise();
	}

	statusMsgSource = editorStack;
	updateRecentFileActions();
}

//------------------------------------------------------

void MainWindow::removeModel(int index) {
// 	qDebug() << "Removing model" << index << "from View (current is"<< model_index.model << ")";
	if (index == model_index.model) selectModel(-1);
    SharedMorphModel model = config::getOpenModels()[index];
    disconnect(model,SIGNAL(modelPartAdded(int)),this,SLOT(syncModelList()));
    disconnect(model,SIGNAL(modelPartRemoved(int)),this,SLOT(syncModelList()));
    editorStack->removeWidget(modelViewer[model]);
	modelViewer[model]->deleteLater();
    modelViewer.remove(model);
	editorStack->removeWidget(modelAbout[model]);
	modelAbout[model]->deleteLater();
	modelAbout.remove(model);
    modelList->takeTopLevelItem(index);
    if (model_index.model >= index) {
        model_index.model -= 1;
    }
}

void MainWindow::showCurrentModel() {
// 	qDebug() << "showing current model from " << sender();
	if (current_model->parts[model_index.part].label=="ParamSweep") {
        editorStack->setCurrentWidget(sweeper);
		QWidget::setTabOrder(modelList,sweeper);
		docuDock->setCurrentElement( "ParameterSweep" );
	}
	else if (model_index.part==0) {
		if (editorStack->currentWidget() != modelAbout[current_model]) {
			modelAbout[current_model]->update();
			editorStack->setCurrentWidget(modelAbout[current_model]);
			QWidget::setTabOrder(modelList,modelAbout[current_model]);
		}
		docuDock->setCurrentElement(current_model->parts[model_index.part].element->getXPath() );
// 		docuDock->setCurrentNode( current_model->parts[model_index.part].element);
	}
    else {
        modelViewer[current_model]->setModelPart(model_index.part);
        editorStack->setCurrentWidget(modelViewer[current_model]);
// 		docuDock->setCurrentNode( current_model->parts[model_index.part].element);
		QWidget::setTabOrder(modelList,modelViewer[current_model]);
    }
	documentsDock->raise();
}
//------------------------------------------------------

void MainWindow::modelListChanged(QTreeWidgetItem * item) {

    config::modelIndex selection = modelListIndex(item);
//     if (model_index.model != selection.model)
//         config::switchModel(selection.model);
//     else {
        selectModel(selection.model, selection.part);
// 	}
 }
 
void MainWindow::activatePart(QModelIndex idx)
{
	if (idx.isValid() && idx.parent().isValid()) {
		config::modelIndex selection;
		selection.model = idx.parent().row();
		selection.part = idx.row();
		auto model = config::getOpenModels()[selection.model];
		if (model->activatePart(selection.part) )
			modelList->setCurrentItem(modelList->invisibleRootItem()->child(selection.model)->child(selection.part));
// 		selectModel(selection.model,selection.part);
// 		if ( !model->parts[selection.part].enabled ) {
// 			if (model->activatePart(selection.part)) {
// 				QTreeWidgetItem* item = modelList->invisibleRootItem()->child(selection.model)->child(selection.part);
// 				item->setForeground(0,QBrush(Qt::black));
// 				item->setDisabled(false);
// 				modelList->setCurrentItem(item);
// 			}
// 		}
		
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

void MainWindow::selectXMLPath(QString path, int model_id)
{
	QStringList xml_path = path.split("/", QString::SkipEmptyParts);
	if (xml_path[0] == "MorpheusModel") xml_path.pop_front();
	if (xml_path.size()<1) return; 
	QString part_name = xml_path[0];
	
	if (model_id<0) return;
	if (model_id>=0) {
	}
	
	int part_id = -1;
	for (int p=0; p<current_model->parts.size(); p++ ){
		if (current_model->parts[p].label == part_name) {
			part_id=p;
			break;
		}
	}

	selectModel(model_id,part_id);

	if ( part_id>=0) {
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
	if (current_model->rootNodeContr->getModelDescr().auto_fixes.size()<= row) return;
    const MorphModelEdit& e = current_model->rootNodeContr->getModelDescr().auto_fixes[row];

	QDomNode node = e.xml_parent;
	if (e.edit_type == MorphModelEdit::NodeAdd &&  ! e.xml_parent.firstChildElement(e.name).isNull()) {
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


