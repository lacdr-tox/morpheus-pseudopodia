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

QSharedPointer<XSD::SimpleTypeInfo> XSD::getSimpleType(QString name) const {
	auto it = simple_types.constFind(name);
	if (it==simple_types.constEnd()) {
		throw QString("Referred Simple Type '%1' is not defined ").arg(name);
	}
	
	if ( ! (*it)->initialized ) {
		const_cast<XSD*>(this)->initSimpleType(*it);
	}
	return *it;
}

QSharedPointer<XSD::ComplexTypeInfo> XSD::getSimpleContentType(QString name) const {
	auto it = simple_content_types.constFind(name);
	if (it==simple_content_types.constEnd()) {
		throw QString("Referred Simple Type '%1' is not defined ").arg(name);
	}
	
	if ( ! (*it)->initialized ) {
		const_cast<XSD*>(this)->initComplexType(*it);
	}
	return *it;
}

QSharedPointer<XSD::ComplexTypeInfo> XSD::getComplexType(QString name) const {
	auto it = complex_types.constFind(name);
	if (it!=complex_types.constEnd()) {
		if ( ! (*it)->initialized ) {
			const_cast<XSD*>(this)->initComplexType(*it);
		}
		return *it;
	}

	auto it2 = simple_content_types.constFind(name);
	if (it2 != simple_content_types.constEnd()) {
		if ( ! (*it2)->initialized ) {
			const_cast<XSD*>(this)->initComplexType(*it2);
		}
		return *it2;
	}

	auto it3 = simple_types.constFind(name);
	if (it3 != simple_types.constEnd()) {
		if ( ! (*it3)->initialized ) {
			const_cast<XSD*>(this)->initSimpleType(*it3);
		}
		auto info = QSharedPointer<XSD::ComplexTypeInfo>::create();
// 		info->name = it3->name;
		info->text_type = *it3;
		info->content_type = ComplexTypeInfo::SimpleContent;
		
		return info;
	}
	
	throw QString("Referred Complex Type '%1'not defined ").arg(name);
}

void XSD::initSimpleType(QSharedPointer<XSD::SimpleTypeInfo> info) {
	if (info->initialized) return;
	info->initialized = true; // TODO: prevents circular dependencies to create infinite loops, should be done by tracking the initialisation
	if (! info->base_type.isEmpty()) {
		if (simple_types.contains(info->base_type)) {
			auto base_type = getSimpleType(info->base_type);
			if (base_type->mode == SimpleTypeInfo::ENUM) {
				info->is_enum = true;
				info->mode = SimpleTypeInfo::ENUM;
				if (info->value_set.isEmpty()) {
					info->value_set = base_type->value_set;
					info->pattern = base_type->pattern;
					info->default_val = base_type->default_val;
				}
			}
			else if (info->pattern.isEmpty()) {
				info->pattern = base_type->pattern;
				if (info->default_val.isEmpty()) {
					// assume the parental default also works here
					info->default_val = base_type->default_val;
				}
			}
			if (info->default_val.isEmpty()) {
				info->default_val = base_type->default_val;
			}
		}
		else if (simple_content_types.contains(info->base_type)) {
			auto base_type = getSimpleContentType(info->base_type);
			info->attributes.append(base_type->attributes);
			if (info->pattern.isEmpty()) {
				info->pattern = base_type->text_type->pattern;
			}
			if (info->default_val.isEmpty()) {
				info->default_val = base_type->text_type->default_val;
			}
		}
		else {
			auto base_type = getSimpleType(info->base_type);
		}
	}

	if (info->name == "cpmMathExpression")
		info->default_val = "0.0";
	else if (info->name == "cpmDoubleVector" || info->name == "cpmVectorMathExpression")
		info->default_val = "0.0, 0.0, 0.0";
	else if (info->name == "cpmIntegerVector")
		info->default_val =  "0, 0, 0";
	else if (info->name == "cpmDoubleQueue")
		info->default_val =  "0, 0";
	
	info->validator.setRegExp( QRegExp(info->pattern) );
// 	info->initialized = true;
}

