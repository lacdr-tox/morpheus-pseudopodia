#include "jobcontroller.h"

JobView::JobView()
{
    te_output = new QPlainTextEdit(this);
    te_output->setReadOnly(true);
   // te_output->setFont( QFont( "DejaVu Sans Mono" ) );
    QFont font("Monospace");
    font.setStyleHint(QFont::TypeWriter);
    te_output->setFont( font );

    QPalette pal;
    pal.setColor(QPalette::Base, pal.color(QPalette::AlternateBase) );
    te_output->setPalette(pal);

    output_files = new QTreeView();
    dir_model = new QFileSystemModel(this);
    output_files->setModel(dir_model);
	selection_model = output_files->selectionModel();
	output_files->setColumnWidth(0,250);

	output_files->setAllColumnsShowFocus(true);
    output_files->setDragDropMode(QAbstractItemView::DragOnly);
    output_files->setSelectionMode(QAbstractItemView::ExtendedSelection);
    output_files->setSortingEnabled(true);
    output_files->setUniformRowHeights(true);
    connect(output_files, SIGNAL(doubleClicked(QModelIndex)),this,
						  SLOT(fileClicked(QModelIndex)));
	connect(selection_model, SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, 
						  SLOT(previewSelectedFile(const QModelIndex &, const QModelIndex &)));
	
	
    title = new QLabel();
    QFont titlefont = this->font();
	titlefont.setPointSize(titlefont.pointSize()+2);
    title->setFont( titlefont );


    job_group_box = new QGroupBox("");

    QHBoxLayout *sim_header = new QHBoxLayout();
    sim_header->addStretch();
	QToolBar *sim_tools = new QToolBar();
	sim_tools->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	sim_tools->setIconSize(QSize(24,24));
	
	stop_action = new QAction(QIcon::fromTheme("media-playback-stop-symbolic", QIcon::fromTheme("media-playback-stop", style()->standardIcon(QStyle::SP_MediaStop))),"Stop",sim_tools);
	connect(stop_action,SIGNAL(triggered(bool)),this,SLOT(stopJob()));
	sim_tools->addAction(stop_action);
	sim_tools->addSeparator();
	
	open_output_action = new QAction(QIcon::fromTheme("folder",style()->standardIcon(QStyle::SP_DirIcon)),"Output Folder",sim_tools);
	connect(open_output_action, SIGNAL(triggered(bool)), SLOT(openOutputFolder()));
	sim_tools->addAction(open_output_action);

    open_terminal_action = new QAction(QIcon::fromTheme("utilities-terminal",QIcon(":/icons/utilities-terminal.png")),"Terminal", this);
#ifdef Q_WS_MAC
    open_terminal_action->setEnabled(false);
#endif
	connect(open_terminal_action, SIGNAL(triggered()), SLOT(openTerminal()));
	sim_tools->addAction(open_terminal_action);

    sim_tools->addSeparator();
    make_movie_action = new QAction(QIcon::fromTheme("movie",QIcon(":/icons/movie.png")),"Create movie",this);
    connect(make_movie_action, SIGNAL(triggered(bool)), SLOT(makeMovie()));
    sim_tools->addAction(make_movie_action);

	sim_header->addWidget(sim_tools);
//     sim_header->addSpacerItem(new QSpacerItem(0,0,QSizePolicy::Expanding,QSizePolicy::Minimum));
	sim_header->addStretch();
    job_group_box->setLayout(sim_header);

    gr_params = new QGroupBox("ParamSweep");
    QVBoxLayout *par_layout = new QVBoxLayout();
	par_sweep_title = new QLabel("This Job is part of a Parameter Sweep.");
	par_layout->addWidget(par_sweep_title);
	par_sweep_title->setVisible(false);

	QHBoxLayout* l = new QHBoxLayout();
	l->addWidget(new QLabel("Parameter List:"));
	l->addStretch(2);
	b_restore_sweep = new QPushButton("Restore Sweep");
	l->addWidget(b_restore_sweep);
	connect(b_restore_sweep,SIGNAL(clicked(bool)),this,SLOT(restoreSweep()));

    b_make_table = new QPushButton("Image Table");
	l->addWidget(b_make_table);
	connect(b_make_table,SIGNAL(clicked(bool)),this,SLOT(makeTable()));

	par_layout->addLayout(l);
    parameter_list = new QListWidget();
    par_layout->addWidget(parameter_list);
	
    gr_params->setLayout(par_layout);
	
	QGraphicsScene* scene = new QGraphicsScene();
	imagePreview = new QGraphicsView(scene);
	imagePreview->setInteractive(false);
	
	connect(this, SIGNAL(imagePreviewChanged()), this, SLOT(resizeGraphicsPreview()), Qt::QueuedConnection);
	connect(&image_resize_timer,SIGNAL(timeout()),this,SLOT(resizeGraphicsPreview()));
	image_resize_timer.setSingleShot(true);
	image_resize_timer.setInterval(100);

// 	QMovie* movie = new QMovie(this);
//     qDebug() << "QMOVIE: SUPPORTED FORMATS: " << movie->supportedFormats();

	textPreview = new QPlainTextEdit();
	textPreview->setBackgroundRole(QPalette::Base);
	textPreview->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	textPreview->setReadOnly(true );
	textPreview->setVisible(true);
// 	textPreview->setFontFamily("Courier");
	textPreview->setWordWrapMode(QTextOption::WordWrap);

	
	previewStack= new QStackedWidget();
	previewStack->addWidget(imagePreview);
	previewStack->addWidget(textPreview);

// 	gr_preview = new QGroupBox("Preview");
// 	gr_preview->setLayout(previewStack);
// 	gr_preview->setMinimumWidth(250);
// 	gr_preview->hide();
	

	splitter_tree_text = new QSplitter(this);
	splitter_tree_text->setOrientation(Qt::Vertical);
	splitter_tree_text->addWidget( output_files );
	splitter_tree_text->addWidget( te_output );

	splitter_output_preview = new QSplitter(this);
	splitter_output_preview->addWidget( splitter_tree_text );
	splitter_output_preview->addWidget( previewStack );
	connect(splitter_output_preview,SIGNAL(splitterMoved(int,int)), this, SLOT(resizeGraphicsPreview()));
	
	
	QHBoxLayout *l_output = new QHBoxLayout();
    l_output->addWidget(splitter_output_preview);
	QGroupBox* gr_output = new QGroupBox("Output");
    gr_output->setLayout(l_output);
    gr_output->setAlignment(Qt::AlignLeft);
	
 	splitter_params_output = new QSplitter(this);
 	splitter_params_output->setOrientation(Qt::Vertical);
 	splitter_params_output->addWidget( gr_params );
	splitter_params_output->addWidget( gr_output );
	
    QVBoxLayout *gl = new QVBoxLayout();
// 	gl->addLayout(sim_header);
    gl->addWidget(title, 0, Qt::AlignHCenter);
    gl->addWidget(job_group_box);
	gl->addWidget(splitter_params_output);


    setLayout(gl);

}

