#include "abstractattribute.h"

ModelException::ModelException(Type t, QString m) :message(m), type(t)  {};

const char* ModelException::what() const throw() { return message.toStdString().c_str(); }


QMap<QString,QString> ModelDescriptor::getSymbolNames(QString type_name) const {
    QMap<QString,QString>  map;
    // add all symbols
    QStringList predef = xsd.getSimpleTypes()[type_name]->value_set;
    for ( int i=0; i<predef.size(); i++ ) {
		map[predef[i]] = "";
		if (predef[i].startsWith("celltype.")) {
			if (predef[i].endsWith(".size"))
				map[predef[i]] = QString("Population size of cell type \'") + predef[i].split(".")[1] + "\'";
			else if (predef[i].endsWith(".id"))
				map[predef[i]] = QString("Cell type id of \'") + predef[i].split(".")[1] + "\'";
		}
	}
	if (map.contains("cell.type")) {
		map["cell.type"] = "cell type id";
	}
	if (map.contains("cell.id")) {
		map["cell.id"] = "cell's unique id";
	}
	if (map.contains("cell.length")) {
		map["cell.length"] = "cell's length in lattice nodes";
	}
	if (map.contains("cell.orientation")) {
		map["cell.orientation"] = "cell's long axis";
	}
	if (map.contains("cell.volume")) {
		map["cell.volume"] = "cell size in lattice nodes";
	}
	if (map.contains("cell.surface")) {
		map["cell.surface"] = "cell's perimeter in lattice nodes";
	}
	if (map.contains("cell.center")) {
		map["cell.center"] = "cell position in lattice coordinates";
	}
	
    // add associated symbol names
    for ( auto it = symbolNames.constBegin(); it != symbolNames.constEnd(); it++) {
        // key is the symbol attribute, while value is the name attribute
        if (it.key()->isActive() && it.value()->isActive() && map.contains(it.key()->get()) )
		{
			map[it.key()->get()] = it.value()->get();
        }
    }
    return map;
}

AbstractAttribute::AbstractAttribute( QObject* parent, QDomNode xsdDescrNode, QDomNode xmlParentNode, QSharedPointer<ModelDescriptor> m_descr ) : QObject(parent)
{
    this->parentNode = xmlParentNode;
    this->model_descr = m_descr;
    is_active = false;
	is_disabled = false;

	// Parsing the XSD definition
	QString simpleTypeName;
    if (xsdDescrNode.nodeName() == "xs:attribute") {
        name = xsdDescrNode.attributes().namedItem("name").nodeValue();
        is_text = false;
        if(xsdDescrNode.attributes().namedItem("use").nodeValue() == "optional")
            { is_required = false;}
        else
            { is_required = true; }
        simpleTypeName = xsdDescrNode.attributes().namedItem("type").nodeValue();
    }
    else if (xsdDescrNode.nodeName() == "xs:element") {
        is_text = true;
        is_required = true;
        name = "#text";
		simpleTypeName = xsdDescrNode.attributes().namedItem("type").nodeValue();
    }
    else {
		throw ModelException(ModelException::UnknownXSDType, QString("Cannot create an AbstractAttribute for xsdNode %1.").arg(xsdDescrNode.nodeName()));
    }

//    qDebug() << "Attribute type " << simpleTypeName;

    Q_ASSERT( model_descr->xsd.getSimpleTypes().find(simpleTypeName) != model_descr->xsd.getSimpleTypes().end() );
    type = model_descr->xsd.getSimpleTypes()[ simpleTypeName ];
    if ( ! type ) {
        qDebug() << "Unknown attribute type " << simpleTypeName << "!" << endl;
        throw ModelException(ModelException::UnknownXSDType, QString("Unknown attribute type %1 specified for Node %2 !").arg(simpleTypeName).arg(getXMLPath()));
    }
    
    if ( XSD::dynamicTypeRefs.contains(type->name) )
        type->is_enum = true;

	if (xsdDescrNode.attributes().contains("default")) {
		default_value = xsdDescrNode.attributes().namedItem("default").nodeValue();
		if ( ! isValid(default_value) ) {
			qDebug() << "Invalid default value "<< default_value<<" in attribute " << getXMLPath() << " of type "<< xsdDescrNode.attributes().namedItem("type").nodeValue();
			default_value = type->default_val;
		}
	}
	else {
		default_value = type->default_val;
	}
	value = default_value;

    if (!xsdDescrNode.firstChildElement("xs:annotation").firstChildElement("xs:documentation").isNull())
    {
        QStringList docu_lines = xsdDescrNode.firstChildElement("xs:annotation").firstChildElement("xs:documentation").text().split("\n");
        for (int i=0; i<docu_lines.size(); i++) { docu_lines[i] = docu_lines[i].trimmed(); }
        while (docu_lines.size()>0 && docu_lines.first() == "" ) docu_lines.removeFirst();
        while (docu_lines.size()>0 && docu_lines.last() == "" ) docu_lines.removeLast();
        docu = docu_lines.join("\n");
    }
    else
    {
        docu = type->docu;
    }

    if ( ! is_text ) {
        if (xmlParentNode.attributes().contains(this->name) ) {
            value = xmlParentNode.attributes().namedItem(this->name).nodeValue();
            if ( XSD::dynamicTypeDefs.contains(type->name) ) {
                int type_index = XSD::dynamicTypeDefs.indexOf(type->name);
                model_descr->xsd.registerEnumValue(XSD::dynamicTypeRefs[type_index], this->value);
            }
            is_active = true;
        }
        else {
            is_active = false;
        }
    }
    else {
        is_active = true;
        if(parentNode.firstChild().nodeType() == QDomNode::TextNode) {
            textNode = parentNode.firstChild().toText();
            value = parentNode.firstChild().nodeValue();
        }
        else {
            textNode = parentNode.ownerDocument().createTextNode(value);
            parentNode.insertBefore(textNode, parentNode.firstChild());
        }
    }
    orig_active = is_active;
    orig_value = value;
    is_changed = false;

    if ( ! isValid(value) &&  ! XSD::dynamicTypeRefs.contains(type->name) ) {
        MorphModelEdit ed;
        ed.info = QString("Invalid value '") + value + "' of attribute '" + name + "' was changed into '" + default_value + "'";
        ed.edit_type = AttribChange;
        ed.value = name + "=\"" + value + "\"";
        ed.xml_parent = parentNode;
        ed.name = name;

        set(default_value);
        model_descr->auto_fixes.append(ed);
    }
    
    if ( type->name == "cpmDoubleSymbolRef" && (value=="CELLID" || value=="CELLTYPE" ) ) {
        MorphModelEdit ed;
        QString new_value = value=="CELLID" ? "cell.id" : "cell.type";
        ed.info = QString("Deprecated value '") + value + "' of attribute '" + name + "' was changed into '" + new_value + "'";
        ed.edit_type = AttribChange;
        ed.value = name + "=\"" + value + "\"";
        ed.xml_parent = parentNode;
        ed.name = name;

        set(new_value);
        model_descr->auto_fixes.append(ed);
    }
}