QSharedPointer<XSD::SimpleTypeInfo>  XSD::parseSimpleType(QDomNode xsdNode) {
	auto info = QSharedPointer<XSD::SimpleTypeInfo>::create();
	info->is_enum = false;
	info->xsdNode = xsdNode;
	info->initialized = false;
	if (xsdNode.attributes().contains("name")) {
		info->name = xsdNode.attributes().namedItem("name").nodeValue();
	}
	
// 	auto sub_node = info->xsdNode.firstChildElement();
	auto xrestriction = info->xsdNode.firstChildElement("xs:restriction");
	if (xrestriction.isElement()) {
		info->base_type = xrestriction.attributes().namedItem("base").nodeValue();
		if ( ! xrestriction.firstChildElement("xs:enumeration").isNull() )
		{
			info->is_enum = true;
			info->mode = SimpleTypeInfo::ENUM;
			QDomNodeList enums = xrestriction.elementsByTagName("xs:enumeration");
			for (int i=0; i<enums.size(); i++ )
			{
				info->value_set.push_back( enums.at(i).attributes().namedItem("value").nodeValue() );
			}
			if (info->value_set.isEmpty()) {
				throw QString("Invalid SimpleType restriction in XSD type definition of type '%1'. No enumeration values specified").arg(info->name);
			}
			else {
				info->default_val = info->value_set.first();
				info->pattern = info->value_set.join("|");
				QString escape = "\\";
				info->pattern.replace(QRegExp("([\\[\\]\\(\\)\\.\\+])"), escape + "\\1");
				info->pattern.prepend("^(").append(")$");
				
			}
		}
		else if ( ! xrestriction.firstChildElement("xs:pattern").isNull() ) {
			info->pattern = xrestriction.firstChildElement("xs:pattern").attributes().namedItem("value").nodeValue();
		}
		else if (! xrestriction.firstChildElement("xs:minInclusive").isNull() | ! xrestriction.firstChildElement("xs:maxInclusive").isNull()){
			info->mode = SimpleTypeInfo::NUMERIC_CONSTRAINT;
			info->num_min_value = numeric_limits<double>::min();
			info->num_max_value = numeric_limits<double>::max();
			if (! xrestriction.firstChildElement("xs:minInclusive").isNull()) {
				info->num_min_value = xrestriction.firstChildElement("xs:minInclusive").attributes().namedItem("value").nodeValue().toDouble();
			}
			if (! xrestriction.firstChildElement("xs:maxInclusive").isNull()) {
				info->num_max_value = xrestriction.firstChildElement("xs:maxInclusive").attributes().namedItem("value").nodeValue().toDouble();
			}
		}
		else if (xrestriction.firstChildElement().isNull()) {
// 			qDebug() << "Empty restriction in XSD type definition of type " << info->name;
		}
		else {
			throw QString("Invalid restriction '%1' in XSD type definition of type '%2'.")
				.arg(xrestriction.firstChildElement().isElement() ? xrestriction.firstChildElement().toElement().nodeName() : "")
				.arg(info->name);
			/* TODO Fail on Length and WhiteSpace Restrictions. */
		}
	}
	else if (info->xsdNode.firstChildElement("xs:extension").isElement()) {
		auto type_root = info->xsdNode.firstChildElement("xs:extension");
		info->base_type = type_root.attributes().namedItem("base").nodeValue();
		
		auto list = type_root.firstChildElement("xs:list");
		if (list.isElement()) {
			info->mode = SimpleTypeInfo::CS_LIST;
			info->separator = ",";
			if (list.attributes().namedItem("separator").isAttr()) {
				info->separator = list.attributes().namedItem("separator").nodeValue();
			}
		}
		
		auto attr = type_root.firstChildElement("xs:attribute");
		while ( attr.isElement() ) {
			info->attributes.append(attr);
			attr = attr.nextSiblingElement("xs:attribute");
		}
	}
	else {
		QTextStream s;
		xsdNode.save(s,QDomNode::EncodingPolicy::EncodingFromTextStream);
		throw QString("Ill defined simpleType / simpleContent\n") + *s.string();
	}
	
	auto docu =  info->xsdNode.firstChildElement("xs:annotation").firstChildElement("xs:documentation");
	if( ! docu.isNull() ) {
		QStringList docu_lines = docu.text().trimmed().split("\n",QString::KeepEmptyParts);
		for ( auto& line : docu_lines) { line = line.trimmed(); }
		info->documentation = docu_lines.join("\n");
	}
	else {
		info->documentation = "";
	}

	if (! init_phase) {
		initSimpleType(info);
	}

	return info;
}

