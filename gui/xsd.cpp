#include "xsd.h"
#include "config.h"
#include "settingsdialog.h"


using namespace std;

//------------------------------------------------------------------------------

const QStringList XSD::dynamicTypeDefs = QStringList() << "cpmCellTypeDef" << "cpmDoubleSymbolDef" << "cpmVectorSymbolDef" << "cpmQueueSymbolDef"
		<< "cpmWritableDoubleSymbolDef" <<  "cpmWritableDoubleVectorSymbolDef";
const QStringList XSD::dynamicTypeRefs = QStringList() << "cpmCellTypeRef" << "cpmDoubleSymbolRef" << "cpmVectorSymbolRef" << "cpmQueueSymbolRef"
		<<  "cpmWritableDoubleSymbolRef" << "cpmWritableDoubleVectorSymbolRef";

XSD::XSD() {

//     QString schema = config::getInstance()->getApplication().general_xsdPath;
// 
//     while ( !QFile::exists( schema ) ) {
// 
//         stringstream error;
//         error << "Cannot locate the Schema for Morpheus Models\n";
//         error << "Please provide a path to the XSD file (i.e. morpheus.xsd)\n";
//         QMessageBox::warning((QWidget*)0, "Error", error.str().c_str());
//         settingsDialog settingsDia;
// 		int ret = settingsDia.exec();
// 		if (ret==QDialog::Rejected) {
// 			throw QString("Unable to open Schema for Morpheus Models.\n Cannot create any model.");
// 		}
// 
//         schema = config::getInstance()->getApplication().general_xsdPath;
// 
//     }
// 
//     schema_path = schema;
//     cout << "Loading XML-Schema from " << schema_path.toStdString() << endl;
    QFile xsdSchemaFile(":/morpheus.xsd");

    xsdSchemaFile.open(QIODevice::ReadOnly);
    if ( ! xsdSchema.setContent(&xsdSchemaFile) ) {
         cerr <<"Cannot create internal DOM structure from the given XSD document!" << std::endl;
         exit(-1);
    }
    xsdSchemaFile.close();

    createTypeMaps();

}

//------------------------------------------------------------------------------

QString XSD::getXSDSchemaFile() {
    return schema_path;
}

//------------------------------------------------------------------------------

QDomElement XSD::getXSDElement()
{
    return xsdSchema.documentElement();
}

//------------------------------------------------------------------------------

void XSD::createTypeMaps()
{
    simple_types.clear();
    complex_types.clear();

    QDomNodeList list = xsdSchema.elementsByTagName("xs:simpleType");
    for(int i = 0; i < list.size(); i++)
    {
        SimpleTypeInfo info;
        info.is_enum = false;
        info.xsdNode = list.at(i);
        info.name = info.xsdNode.attributes().namedItem("name").nodeValue();
        info.base_type = info.xsdNode.firstChildElement("xs:restriction").attributes().namedItem("base").nodeValue();
        if( ! info.xsdNode.firstChildElement("xs:annotation").firstChildElement("xs:documentation").isNull() ) {
            QStringList docu_lines = info.xsdNode.firstChildElement("xs:annotation").firstChildElement("xs:documentation").text().split("\n",QString::KeepEmptyParts);
            for (int i=0; i<docu_lines.size(); i++) { docu_lines[i] = docu_lines[i].trimmed(); }
            while (docu_lines.size()>0 && docu_lines.first() == "" ) docu_lines.removeFirst();
            while (docu_lines.size()>0 && docu_lines.last() == "" ) docu_lines.removeLast();
            info.docu = docu_lines.join("\n");

        }
        else {
            info.docu = "(empty)";
        }

        if ( ! info.xsdNode.firstChildElement("xs:restriction").firstChildElement("xs:enumeration").isNull() )
        {
            QDomNodeList enums = info.xsdNode.firstChildElement("xs:restriction").toElement().elementsByTagName("xs:enumeration");
            for (int i=0; i<enums.size(); i++ )
            {
                info.value_set.push_back( enums.at(i).attributes().namedItem("value").nodeValue() );
            }
            info.pattern = QString("^(") + info.value_set.join("|") + ")$";
            info.is_enum = true;
        }
        else if (info.base_type == "xs:boolean" ){
            info.value_set.push_back("false");
            info.value_set.push_back("true");
            info.pattern = "^\\b(true|false)\\b$";
            info.is_enum = true;
        }
        else if ( ! info.xsdNode.firstChildElement("xs:restriction").firstChildElement("xs:pattern").isNull() ) {
            info.pattern = info.xsdNode.firstChildElement("xs:restriction").firstChildElement("xs:pattern").attributes().namedItem("value").nodeValue();
        }
        else
        {
            info.pattern = XSD::getPattern(info.base_type);
        }
        info.default_val = XSD::getDefaultValue(info);
        info.validator = new QRegExpValidator( QRegExp (info.pattern), 0 );
        
        if ( info.name.endsWith("SymbolRef") ) {
            QStringList tmp = info.value_set;
            info.value_set.clear();
            simple_types[info.name] = QSharedPointer< XSD::SimpleTypeInfo>( new XSD::SimpleTypeInfo(info) );
            for (int i=0; i<tmp.size(); i++ )
            {
                registerEnumValue(info.name,tmp[i]);
            }
        }
        else {
            simple_types[info.name] = QSharedPointer< XSD::SimpleTypeInfo>( new XSD::SimpleTypeInfo(info) );
        }
            
    }

    list = xsdSchema.elementsByTagName("xs:complexType");
    for(int i = 0; i < list.size(); i++)
    {
        QDomNode node = list.at(i);
        QString simpleName = node.attributes().namedItem("name").nodeValue();
        complex_types[simpleName] = node;
    }
}