//------------------------------------------------------------------------------

QString AbstractAttribute::get()
{return this->value;}

//------------------------------------------------------------------------------

bool AbstractAttribute::set(QString att)
{
    if(this->isValid(att))
    {
        QString old_value = this->value;
        this->value = att;

        if (is_active) {

            if ( XSD::dynamicTypeDefs.contains(type->name) ) {
                int type_index = XSD::dynamicTypeDefs.indexOf(type->name);
                model_descr->xsd.removeEnumValue(XSD::dynamicTypeRefs[type_index], old_value);
                model_descr->xsd.registerEnumValue(XSD::dynamicTypeRefs[type_index], this->value);
            }

            if (is_text) {
                textNode.setNodeValue(value);
				if (model_descr->track_next_change) {
					MorphModelEdit e;
					e.edit_type = TextChange;
					e.info = QString("Text of node %1 was set to %2").arg(this->parentNode.nodeName(),this->value);
					e.xml_parent = this->parentNode;
					e.name = this->name;
					e.value = this->value;
					model_descr->auto_fixes.append(e);
					model_descr->track_next_change = false;
				}
			}
            else {
                parentNode.toElement().setAttribute(this->name, value);
				if (model_descr->track_next_change) {

					MorphModelEdit e;
					e.edit_type = AttribChange;
					e.info = QString("Attribute \"%1\" of node %2 was set to %3").arg(this->name,this->parentNode.nodeName(),this->value);
					e.xml_parent = this->parentNode;
					e.name = "";
					e.value = this->value;
					model_descr->auto_fixes.append(e);
					model_descr->track_next_change = false;
				}
			}
			model_descr->edits++;
			
			emit changed(this);
        }

        return true;
    }
    else
    {
        cout << "Value: '" << value.toStdString() << "' for attribute: '" << this->name.toStdString() << "' from Node: '" << parentNode.nodeName().toStdString() << "', isn't valid!" << endl;
        return false;
    }
}


