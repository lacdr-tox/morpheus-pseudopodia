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

#ifndef JOBVIEW_H
#define JOBVIEW_H

#include <QtGui>
#include "job_queue.h"
#include "jobviewmodel.h"
#include "job_progress_delegate.h"

class JobQueueView : public QSplitter {

Q_OBJECT
public:
    explicit JobQueueView ( QWidget* parent = 0 );
	JobViewModel::Grouping getGroupBy();

private:
	QMenu* jobQueueMenu;
    QMenu* jobQueueGroupingMenu;
	QTreeView* jobQueueTreeView;
	QListWidget* jobQueueStatusText;
	QProgressBar* jobQueueProgressBar;
	JobViewModel* job_view_model;
	static const int maxMessageItems = 30;

	QTimer progress_shut_timer;

public slots:
	void setGroupBy(JobViewModel::Grouping g);
	void addJob(const QModelIndex&, int);
	void addMessage(QString message, int progress);
	void addCriticalMessage(QString message, int job_id);

private slots:
	void selectJob(const QModelIndex&);
	void unselectJobs(const QModelIndex& parent, int min_row, int max_row);
	void selectStatus(QListWidgetItem *);
	void showJobQueueMenu(QPoint p);
	void jobQueueMenuTriggered(QAction*);

signals:
	void jobSelected(int job_id);

	void sweepSelected(QList<int> job_ids);
	void erronousXMLPath(QString path, int model_id);

	void removeJob(int job_id,bool remove_data);
	void stopJob(int job_id);
	void debugJob(int job_id);
};

#endif
