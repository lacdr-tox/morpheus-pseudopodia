#include     "jobviewmodel.h"

JobViewModel::JobViewModel(JobQueue* jobs, QObject *parent) :
    QAbstractItemModel(parent)
{
// 	qDebug() << "JobViewModel thread " << QThread::currentThreadId();
    this->jobs =jobs;
    connect(jobs,SIGNAL(processChanged(int)),this,SLOT(changeJob(int)));
    connect(jobs,SIGNAL(processAdded(int)),this,SLOT(addJob(int)));
    connect(jobs,SIGNAL(processRemoved(int)),this,SLOT(removeJob(int)));
    current_grouping = MODEL;
    sort_order = Qt::AscendingOrder;
	sort_column = 0;
    root_item = QSharedPointer<TreeItem>(new TreeItem);
    root_item->name = "root";
    root_item->type = GROUP;
//    qDebug() << "JobViewModel::JobViewModel";
	
//     if (QIcon::hasThemeIcon("process-working")) icon_for_state[ProcessInfo::RUN] = QIcon::fromTheme("process-working");
	if (QIcon::hasThemeIcon("process-working-symbolic")) icon_for_state[ProcessInfo::RUN] = QIcon::fromTheme("process-working-symbolic");
	else icon_for_state[ProcessInfo::RUN] = QIcon(":/icons/process-working.svg");
	
// 	if (QIcon::hasThemeIcon("process-completed")) icon_for_state[ProcessInfo::DONE] = QIcon::fromTheme("process-completed");
	if (QIcon::hasThemeIcon("process-completed-symbolic")) icon_for_state[ProcessInfo::DONE] = QIcon::fromTheme("process-completed-symbolic");
	else icon_for_state[ProcessInfo::DONE] = QIcon(":/icons/process-completed.svg");
	
// 	if (QIcon::hasThemeIcon("process-stop")) icon_for_state[ProcessInfo::EXIT] = QIcon::fromTheme("process-stop");
	if (QIcon::hasThemeIcon("process-stopped-symbolic")) icon_for_state[ProcessInfo::EXIT] = QIcon::fromTheme("process-stopped-symbolic");
	else icon_for_state[ProcessInfo::EXIT] = QIcon(":/icons/process-stopped.svg");
	
// 	if (QIcon::hasThemeIcon("process-pending")) icon_for_state[ProcessInfo::PEND] = QIcon::fromTheme("process-pending");
	if (QIcon::hasThemeIcon("process-pending-symbolic")) icon_for_state[ProcessInfo::PEND] = QIcon::fromTheme("process-pending-symbolic");
	else icon_for_state[ProcessInfo::PEND] = QIcon(":/icons/process-pending.svg");

	updateLayout();
	
	
}

QModelIndex JobViewModel::index( int row, int column, const QModelIndex & parent ) const {
    TreeItem* item = indexToItem(parent);
	if(row >= item->childs.size())
		row = item->childs.size()-1;
    Q_ASSERT(row < item->childs.size());
    QModelIndex idx =  createIndex(row,column,item->childs[row].data());
//    qDebug() << "JobViewModel::index " << row << "," << column << "," << parent.internalId();
//    qDebug() << "JobViewModel::index created " << idx.row() << ", " << idx.column() << ", " << idx.internalId();
    return idx;

}

QModelIndex JobViewModel::parent( const QModelIndex& index) const {
    QModelIndex idx;
    if (index.isValid()) {
        TreeItem* i  = indexToItem(index);
        if (i->parent != root_item) {
            int row = i->parent->parent->childs.indexOf(i->parent);
            idx = createIndex(row,0,i->parent.data());
        }
//        qDebug() << "JobViewModel::parent of " << index.row() << "," << index.column() << "," << index.internalId() << (i.type == GROUP ? "GROUP" : "PROCESS") << " >> " << idx.row() << ", " << idx.column() << ", " << idx.internalId();
    }
    return idx;
}

void JobViewModel::setGroupBy(Grouping grouping) {
    if (current_grouping != grouping) {
       current_grouping = grouping;
       updateLayout();
    }
}

JobViewModel::Grouping JobViewModel::getGroupBy() {
    return current_grouping;
}

int JobViewModel::columnCount(const QModelIndex&) const {
	if (current_grouping == SWEEP)
		return 3;
	else
		return 2;
}

int JobViewModel::rowCount(const QModelIndex& parent) const {

    TreeItem* item = indexToItem(parent);
    int rows = item->childs.size();
//    qDebug() << "JobViewModel::rowCount " << parent.row() << "," << parent.column() << "," << parent.internalId() << " >> " << rows ;
    return rows;
}

void JobViewModel::sort( int column, Qt::SortOrder order )  {
	
    if (order != sort_order) {
        // swap current order
        sort_order = order;
		sort_column = column;
        updateLayout();
    }
};

