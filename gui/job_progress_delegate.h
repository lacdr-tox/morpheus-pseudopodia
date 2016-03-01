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

#ifndef JOB_PROGRESS_DELEGATE_H
#define JOB_PROGRESS_DELEGATE_H

#include <QStyledItemDelegate>
#include <QApplication>
#include <QDebug>

class JobProgressDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit JobProgressDelegate(QObject *parent = 0);
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
                               const QModelIndex &index) const;
signals:

public slots:

};

#endif // JOB_PROGRESS_DELEGATE_H