//------------------------------------------------------------------------------

QString XSD::getDefaultValue(XSD::SimpleTypeInfo info)
{

    if ( ! info.value_set.empty() )
        { return info.value_set.front(); }
    else
    {
        if ( info.base_type == "xs:integer" )
            {return "0";}
        if ( info.base_type == "xs:decimal" )
            {return "0.0";}
        if ( info.base_type == "xs:double" )
            {return "0.0";}
		if (info.name == "cpmDoubleVector")
			return "0.0, 0.0, 0.0";
		if (info.name == "cpmIntegerVector")
			return "0, 0, 0";
		if (info.name == "cpmDoubleQueue")
			return "0, 0";

        if ( info.base_type == "xs:string" ) {
            if (info.pattern == "-?\\d+(\\.\\d+)?\\s-?\\d+(\\.\\d+)?\\s-?\\d+(\\.\\d+)?")
                return "0.0, 0.0, 0.0";
            if (info.pattern == "-?\\d+(\\.\\d+)?(e[-\\+]\\d{2})?\\s-?\\d+(\\.\\d+)?(e[-\\+]\\d{2})?\\s-?\\d+(\\.\\d+)?(e[-\\+]\\d{2})?")
                return "0.0, 0.0, 0.0";
            if (info.pattern == "-?\\d+\\s-?\\d+\\s-?\\d+")
                {return "0 0 0";}
            if (info.pattern == "(-?\\d+\\s-?\\d+\\s-?\\d+;){1,}")
                {return "0 0 0";}
            if (info.pattern == "(-?\\d+\\s-?\\d+\\s-?\\d+;)*(-?\\d+\\s-?\\d+\\s-?\\d+)")
                return "0 0 0";
            if (info.pattern == "(-?\\d+(\\.\\d+)?\\s)*-?\\d+(\\.\\d+)?")
                {return "0";}
            if (info.pattern == ".*")
                {return "";}
//            QMessageBox::information(null,"Error", "XSD::getDefaultValue: No default value available for the pattern specified in XSD!!!\nPattern: '" + info.pattern + "'!");
            Q_ASSERT(0);
        }

        // info.base_type == "xs:token"  | "xs:normalizedString" | "xs:string"
        return "";
    }

}

//------------------------------------------------------------------------------

QString XSD::getPattern(QString base)
{
    if(base == "xs:token")
    {return QString("^\\S*$");}

    if(base == "xs:integer")
    {return QString("^-?\\d+$");}

    if(base == "xs:boolean")
    {return QString("^\\b(true|false)\\b$");}

    if(base == "xs:decimal")
//    {return QString("^-?\\d+(\\.\\d+)?$");}
    {return QString("^-?\\d+(\\.\\d+)?(e[+-]?\\d+)?$");}

    if(base == "xs:double")
//    {return QString("^-?\\d+(\\.\\d+)?$");}
    {return QString("^-?\\d+(\\.\\d+)?(e[+-]?\\d+)?$");}

    if(base == "xs:normalizedString")
    {return QString(".*");}

    if(base == "xs:string")
    {return QString(".*");}

    return QString(".*");
}

