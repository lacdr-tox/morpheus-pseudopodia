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

#ifndef PARAMSWEEPMODEL_H
#define PARAMSWEEPMODEL_H

#include <QAbstractItemModel>
#include <QDataStream>
#include <QList>
#include <QMessageBox>
#include "model_attribute.h"
// #include "model_index_mime_data.h"
#include "model_node.h"

class ParamItem {
// Q_OBJECT
public:
	ParamItem(ParamItem *parent, AbstractAttribute *item);
	~ParamItem();
	AbstractAttribute* attribute() const;
	QString range() const;
	QString type() const;
	void setRange(QString range);
	QStringList values() const;
	int childCount() const;
	int childPos(const ParamItem *) const;
	int row() const;
	ParamItem* child(int i) const;
	ParamItem* findItem(const AbstractAttribute* attr) const;
	ParamItem* parent() const;
	ParamItem* addChild(AbstractAttribute* item, int pos=-1);
	int addChild(QSharedPointer<ParamItem> item, int pos=-1);
	QSharedPointer<ParamItem> removeChild(int i);
	void moveChild(int from, int to);

private:
	AbstractAttribute *data_item;
	ParamItem *parent_item;
	QString type_name;
	QList<QSharedPointer<ParamItem> > childs;
	QStringList data_values;
	QString range_value;
	QMap<const AbstractAttribute*,ParamItem*>* attrib2item;
// signals:
};

class ParamSweepModel : public QAbstractItemModel
{
Q_OBJECT
public:
	explicit ParamSweepModel ( QObject* parent = 0 );
// 	~ParamSweepModel ();

	AbstractAttribute* getAttribute( const QModelIndex& index );
	virtual QVariant data ( const QModelIndex& index, int role = Qt::DisplayRole ) const;
	virtual int columnCount ( const QModelIndex& parent = QModelIndex() ) const;
	virtual int rowCount ( const QModelIndex& parent = QModelIndex() ) const;
	virtual QModelIndex parent ( const QModelIndex& child ) const;
	virtual QModelIndex index ( int row, int column, const QModelIndex& parent = QModelIndex() ) const;
    virtual bool setData ( const QModelIndex& index, const QVariant& value, int role = Qt::EditRole );
	virtual QVariant headerData ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

	virtual QMimeData* mimeData ( const QModelIndexList& indexes ) const;
	virtual QStringList mimeTypes() const;
	virtual Qt::DropActions supportedDropActions() const;
	virtual Qt::ItemFlags flags ( const QModelIndex& index ) const;
    virtual bool dropMimeData ( const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent );

	bool contains(AbstractAttribute *attr) const;
	void clear();
	QByteArray store() const;
	bool restore( const QByteArray& data, nodeController* modelRootNode );
	void createJobList(QList<AbstractAttribute*>& params, QList<QStringList>& values) const;
	
private:
// 	SharedMorphModel model;
	QSharedPointer<ParamItem> root_item;
	ParamItem* indexToItem(const QModelIndex& index) const;
	QModelIndex itemToIndex(ParamItem* item, int col=0) const;
public slots:
	void addAttribute(AbstractAttribute *item);
	void removeAttribute(AbstractAttribute *item);
	
};

#endif // PARAMSWEEPMODEL_H
