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

#ifndef NODECONTROLLER_H
#define NODECONTROLLER_H

#include "abstractattribute.h"
#include "nodeadapter.h"
#include <QtGui>
#include "xsd.h"
#include <QSharedPointer>
#include <vector>

using namespace std;


/*!
Objects of this class are representing xml-nodes with all their properties.
e.g.: all childnodes, attributes, text and disabled status
      informations about potential childnodes and attributes are read from the schema-file.
	Method to add and remove childnodes and attributes are provided.
	This class allows to modify a xml-node in all ways permitted by the node-definition in the schema.
*/

class nodeController : public QObject
{
	Q_OBJECT
public:
	nodeController( QDomNode xml_node ) : nodeController( nullptr, XSD::ChildInfo(), xml_node) {};
    nodeController(nodeController* parent, XSD::ChildInfo info, QDomNode xml_node);
    /**
      Creates an object that describes a given xml-node and provides interfaces to manipulate this node.
      \param node XML-Node which should be represented by this nodeController
      \param xsd_element_node
      \param parent nodeController which represents the parent of this xml-node
    */
    ~nodeController();
    QString getName() const { return name; }
    QStringList getXPath() const { if (!parent) return QStringList() << name; else return parent->getXPath() << name; };
	 QStringList getQualifiedXPath() const;
    nodeController* getParent() const { return parent; }
    QDomNode cloneXML() const;

	QList<QString> getAddableChilds(bool unfiltered=false);  /// Returns a list of childs allowed to be added to the current state of the nodeController.*/
	const XSD::GroupInfo& childInformation() { return node_type->child_info; };
	XSD::ChildInfo childInformation(QString n);                    /// Returns information about the named child. */

	AbstractAttribute* attribute(QString name);              /// Get the attribute named @p name or Null if the attribute is not valid. */
	AbstractAttribute* getAttributeByPath(QStringList path); /// Get the attribute identified by the relative @p path or Null if the attribute is not present. */
    QList<AbstractAttribute*> getRequiredAttributes();       /// Get a list of required attributes.
    QList<AbstractAttribute*> getOptionalAttributes();       /// Get a list of optional attributes.

    bool isDeletable();                           /// Returns whether the node is deletable, i.e. not required.

    QString getNodeInfo();                            /// Information about the node extracted from Schema.
    const ModelDescriptor& getModelDescr() const { return *model_descr;}
    void trackNextChange();
	void trackInformation(QString info);
	void clearTrackedChanges();                   /// Removes all tracked Changes from the ModelDescriptor
    void saved();                                 /// Broadcast to all Nodes that the model was saved, resets all edit trackers.

    bool hasText();                               /// Return whether the xmlNode can have normal textblocks. */
    QString getText();                            /// Text of xml-node.
    bool setText(QString txt);                    /// Set the text of the node to @p txt.
    QSharedPointer<XSD::SimpleTypeInfo> textType();
    AbstractAttribute* textAttribute();           /// Return the attribut which represents the text of the xml-node.

	QList<nodeController*> activeChilds(QString childName = ""); /// Find all active child nodes named @p childName
	nodeController* firstActiveChild(QString childName = "");  /// Find the first active child named @p childName
// 	nodeController* findGroupChild(QString group); /// Returns the child of the group @p group. Returns NULL if none exists.
	const QList< nodeController* >& getChilds() { return childs; } 
    nodeController* find(QDomNode xml_node);      /// Find the nodeController of the XML node @p xml_node in the document
    nodeController* find(QStringList path);           /// Find the nodeController addressed by @p path in the document

	nodeController* insertChild(QString childName, int pos=-1); /// Insert a child with name @p childName. Appends the child if no position is provided.
	nodeController* insertChild(QDomNode xml_node, int pos=-1); /// Insert the given XML node @p xml_node. Appends the child if no position is provided.
	nodeController* removeChild(int pos);      /// Remove child at position @p pos. Returns a SmartPointer to removed nodeController, which is destroyed if not stored somewhere else.

	bool isChildRequired(nodeController*);
	void moveChild(int from, int to);             /// Move the child at position @p from to position @p to+
	bool canInsertChild(QString child, int dest_pos);   /// Test whether the foreign child @p child can be inserted to this node at @p dest_pos
	bool canInsertChild(nodeController* child, int dest_pos);   /// Test whether the foreign child @p child can be inserted to this node at @p dest_pos
	bool insertChild(nodeController* child, int dest_pos);   /// Move the foreign child @p child to this node at @p dest_pos

	bool isDisabled();                            /// Returns whether the nodeController is disabled or not.
	bool isInheritedDisabled();
	
	/**
	  Set the disabled state of a nodeController.
	  Disabling the nodeController moves it's xml to a Comment node. 
	  Enabling it reactivates the node from the comment node.
	*/
	bool setDisabled(bool b);
    

signals:
	void dataChanged(nodeController*);

private:
    friend class MorphModel;
    QString name; /*!< Name of the xmlNode (nodeController).*/
    nodeController* parent; /*!< Parent-nodeController.*/
    QDomNode xmlNode; /*!<  Reference to the XML-Node in the Document the Controller takes care of.
						   This might be a comment or a <Disabled> node, if the node is disabled */
    QSharedPointer<XSD::ComplexTypeInfo> node_type; /*!< Node of the xml-schema which describes the xml-node.*/

    AbstractAttribute* text; /*!< Attribute which presents the text of the xml-node. */
    QMap<QString, AbstractAttribute* > attributes; /*!< Contains all possible attributes and their informations.*/
    QList< nodeController* > childs; /*!< Contains all controllers which are representing the childnodes.*/
    QList<QObject*> adapters;

    bool disabled; /*!< */
    QDomNode xmlDataNode; /*!<  Reference to the XML-Node of the data. If disabled, this node is not part of the document but child of the xmlDisabledNode ...*/
    QDomNode xmlDisabledNode; /*!<  Reference to the XML-Node <Disabled>, containing the data node if disabled. */
    bool inherited_disabled; /*!< The nodeController is disabled, because it's parent is disabled */
    bool orig_disabled; /*!< Indicates whether the node was disabled on loading or saving. */

    QSharedPointer<ModelDescriptor> model_descr; /// Contains all information grabbed upon the model

    int childIndex(nodeController* node) const;
	
    void inheritDisabled(bool);
//     void parseChilds();
    void parseAttributes();
//     NodeInfo parseChildInfo(QDomNode, const NodeInfo& default_info);
    void setRequiredElements(); /*!< Parses all possible child-nodes and attributes and sets all these who are required ones. */
};

#endif // NODECONTROLLER_H
