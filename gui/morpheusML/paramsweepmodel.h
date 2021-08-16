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

	AbstractAttribute* getAttribute( const QModelIndex& index );
	QVariant data ( const QModelIndex& index, int role = Qt::DisplayRole ) const override;
	int columnCount ( const QModelIndex& parent = QModelIndex() ) const override;
	int rowCount ( const QModelIndex& parent = QModelIndex() ) const override;
	QModelIndex parent ( const QModelIndex& child ) const override;
	QModelIndex index ( int row, int column, const QModelIndex& parent = QModelIndex() ) const override;
    bool setData ( const QModelIndex& index, const QVariant& value, int role = Qt::EditRole ) override;
	QVariant headerData ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const override;

	QMimeData* mimeData ( const QModelIndexList& indexes ) const override;
	QStringList mimeTypes() const override;
	Qt::DropActions supportedDropActions() const override;
	Qt::ItemFlags flags ( const QModelIndex& index ) const override;
    bool dropMimeData ( const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent ) override;

	bool contains(AbstractAttribute *attr) const;
	void clear();
	QByteArray store() const;
	bool restore( const QByteArray& data, nodeController* modelRootNode );
	bool presetRandomSeeds() const { return preset_random_seeds; }
	/// Provide the root Element for the RandomSeed/@value
	void setRandomSeedRoot(nodeController* root) {random_seed_root = root;};
	void createJobList(QList<AbstractAttribute*>& params, QList<QStringList>& values) const;
	
private:
// 	SharedMorphModel model;
	QSharedPointer<ParamItem> root_item;
	bool preset_random_seeds;
	nodeController* random_seed_root;
	ParamItem* indexToItem(const QModelIndex& index) const;
	QModelIndex itemToIndex(ParamItem* item, int col=0) const;
public slots:
	void addAttribute(AbstractAttribute *item);
	void removeAttribute(AbstractAttribute *item);
	void setPresetRandomSeeds(bool enabled);
	
};

#endif // PARAMSWEEPMODEL_H
