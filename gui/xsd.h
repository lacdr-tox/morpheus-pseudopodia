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
This class reads the  MorpheusML language scheme, which describes the structure of all  morpheus modells.<br>
The class provides pre-processed information on node and attribute types. 
In addition, dynamic enum types like defined symbols and cell types are managed by the XSD class.<br>
*/
class XSD
{

public:
	/*! This container describes all informations for a simpletype. */
	struct SimpleTypeInfo {
		
		QString name; /*!< Name of the type.*/
		QDomNode xsdNode; /*!< schema-node of the type.*/
		QString default_val; /*!< Default-value of the typ.*/
		QString documentation; /*!< Short informations about this attribute.*/
		QString pattern; /*!< RegEx Pattern that can validate if a value conforms to the type */
		QString base_type;
		QString separator;
		double num_min_value, num_max_value;
		QList<QDomNode> attributes;
		enum validation_mode { PATTERN, NUMERIC_CONSTRAINT, ENUM, CS_LIST };
		bool is_enum; /*!< Describes if it is a enum or not */
		validation_mode mode;
		QStringList value_set; /*! All values allowed in a enum type */
		
		bool initialized;
		QHash<QString,int> value_instances;
		QRegExpValidator validator; /*!< Validator that is used to mask the values for this type. */
	};
	
	struct ComplexTypeInfo;
	struct ChildInfo {
		QString name;
		QString type_name;
		QString default_val;
// 		QString pluginClass;
// 		QString documentation;
		
		QString min_occurs;
		QString max_occurs;
		QSharedPointer<ComplexTypeInfo> type;
	};

	/// Groups
	struct GroupInfo {
		QString name;
		
		QString min_occurs;
		QString max_occurs;
		
		bool is_choice;
		QString default_choice;
		QMap<QString,ChildInfo> children;
	};

	struct ComplexTypeInfo {
		QString name;
		QDomNode xsdNode;
		QString base_type;
		enum { SimpleContent, ComplexContent } content_type;
		
		bool initialized;
		
		bool is_abstract;
		QString pluginClass;
		QString documentation;
		
		QSharedPointer<SimpleTypeInfo> text_type;
		
		QList<QDomNode> attributes;
		GroupInfo child_info;
	};



    XSD();

    const static QStringList dynamicTypeDefs;
    const static QStringList dynamicTypeRefs;
    void registerEnumValue(QString simple_type, QString value); /*!< Registers a new value for the given simpletype. */
    void removeEnumValue(QString simple_type, QString value); /*!< Removes the value from the given simpletype. */

    QString getXSDSchemaFile(); /*!< Returns the path to the cpmschema-file. */
    QDomElement getXSDElement(); /*!< Returns the rootnode of the cpmschema. */
	QSharedPointer<XSD::SimpleTypeInfo> parseSimpleType(QDomNode xsdNode);
	XSD::GroupInfo parseGroup(QDomNode xsdNode);
	QSharedPointer<XSD::ComplexTypeInfo> parseComplexType(QDomNode xsdNode);

//     const QHash<QString, QSharedPointer< SimpleTypeInfo> >& getSimpleTypes() const { return simple_types; } /*!< Returns a list of all simpleTypes. */
	QSharedPointer< SimpleTypeInfo> getSimpleType(QString name) const; /// only look up simple types  (for attributes)
	QSharedPointer<ComplexTypeInfo> getComplexType(QString name) const; /// look up all types (for nodes)
	QSharedPointer<ComplexTypeInfo> getSimpleContentType(QString name) const; /// look up simple types and the simpleContent of complexTypes (for deriving simpleContent types)
//     const QHash<QString, QDomNode>& getComplexTypes() const { return complex_types; } /*!< Returns a list of all complexTypes. */
    
    /*!< Returns the xsd-schema-file.*/

private:
	Q_DISABLE_COPY(XSD);
	bool init_phase;
    void createTypeMaps(); /*!< Creates the map of types from reading the cpmschema. */
	void initSimpleType(QSharedPointer<XSD::SimpleTypeInfo> info);
// 	void initGroup(QSharedPointer<XSD::GroupInfo> info);
	void initComplexType(QSharedPointer<XSD::ComplexTypeInfo> info);
    QString schema_path; /*!< The stored cpmschema. */
    QHash<QString, QSharedPointer<ComplexTypeInfo> > complex_types; /*!< All complextypes. */
    QHash<QString, QSharedPointer<ComplexTypeInfo> > simple_content_types; /*!< All complextypes with simple content. */
    QHash<QString, QSharedPointer<SimpleTypeInfo> > simple_types; /*!< All simpletypes. */
    QHash<QString, GroupInfo> group_defs;
    QDomDocument xsdSchema;

//     static QString getDefaultValue(XSD::SimpleTypeInfo); /*!< Returns the default-value for the given simpletype. */
//     QString getPattern(QString simple_type); /*!< Returns the pattern for the given simpletype. */
};

#endif // XSD_H
