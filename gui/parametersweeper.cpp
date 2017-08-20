#include "parametersweeper.h"
// #include "mainwindow.h"

parameterSweeper::parameterSweeper()
{
	QVBoxLayout *main_layout = new QVBoxLayout(this);
	this->setLayout(main_layout);

	QGridLayout *grid_layout = new QGridLayout();
	main_layout->addLayout(grid_layout);
	
	QHBoxLayout *line_layout = new QHBoxLayout();
	main_layout->addLayout(line_layout);
	QLabel * sweep_label = new QLabel("Name of the Sweep:");
	grid_layout->addWidget(sweep_label,0,0,1,1);
// 	line_layout->addWidget(scan_label);
	sweep_name = new QLineEdit();
	sweep_name -> setText("Sweep");
	grid_layout->addItem(new QSpacerItem(10,0,QSizePolicy::Preferred,QSizePolicy::Preferred),0,1,2,1);
	grid_layout->addWidget(sweep_name,0,2,1,1);
	grid_layout->addItem(new QSpacerItem(20,0,QSizePolicy::MinimumExpanding,QSizePolicy::Preferred),0,3,2,1);
	

	QLabel * nsweep_label = new QLabel("Number of Jobs:");
	grid_layout->addWidget(nsweep_label,1,0,1,1);
	n_sweeps = new QLineEdit();
	n_sweeps -> setText("0");
	n_sweeps -> setReadOnly(true);
	grid_layout->addWidget(n_sweeps,1,2,1,1);
	
	main_layout->addSpacerItem(new QSpacerItem(0,20,QSizePolicy::Minimum,QSizePolicy::Preferred));
	QLabel * param_label = new QLabel("Parameters and Value Sets");
	main_layout->addWidget(param_label);
	
	param_sweep_view = new QTreeView(this);
	param_sweep_view->setTextElideMode(Qt::ElideLeft);

	param_sweep_view->setDragDropMode(QAbstractItemView::InternalMove);
	param_sweep_view->setSelectionBehavior(QAbstractItemView::SelectRows);
	param_sweep_view->setSelectionMode(QAbstractItemView::SingleSelection);
	edit_delegate = QSharedPointer<attrController>(new attrController(this, NULL,true));
	param_sweep_view->setItemDelegate(edit_delegate.data());
	main_layout->addWidget(param_sweep_view);
	

	
	
	QLabel *lb_valueInformation = new QLabel( ""
"Syntax:\n"
"List:\tvalue1; value2; value3\n"
"Range:\tmin:stepping:max\n"
"\n\n"
" Stepping\t\tDescription\t\tExample\n\n"
" [width]\t\tinterval spacing\t\t0:2:10 -> 0;2;4;6;8;10\n"
" #[count]\t\tnumber of intervals\t\t0:#2:10 -> 0;5;10\n"
" #[count]log\tlogarithmic scale\t\t1:#2log:100 -> 1;10;100\n"
"\n\n"
"Pairwise parameters:\n"
" Grouped parameters are sweeped pairwise instead of crosswise.\n\n"
);
	main_layout->addWidget(lb_valueInformation);
	main_layout->addStretch();

	connect(param_sweep_view,SIGNAL(doubleClicked(QModelIndex)),this, SLOT(paramDoubleClicked(QModelIndex)));
    
}

//------------------------------------------------------------------------------


void parameterSweeper::paramDoubleClicked ( const QModelIndex& index )
{
	AbstractAttribute* attrib = model->param_sweep.getAttribute(index);
	if (index.column() == 2 && attrib) {
		edit_delegate->setAttribute(attrib);
	}
	if (index.column() == 0 && attrib) {
		emit attributeDoubleClicked(attrib);
	}
}


//------------------------------------------------------------------------------

void parameterSweeper::updateJobCount()
{
	QList <AbstractAttribute* > params;
	QList <QStringList > values;
	model->param_sweep.createJobList(params,values);
    n_sweeps->setText(QString::number(values.size()));
}

//------------------------------------------------------------------------------

void parameterSweeper::selectModel(int index) {
    if (model) {
		model->disconnect( SIGNAL(dataChanged(QModelIndex,QModelIndex)));
		model->disconnect( SIGNAL(layoutChanged()));
    }
    model = config::getOpenModels()[index];
	sweep_name->setText(model->rootNodeContr->getModelDescr().title + "_sweep");
	param_sweep_view->setModel(&model->param_sweep);
	param_sweep_view->setColumnWidth(0,300);
	param_sweep_view->setColumnWidth(1,150);
	param_sweep_view->setColumnWidth(2,200);

	connect(&model->param_sweep, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(updateJobCount()));
	connect(&model->param_sweep, SIGNAL(layoutChanged()), this, SLOT(updateJobCount()));
	updateJobCount();
}

void parameterSweeper::loadSweep(int sweep_id)
{
	QSqlQuery query;
	query.prepare(QString("SELECT * FROM sweeps WHERE id=%1").arg(sweep_id));
	if ( ! query.exec())
		qDebug() << "Unable to get job info " <<query.lastQuery() ;
    if ( query.next() ) {
		int name_idx = query.record().indexOf("name");
		int paramData_idx = query.record().indexOf("paramData");

		if ( ! query.value(paramData_idx).isNull()) {
			QList<QList<QPair<QString,QString> > > param_data;
			QByteArray data = query.value(paramData_idx).toByteArray() ;
			model->param_sweep.restore(data,model->rootNodeContr);
		}
	}
}

