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

Q_DECLARE_METATYPE(QDomNode);

//class domNodeViewer;
struct MorphModelPart {
    QString label;
	bool enabled;
    nodeController* element;
    QModelIndex element_index;
	static const QList<QString> all_parts_sorted;
	static const QMap<QString,int> all_parts_index;
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
	static const int morpheus_ml_version = 4;

// The interface for QAbstractItemModel ...
    virtual QModelIndex index( int row, int column, const QModelIndex &parent) const override;
    QModelIndex itemToIndex(nodeController* p_node) const;
    nodeController* indexToItem(const QModelIndex& idx) const;

    virtual QModelIndex parent( const QModelIndex &child ) const override;
    virtual Qt::ItemFlags flags( const QModelIndex & index ) const override;

    virtual int rowCount( const QModelIndex &parent ) const override;
    virtual int columnCount ( const QModelIndex & parent = QModelIndex() ) const override;

    virtual QVariant data( const QModelIndex &index, int role ) const override;
    virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const override;

    Qt::DropActions supportedDropActions () const override { return Qt::CopyAction | Qt::MoveAction;};
	 Qt::DropActions supportedDragActions() const override { return Qt::CopyAction | Qt::MoveAction; } 
    QStringList mimeTypes () const override;
    QMimeData* mimeData(const QModelIndexList &indexes) const override;
    bool dropMimeData( const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent ) override;

    QModelIndex insertNode(const QModelIndex& parent, QDomNode child, int pos = -1);
    QModelIndex insertNode(const QModelIndex& parent, QString child, int pos = -1);
	void setDisabled(const QModelIndex &node, bool disabled);

    // bool removeRows ( int row, int count, const QModelIndex & parent = QModelIndex() );
    void removeNode(const QModelIndex &parent, int row);

	bool activatePart(int idx);
    bool addPart(QString name);
    bool addPart(QDomNode xml);
    void removePart(QString name);
	void removePart(int idx);
    QList<MorphModelPart> parts; /*!< Keeps the model parts for the editor */
    
    /// Find the node given by @p path. If the node does not exist, create it if @p create.
    nodeController* find(QStringList path, bool create=false);
	nodeController* getRoot() { return rootNodeContr;};
	const ModelDescriptor& getModelDescr() const { return rootNodeContr->getModelDescr(); }

    bool isSweeperAttribute(AbstractAttribute* attr) const;
    bool isEmpty() const;

	MorpheusXML xml_file;
	
	enum GRAPH_TYPE { SVG, PNG, DOT };
	QString getDependencyGraph(GRAPH_TYPE type);
    nodeController* rootNodeContr; /*!< root nodeController, which handels the root xml-node. */
    bool sweep_lock;
	
private:
	friend class parameterSweeper;
	friend class JobQueue;

	struct AutoFix {
		QString match_path;
		QString target_path;
		QString require_path;
		enum { COPY, MOVE } operation;
		bool replace_existing;
		AutoFix() : operation(COPY), replace_existing(true) {};
	};
	QList<MorphModelEdit> applyAutoFixes(QDomDocument document);
	
	QDir temp_folder;
	int dep_graph_model_edit_stamp;
	ParamSweepModel param_sweep;

	void initModel();
	void loadModelParts();
	
	void prepareActivationOrInsert(nodeController* node, QString name);
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
    void modelPartAdded(int idx);
    void modelPartRemoved(int idx);
//     void sweeperAttributeAdded(AbstractAttribute* attr);
//     void sweeperAttributeRemoved(AbstractAttribute* attr);
};

typedef QPointer<MorphModel> SharedMorphModel;

#endif // MODELDATA_H