void AbstractAttribute::inheritDisabled(bool disable) {
	if (is_disabled == disable) return;
	if (disable) {
		if ( XSD::dynamicTypeDefs.contains(type->name) ) {
			int type_index = XSD::dynamicTypeDefs.indexOf(type->name);
			model_descr->xsd.removeEnumValue(XSD::dynamicTypeRefs[type_index], this->value);
		}
		is_disabled = true;
	}
	else {
		if ( XSD::dynamicTypeDefs.contains(type->name) ) {
			int type_index = XSD::dynamicTypeDefs.indexOf(type->name);
			model_descr->xsd.registerEnumValue(XSD::dynamicTypeRefs[type_index], this->value);
		}
		is_disabled = false;
	}
}

//------------------------------------------------------------------------------

void AbstractAttribute::setActive(bool a)
{
    if (a==is_active) return; // no noop
    if ( is_text ) return;  // text cannot be deactivated

    if ( ! is_active) {
        parentNode.toElement().setAttribute(this->name, value);

        if ( XSD::dynamicTypeDefs.contains(type->name) ) {
            int type_index = XSD::dynamicTypeDefs.indexOf(type->name);
            model_descr->xsd.registerEnumValue(XSD::dynamicTypeRefs[type_index], this->value);
        }
    }
    else {
        parentNode.attributes().removeNamedItem(name);

        if ( XSD::dynamicTypeDefs.contains(type->name) ) {
            int type_index = XSD::dynamicTypeDefs.indexOf(type->name);
            model_descr->xsd.removeEnumValue(XSD::dynamicTypeRefs[type_index], this->value);
        }
    }
    is_active = a;
    if (is_changed) {
        if ((is_active == orig_active) && ( ! is_active || (orig_value == value))) {
            is_changed = false;
            model_descr->change_count--;
        }
    }
    else {
        if ((is_active != orig_active) || ( is_active && (orig_value != value))) {
            is_changed = true;
            model_descr->change_count++;
        }
    }
    model_descr->edits++;
    emit changed(this);
}

//------------------------------------------------------------------------------

bool AbstractAttribute::isValid( QString att )
{
    int pos = 0;
    QValidator::State ret = type->validator->validate(att, pos);

    if( ret == 1 || ret == 2)
    {
        return true;
    }
    else
    {
        return false;
    }
}

//------------------------------------------------------------------------------

QStringList AbstractAttribute::getEnumValues()
{
    return this->type->value_set;
}

//------------------------------------------------------------------------------

QSharedPointer<XSD::SimpleTypeInfo> AbstractAttribute::getType() { return type; }

//------------------------------------------------------

QString AbstractAttribute::getXMLPath()
{
	QStringList path(QString("@") + name);

	QDomNode tmp = parentNode;
	while(tmp.nodeName() != "#document" && ! tmp.isNull() )
	{
// 		qDebug() << tmp.nodeName();
		QString name;
		
		if(tmp.nodeName() == "Contact")
		{
			QString ct1, ct2;
			ct1 = tmp.attributes().namedItem("type1").nodeValue();
			ct2 = tmp.attributes().namedItem("type2").nodeValue();
			name = ct1 + "," + ct2;
		}
		else if(!tmp.attributes().namedItem("symbol").isNull())
		{
			name = QString("symbol=")+ tmp.attributes().namedItem("symbol").nodeValue();
		}
		else if(!tmp.attributes().namedItem("name").isNull())
		{
			name = QString("name=") + tmp.attributes().namedItem("name").nodeValue();
		}
		else {
			bool has_multiple = false; int child_id; 
			QDomNode next = tmp.parentNode().firstChildElement(tmp.nodeName()); int i=0;
			if (next == tmp) 
				child_id = i;
			next = tmp.parentNode().nextSiblingElement(tmp.nodeName()); i++;
			while ( ! next.isNull()) {
				has_multiple = true;
				if (next == tmp) {
					child_id = i;
					break;
				}
				next = tmp.parentNode().nextSiblingElement(tmp.nodeName()); i++;
			}
			if (has_multiple)
				name = QString::number(child_id);
			else 
				name="";
		}
		if (name.isEmpty())
			path.prepend(tmp.nodeName());
		else 
			path.prepend(tmp.nodeName()+"["+name+"]");
		tmp = tmp.parentNode();
	}
	return path.join("/");
}

//------------------------------------------------------

void AbstractAttribute::saved() {
    is_changed=false;
    orig_active = is_active;
    orig_value = value;
}

//------------------------------------------------------

AbstractAttribute::~AbstractAttribute()
{
    emit deleted(this);
    setActive(false);
    if (is_changed)
        model_descr->change_count--;
}


