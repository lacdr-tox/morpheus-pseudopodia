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
    nodeController(nodeController* parent, QDomNode xml_node);
    /**
      Creates an object that describes a given xml-node and provides interfaces to manipulate this node.
      \param node XML-Node which should be represented by this nodeController
      \param xsd_element_node
      \param parent nodeController which represents the parent of this xml-node
    */
    ~nodeController();

    /** Properties of a possible childNode.*/
    struct NodeInfo
    {
        QString name;            /// Name of the possible childNode.
        QDomNode xsdChildDef;    /// Corresponding schema definition for children of this type.
        bool is_complexType;      /// Child has a complexType
        QDomNode xsdChildType;   /// Type schema of this childNode
        QSharedPointer<XSD::SimpleTypeInfo>  simpleChildType; /// Simple Type of this childNode
        QString minOccure;       /// Minimal occurence of this childNode.
        QString maxOccure;       /// Maximal occurence of this childNode.
        QString info;            /// Short informations about this childNode.
        QString group;           /// Name of the group, the childNode belongs to.
        bool is_exchanging;       /// True if only one child of the same group may exist.
        bool is_default_choice;  /// True if this ChildNode is the default choice of the group.
        QString pluginClass;     /// The name of the plugin class which the child belongs to.
    };

    QString getName() const { return name; }
    QStringList getXPath() const { if (!parent) return QStringList() << name; else return parent->getXPath() << name; };
	 QStringList getQualifiedXPath() const;
    nodeController* getParent() const { return parent; }
    QDomNode cloneXML() const {return xmlDataNode.cloneNode(true);}

    QList<QString> getAddableChilds(bool unfiltered=false);  /// Returns a vector with all childNodes which can be added to the current state of the nodecontroller.*/
    NodeInfo childInformation(QString n);                    /// Returns information about the named child. */

    AbstractAttribute* attribute(QString name);              /// Get the attribute named @p name or Null if the attribute is not valid. */
	AbstractAttribute* getAttributeByPath(QStringList path); /// Get the attributes or Null if the attribute is not valid. */
    QList<AbstractAttribute*> getRequiredAttributes();       /// Get all required attributes.
    QList<AbstractAttribute*> getOptionalAttributes();       /// Get all optional attributes.

    bool isDeletable();                           /// Returns whether the node is deletable or not.

    QString getInfo();                            /// Information about the node extracted from Schema.
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

	nodeController* firstChild(QString childName);/// Find the first child named @p childName
	nodeController* findGroupChild(QString group); /// Returns the child of the group @p group. Returns NULL if none exists.
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
	  Enablibling it reactivates the node from the comment node.
	*/
	bool setDisabled(bool b);
    

private:
    friend class MorphModel;
    QString name; /*!< Name of the xmlNode (nodeController).*/
    nodeController* parent; /*!< Parent-nodeController.*/
    QDomNode xmlNode; /*!<  Reference to the XML-Node in the Document the Controller takes care of.
						   This might be a comment or a <Disabled> node, if the node is disabled */
    QDomNode xsdTypeNode; /*!< Node of the xml-schema which describes the xml-node.*/
    AbstractAttribute* text; /*!< Attribute which presents the text of the xml-node. */
    QMap<QString, AbstractAttribute* > attributes; /*!< Contains all possible attributes and their informations.*/

    QList< nodeController* > childs; /*!< Contains all controllers which are representing the childnodes.*/
    QMap<QString, NodeInfo> childInfo; /*!< Contains all possible childNodes and their informations which can be added.*/

    QList<QObject*> adapters;

    bool disabled; /*!< state o qtreeview dropevent movethe nodeController (diabled means, the xml-node isn't set for this) */
    QDomNode xmlDataNode; /*!<  Reference to the XML-Node of the data. If disabled, this node is not part of the document ...*/
    QDomNode xmlDisabledNode; /*!<  Reference to the XML-Node <Disabled>, containing the data node if disabled. */
    bool inherited_disabled; /*!< The nodeController is disabled, because it's parent is disabled */
    bool orig_disabled; /*!< Indicates whether the node was disabled on loading or saving. */

    QSharedPointer<ModelDescriptor> model_descr; /// Contains all information grabbed upon the model
    bool is_complexType;

    int childIndex(nodeController* node) const;
	
    void inheritDisabled(bool);
    void parseChilds();
    void parseAttributes();
    NodeInfo parseChildInfo(QDomNode, const NodeInfo& default_info);
    void setRequiredElements(); /*!< Parses all possible child-nodes and attributes and sets all these who are required ones. */

    QDomNode getSubNode(QDomNode);
};

#endif // NODECONTROLLER_H