QVariant JobViewModel::headerData( int section, Qt::Orientation /*orientation*/, int role) const {
    QVariant r;
    if (role == Qt::DisplayRole || role == Qt::EditRole)
        switch (section) {
        case 0:
			if (current_grouping == SWEEP )
				r= QString("Id");
			else
				r = QString("Process");
			break;
        case 1:
			if (current_grouping == SWEEP )
				r = QString("Sweep");
			else
				r = QString("Progress");
			break;
		case 2:
			if (current_grouping == SWEEP )
				r = QString("Model");
			break;
        default: r = QString("???");
			break;
        }
    return r;
}

QVariant JobViewModel::data( const QModelIndex & index, int role ) const {

	QVariant r;
	TreeItem* item = indexToItem(index);
	if ( item->type == GROUP ) {
		if (index.column() == 0) {
			if (role == Qt::DisplayRole || role == Qt::EditRole) {
					if( current_grouping == SWEEP)
						r = QVariant(item->key_nr);
					else
						r = QVariant(item->name);
			}
			if (role == Qt::DecorationRole)
				r =  QVariant(QIcon::fromTheme("folder", QIcon(":/icons/folder.png")));
		}
		if (index.column() == 1) {
			if (role == Qt::DisplayRole && current_grouping == SWEEP) {
				 r = QVariant(item->name);
			}
			if (role == Qt::ToolTipRole) {
				if (!item->hint.isEmpty()) {
					return QVariant(item->hint);
				}
			}
		}
		if (index.column() == 2) {
			 if (role == Qt::DisplayRole && current_grouping == SWEEP) {
				 r = QVariant(item->childs.first()->process->info().title);
			 }
		}
    }
    else {
		if (index.column() ==0) {
			if (role == Qt::DisplayRole || role == Qt::EditRole) {
				if (current_grouping != SWEEP)
					r = QVariant(item->name);
			}
			if (role == Qt::DecorationRole) {
				r = icon_for_state[item->process->state()];
			}
		}
		else if (index.column() == 1) {
			if (role == Qt::DisplayRole || role == Qt::EditRole) {
				if (current_grouping != SWEEP ) {
					const ProcessInfo& info = item->process->info();
					QList<QVariant> l;
					l.append(QVariant(info.progress));
					l.append(QVariant(info.run_time_string));
					l.append(QVariant(info.resource == ProcessInfo::remote));
					r = l;
				}
				else
					r= QVariant(item->name);
			}
		}
		else if (index.column() == 2 && current_grouping == SWEEP ) {
			if (role == Qt::DisplayRole || role == Qt::EditRole) {
				const ProcessInfo& info = item->process->info();
				QList<QVariant> l;
				l.append(QVariant(info.progress));
				l.append(QVariant(info.run_time_string));
				l.append(QVariant(info.resource == ProcessInfo::remote));
				r = l;
			}
		}
    }
//    qDebug() << "JobViewModel::data " << role << " " << index.row() << "," << index.column() << "," << index.internalId() << " >> " << r;
    return r;
}

QString JobViewModel::getProcessGroup( QSharedPointer<abstractProcess> p)  {
    QString key;

    return key;
}

JobViewModel::TreeItem* JobViewModel::indexToItem(const QModelIndex& idx) const {
    if (idx.isValid()) {
        return static_cast<TreeItem*>(idx.internalPointer());
    }
    else {
        return const_cast<TreeItem*>(root_item.data());
    }
}

QModelIndex JobViewModel::indexOfJob(int job) const {
    if (job_indices.contains(job)) {
        return job_indices[job];
    }
    else return QModelIndex();
}

