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

#ifndef JOBVIEWMODEL_H
#define JOBVIEWMODEL_H

#include <QAbstractItemModel>
#include "job_queue.h"

class JobViewModel : public QAbstractItemModel
{
    Q_OBJECT
private:
    struct TreeItem;

public:
    enum Grouping { PROC_ID=1, SWEEP=3,MODEL=0,STATE=2 };

    explicit JobViewModel(JobQueue* jobs,  QObject *parent = 0);

	// QAbstractItemModel Interface
    QModelIndex index( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
    QModelIndex indexOfJob(int job) const;
    QModelIndex parent( const QModelIndex& index) const;
    QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
    QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
    void sort( int column, Qt::SortOrder order = Qt::AscendingOrder );
    int rowCount( const QModelIndex & parent = QModelIndex() ) const;
    int columnCount( const QModelIndex & parent = QModelIndex() ) const;

	// Interface for the gui
    void setGroupBy(Grouping g);
    JobViewModel::Grouping getGroupBy();
    QList<int> getJobIds(QModelIndex index) const;
	bool isGroup ( const QModelIndex& index ) { return indexToItem(index)->type == GROUP; };


private:
    enum EntryType { GROUP, PROCESS };
    struct TreeItem {
        QSharedPointer<TreeItem> parent;
        QList<QSharedPointer<TreeItem> > childs;
        EntryType type;
        QString key_name,name;
		int key_nr;
		QSharedPointer<abstractProcess> process;
		QString hint;
    };
    QSharedPointer<TreeItem> root_item;
    QMap<int, QModelIndex> job_indices;
	QMap<ProcessInfo::status,QIcon> icon_for_state;

    TreeItem* indexToItem(const QModelIndex& index) const;
    Grouping current_grouping;
    QString getProcessGroup(QSharedPointer<abstractProcess> p) ;
    Qt::SortOrder sort_order;
    JobQueue* jobs;
    int sort_column;
private slots:
    void addJob(int);
    void changeJob(int);
    void removeJob(int);
public slots:

    void updateLayout();

};

#endif // JOBVIEWMODEL_H