XSD::GroupInfo XSD::parseGroup(QDomNode xsdNode) {
// 	QDomNodeList xsd_childs = xsdNode.childNodes();
// 	QList<QDomNode> child_elements;
	XSD::GroupInfo info;
	
	if (xsdNode.nodeName() == "xs:group") {
		info.name = xsdNode.attributes().namedItem("name").nodeValue();
		xsdNode = xsdNode.firstChildElement();
		while (xsdNode.nodeName() != "xs:choice" && xsdNode.nodeName() != "xs:all" &&  xsdNode.nodeName() != "xs:sequence") {
			if (xsdNode.isNull()) return info;
			xsdNode = xsdNode.nextSiblingElement();
		}
	}
	
	info.max_occurs="1";
	if (xsdNode.attributes().contains("maxOccurs"))
		info.max_occurs = xsdNode.attributes().namedItem("maxOccurs").nodeValue();
	info.min_occurs="1";
	if (xsdNode.attributes().contains("minOccurs"))
		info.min_occurs = xsdNode.attributes().namedItem("minOccurs").nodeValue();
	
	QString default_min_occurs = "";
	QString default_max_occurs = "1";
	if ( xsdNode.nodeName() == "xs:choice" ) {
		info.is_choice = true;
		default_min_occurs = "0";
	}
	else if ( xsdNode.nodeName() == "xs:all" ) {
		info.is_choice = false;
		default_min_occurs = "1";
	}
	else if ( xsdNode.nodeName() == "xs:sequence" ) {
		throw QString("Prohibited group specifier ") + xsdNode.nodeName() + " found in XSD schema.";
	}
	else { throw QString("Unknown group specifier ") + xsdNode.nodeName() + " in XSD schema."; }

	// Collapse all childs into the elements list
	auto child = xsdNode.firstChildElement();
	while ( ! child.isNull() )
	{
		if (child.nodeName()=="xs:group") {
			// parsing elements referenced via a group ref
			QString group_name = child.attributes().namedItem("ref").nodeValue();
			auto gr_ref = group_defs.constFind(group_name);
			if (gr_ref==group_defs.constEnd()) {
				throw(QString("Unable to find referred XSD group %1.").arg(group_name));
			}
			const auto& group = gr_ref.value();
			if (info.is_choice != group.is_choice) {
				QString error = QString("Unable to merge XSD group %1 into Type %2.\n Incompatible compositors.\n").arg(group_name).arg(info.name);
				QTextStream err_stream(&error);
				xsdNode.save(err_stream,0);
				throw error;
			}

			for (auto gr_child = group.children.begin(); gr_child != group.children.end(); gr_child++) {
				info.children.insert(gr_child.key(), gr_child.value());
			}
			if (info.is_choice && info.default_choice.isEmpty())
				info.default_choice = gr_ref.value().default_choice;
		}
		else if(child.nodeName() == "xs:element") {
			XSD::ChildInfo child_info;
			auto attr = child.attributes() ;
			child_info.name = attr.namedItem("name").nodeValue();
			
			child_info.max_occurs = default_max_occurs;
			child_info.min_occurs = default_min_occurs;
			if ( attr.contains("minOccurs")) { child_info.min_occurs = attr.namedItem("minOccurs").nodeValue(); }
			if ( attr.contains("maxOccurs")) { child_info.max_occurs = attr.namedItem("maxOccurs").nodeValue(); }
			if ( attr.contains("default")) { child_info.default_val = attr.namedItem("default").nodeValue(); }

			if ( attr.contains("type") ) { 
				child_info.type_name = attr.namedItem("type").nodeValue();
			}
			else {
				auto xsd_type = child.firstChildElement("xs:complexType");
				if ( ! xsd_type.isNull() ) {
					child_info.type = parseComplexType(xsd_type);
				}
				else if ( !(xsd_type = child.firstChildElement("xs:SimpleType")).isNull() ) {
					child_info.type = QSharedPointer<XSD::ComplexTypeInfo>::create();
					child_info.type->text_type = parseSimpleType(xsd_type);
					child_info.type->content_type = ComplexTypeInfo::SimpleContent;
				}
				else {
					throw QString("Embedded simple or complex Type expected when parsing element %1").arg(child_info.name);
				}
			}
			
			if (info.is_choice && info.default_choice.isEmpty())
				info.default_choice = child_info.name;
			
			info.children[child_info.name] = child_info;
		}
		else {
			throw QString("Unknown child type %1 ").arg(child.nodeName());
		}
		child = child.nextSiblingElement();
	}

//     // need to get the referred type in order to read the docu
//     QDomNode infoNode;
//     if ( ! elemNode.firstChildElement("xs:annotation").firstChildElement("xs:documentation").isNull() ) {
//         infoNode = elemNode;
//     }
//     else {
// 		if (child_info.is_complexType) {
// 			infoNode = child_info.xsdChildType;
// 		}
// 		else {
// 			child_info.info = child_info.simpleContentType->docu;
// 		}
//     }
// 
//     if (!infoNode.isNull() ) {
//         QStringList docu_lines = infoNode.firstChildElement("xs:annotation").firstChildElement("xs:documentation").text().split("\n");
//         for (int i=0; i<docu_lines.size(); i++) { docu_lines[i] = docu_lines[i].trimmed(); }
//         while (docu_lines.size()>0 && docu_lines.first() == "" ) docu_lines.removeFirst();
//         while (docu_lines.size()>0 && docu_lines.last() == "" ) docu_lines.removeLast();
//         child_info.info = docu_lines.join("\n");
//     }
// 
// 
// 	QDomNode appInfoNode;
// 	if(!elemNode.firstChildElement("xs:annotation").firstChildElement("xs:appinfo").isNull()) {
// 		appInfoNode = elemNode;
// 	}
// 	else {
// 		appInfoNode = child_info.xsdChildType;
// 	}
//     child_info.pluginClass = appInfoNode.firstChildElement("xs:annotation").firstChildElement("xs:appinfo").text();
	
	return info;
}