void JobViewModel::addJob(int job_gid) {
    QSharedPointer<TreeItem>  job_item(new TreeItem);
    job_item->type = PROCESS;
    job_item->process = jobs->getJobs()[job_gid];
	if (!job_item->process) {
		qDebug() << "Oops: invalid process for job id " << job_gid;
		return;
	}
    job_item->name = QString("Job %1").arg(job_item->process->jobID());
	
	QString group_name; // What is going to displayed as a title
	QString group_key_name; // The grouping key used for sorting
	int group_key_app; // running id, get sorting of appended id's correctly sweep_1 sweep_102 sweep_2
	switch (current_grouping) {
		case PROC_ID:
			group_name = QString::number(job_gid/100).append("xx");
			group_key_name = "j";
			group_key_app = job_gid/100;
			
			break;
		case MODEL:
		{
			group_name = job_item->process->info().title;
			group_key_name = group_name;
			group_key_app = 0;
			QRegExp find_app("([\\d]+)$");
			int pos = find_app.indexIn(group_key_name);
			if ( pos != -1) {
				group_key_app = find_app.cap().toInt();
				group_key_name.replace(find_app,"");
			}
			
			break;
		}
		case SWEEP:
		{
			QSqlQuery q;
			q.prepare(QString("SELECT sweeps.name, sweeps.id FROM sweep_jobs JOIN sweeps ON sweeps.id = sweep_jobs.sweep WHERE sweep_jobs.job = %1 ").arg(job_gid));
			if ( ! q.exec() ) {
				qDebug()<< "Unable to retrieve sweep information for Job " << job_gid << "from SQL: " << q.lastError();
			}
			if (q.next()) {
				group_name = q.value(q.record().indexOf("name")).toString()/* + " (" + q.value(q.record().indexOf("id")).toString() + ")"*/;
				group_key_name = q.value(q.record().indexOf("name")).toString() /*+ "__" + q.value(q.record().indexOf("id")).toString()*/;
				group_key_app = q.value(q.record().indexOf("id")).toInt();
// 				QRegExp find_app("([\\d\\.]+)$");
// 				int pos = find_app.indexIn(group_key_name);
// 				if ( pos != -1) {
// 					group_key_app = find_app.cap();
// 					group_key_name.replace(find_app,"");
// 				}
			}
			else {
				group_name = "Interactive";
				group_key_name = "";
			}
			break;
		}
		case STATE:
			group_key_name = QString::number(job_item->process->state());
			group_name = job_item->process->getStateString();
			break;
		default:
			group_key_name = "";
			group_name = "unknwn";
	}

	if (group_key_name == "")
		return;
	// parent_tree_index
	int group_row = -1;

	QList<QSharedPointer<TreeItem> > group_items = root_item->childs;
	for (int i=0; i<group_items.size(); i++) {
		if (group_items[i]->key_name == group_key_name && group_items[i]->key_nr == group_key_app) {
			group_row = i;
			break;
		}
	}
	if ( group_row >= 0) {
		// The groups already existing
		QSharedPointer<TreeItem> group_item = root_item->childs[group_row];
		QModelIndex group_index = index(group_row,0,QModelIndex());
		job_item->parent = group_item;

		int position = 0;
		while (1) {
			if (group_item->childs.size() == position)
				break;
// 			if (current_order == Qt::AscendingOrder) {
// 				if (group_item->childs[position]->process->jobID() > job_gid)
// 					break;
// 			}
// 			else {
				if (group_item->childs[position]->process->jobID() < job_gid)
					break;
// 			}
			position++;
		}

		beginInsertRows(group_index, position, position);
		group_item->childs.insert(position, job_item);
		job_indices[job_gid] = index(position,0,group_index);

		// update the job -> ModelIndex associations
		for (int i = position+1; i<group_item->childs.size(); i++) {
			job_indices[group_item->childs[i]->process->jobID()] = index(i,0,group_index);
		}
		endInsertRows();
	}
	else {
		// The group does not exist yet
		group_row = 0;
		for (group_row = 0; group_row < root_item->childs.size(); group_row++) {
			if (current_grouping == SWEEP && sort_column==0) {
				if ( (group_key_app > root_item->childs[group_row]->key_nr && sort_order == Qt::DescendingOrder) ||
					 (group_key_app < root_item->childs[group_row]->key_nr && sort_order == Qt::AscendingOrder) )
				break;
			}
			if (current_grouping == SWEEP && sort_column==2) {
				int res = group_name.compare( root_item->childs[group_row]->childs.first()->process->info().title, job_item->process->info().title, Qt::CaseInsensitive);
				if ( (res < 0 && sort_order == Qt::AscendingOrder) || (res > 0 && sort_order == Qt::DescendingOrder))
					break;
				if (res == 0) {
					if ((group_key_app > root_item->childs[group_row]->key_nr) )
						break;
				}
			}
			else {
				int res = group_name.compare( root_item->childs[group_row]->key_name, Qt::CaseInsensitive);
				if ( (res < 0 && sort_order == Qt::AscendingOrder) || (res > 0 && sort_order == Qt::DescendingOrder))
					break;
				if (res == 0) {
					if ((group_key_app > root_item->childs[group_row]->key_nr) )
						break;
				}
			}
			
		}
//         group_row  = root_item->childs.size();
		beginInsertRows(QModelIndex(),group_row,group_row);
		QSharedPointer<TreeItem> group_item(new TreeItem);
		group_item->key_name = group_key_name;
		group_item->key_nr = group_key_app;
		group_item->name = group_name;
		group_item->type = GROUP;
		group_item->parent = root_item;
		group_item->hint = job_item->process->info().title;
		if (current_grouping == SWEEP) {
			group_item->hint = QString("Model:  %1\nSweep ID: %2").arg(job_item->process->info().title).arg(group_key_app);
		}
		root_item->childs.insert(group_row, group_item);
		QModelIndex group_index = index(group_row,0,QModelIndex());

		job_item->parent = group_item;
		group_item->childs.append(job_item);
		job_indices[job_gid] = index(group_row,0,group_index);
		endInsertRows();

	}
}

