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

#ifndef ABSTRACTATTRIBUTE_H
#define ABSTRACTATTRIBUTE_H

#include <QtGui>
#include "xsd.h"
#include <exception>

using namespace std;

class AbstractAttribute;

template <class T>
bool operator <( const QSharedPointer<T> a, const QSharedPointer<T> b) {
    return a.data() < b.data();
}

// typedef AbstractAttribute* qspAttribute;
typedef QMap< AbstractAttribute*, AbstractAttribute* > AttributeMap;

enum ModelEditType {AttribAdd, AttribRemove, AttribRename, AttribChange,  NodeAdd, NodeRemove, NodeRename, NodeMove, TextChange};
struct MorphModelEdit {
    QString info;
    QString name, value;
    ModelEditType edit_type;
    QDomNode xml_parent;
};

struct ModelDescriptor {
    AttributeMap symbolNames;
    XSD xsd;
	bool track_next_change;
    QList<MorphModelEdit> auto_fixes;
    QList<MorphModelEdit> edit_operations;
    QList<AbstractAttribute*> terminal_names;
	QList<AbstractAttribute*> sys_file_refs;
    QString title;
    QString details;
    int edits;
    QMap<QString,QString> getSymbolNames(QString type_name) const;
};

class ModelException : public exception {
public:
	enum Type { UnknownXSDType, UndefinedNode, InvalidNodeIndex, FileIOError };
	virtual const char* what() const throw();
	QString message;
	Type type;
	ModelException(Type t, QString n);
	~ModelException() throw() {};
};

/*!
The AbstractAttribute class describes the properties of a xml-attribut, which you find at the given QDomNode of a xml-file.<br>
It also provides an interface to change the value of the xml-attribute.
*/
class AbstractAttribute : public QObject
{
Q_OBJECT

protected:
    QString name; /*!< Name of the xml-attribut.*/
    QString value; /*!< Value of the attribut.*/
    QString orig_value; /*!< Value of the xml-attribut, as it was initialized.*/
    QString default_value; /*!< Value of the attribut.*/
    bool is_required; /*!< If variable is true then this attribute is required and have to be set.*/
    QSharedPointer< XSD::SimpleTypeInfo> type; /*!< Type of the attribute.*/
    QString docu; /*!< Documentation of the attribut, written in the xml-schema. */
    QDomNode parentNode; /*!< Xml-node, which contains the attribut. */
    QDomText textNode;
    bool is_active; /*!< If variable is true, the attribut is active and set in xml-file otherwise it is inactive and can be set. */
    bool is_disabled;
    bool orig_active; /*!< Indicates whether the attribut was set in xml-file originally. */
    bool is_changed;
    bool is_text; /*!< If variable is true, the attribut is a pure text-node. */
    QSharedPointer<ModelDescriptor> model_descr;

public:
    AbstractAttribute( QObject* parent, QDomNode xsdDescrNode, QDomNode xmlParentNode, QSharedPointer<ModelDescriptor> m_descr);
    /*!<
      To create a new attribut use a constructor of the inheriting child-classes!
      \param xsdDescrNode xml-schema-node, which describes the structure and content of the attribut
      \param xmlParentNode xml-node, which contains the attribut
      */
    ~AbstractAttribute();

    QString get(); /*!< Returns the value of parameter.*/
    QString getName() { return name; } /*!< Returns the name of parameter. */
    QString getDocu() { return docu; } /*!< Returns the documentation of parameter. */
    QDomNode getParentNode() { return parentNode; }
    const ModelDescriptor& getModelDescr() { return *model_descr; } /*!< Returns the descriptor of the model containing this attribute. */
    bool isRequired() { return is_required; } /*!< Returns if the parameter is required and have to be set. */
    bool isActive() { return is_active; } /*!< Returns if the parameter is active and set in the xml-file. */
    bool isDisabled() { return is_disabled; }
public slots:
	bool set(QString); /*!< Sets the value of parameter.*/
public:
	bool set(const std::string& s) { return this->set(QString::fromStdString(s)); } /*!< Sets the value of parameter.*/
	bool set(const char* s) { return this->set(QString::fromUtf8(s)); } /*!< Sets the value of parameter.*/
	bool set(double d) { return this->set(QString::number(d)); } /*!< Sets the value of parameter.*/
    void setActive(bool ); /*!< Sets the state of parameter (if it is active or not). */
	void inheritDisabled(bool ); /*!< Notify the attribut that the parent was disabled */
    QStringList getEnumValues();
    /*!<
      Returns a list of all values the parameter can have.
      If it isn't an enum the list will be empty.
      */

    QString getXMLPath(); /*!< Returns the absolute path of xml-attribute inside xml-file. */
    void saved(); /*!< broadcast that the underlying xml was saved */

    bool isValid(QString);/*!< Checks if the value for attribut is valid or not.*/
    QString getPattern() { return type->pattern; } /*!< Returns a pattern, which describes all possible values for the parameter. */
    QSharedPointer< XSD::SimpleTypeInfo> getType(); /*!< Returns the complete description for the type of parameter. */


signals:
    void deleted(AbstractAttribute*); /*!< Signal is emitted when the destructor of the attribut is called. */
    void changed(AbstractAttribute*);
};

#endif // ABSTRACTATTRIBUTE_H