//--------------------------------------------------------------------------
void JobView::setSweep(QList<int> job_ids) {
    parameter_list->clear();
    QSqlQuery query;
    query.prepare(QString("SELECT * FROM sweeps JOIN sweep_jobs ON sweeps.id = sweep_jobs.sweep WHERE sweep_jobs.job=%1").arg(job_ids.first()));
    if ( ! query.exec())
        qDebug() << "Unable to get job info " <<query.lastQuery() ;
    if ( query.next() ) {
        int id_idx = query.record().indexOf("id");
        int name_idx = query.record().indexOf("name");
        int subdir_idx = query.record().indexOf("subDir");
        int header_idx = query.record().indexOf("header");
        int paramSet_idx = query.record().indexOf("paramSet");
        int paramData_idx = query.record().indexOf("paramData");

        current_sweep.id = query.value(id_idx).toInt();
        current_sweep.name = query.value(name_idx).toString();
        current_sweep.subdir =  query.value(subdir_idx).toString();
        current_sweep.fullpath = config::getApplication().general_outputDir + "/" + current_sweep.subdir;
        current_sweep.summary_filename = current_sweep.fullpath +"/sweep_summary.txt";
		current_sweep.job_ids = job_ids;

        title->setText(QString("Sweep %1: %2").arg(current_sweep.id).arg(current_sweep.name));
        output_files->setRootIndex( dir_model->index( current_sweep.fullpath ));

        dir_model->setRootPath( current_sweep.fullpath );
        updateSweepData( current_sweep.summary_filename );

		QStringList params = query.value(paramSet_idx).toString().split(";");
		if ( ! query.value(paramData_idx).isNull()) {
			QList<QList<QPair<QString,QString> > > param_data;
			QByteArray data = query.value(paramData_idx).toByteArray() ;
			QDataStream param_data_stream(data);
			param_data_stream >> param_data;
			uint pix=0;
			for (uint i=0; i<param_data.size();i++)
				for (uint j=0; j<param_data[i].size();j++) {
					QString p = param_data[i][j].first.remove("MorpheusModel") + " = " + param_data[i][j].second;
					parameter_list->addItem(p);
				}
		}
		view_sweep = true;
		gr_params->show();
		previewStack->hide();
		par_sweep_title->setVisible(false);
		b_restore_sweep->setEnabled(true);
		b_make_table->setEnabled(true);
	}
	else {
		view_sweep = false;
		gr_params->hide();
		previewStack->hide();
		b_restore_sweep->setEnabled(false);
		b_make_table->setEnabled(false);
		
    }
}

