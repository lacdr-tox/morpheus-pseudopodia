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

#ifndef IndexListMimeData_H
#define IndexListMimeData_H

#include <QMimeData>
#include <QAbstractItemModel>
#include <QStringList>


class IndexListMimeData : public QMimeData
{
    Q_OBJECT
    QModelIndexList indexes;
public:
    static QString indexList;
    void setIndexList(const QModelIndexList & _indexes) { indexes  = _indexes;}
    virtual QStringList formats () const { QStringList f = QMimeData::formats(); if (! indexes.empty()) f << indexList; return f;}
    const QModelIndexList & getIndexList() const { return indexes; }
};

#endif
