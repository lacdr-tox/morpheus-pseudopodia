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

#ifndef MODELDATA_H
#define MODELDATA_H

#include <QAbstractItemModel>
#include "morpheus_xml.h"
#include "nodecontroller.h"
#include "model_index_mime_data.h"
#include "paramsweepmodel.h"


//class domNodeViewer;
struct MorphModelPart {
    QString label;
    nodeController* element;
    QModelIndex element_index;
	static map<QString, uint> order;
	static bool compare_labels(MorphModelPart a, MorphModelPart b){
		return (order[a.label] < order[b.label]);
	}
};



class MorphModel: public QAbstractItemModel
{
    Q_OBJECT
public:

    explicit MorphModel(QObject *parent = 0);
    /*!< Creates an empty model from scratch */
    explicit MorphModel( QString filePath, QObject *parent = 0);
    /*!< Creates a model description from file @param filepath. */
    explicit MorphModel( QDomDocument content, QObject *parent = 0);
	
    ~MorphModel();
    /*!< Creates a model description from file @param filepath. */
    bool close();
	static const int morpheus_ml_version = 2;

// The interface for QAbstractItemModel ...
    virtual QModelIndex index( int row, int column, const QModelIndex &parent) const;
    QModelIndex itemToIndex(nodeController* p_node) const;
    nodeController* indexToItem(const QModelIndex& idx) const;

    virtual QModelIndex parent( const QModelIndex &child ) const;
    virtual Qt::ItemFlags flags( const QModelIndex & index ) const;

    virtual int rowCount( const QModelIndex &parent ) const;
    virtual int columnCount ( const QModelIndex & parent = QModelIndex() ) const;

    virtual QVariant data( const QModelIndex &index, int role ) const;
    virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

    Qt::DropActions supportedDropActions () const;
    QStringList mimeTypes () const;
    QMimeData* mimeData(const QModelIndexList &indexes) const;
    bool dropMimeData( const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent );

    bool insertNode(const QModelIndex &parent, QDomNode child, int pos = -1);
    bool insertNode(const QModelIndex &parent, QString child, int pos = -1);
	void setDisabled(const QModelIndex &node, bool disabled);

    // bool removeRows ( int row, int count, const QModelIndex & parent = QModelIndex() );
    void removeNode(const QModelIndex &parent, int row);

    bool addPart(QString name);
    bool addPart(QDomNode xml);
    void removePart(int idx);
    QList<MorphModelPart> parts; /*!< Keeps the model parts for the editor */

    bool isSweeperAttribute(AbstractAttribute* attr) const;
    bool isEmpty() const;

	MorpheusXML xml_file;
	QString getDependencyGraph();
    nodeController* rootNodeContr; /*!< root nodeController, which handels the root xml-node. */
    bool sweep_lock;
	
private:
    friend class parameterSweeper;
	friend class JobQueue;

	QDir temp_folder;
	int dep_graph_model_edit_stamp;
	ParamSweepModel param_sweep;
//     QList<QList<AbstractAttribute*> > sweeperAttributes;
//     QMap<AbstractAttribute*, QString> sweeperValues;
    struct AutoFix {
        QString match_path;
        QString move_path;
        bool rename;
    };
    QList<MorphModelEdit> applyAutoFixes(QDomDocument document);
	void initModel();
    void loadModelParts();
	bool removeDir(QString dir_path);
//     void addRecentFile(QString fileName);
    /*!< Sets the name of current loaded file to 'filename'. */
    QModelIndex internalDropIndex;
    int internalDropRow;

public slots:
    bool addSweeperAttribute(AbstractAttribute* attr);
    /*!< Adds an attribute to the parameter sweeper list */
    void removeSweeperAttribute(AbstractAttribute* attr);
    /*!< Removes an attribute from the parameter sweeper list */

signals:
    void modelPartAdded();
    void modelPartRemoved();
//     void sweeperAttributeAdded(AbstractAttribute* attr);
//     void sweeperAttributeRemoved(AbstractAttribute* attr);
};

typedef QSharedPointer< MorphModel > SharedMorphModel;

#endif // MODELDATA_H
