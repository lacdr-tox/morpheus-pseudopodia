#include "jobview.h"
JobQueueView::JobQueueView ( QWidget* parent) : QSplitter ( parent )
{
	jobQueueTreeView = new QTreeView(this);
	JobQueue* job_queue = config::getJobQueue();
	connect(this,SIGNAL(stopJob(int)),job_queue,SLOT(stopProcess(int)));
	connect(this,SIGNAL(debugJob(int)),job_queue,SLOT(debugProcess(int)));
	connect(this,SIGNAL(removeJob(int,bool)),job_queue,SLOT(removeProcess(int,bool)));
	connect(job_queue,SIGNAL(criticalMessage(QString,int)),this,SLOT(addCriticalMessage(QString,int)));
	connect(job_queue,SIGNAL(statusMessage(QString,int)),this,SLOT(addMessage(QString,int)));

	job_view_model = new JobViewModel(job_queue,this);
	connect(job_view_model,SIGNAL(rowsInserted(const QModelIndex&,int,int)),this,SLOT(addJob(const QModelIndex& ,int)));

	jobQueueTreeView->setModel(job_view_model);
// 	connect(jobQueueTreeView->selectionModel(),SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(selectJob(const QModelIndex&)));
	connect(jobQueueTreeView,SIGNAL(clicked(const QModelIndex&)), this, SLOT(selectJob(const QModelIndex&)) );

	jobQueueTreeView->setSortingEnabled(true);
	jobQueueTreeView->sortByColumn(0,Qt::AscendingOrder);
	jobQueueTreeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
	jobQueueTreeView->setItemDelegateForColumn(1,new JobProgressDelegate());
	jobQueueTreeView->setItemDelegateForColumn(2,new JobProgressDelegate());
	jobQueueTreeView->setColumnWidth(0,200);

	jobQueueMenu = new QMenu();
	jobQueueMenu->addAction(QThemedIcon("list-remove",QIcon(":/list-remove.png")),"Remove");
	//jobQueueMenu->addAction(QThemedIcon("media-playback-start",QIcon(":/start.png")),"Continue");
	jobQueueMenu->addAction(QThemedIcon("media-playback-stop",QIcon(":/stop.png")),"Stop");
	jobQueueMenu->addAction(QThemedIcon("system-run",QIcon(":/debug.png")),"Debug");
	jobQueueMenu->addSeparator();

	jobQueueGroupingMenu = new QMenu();
	jobQueueGroupingMenu->addAction("Model");
	jobQueueGroupingMenu->addAction("Process Id");
	jobQueueGroupingMenu->addAction("State");
	jobQueueGroupingMenu->addAction("Sweep");
	jobQueueGroupingMenu->setTitle("Group By");

	for ( int i=0; i<jobQueueGroupingMenu->actions().size(); i++) {

		jobQueueGroupingMenu->actions()[i]->setCheckable(true);jobQueueGroupingMenu->actions().size();
		if (i==job_view_model->getGroupBy())
			jobQueueGroupingMenu->actions()[i]->setChecked(true);jobQueueGroupingMenu->actions().size();
	}
	jobQueueMenu->addMenu(jobQueueGroupingMenu);
	connect(jobQueueMenu,SIGNAL(triggered(QAction*)),this,SLOT(jobQueueMenuTriggered(QAction*)));
	jobQueueTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(jobQueueTreeView,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(showJobQueueMenu(QPoint)));

	jobQueueStatusText = new QListWidget();
	jobQueueStatusText->setSelectionMode(QAbstractItemView::NoSelection);
	jobQueueStatusText->setStyleSheet( "QListWidget::item { border-bottom: 1px solid grey; }" );
	connect(jobQueueStatusText,SIGNAL(itemClicked(QListWidgetItem*)),this, SLOT(selectStatus(QListWidgetItem *)));

	jobQueueProgressBar = new QProgressBar();
	jobQueueProgressBar->setRange(0,100);

	this->setLineWidth(2);
	this->setOrientation(Qt::Vertical);
	this->addWidget(jobQueueTreeView);
	this->addWidget(jobQueueStatusText);
	this->addWidget(jobQueueProgressBar);
	jobQueueProgressBar->hide();
	this->setStretchFactor(0,8);
	this->setStretchFactor(1,2);
	this->setStretchFactor(2,1);

}