QSharedPointer<XSD::ComplexTypeInfo> XSD::parseComplexType(QDomNode xsdNode) {
	Q_ASSERT(xsdNode.nodeName() == "xs:complexType");
	auto info = QSharedPointer<XSD::ComplexTypeInfo>::create();
	info->name = xsdNode.attributes().namedItem("name").nodeValue();
	info->xsdNode = xsdNode;
	if (info->xsdNode.firstChildElement("xs:simpleContent").isNull())
		info->content_type = ComplexTypeInfo::ComplexContent;
	else
		info->content_type = ComplexTypeInfo::SimpleContent;
	
	if (! init_phase) {
		initComplexType(info);
	}
	
	return info;
}

void XSD::initComplexType(QSharedPointer<XSD::ComplexTypeInfo> info)
{
	if (info->initialized) return;

	auto sub_node = info->xsdNode.firstChildElement();
	while (! sub_node.isNull()) {
		if (sub_node.nodeName() == "xs:attribute") {
			// TODO override existing attributes with same name
			auto attr_name = sub_node.attributes().namedItem("name").nodeValue();
			for (const auto& attr : info->attributes) {
				if (attr.attributes().namedItem("name").nodeValue() == attr_name){
					info->attributes.removeAll(attr);
					break;
				}
			}
			info->attributes.append(sub_node);
		}
		else if (sub_node.nodeName() == "xs:sequence" || sub_node.nodeName() == "xs:choice" || sub_node.nodeName() == "xs:all") {
			
			if (info->base_type.isEmpty())  {
				info->child_info = parseGroup(sub_node);
			}
			else {
				
				auto child_group = parseGroup(sub_node); 
				if (info->content_type == ComplexTypeInfo::SimpleContent) {
					throw QString("Refusing to derive a ComplexType %1 from SimpleType %2. Mixing child elements (choice/sequence/all) and simpleContent is not allowed.").arg(info->name).arg(info->base_type);
				}
				if (!info->child_info.children.isEmpty() && info->child_info.is_choice !=  child_group.is_choice) {
					throw QString("Refusing to derive a ComplexType %1 from ComplexType %2. ComplexTypes have different element containment (choice/sequence/all)").arg(info->name).arg(info->base_type);
				}
				
				info->child_info.is_choice = child_group.is_choice;
				info->child_info.min_occurs = child_group.min_occurs;
				info->child_info.max_occurs = child_group.max_occurs;
				
				if (sub_node.attributes().contains("minOccurs"))
					info->child_info.min_occurs = child_group.min_occurs;
				if (sub_node.attributes().contains("maxOccurs"))
					info->child_info.max_occurs = child_group.max_occurs;
				
// 				info->child_info.children.unite(child_group.children);
				// manually assign to allow key override
				for (const auto& child : child_group.children)
					info->child_info.children[child.name] = child;
				
				if (info->child_info.default_choice.isEmpty())
					info->child_info.default_choice = child_group.default_choice;
			}
		}
		else if (sub_node.nodeName() == "xs:complexContent") {
			sub_node = sub_node.firstChildElement();
			continue;
		}
		else if (sub_node.nodeName() == "xs:simpleContent") {
			auto simple_content = parseSimpleType(sub_node);
// 		simple_content->name = info->name;
// 		auto compl_type = QSharedPointer<XSD::ComplexTypeInfo>::create();
			info->content_type = XSD::ComplexTypeInfo::SimpleContent;
			info->text_type = simple_content;
			info->attributes.swap( simple_content->attributes );
			simple_content_types[info->name] = info;
			initSimpleType(info->text_type);
			// in case we have attributes attached to the simple content
			info->attributes.append( info->text_type->attributes );
		}
		else if (sub_node.nodeName() == "xs:extension") {
			
			info->base_type = sub_node.attributes().namedItem("base").nodeValue();
			auto base_type = getComplexType(info->base_type);
			
			if (base_type->content_type == ComplexTypeInfo::SimpleContent) {
				info->text_type = base_type->text_type;
				info->attributes.append( base_type->attributes );
				info->content_type = ComplexTypeInfo::SimpleContent;
			}
			else {
				info->attributes = base_type->attributes;
				info->child_info = base_type->child_info;
			}
			
			sub_node =  sub_node.firstChildElement();
			continue;
		}
		else if (sub_node.nodeName() == "xs:annotation") {
			auto docu =  sub_node.firstChildElement("xs:documentation");
			if( ! docu.isNull() ) {
				QStringList docu_lines = docu.text().trimmed().split("\n",QString::KeepEmptyParts);
				for ( auto& line : docu_lines) { line = line.trimmed(); }
				info->documentation = docu_lines.join("\n");
			}
			auto appinfo = sub_node.firstChildElement("xs:appinfo");
			if( ! appinfo.isNull() ) {
				info->pluginClass = appinfo.text();
			}
		}
		else {
			throw QString("Unknown definition '%1' in complexType.").arg(sub_node.nodeName());
		}
		sub_node = sub_node.nextSiblingElement();
	}
	
	
	if (info->content_type == ComplexTypeInfo::SimpleContent) {
	}
	else/* if (info->content_type == ComplexTypeInfo::ComplexContent) */ {
		// Initialize all child types
		for (auto& child : info->child_info.children) {
			if (child.type)
				initComplexType(child.type);
			else
				child.type = getComplexType(child.type_name);
		}
	}
	
	info->initialized = true;
}