//------------------------------------------------------------------------------
/*
void parameterSweeper::restoreAttrs() {
    model->param_sweep.clear();
    for(int i = 0; i < model->sweeperAttributes.size(); i++) {
        QList<AbstractAttribute*> list = model->sweeperAttributes.at(i);
        bool isPair = false;
		QTreeWidgetItem* pair_parent = NULL;
        if(list.size() > 1) {
			pair_parent = new QTreeWidgetItem(QStringList("pair"));
            trW->addTopLevelItem(pair_parent);
            isPair = true;
        }
        for(int j = 0; j < list.size(); j++) {
            AbstractAttribute *a = list.at(j);
            QStringList list_val;
            QString type_name  = a->getType()->name;
            type_name.replace("cpm","");

            list_val << a->getXMLPath() << type_name <<  a->get();

            QTreeWidgetItem *item = new QTreeWidgetItem(list_val);
// 			item->setData(3,Qt::DecorationRole,QThemedIcon("list-remove",QIcon(":/list-remove")));
            item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);

            if(isPair) {
                pair_parent->addChild(item);
            }
            else {
                trW->addTopLevelItem(item);
            }

            map_attrToItem[a] = item;

            QSharedPointer<delegate_class> dele(new delegate_class(this, a,true));
            trW->setItemDelegateForRow( trW->topLevelItemCount()-1, dele.data());

            map_itemToDele[item] = dele;
            item->setData(2, 0, model->sweeperValues[a]);
        }
    }

    updateValueList();
}*/

//------------------------------------------------------------------------------

int parameterSweeper::newSweepID() {
	QSqlQuery q;
	bool ok = q.exec("SELECT MAX(id) FROM sweeps");
	if(!ok) qDebug() << "Error getting MAX(id) from sweeps database: " << q.lastError();
	q.next();
	QSqlRecord r = q.record();
	int index = r.indexOf("MAX(id)");
	int last_id = r.value( index ).toInt();
	int new_id = last_id + 1;
	qDebug() << "newSweepID = " << new_id << " | index = " << index ;
	q.finish();
	return new_id;
}

//------------------------------------------------------------------------------

void parameterSweeper::submitSweep()
{
	// at first, squeeze all params into a data baloon
	QByteArray param_data = model->param_sweep.store();
	
    // secondary, create the cross- and pairwise parameter combinations
	QList<AbstractAttribute* > parameters;
	QList<QStringList> parameter_sets;
	model->param_sweep.createJobList(parameters, parameter_sets);
    if (parameter_sets.empty()) {
        QMessageBox(QMessageBox::Warning,QString("Action denied"),QString("No valid parameter sets defined!!"));
        return;
    }
    
    jobSummary summary(parameters, parameter_sets, sweep_name->text());
    summary.exec();
    if (summary.result() == QDialog::Accepted) {

		parameter_sets = summary.getJobList();
		int sweep_id = newSweepID();
		QString sweep_name = summary.getGroupName();
		sweep_name.replace(" ","_");
		QString sweep_sub_dir = sweep_name + "_" + QString::number(sweep_id);
		QString sweep_header;
		
		QTextStream ts_header(&sweep_header);
		ts_header << "# Parametersweep Date: " << QDate::currentDate().toString() << " Time: " << QTime::currentTime().toString() << endl;
		ts_header << "# Starting " << parameter_sets.size() << " Jobs" << endl;
		for (int s=0; s<parameters.size(); s++) {
			ts_header << "# Parameter P" << s << " : " << parameters[s]->getXMLPath() << endl;
		}
		ts_header << "# JOB LIST" <<endl;
		ts_header << "# Folder";
		for (int s=0; s<parameters.size(); s++) {
			ts_header << "\tP" << s;
		}
		ts_header << endl;
	
		QSqlQuery insert_sweep;
		insert_sweep.exec("BEGIN TRANSACTION");
		insert_sweep.prepare(
			"INSERT INTO sweeps( id, name, header, subDir, paramData)"
			"VALUES( :id, :name, :header, :subDir, :paramData)"
					 );
		insert_sweep.bindValue(":id", sweep_id);
		insert_sweep.bindValue(":name", sweep_name);
		insert_sweep.bindValue(":header", sweep_header);
		insert_sweep.bindValue(":subDir", sweep_sub_dir);
		insert_sweep.bindValue(":paramData", QVariant(param_data));
		if ( ! insert_sweep.exec() ) {
			qDebug() << "Unable to insert Sweep into SQL DB: " << insert_sweep.lastError();
		}

		QSqlQuery insert_sweep_job;
		insert_sweep_job.prepare(
			"INSERT INTO sweep_jobs(sweep, paramSet) "
			"VALUES(:sweep, :paramSet)"
		);
		// now create all the jobs (set values and create modeldescriptions (.xml))
		for (int i=0; i<parameter_sets.size(); i++) {
			QStringList sweep_params;
			// Create a parameter set and save it to the DB
			for (int j=0; j<parameters.size(); j++) {
				sweep_params.append(parameter_sets[i][j]);
			}
			qDebug() << sweep_params;
			insert_sweep_job.bindValue(":sweep", sweep_id);
			insert_sweep_job.bindValue(":paramSet", sweep_params.join(";"));
			if ( ! insert_sweep_job.exec() ) {
				qDebug() << "Unable to insert SweepJob into SQL DB: " << insert_sweep_job.lastError();
			}
			int sweep_job = insert_sweep_job.lastInsertId().toInt();
			qDebug() << "Received sweep_job_id " << sweep_job;
		}
		insert_sweep.exec("COMMIT TRANSACTION");
		emit createSweep(model, sweep_id);
	}
}

//------------------------------------------------------------------------------

parameterSweeper::~parameterSweeper()
{
}