//------------------------------------------------------------------------------

void XSD::registerEnumValue(QString simple_type, QString value)
{
    Q_ASSERT(simple_types.find(simple_type) != simple_types.end() );

	if (simple_type == "cpmCellTypeRef") {
		registerEnumValue("cpmDoubleSymbolRef", QString("celltype.").append(value).append(".id"));
		registerEnumValue("cpmDoubleSymbolRef", QString("celltype.").append(value).append(".size"));
	}

    if ( ! simple_types[simple_type]->value_set.contains( value ) )
    {
        simple_types[simple_type]->value_set.push_back(value);
        simple_types[simple_type]->value_instances[value]=1;
        simple_types[simple_type]->value_set.sort();
        simple_types[simple_type]->is_enum=true;

        if (simple_type == "cpmVectorSymbolRef") {
            registerEnumValue("cpmDoubleSymbolRef", value+".x");
            registerEnumValue("cpmDoubleSymbolRef", value+".y");
            registerEnumValue("cpmDoubleSymbolRef", value+".z");
			registerEnumValue("cpmDoubleSymbolRef", value+".abs");
			registerEnumValue("cpmDoubleSymbolRef", value+".phi");
			registerEnumValue("cpmDoubleSymbolRef", value+".theta");
        }
        else if (simple_type == "cpmWritableDoubleVectorSymbolRef") {
			registerEnumValue("cpmDoubleVectorSymbolRef", value);
        }
        else  if (simple_type == "cpmWritableDoubleSymbolRef") {
			 registerEnumValue("cpmDoubleSymbolRef", value);
        }
        
//         qDebug() << "registering enum value " << value << " for type " << simple_type;
        // --> update pattern !!
        QStringList enums = simple_types[simple_type]->value_set;
        simple_types[simple_type]->pattern = QString("^(") + enums.join("|") + ")$";
        simple_types[simple_type]->validator->setRegExp(QRegExp(simple_types[simple_type]->pattern));
    }
    else {
        simple_types[simple_type]->value_instances[value]+=1;
    }
}

//------------------------------------------------------------------------------

void XSD::removeEnumValue(QString simple_type, QString value)
{
    Q_ASSERT(simple_types.find(simple_type) != simple_types.end() );

    if ( simple_types[simple_type]->value_set.contains( value ) && simple_types[simple_type]->value_instances.contains(value))
    {
		if (simple_type == "cpmCellTypeRef") {
			removeEnumValue("cpmDoubleSymbolRef", QString("celltype.").append(value));
		}
        if (simple_types[simple_type]->value_instances[value] == 1) {
            simple_types[simple_type]->value_instances.remove(value);
            simple_types[simple_type]->value_set.removeAll(value);
            simple_types[simple_type]->is_enum=true;

            if (simple_type == "cpmVectorSymbolRef") {
                removeEnumValue("cpmDoubleSymbolRef", value+".x");
                removeEnumValue("cpmDoubleSymbolRef", value+".y");
                removeEnumValue("cpmDoubleSymbolRef", value+".z");
				removeEnumValue("cpmDoubleSymbolRef", value+".abs");
				removeEnumValue("cpmDoubleSymbolRef", value+".phi");
				removeEnumValue("cpmDoubleSymbolRef", value+".theta");
            }

            // --> update pattern !!
            QStringList enums = simple_types[simple_type]->value_set;
            if(enums.size() != 0)
            {
                simple_types[simple_type]->pattern = QString("^(") + enums.join("|") + ")$";
            }
            else
            {
                simple_types[simple_type]->pattern = "^(...)$";
            }
            simple_types[simple_type]->validator->setRegExp(QRegExp(simple_types[simple_type]->pattern));
        }
        else {
            simple_types[simple_type]->value_instances[value] -= 1;

        }
    }
    else
        cout << "XSD: not removing non-dynamic value '" << value.toStdString() << "' from SimpleType '" << simple_type.toStdString() << "' !"<< endl;
}
