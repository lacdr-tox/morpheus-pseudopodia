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
    /**
     * Creates an object that describes a given xml-node and provides interfaces to manipulate this node.
     * \param node XML-Node which should be represented by this nodeController
     * \param xsd_element_node
     * \param parent nodeController which represents the parent of this xml-node
     */
    nodeController(nodeController* parent, XSD::ChildInfo info, QDomNode xml_node);
    ~nodeController();
	
	/// Node tag name
    QString getName() const { return name; }
    /// Get a non-qualified XPath to the node.
    QStringList getXPath() const { if (!parent) return QStringList() << name; else return parent->getXPath() << name; };
	/// Get a qualified, i.e. unique, XPath to the node.
	QStringList getQualifiedXPath() const;
	/// Get the node's parent. The root node returns a nullptr.
    nodeController* getParent() const { return parent; }
    /// Get a copy of the nodes xml including all subnodes.
    QDomNode cloneXML() const;
	/// Returns a list of childs allowed to be added to the current state of the nodeController.*/
	QList<QString> getAddableChilds(bool unfiltered=false);
	
	const XSD::GroupInfo& childInformation() { return node_type->child_info; };
	
	/// Returns information about the named child. */
	XSD::ChildInfo childInformation(QString n);
	
	/// Get the attribute named @p name or nullptr if the attribute is not valid. */
	AbstractAttribute* attribute(QString name);
	/// Get the attribute identified by the relative @p path or nullptr if the attribute is not present. */
	AbstractAttribute* getAttributeByPath(QStringList path);
	/// Get a list of required attributes.
    QList<AbstractAttribute*> getRequiredAttributes();
	/// Get a list of optional attributes.
    QList<AbstractAttribute*> getOptionalAttributes();

	/// Node is deletable, i.e. not required.
	bool isDeletable();

	/// Information about the node extracted from Schema
	QString getNodeInfo();
	const ModelDescriptor& getModelDescr() const { return *model_descr;}
	/// Track next change to the node the ModelDescriptor
	void trackNextChange();
	/// Track an information to the ModelDescriptor
	void trackInformation(QString info);
	/// Removes all tracked Changes from the ModelDescriptor
	void clearTrackedChanges();
	
	void setStealth(bool enabled = true);
	/// Broadcast to that the model was saved. Resets all edit trackers.
	void saved();
	/// Return whether the xmlNode can have normal textblocks.
	bool hasText();
	/// Text of the node.
	QString getText();
	/// Set the text of the node to @p txt.
    bool setText(QString txt);
	/// Get the xsd type descriptor for the node text.
    QSharedPointer<XSD::SimpleTypeInfo> textType();
	/// Return the attribut which represents the text of the xml-node.
    AbstractAttribute* textAttribute();
	QStringList inheritedTags() const;
	QStringList subtreeTags() const;
	QStringList getEffectiveTags() const;
	QStringList getTags() const { return tags; };
	bool allowsTags() const { return attributes.contains("tags"); } 
	
	/// Find all active child nodes named @p childName. If name is omitted, all active childs are returned.
	QList<nodeController*> activeChilds(QString childName = "");
	/// Find the first active child named @p childName. If name is omitted, the node's first active child is returned.
	nodeController* firstActiveChild(QString childName = "");  

	const QList< nodeController* >& getChilds() { return childs; } 
	/// Find the nodeController of the XML node @p xml_node in the document
    nodeController* find(QDomNode xml_node);
	/// Find the nodeController addressed by @p path in the document. if @p create is true, try to create the path of not present.
    nodeController* find(QStringList path, bool create = false);
	///< Insert a child with name @p childName. Appends the child if no position is provided.
	nodeController* insertChild(QString childName, int pos=-1);
	///< Insert the given XML node @p xml_node. Appends the child if no position is provided.
	nodeController* insertChild(QDomNode xml_node, int pos=-1);
	///< Remove child at position @p pos. Returns a SmartPointer to removed nodeController, which is destroyed if not stored somewhere else.
	nodeController* removeChild(int pos);

	bool isChildRequired(nodeController*);
	/// Move the child at position @p from to position @p to
	void moveChild(int from, int to);                       
	/// Test whether the foreign child @p child can be inserted to this node at @p dest_pos
	bool canInsertChild(QString child, int dest_pos);
	/// Test whether the foreign child @p child can be inserted to this node at @p dest_pos
	bool canInsertChild(nodeController* child, int dest_pos);
	/// Move the foreign child @p child to this node at @p dest_pos
	bool insertChild(nodeController* child, int dest_pos);

	/// Returns whether the nodeController is disabled or not.
	bool isDisabled();
	/// Returns whether the nodeController is disabled or indirectly disabled by a parental node.
	bool isInheritedDisabled();
	
	/**
	  Set the disabled state of a nodeController.
	  Disabling the nodeController moves it's xml to a Comment node. 
	  Enabling it reactivates the node from the comment node.
	*/
	bool setDisabled(bool b);
	void synchDOM();

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
    QStringList tags;
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

Q_DECLARE_METATYPE(nodeController*);

#endif // NODECONTROLLER_H