void JobView::setJob(int job_id) {
	
	if (current_job && current_job->jobID() == job_id)
		return;
	
// 	qDebug() << "setJob";
	if (current_job)
		this->disconnect(current_job.data(),0,this,0);
	
	current_job = config::getJobQueue()->getJobs()[job_id];

	view_sweep = false;
	b_restore_sweep->setEnabled(false);
	b_make_table->setEnabled(false);
	previewStack->hide();
	gr_params->hide();
	te_output->clear();
	text_shown = 0;
	
	if (! current_job) {
		title->setText("none");
		return;
	}
	
	title->setText(QString("Job %1: %2").arg(current_job->jobID()).arg(current_job->info().title));
	
	connect(current_job.data(),SIGNAL(stateChanged(abstractProcess*)), this, SLOT(updateJobState()));
	connect(current_job.data(),SIGNAL(outputChanged(abstractProcess*)),this, SLOT(updateJobData()));
	updateJobState();
	 
	updateJobData();
	
	const ProcessInfo& info = current_job->info();
	output_files->setRootIndex(dir_model->index(config::getApplication().general_outputDir + "/" + info.sim_dir));
	dir_model->setRootPath(config::getApplication().general_outputDir + "/" + info.sim_dir);
	

	parameter_list->clear();
	QSqlQuery query;
	query.prepare(QString("SELECT * FROM sweep_jobs JOIN sweeps ON sweeps.id=sweep_jobs.sweep WHERE sweep_jobs.job=%1").arg(job_id));
	if ( ! query.exec())
		qDebug() << "Unable to get job info " <<query.lastQuery() ;
    if ( query.next() ) {
		int name_idx = query.record().indexOf("name");
		int subdir_idx = query.record().indexOf("subDir");
		int header_idx = query.record().indexOf("header");
		int paramSet_idx = query.record().indexOf("paramSet");
		int paramData_idx = query.record().indexOf("paramData");
		int sweep_id_idx = query.record().indexOf("sweep");
		
		QStringList params = query.value(paramSet_idx).toString().split(";");
		par_sweep_title->setText(QString("This job is part of parameter sweep \"%1\" (id=%2).").arg(query.value(name_idx).toString()).arg(query.value(sweep_id_idx).toInt()));
		par_sweep_title->setVisible(true);
// 		par_path->setText(query.value(subdir_idx).toString());
		
		if ( ! query.value(paramData_idx).isNull()) {
			QList<QList<QPair<QString,QString> > > param_data;
			QByteArray data = query.value(paramData_idx).toByteArray() ;
			QDataStream param_data_stream(data);
			param_data_stream >> param_data;
			uint pix=0;
			for (uint i=0; i<param_data.size();i++) 
				for (uint j=0; j<param_data[i].size();j++) {
					QString p = param_data[i][j].first.remove("MorpheusModel") + " = " + params[pix++];
					parameter_list->addItem(p);
				}
		}
		
		gr_params->show();
    }
}

void JobView::updateSweepState(){
    //qDebug() << "JobView::updateSweepState: not yet implemented. Should check whether jobs that are part of this sweep are still running. If so, Stop-button should be enabled. Else disabled.";
    for(uint i=0; i< current_sweep.job_ids.size(); i++){
		ProcessInfo::status job_state =  map_idToProcess[ current_sweep.job_ids[i] ]->state();
        if( job_state == ProcessInfo::RUN || job_state == ProcessInfo::PEND ){
            stop_action->setEnabled(true);
			return;
		}
    }
	// if sweep does not contain running/pending jobs, disable stop button
	stop_action->setEnabled(false);
}

void JobView::updateSweepData( QString filename ) {

    QFile sweepsummary( filename );

    if ( sweepsummary.open(QFile::ReadOnly | QIODevice::Text))
    {
        QTextStream in(&sweepsummary);
        QString text_sweepsummary("");
        text_sweepsummary += in.readAll();
        sweepsummary.close();
        te_output->setPlainText(text_sweepsummary);
    }
    else{
        qDebug() << "JobView::updateSweepData: Could not open " << filename;
    }
}