void JobViewModel::removeJob(int job_gid) {
    // job was removed
    Q_ASSERT(job_indices.contains(job_gid));
    QModelIndex idx = job_indices[job_gid];

    QModelIndex group_index = idx.parent();
    TreeItem* group_item = indexToItem(group_index);
    if (group_item->childs.size()==1) {
         beginRemoveRows(group_index.parent(),group_index.row(),group_index.row());
         group_item->parent->childs.removeAt(group_index.row());
         endRemoveRows();
    }
    else {
        beginRemoveRows(group_index,idx.row(),idx.row());


        job_indices.remove(job_gid);
        group_item->childs.removeAt(idx.row());
        // update the job -> ModelIndex associations
        for (int i=idx.row(); i<group_item->childs.size(); i++) {
            job_indices[group_item->childs[i]->process->jobID()] = index(i,0,group_index);
        }
        endRemoveRows();
    }
//    qDebug() << "findished job removal";
}

void JobViewModel::changeJob(int job_gid) {
//    qDebug() << "job changed " << job_gid;
    Q_ASSERT(job_indices.keys().contains(job_gid));
    emit dataChanged(job_indices[job_gid], index(job_indices[job_gid].row(), 3, job_indices[job_gid].parent()));
}


void JobViewModel::updateLayout() {
    beginResetModel();
    // time to remember PersistentIndices
    QList<QModelIndex> persIdx = persistentIndexList();

    QMap<QString, QList< QSharedPointer<abstractProcess> > > data;
    QMap<QString, QList< QSharedPointer<abstractProcess> > >::Iterator dit;

    // this frees the whole tree, since it consists of scoped pointers.
    root_item->childs.clear();

    QMap<int, QSharedPointer<abstractProcess> > plain_list = jobs->getJobs();
    QMap<int, QSharedPointer<abstractProcess> >::Iterator pit;
    // sort jobs by group
    for (pit=plain_list.begin(); pit != plain_list.end(); pit++) {
        addJob(pit.key());
    }
//    // sort jobs by group
//    for (pit=plain_list.begin(); pit != plain_list.end(); pit++) {
//        QString group = getProcessGroup(pit.value().data());
//        if (current_order == Qt::AscendingOrder)
//            data[group].append(pit.value());
//        else
//            data[group].prepend(pit.value());
//    }

//    // this frees the whole tree, since it consists of scoped pointers.
//    root_item->childs.clear();
//    QMap<int, QModelIndex> old_job_indices;
//    swap(job_indices,old_job_indices);

//    for (dit=data.begin(); dit!=data.end(); dit ++) {
//        QSharedPointer<TreeItem>  t1(new TreeItem);
//        t1->name = dit.key();
//        if (t1->name.isEmpty())
//            t1->name = "none";
//        t1->parent = root_item;
//        t1->type = GROUP;
//        root_item->childs.append(t1);
//        QModelIndex group_index = index( root_item->childs.size()-1,0,QModelIndex());
//        for (int i=0; i<dit.value().size();i++) {
//            QSharedPointer<TreeItem>  t2(new TreeItem);
//            t2->type = PROCESS;
//            t2->parent = t1;
//            t2->process = dit.value()[i].data();
//            t2->name = QString("P %1").arg(t2->process->jobID());
//            t1->childs.append(t2);
//            job_indices[t2->process->jobID()] = index(t1->childs.size()-1,0,group_index);
//        }
//    }


//    for (int i=0; i<persIdx.size(); i++) {
//        qDebug() << "remembered " <<old_job_indices.size() <<"old indices";
//        QMap<int, QModelIndex>::ConstIterator it;
//        QModelIndex new_index;
//        for (it=old_job_indices.begin(); it!=old_job_indices.end(); it++) {
//            if (it.value().internalPointer() == persIdx[i].internalPointer()) {
//                new_index = index(job_indices[it.key()].row(), it.value().column(), job_indices[it.key()].parent());
//                break;
//            }
//        }

//        changePersistentIndex(persIdx[i], new_index);
//        if ( ! new_index.isValid() ) {
//            qDebug() << "No index corresponding " << persIdx[i];

//        } changePersistentIndex(persIdx[i], QModelIndex());
//    }

    endResetModel();
}


QList<int> JobViewModel::getJobIds(QModelIndex index) const {
    QList<int> r;
    if (index.isValid()) {
        TreeItem* item = indexToItem(index);
        if (item->type == PROCESS) {
            r.append(item->process->jobID());
        }
        else {
            for (int i=0; i<item->childs.size(); i++) {
               r.append(item->childs[i]->process->jobID());
            }
        }
    }
    qSort(r); // sorting job ids fixed issue with stopping multiple jobs (that not all jobs were stopped)
    return r;
}