//------------------------------------------------------------------------------

void XSD::createTypeMaps()
{
	simple_types.clear();
	complex_types.clear();
	init_phase = true;

	auto info = QSharedPointer<SimpleTypeInfo>::create();
	info->initialized = false;
	info->name = "xs:token";
	info->pattern = "^\\S*$";
	info->mode = SimpleTypeInfo::PATTERN;
	info->default_val= "";
	simple_types[info->name] = info;

	info = QSharedPointer<SimpleTypeInfo>::create();
	info->initialized = false;
	info->name ="xs:integer";
	info->pattern = "^-?\\d+$";
	info->mode = SimpleTypeInfo::PATTERN;
	info->default_val= "0";
	simple_types[info->name] = info;

	info = QSharedPointer<SimpleTypeInfo>::create();
	info->initialized = false;
	info->name ="xs:decimal";
	info->pattern = "^-?\\d+(\\.\\d+)?(e[+-]?\\d+)?$";
	info->mode = SimpleTypeInfo::PATTERN;
	info->default_val= "0";
	simple_types[info->name] = info;

	info = QSharedPointer<SimpleTypeInfo>::create();
	info->initialized = false;
	info->name ="xs:double";
	info->pattern = "^-?\\d+(\\.\\d+)?(e[+-]?\\d+)?$";
	info->mode = SimpleTypeInfo::PATTERN;
	info->default_val= "0";
	simple_types[info->name] = info;

	info = QSharedPointer<SimpleTypeInfo>::create();
	info->initialized = false;
	info->name ="xs:normalizedString";
	info->pattern = ".*";
	info->mode = SimpleTypeInfo::PATTERN;
	info->default_val= "";
	simple_types[info->name] = info;

	info = QSharedPointer<SimpleTypeInfo>::create();
	info->initialized = false;
	info->name ="xs:string";
	info->pattern = ".*";
	info->mode = SimpleTypeInfo::PATTERN;
	info->default_val= "";
	simple_types[info->name] = info;

	info = QSharedPointer<SimpleTypeInfo>::create();
	info->initialized = false;
	info->name ="xs:boolean";
	info->value_set << "true" << "false";
	info->pattern = "^(true|false)$";
	info->default_val= "true";
	info->is_enum= true;
	info->mode = SimpleTypeInfo::ENUM;
	simple_types[info->name] = info;

	QDomNodeList list = xsdSchema.elementsByTagName("xs:simpleType");
	for(int i = 0; i < list.size(); i++)
	{
		if (list.at(i).attributes().namedItem("name").nodeValue().isEmpty())
			continue;
		auto info = parseSimpleType(list.at(i));
		if ( dynamicTypeRefs.contains(info->name) ) {
			QStringList tmp = info->value_set;
			info->value_set.clear();
			simple_types[info->name] = info;
			for (int i=0; i<tmp.size(); i++ )
			{
				registerEnumValue(info->name,tmp[i]);
			}
		}
		else {
			simple_types[info->name] = info;
		}
// 		qDebug() << "Added Type "<< info->name;
	}
	
	list = xsdSchema.elementsByTagName("xs:group");
	for(int i = 0; i < list.size(); i++)
	{
		if (list.at(i).attributes().namedItem("name").nodeValue().isEmpty())
			continue;
		auto info = parseGroup(list.at(i));
		group_defs[info.name] = info;
// 		qDebug() << "Added Group "<< info.name;
	}

	list = xsdSchema.elementsByTagName("xs:complexType");
	for(int i = 0; i < list.size(); i++)
	{
		if (list.at(i).attributes().namedItem("name").nodeValue().isEmpty())
			continue;
		auto info = parseComplexType(list.at(i));
		if (info->content_type == ComplexTypeInfo::SimpleContent) {
			simple_content_types[info->name] = info;
		}
		complex_types[info->name] = info;
// 		qDebug() << "Added Type "<< info->name;
	}

	for (auto type : simple_types) {
		initSimpleType(type);
	}
// 	for (auto type : complex_types) {
// 		initComplexType(type);
// 	}
	init_phase = false;
// 	qDebug() << "done";
}