void JobView::updateJobState(){
    if (current_job->state() == ProcessInfo::RUN || current_job->state() == ProcessInfo::PEND){
        stop_action->setEnabled(true);
        QApplication::setWindowIcon( QIcon(":/morpheus-running.png") );
    }
    else{
        stop_action->setEnabled(false);
        QApplication::setWindowIcon( QIcon(":/morpheus.png") );
    }
}

void JobView::updateJobData() {
	if(!view_sweep){
		const QString& output = current_job->getOutput();
		
		bool scroll_down = te_output->textCursor().atEnd() || text_shown == 0;
		
		if (text_shown == 0)
			te_output->setPlainText(output);
		else {
			te_output->appendPlainText(output.right(output.size()-text_shown));
		}
		text_shown = output.size();
		
		if (scroll_down)
			te_output->moveCursor(QTextCursor::End);
	}
}

void JobView::stopJob() {
	if(!view_sweep){
		emit stopJob( current_job->info().job_id);
	}
	else{
		QMessageBox::StandardButton r =
				QMessageBox::question(this,
					"Stop sweep jobs...",
					QString("Do you want to stop %1 running/pending jobs").arg(current_sweep.job_ids.size()),
					QMessageBox::Yes | QMessageBox::No
					);
		if (r == QMessageBox::Yes) {
			for (int i=0; i<current_sweep.job_ids.size(); i++) {
				qDebug() << "Stopping job: " << current_sweep.job_ids[i];
				emit stopJob(current_sweep.job_ids[i]);
			}
		}
	}
}

void JobView::openOutputFolder() {
	QString current_folder;
    if (view_sweep)
        current_folder = config::getApplication().general_outputDir + "/" + current_sweep.subdir;
    else
		current_folder = config::getApplication().general_outputDir + "/" + current_job->info().sim_dir;
	qDebug() << "opening output folder " << current_folder;
	QUrl url = QUrl::fromLocalFile(current_folder);
	url.setScheme("file");
	QDesktopServices::openUrl(url );
}

void JobView::openTerminal() {

    QString out_dir("/");
    if(!view_sweep)
        out_dir = config::getApplication().general_outputDir + "/" +current_job->info().sim_dir;
    else
        out_dir = current_sweep.fullpath;

    qDebug() << "Open Terminal @ " << out_dir;
	QString terminal_command;
#ifdef Q_OS_LINUX
	QStringList terminal_apps( { "x-terminal-emulator" , "konsole" , "gnome-terminal" , "xfce4-terminal" , "terminal"});

	for (auto app : terminal_apps) {
		terminal_command = config::getPathToExecutable(app);
		if (!terminal_command.isEmpty()) {
			QProcess::startDetached(terminal_command, QStringList(), out_dir);
			return;
		}
	}
// 	if ( ! (terminal_command = config::getPathToExecutable("konsole")).isEmpty() ) {
// 		QProcess::startDetached(terminal_command, QStringList() << QString("--workdir") << out_dir, out_dir);
// 		return;
// 	}
// 	if ( ! (terminal_command = config::getPathToExecutable("gnome-terminal")).isEmpty() ) {
// 		QProcess::startDetached(terminal_command, QStringList() << QString("--working-directory=")+out_dir << out_dir, out_dir);
// 		return;
// 	}
// 	if ( ! (terminal_command = config::getPathToExecutable("terminal")).isEmpty() ) {
// 		QProcess::startDetached(terminal_command, QStringList(), out_dir);
// 		return;
// 	}
#elif defined Q_OS_WIN32
	if ( ! (terminal_command = config::getPathToExecutable("PowerShell.exe")).isEmpty() ) {
		QProcess::startDetached(terminal_command, QStringList(), out_dir);
		return;
	}
	if ( ! (terminal_command = config::getPathToExecutable("cmd.exe")).isEmpty() ) {
		QProcess::startDetached(terminal_command, QStringList() << QString("/E:ON") << QString("/F:ON") , out_dir);
		return;
	}
#else //might be an apple?
	if ( ! (terminal_command = config::getPathToExecutable("open")).isEmpty() ) {
		// QProcess::startDetached(terminal_command, QStringList() << "-a" << "Terminal.app" << out_dir, out_dir);
		QProcess::startDetached(terminal_command, QStringList() << "-a" << "Terminal.app" << ".", out_dir);
		return;
	}
#endif

	QMessageBox::warning(NULL, "Morpheus error", "Unable to find a suitable terminal application!" );
}