void JobQueueView::selectStatus(QListWidgetItem * message_item) {
	QString message = message_item->text();	
	QRegExp xml_path("XMLPath: ([\\w\\[\\]/]+)");
	if (xml_path.indexIn(message) != -1) {
// 		qDebug() << "Matched XMLPath is " << xml_path.cap(1);
		JobQueue* job_queue = config::getJobQueue();
		auto job = job_queue->getJobs()[message_item->data(Qt::UserRole).toInt()];
// 		QString modelname = job->info().model_file;
		int model_index = config::openModel(job->info().model_file);
		emit erronousXMLPath(xml_path.cap(1), model_index);
	}
	
}

void JobQueueView::addCriticalMessage ( QString message, int job_id )
{
	QListWidgetItem* item = new QListWidgetItem(QThemedIcon("dialog-error",QIcon(":/stop.png")),message);
	item->setData(Qt::UserRole,job_id);
	jobQueueStatusText->addItem(item);
	if (jobQueueStatusText->count()> maxMessageItems)
		jobQueueStatusText->takeItem(0);
	jobQueueStatusText->setCurrentItem(item);

	bool popup = config::getJobQueue()->getJobs()[job_id]->info().resource != ProcessInfo::remote;
	if (popup) {
		selectStatus(item);
		QMessageBox::critical(this,"Simulation error", message);
	}
}


void JobQueueView::addMessage ( QString message, int progress )
{
	QListWidgetItem* item = new QListWidgetItem(message);
	jobQueueStatusText->addItem(item);
	if (jobQueueStatusText->count()> maxMessageItems)
		jobQueueStatusText->takeItem(0);
	jobQueueStatusText->setCurrentItem(item);
	
	if (progress >= 0) {
		jobQueueProgressBar->setValue(progress);
		jobQueueProgressBar->show();
		progress_shut_timer.stop();
		progress_shut_timer.singleShot(10000, jobQueueProgressBar, SLOT(hide()));
	}
}


void JobQueueView::addJob(const QModelIndex & parent, int child) {

	QModelIndex f_index = jobQueueTreeView->model()->index(child,0,parent);

	QList<int> job_ids = job_view_model->getJobIds(f_index);
	if (job_ids.empty()) {
// 		qDebug() << "Received MainWindow::jobAdded but no job found for index " << f_index;
// 		qDebug() << "Parent was " << parent << ". Row was " << child;
		return;
	}
	else {
		QSharedPointer<abstractProcess> process = config::getJobQueue()->getJobs()[job_ids.first()];
		if (! process) {
			qDebug() << "Oops in JobQueueView::addJob() : Process " <<job_ids.first() << " is empty";
			return;
		}
		if (process->state() == ProcessInfo::PEND ||process->state() == ProcessInfo::RUN  ) {
			if ( ! parent.isValid() && f_index.isValid()) {
				jobQueueTreeView->setExpanded(f_index,true);
				f_index = jobQueueTreeView->model()->index(0,0,f_index);
				jobQueueTreeView->setCurrentIndex(f_index);
			} else {
				jobQueueTreeView->setExpanded(parent,true);
				jobQueueTreeView->setCurrentIndex(f_index);
			}
			jobSelected(job_ids.first());
		}
		else {
//            qDebug() << "Received MainWindow::jobAdded but job " <<  process->info().job_id << " is " << process->getStateString();
		}
	}
}
void JobQueueView::showJobQueueMenu ( QPoint p )
{
	if ( ! jobQueueTreeView->selectionModel()->selection().contains(jobQueueTreeView->indexAt(p)))
		jobQueueTreeView->setCurrentIndex(jobQueueTreeView->indexAt(p));

	QSet<int> c_job_ids;
	QModelIndexList indices = jobQueueTreeView->selectionModel()->selectedRows();
	for (int i=0; i<indices.size(); i++ ) {
		c_job_ids.unite(QSet<int>::fromList(job_view_model->getJobIds(indices[i])));
	}
	QList<int> job_ids = c_job_ids.toList();

	const QMap<int, QSharedPointer<abstractProcess> >& jobs  = config::getJobQueue()->getJobs();
	int running_pending_jobs = 0;
	int killed_jobs = 0;
	int debuggable_jobs = 0;
	for (int i=0; i<job_ids.size(); i++) {
		if (jobs[job_ids[i]]->state() == ProcessInfo::RUN || jobs[job_ids[i]]->state() == ProcessInfo::PEND )
			running_pending_jobs++;
		if (jobs[job_ids[i]]->state() == ProcessInfo::EXIT)
			killed_jobs++;
		if (jobs[job_ids[i]]->state() != ProcessInfo::PEND && jobs[job_ids[i]]->state() != ProcessInfo::RUN && jobs[job_ids[i]]->state() == ProcessInfo::EXIT  )
			debuggable_jobs++;
	}

	// Stopping Jobs is only enabled when there are running or pending jobs in the selection
// jobQueueMenu->actions()[1]->setEnabled((killed_jobs>0) && false);			// Continue
	jobQueueMenu->actions()[1]->setEnabled((running_pending_jobs>0));	// Stop
	jobQueueMenu->actions()[2]->setEnabled((debuggable_jobs==1));		// Debug

//     jobQueueMenu->addAction(QThemedIcon("list-remove",QIcon(":/list-remove.png")),"Remove");
//     jobQueueMenu->addAction(QThemedIcon("media-playback-start",QIcon(":/start.png")),"Continue");
//     jobQueueMenu->addAction(QThemedIcon("media-playback-stop",QIcon(":/stop.png")),"Stop");
//     jobQueueMenu->addAction(QThemedIcon("system-run",QIcon(":/start.png")),"Debug");

	jobQueueMenu->exec(jobQueueTreeView->mapToGlobal(p));
}