//------------------------------------------------------------------------------

// QString XSD::getPattern(QString base)
// {
// 	if (simple_types.contains(base)) {
// 		if ( ! simple_types[base]->pattern.isEmpty())
// 			return simple_types[base]->pattern;
// 		else 
// 			return getPattern(simple_types[base]->base_type);
// 	}
// //     if(base == "xs:token")
// //     {return QString("^\\S*$");}
// // 
// //     if(base == "xs:integer")
// //     {return QString("^-?\\d+$");}
// // 
// //     if(base == "xs:boolean")
// //     {return QString("^\\b(true|false)\\b$");}
// // 
// //     if(base == "xs:decimal")
// // //    {return QString("^-?\\d+(\\.\\d+)?$");}
// //     {return QString("^-?\\d+(\\.\\d+)?(e[+-]?\\d+)?$");}
// // 
// //     if(base == "xs:double")
// // //    {return QString("^-?\\d+(\\.\\d+)?$");}
// //     {return QString("^-?\\d+(\\.\\d+)?(e[+-]?\\d+)?$");}
// // 
// //     if(base == "xs:normalizedString")
// //     {return QString(".*");}
// // 
// //     if(base == "xs:string")
// //     {return QString(".*");}
// 
//     return QString(".*");
// }

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
		QString pattern = enums.join("|");
		QString escape = "\\";
		pattern.replace(QRegExp("([\\[\\]\\(\\)\\.\\+])"), escape + "\\1");
		simple_types[simple_type]->pattern =pattern;
		simple_types[simple_type]->validator.setRegExp(QRegExp(pattern));
	}
	else {
		simple_types[simple_type]->value_instances[value]+=1;
	}
// 	qDebug() << "Adding instance of Value " << value << " to Simple Type " << simple_type << " -> " << simple_types[simple_type]->value_instances[value];
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
// 			qDebug() << "Removing last instance of Value " << value << " from Simple Type " << simple_type;
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
			QString pattern;
			if(enums.size() != 0)
			{
				pattern = enums.join("|");
				QString escape = "\\";
				pattern.replace(QRegExp("([\\[\\]\\(\\)\\.\\+])"), escape + "\\1");
				simple_types[simple_type]->pattern = pattern.prepend("^(").append(")$");
			}
			else
			{
				pattern = "^(...)$";
			}
			simple_types[simple_type]->pattern = pattern;
			simple_types[simple_type]->validator.setRegExp(QRegExp(pattern));
		}
		else {
			simple_types[simple_type]->value_instances[value] -= 1;
// 			qDebug() << "Removing Value " << value << " from Simple Type " << simple_type << " -> " << simple_types[simple_type]->value_instances[value];
		}
	}
	else
		cout << "XSD: not removing non-dynamic value '" << value.toStdString() << "' from SimpleType '" << simple_type.toStdString() << "' !"<< endl;
}