//---------------------------------------------------------------------------------------------------



void JobView::fileClicked(QModelIndex idx) {
    QString file_name = dir_model->fileName(idx);
// 	qDebug() << "fileClicked: " << QString(file_name);
    if (file_name.endsWith(".xml") || file_name.endsWith(".xml.gz")) {
        if (QMessageBox::question(this, "Open Snapshot","Do you want to open the snapshot as a new model?", QMessageBox::Yes, QMessageBox::No ) == QMessageBox::Yes){
            config::openModel(dir_model->filePath(idx));
        }
    }
    else {
        QDesktopServices::openUrl(QUrl::fromLocalFile(dir_model->filePath(idx)));
    }
}

void JobView::resizeGraphicsPreview() {
	QList <QGraphicsItem* > items = imagePreview->items();
	if (!items.empty()) {
		imagePreview->fitInView(items[0],Qt::KeepAspectRatio);
		imagePreview->scene()->setSceneRect(items[0]->boundingRect());
	}
	
}

void JobView::previewSelectedFile(const QModelIndex & selected, const QModelIndex & deselected){

	QString file_name = dir_model->filePath(selected);
	
	QImageReader reader;
	reader.setFileName(file_name);
	if (! reader.format().isEmpty()) {
// 		qDebug() <<  reader.format();
		QImage image = reader.read();
		if (!image.isNull() ){
			imagePreview->scene()->clear();
// 			qDebug() << "previewing image: Filename: " << QString(file_name) << " count: " << reader.imageCount();
			QGraphicsPixmapItem* item = new QGraphicsPixmapItem(QPixmap::fromImage(image));
			item->setTransformationMode(Qt::SmoothTransformation);
			imagePreview->scene()->addItem(item);
			previewStack->show();
			previewStack->setCurrentWidget(imagePreview); 
			emit imagePreviewChanged();
		} else {
			previewStack->hide();
		}
	}
	else
	{
		/* OTHERWISE, DISPLAY TEXT FILE*/
 		QFile file(file_name);
		QFileInfo file_info(file);
		if( !( file_name.endsWith(".txt") || file_name.endsWith(".log") || file_name.endsWith(".dat") 
            || file_name.endsWith(".plt") || file_name.endsWith(".xml") ||  file_name.endsWith(".out")
            || file_name.endsWith(".err") || file_name.endsWith(".gp")  ||  file_name.endsWith(".csv"))
			|| file_info.size() == 0 ){
//			file_name.endsWith(".gz") || file_name.endsWith(".zip") || file_info.isExecutable() ){
			previewStack->hide();
			return;
		}
		
		if( ( file_info.size() / (1024*1024) ) > 10){
			textPreview->setPlainText("<< File too large for preview (>10 Mb). >>");
			textPreview->setWordWrapMode(QTextOption::NoWrap);
			previewStack->setCurrentWidget(textPreview);
			previewStack->show();
			return;
		}
		else if (file.open(QFile::ReadOnly | QFile::Text)){
			textPreview->setPlainText(file.readAll());
			textPreview->setWordWrapMode(QTextOption::NoWrap);
			previewStack->setCurrentWidget(textPreview);
			previewStack->show();
		}
	}


	return;
} 

void JobView::restoreSweep() {
	QSqlQuery query;
	query.prepare(QString("SELECT sweeps.* FROM sweep_jobs JOIN sweeps ON sweeps.id=sweep_jobs.sweep WHERE sweep_jobs.job=%1").arg(current_job->jobID()));
	if ( ! query.exec())
		qDebug() << "Unable to get job info " <<query.lastQuery() ;
    if ( query.next() ) {
		int id_idx = query.record().indexOf("id");
		qDebug() << "Requesting sweep id " << id_idx;
		emit selectSweep( query.value(id_idx).toInt());
	}
}

void JobView::makeMovie(){

    MovieDialog* md;
    if( view_sweep ){
        qDebug() << "SWEEP: Path: " << current_sweep.fullpath;
        md = new MovieDialog( current_sweep.fullpath, this, true);
    }
    else{
        qDebug() << "NO SWEEP: Path: " << dir_model->rootPath();
        md = new MovieDialog( dir_model->rootPath(), this, false);
    }

}

void JobView::makeTable(){

	ImageTableDialog* table = new ImageTableDialog( current_sweep.fullpath, this );

}

JobView::~JobView(){}