void JobQueueView::selectJob ( const QModelIndex& index)
{
	QList<int> job_ids = job_view_model->getJobIds(index);
	if (job_view_model->isGroup(index) && job_view_model->getGroupBy() == JobViewModel::SWEEP) {
		emit sweepSelected(job_ids);
	}
	else {
		emit jobSelected(job_ids.last());
	}
}

void JobQueueView::jobQueueMenuTriggered(QAction * a ) {

	int current_grp=0;
	if (a->text() == "Model") {
		if (job_view_model->getGroupBy() == JobViewModel::SWEEP) {
			jobQueueTreeView->setColumnWidth(0,jobQueueTreeView->columnWidth(1));
			jobQueueTreeView->setColumnWidth(1,jobQueueTreeView->columnWidth(2));
		}
		job_view_model->setGroupBy(JobViewModel::MODEL);
		current_grp=0;
	}
	else if (a->text() == "Process Id") {
		if (job_view_model->getGroupBy() == JobViewModel::SWEEP) {
			jobQueueTreeView->setColumnWidth(0,jobQueueTreeView->columnWidth(1));
			jobQueueTreeView->setColumnWidth(1,jobQueueTreeView->columnWidth(2));
		}
		job_view_model->setGroupBy(JobViewModel::PROC_ID);
		current_grp=1;
	}
	else if (a->text() == "State") {
		if (job_view_model->getGroupBy() == JobViewModel::SWEEP) {
			jobQueueTreeView->setColumnWidth(0,jobQueueTreeView->columnWidth(1));
			jobQueueTreeView->setColumnWidth(1,jobQueueTreeView->columnWidth(2));
		}
		job_view_model->setGroupBy(JobViewModel::STATE);
		current_grp=2;
	}
	else if (a->text() == "Sweep") {
		bool update_layout = job_view_model->getGroupBy() != JobViewModel::SWEEP;
		int cw1;int cw0;
		if (update_layout) {
			cw0 = jobQueueTreeView->columnWidth(0);
			cw1 = jobQueueTreeView->columnWidth(1);
		}
		job_view_model->setGroupBy(JobViewModel::SWEEP);
		if (update_layout) {
			jobQueueTreeView->setColumnWidth(2,cw1);
			jobQueueTreeView->setColumnWidth(1,cw0);
			jobQueueTreeView->setColumnWidth(0,100);
		}

		current_grp=3;
	}
//    else if (a->text() == "Continue") {
//        QMessageBox::information(this,
//                    "Hihi...",
//                    QString("Sorry, can't continue job") );
//    }
	else if (a->text() == "Remove") {
		QSet<int> c_job_ids;
		QModelIndexList indices = jobQueueTreeView->selectionModel()->selectedRows();
		for (int i=0; i<indices.size(); i++ ) {
			c_job_ids.unite(QSet<int>::fromList(job_view_model->getJobIds(indices[i])));
		}

		QList<int> job_ids = c_job_ids.toList();
		int running_jobs = 0;
		const QMap<int, QSharedPointer<abstractProcess> >& jobs  = config::getJobQueue()->getJobs();
		for (int i=0; i<job_ids.size(); i++) {
			if (jobs[job_ids[i]]->state() == ProcessInfo::RUN) running_jobs++;
		}

		QString message;
		message = QString("%1 job%2 will be removed").arg(job_ids.size()).arg(job_ids.size()>1?"s":"");
		if (running_jobs>0)
			message += QString(", whereof %1 %2 running.").arg(running_jobs).arg(running_jobs==1?"is":"are");
		else
			message += ".";
		message += "\n\nDo you also want to remove the data along with the jobs?";
		QMessageBox::StandardButton rbutton;
		rbutton = QMessageBox::question(this,"Remove Jobs", message, QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Yes );

		bool remove_data;
		if ( rbutton == QMessageBox::Cancel ) {
			return;
		}
		else if ( rbutton == QMessageBox::Yes ) {
			remove_data = true;
		}
		else {
			remove_data = false;
		}

		QDialog* remove_dialog = new QDialog(0, Qt::Dialog);
		remove_dialog->setModal(true);
		remove_dialog->setWindowTitle("Removing jobs");
		remove_dialog->setLayout(new QVBoxLayout());
		remove_dialog->layout()->setAlignment(Qt::AlignHCenter);

		QProgressBar* pBar = new QProgressBar(this);
		pBar->setRange(0, job_ids.size());
		remove_dialog->layout()->addWidget(pBar);
		remove_dialog->open();

		for (int i=0; i<job_ids.size(); i++) {
			pBar->setValue(i);
			pBar->update();

			emit removeJob(job_ids[i], remove_data);
		}
		remove_dialog->close();

	}
	else if (a->text() == "Stop") {
		QSet<int> c_job_ids;
		QModelIndexList indices = jobQueueTreeView->selectionModel()->selectedRows();
		for (int i=0; i<indices.size(); i++ ) {
			c_job_ids.unite(QSet<int>::fromList(job_view_model->getJobIds(indices[i])));
		}
		QList<int> job_ids = c_job_ids.toList();
		if (job_ids.size() == 1) {
			emit stopJob(job_ids.front());
		}
		else if (job_ids.size() > 1) {
			const QMap<int, QSharedPointer<abstractProcess> >& jobs  = config::getJobQueue()->getJobs();
			QList<int> running_pending_jobs;
			for (int i=0; i<job_ids.size(); i++) {
				if (jobs[job_ids[i]]->state() == ProcessInfo::RUN || jobs[job_ids[i]]->state() == ProcessInfo::PEND)
					running_pending_jobs.append(job_ids[i]);
			}
			if (running_pending_jobs.isEmpty()) {
				// No running jobs in selection
				QMessageBox::information(this,"Information","No Running jobs in selection.",QMessageBox::Ok);
			}
			else {
				QMessageBox::StandardButton r =
						QMessageBox::question(this,
							"Stop Jobs ..",
							QString("Do you want to stop %1 running/pending jobs").arg(running_pending_jobs.size()),
							QMessageBox::Yes | QMessageBox::No
							);
				if (r == QMessageBox::Yes) {
					QSqlQuery("BEGIN TRANSACTION");
					for (int i=0; i<running_pending_jobs.size(); i++) {
						emit stopJob(running_pending_jobs[i]);
					}
					QSqlQuery("COMMIT TRANSACTION");
				}
			}
		}
	}
	else if (a->text() == "Debug") {
		QSet<int> c_job_ids;
		QModelIndexList indices = jobQueueTreeView->selectionModel()->selectedRows();
		for (int i=0; i<indices.size(); i++ ) {
			c_job_ids.unite(QSet<int>::fromList(job_view_model->getJobIds(indices[i])));
		}
		QList<int> job_ids = c_job_ids.toList();
		if (job_ids.size() == 1) {
			emit debugJob(job_ids.front());
		}
		else if (job_ids.size() > 1) {
			QMessageBox::warning(this,"Warning","Cannot debug multiple jobs.\n\nSelect a single job to debug.",QMessageBox::Ok);
		}

	}

	for (int i=0; i<4; i++) {
		if (i==current_grp)
			jobQueueGroupingMenu->actions()[i]->setChecked(true);
		else
			jobQueueGroupingMenu->actions()[i]->setChecked(false);
	}
}


JobViewModel::Grouping JobQueueView::getGroupBy()
{
	return job_view_model->getGroupBy();
}


void JobQueueView::setGroupBy ( JobViewModel::Grouping g )
{
	job_view_model->setGroupBy(g);
}
