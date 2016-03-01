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

#ifndef XSD_H
#define XSD_H

#include <QtGui>
#include <QString>
#include <QHash>
#include <QDomNode>
#include <QFile>
#include <QSharedPointer>
#include <iostream>
#include <sstream>

/*!
This class reads the cpm-schema, which describes the structure of all cpm-modells, whom are valide ones.<br>
So the class can give informations about all xml-nodes and xml-attributes which can be used in a new cpm-modell.<br>
For example: how often a node can appear, which kinds a node should have and which values a attribute can have.<br>
With this class you can also add new values for specific attributes or remove old values.<br>
*/
class XSD
{
public:
    /*! This container describes all informations for a simpletype. */
    struct SimpleTypeInfo {
        QString name; /*!< Name of the type.*/
        QDomNode xsdNode; /*!< schema-node of the type.*/
        QString default_val; /*!< Default-value of the typ.*/
        QString docu; /*!< Short informations about this attribute.*/
        QString pattern; /*!< RegEx Pattern that can validate if a value conforms to the type */
        QString base_type;
        bool is_enum; /*!< Describes if it is a enum or not */
        QStringList value_set; /*! All values allowed in a enum type */
        QHash<QString,int> value_instances;
        QRegExpValidator *validator; /*!< Validator that is used to mask the values for this type. */
    };

    XSD();

    const static QStringList dynamicTypeDefs;
    const static QStringList dynamicTypeRefs;
    void registerEnumValue(QString simple_type, QString value); /*!< Registers a new value for the given simpletype. */
    void removeEnumValue(QString simple_type, QString value); /*!< Removes the value from the given simpletype. */

    QString getXSDSchemaFile(); /*!< Returns the path to the cpmschema-file. */
    QDomElement getXSDElement(); /*!< Returns the rootnode of the cpmschema. */
    const QHash<QString, QSharedPointer< SimpleTypeInfo> >& getSimpleTypes() const { return simple_types; } /*!< Returns a list of all simpleTypes. */
    const QHash<QString, QDomNode>& getComplexTypes() const { return complex_types; } /*!< Returns a list of all complexTypes. */
    /*!< Returns the xsd-schema-file.*/

private:

    void createTypeMaps(); /*!< Creates the map of types from reading the cpmschema. */
    QString schema_path; /*!< The stored cpmschema. */
    QHash<QString, QDomNode> complex_types; /*!< All complextypes. */
    QHash<QString, QSharedPointer< SimpleTypeInfo> > simple_types; /*!< All simpletypes. */
    QDomDocument xsdSchema;

    static QString getDefaultValue(XSD::SimpleTypeInfo); /*!< Returns the default-value for the given simpletype. */
    static QString getPattern(QString simple_type); /*!< Returns the pattern for the given simpletype. */
};

#endif // XSD_H
