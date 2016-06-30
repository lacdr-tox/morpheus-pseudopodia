#include "nodecontroller.h"

nodeController::nodeController(nodeController* parent, QDomNode xml_node) :
QObject(parent)
{
    /**
      Input xml_node xsd_node (<element name="??" (type="??") />) of the element definition, parent for edit puroses
      Sketch for parsing xsd info for the element that is associated with this nodeController
      1. is this a complex type (either inline or by reference)
      1a. find out the info for all attributes
      1b. find all element childs and under which conditions they may be there

      2. is this a simple type or a mixed type
      2a. set the text type by retrieving the type info from XSD
      2b. create the validator
    */
    xmlNode = xml_node;
    this->parent = parent;
    text = NULL;

    if (xmlNode.isComment()) {
        disabled = true;
        QDomDocument doc;
        doc.setContent(xmlNode.toComment().data());

        // DocumentElement must be <Disabled>
        // the first child below is the real disabled node ...
        // this is asured by the parent node ...
        Q_ASSERT(doc.documentElement().nodeName() == "Disabled");
		xmlDisabledNode = doc.documentElement();
        xmlDataNode = xmlDisabledNode.firstChild();
    }
    else if(xmlNode.nodeName() == "Disabled" ) {
		disabled = true;
		xmlDisabledNode = xmlNode;
		xmlDataNode =  xmlNode.firstChild();
	}
    else {
        disabled = false;
        xmlDataNode = xmlNode;
    }
    name = xmlDataNode.nodeName();
    orig_disabled = disabled;

    // All child nodeControllers share the same symbolName map
    // Only the root node creates one
    if (parent) {
        model_descr = parent->model_descr;
        inherited_disabled = parent->isDisabled();
        xsdTypeNode = parent->childInformation(name).xsdChildType;
		is_complexType = parent->childInformation(name).is_complexType;
    }
    else {
        // This is an xml root element, and we create a descriptor,
        // and a schema and take it's root element as xsd type ...
		try {
			model_descr = QSharedPointer<ModelDescriptor>( new ModelDescriptor);
			model_descr->change_count =0;
			model_descr->edits =0;
		}
		catch (QString s) {
			qDebug() << "Error creating Root NodeController: " << s;
			throw s;
		}
		model_descr->edits = 0;
		model_descr->track_next_change = false;
		inherited_disabled = false;
		QDomNode xsd_element_node = model_descr->xsd.getXSDElement().firstChildElement("xs:element");
		NodeInfo default_info;
		default_info.maxOccure="1";
		default_info.minOccure="1";
		default_info.is_exchanging = false;
		NodeInfo rootInfo = parseChildInfo(xsd_element_node, default_info);

		xsdTypeNode = rootInfo.xsdChildType;
		is_complexType = rootInfo.is_complexType;
    }

    if (xsdTypeNode.isNull()) {
        throw( QString("Unable to create NodeController for \"") + name + "\". " + "Type information for Node " + name + " is not defined in XSD !!");
    }

    // 1a is a complex type
    if ( is_complexType ) {
        parseAttributes();
        parseChilds();
		auto i = attributes.begin();
		while (i != attributes.end()) {
			// register gnuplot terminal attributes
			if (i.value()->getType()->name == "cpmGnuplotTerminalEnum" || i.value()->getType()->name == "cpmLoggerTerminalEnum") {
				model_descr->terminal_names.append(i.value());
// 				qDebug() << "found terminal " << i.value()->get();
			}
			// register system file references
			if (i.value()->getType()->name == "cpmSystemFile") {
				model_descr->sys_file_refs.append(i.value());
// 				qDebug() << "found System File Reference to " << i.value()->get();
			}
			// register the symbol-name-pair as a lookup table to create a symbol - name descriptions.
		    if (i.key() == "symbol" && attributes.find("name") != attributes.end() ) {
				model_descr->symbolNames.insert(*i, attributes["name"]);
			}
			++i;
		}
	}
	else {
		text = new AbstractAttribute(this, parent->childInformation(name).xsdChildDef , xmlDataNode, model_descr);
	}

    // read xml data
	if( is_complexType && xsdTypeNode.firstChildElement("xs:any").isNull() )
	{
		for(uint i = 0; i < xmlDataNode.childNodes().length(); i++)
		{
			QDomNode child = xmlDataNode.childNodes().at(i);

			if (child.nodeType() == QDomNode::CommentNode || (child.nodeType() == QDomNode::ElementNode && child.nodeName() == "Disabled") ) {
				QDomNode disabled_child;
				if (child.nodeType() == QDomNode::CommentNode && child.toComment().data().contains("<Disabled>")) {
					QDomDocument doc;
					doc.setContent(child.toComment().data());
					if (doc.documentElement().nodeName() == "Disabled")
						disabled_child = doc.documentElement().firstChild();
				}
				else if (child.nodeType() == QDomNode::ElementNode) {
					disabled_child = child.firstChild();
				}
				if ( childInfo.contains(disabled_child.nodeName()) ) {
					childs.push_back(new nodeController(this, child));
				}
				else {
					MorphModelEdit e;
					e.edit_type = NodeRemove;
					e.info = QString("Undefined disabled Node '%1' removed from Node '%2'.").arg(disabled_child.nodeName(),name);
					e.xml_parent = xmlDataNode;
					e.name = disabled_child.nodeName();
					QTextStream s(&e.value);
					disabled_child.save(s,4);
					model_descr->auto_fixes.append(e);
					xmlDataNode.removeChild(child);
					i--;
				}
			}
			else if (child.nodeType() == QDomNode::ElementNode)
			{
				if ( this->getAddableChilds().contains(child.nodeName()) ) {
						childs.push_back(new nodeController(this, child));
				}
				else {
					MorphModelEdit e;
					e.edit_type = NodeRemove;
					e.info = QString("Undefined Node '%1' removed from Node '%2'.").arg(child.nodeName(),name);
					e.xml_parent = xmlDataNode;
					e.name = child.nodeName();
					QTextStream s(&e.value);
					child.save(s,4);
					model_descr->auto_fixes.append(e);
					xmlDataNode.removeChild(child);
					i--;
				}
			}
		}
	}
	setRequiredElements();

	// Add watchdogs and other ugly siffers
	if ( name=="Title"  ) {
		model_descr->title = getText();
	}
	if ( name=="Details" ) {
		model_descr->details = getText() ;
	}
	if ( name=="Lattice" ) {
		if (attributes.contains("class") && firstChild("Size")->attributes.contains("value")) {
			QObject* adapter = new LatticeStructureAdapter(this, attributes["class"], firstChild("Size")->attributes["value"]);
			adapters.append(adapter);
		}
		else
			qDebug() << "Cannot register LatticeStructureAdapter! Attributes not found!";
	}
}

//------------------------------------------------------------------------------

void nodeController::parseChilds()
{
    QDomNode subNode = getSubNode(xsdTypeNode);
    QDomNodeList xsd_childs = subNode.childNodes();
    QList<QDomNode> child_elements;
    // Collapse all childs into the elements list
    for(int i = 0; i < xsd_childs.size(); i++)
    {
        if(xsd_childs.at(i).nodeType() == QDomNode::ElementNode)
        {
            if (xsd_childs.at(i).nodeName()=="xs:group") {
                // parsing elements referenced via a group ref
                QDomNode groupNode = xsd_childs.at(i);
                QString groupName = groupNode.attributes().namedItem("ref").nodeValue();
                QDomNodeList xsd_groups = model_descr->xsd.getXSDElement().elementsByTagName("xs:group");
				QDomNode xsd_group_node;
				
				for(int h = 0; h < xsd_groups.size(); h++)
				{
					if( xsd_groups.at(h).attributes().namedItem("name").nodeValue() == groupName) {
						xsd_group_node = getSubNode(xsd_groups.at(h));
						break;
					}
				}
				
				if (xsd_group_node.isNull()) {
					throw(QString("Unable to find referred XSD group %1.").arg(groupName));
				}
				else {
					for(int m = 0; m < xsd_group_node.childNodes().size(); m++)
					{
						if (xsd_group_node.childNodes().at(m).nodeType() ==  QDomNode::ElementNode &&  xsd_group_node.childNodes().at(m).nodeName() == "xs:element")
							child_elements.push_back(xsd_group_node.childNodes().at(m));
					}
				}
			}
			else if(xsd_childs.at(i).nodeName() == "xs:element")
			{ child_elements.push_back(xsd_childs.at(i)); }
		}
	}

    NodeInfo defaultChildInfo;
    defaultChildInfo.info =  QString("");
    defaultChildInfo.pluginClass = QString("");
	
	if(!subNode.attributes().namedItem("minOccurs").isNull())
        defaultChildInfo.minOccure = subNode.attributes().namedItem("minOccurs").nodeValue();
    else
        defaultChildInfo.minOccure = "1";

    if(!subNode.attributes().namedItem("maxOccurs").isNull())
        defaultChildInfo.maxOccure = subNode.attributes().namedItem("maxOccurs").nodeValue();
    else
        defaultChildInfo.maxOccure = "1";
	
    if(subNode.nodeName() == "xs:choice" && defaultChildInfo.maxOccure == "1")
    {
        defaultChildInfo.group = xsdTypeNode.namedItem("name").nodeValue();
        defaultChildInfo.is_exchanging = true;
        defaultChildInfo.is_default_choice = true;
    }
    else {
        defaultChildInfo.group = "";
        defaultChildInfo.is_exchanging = false;
        defaultChildInfo.is_default_choice = false;
    }

    for (int i=0; i<child_elements.size(); i++) {
        NodeInfo child_info = parseChildInfo(child_elements.at(i), defaultChildInfo);
// 		qDebug() << "Registering Child " << child_info.name;
        childInfo[child_info.name] = child_info;
        defaultChildInfo.is_default_choice = false;
    }
}

//------------------------------------------------------------------------------

nodeController::NodeInfo nodeController::parseChildInfo(QDomNode elemNode, const NodeInfo& default_info)
{
	NodeInfo child_info = default_info;
	child_info.xsdChildDef = elemNode;
    QDomNamedNodeMap xsd_attributes =  elemNode.attributes();
    child_info.name = xsd_attributes.namedItem("name").nodeValue();

    if ( xsd_attributes.contains("minOccurs"))
        {child_info.minOccure = xsd_attributes.namedItem("minOccurs").nodeValue();}

    if ( xsd_attributes.contains("maxOccurs"))
        {child_info.maxOccure = xsd_attributes.namedItem("maxOccurs").nodeValue();}
        
    if (xsd_attributes.contains("type") ) {
		QString type_name = xsd_attributes.namedItem("type").nodeValue();
		if (model_descr->xsd.getComplexTypes().contains( type_name ) ) {
			child_info.is_complexType = true;
			child_info.xsdChildType = model_descr->xsd.getComplexTypes()[type_name];
		}
		else if (model_descr->xsd.getSimpleTypes().contains( type_name ) ){
			child_info.is_complexType = false;
			child_info.simpleChildType = model_descr->xsd.getSimpleTypes()[type_name];
			child_info.xsdChildType = child_info.simpleChildType->xsdNode;
		}
		else {
			qDebug() << "Undefined Node Type " << type_name;
			throw QString("Undefined Node Type ") + type_name;
		}
		
	}
	else {
		child_info.xsdChildType = elemNode.firstChildElement("xs:complexType");
		if ( ! child_info.xsdChildType.isNull()) {
			child_info.is_complexType = true;
		}
		else {
			throw QString("Embedded complex Type expected ...");
		}
	}

    // need to get the referred type in order to read the docu
    QDomNode infoNode;
    if ( ! elemNode.firstChildElement("xs:annotation").firstChildElement("xs:documentation").isNull() ) {
        infoNode = elemNode;
    }
    else {
		if (child_info.is_complexType) {
			infoNode = child_info.xsdChildType;
		}
		else {
			child_info.info = child_info.simpleChildType->docu;
		}
    }

    if (!infoNode.isNull() ) {
        QStringList docu_lines = infoNode.firstChildElement("xs:annotation").firstChildElement("xs:documentation").text().split("\n");
        for (int i=0; i<docu_lines.size(); i++) { docu_lines[i] = docu_lines[i].trimmed(); }
        while (docu_lines.size()>0 && docu_lines.first() == "" ) docu_lines.removeFirst();
        while (docu_lines.size()>0 && docu_lines.last() == "" ) docu_lines.removeLast();
        child_info.info = docu_lines.join("\n");
    }


	QDomNode appInfoNode;
	if(!elemNode.firstChildElement("xs:annotation").firstChildElement("xs:appinfo").isNull()) {
		appInfoNode = elemNode;
	}
	else {
		appInfoNode = child_info.xsdChildType;
	}
    child_info.pluginClass = appInfoNode.firstChildElement("xs:annotation").firstChildElement("xs:appinfo").text();
	return child_info;
}

//------------------------------------------------------------------------------

void nodeController::parseAttributes()
{
    QDomNode attrNode;
    for(int  i = 0;  i < this->xsdTypeNode.childNodes().size(); i++)
    {
        attrNode = this->xsdTypeNode.childNodes().at(i);

        if(attrNode.nodeName() == "xs:attribute")
        {
            AbstractAttribute* xml_attribute = new AbstractAttribute(this, attrNode, xmlDataNode, model_descr);
// 			qDebug() << "Registering Attribute node " << xml_attribute->getName();
            attributes[xml_attribute->getName()] = xml_attribute;
			if (isDisabled()) {
				xml_attribute->inheritDisabled(true);
			}
        }
    }
    // reverse checking, whether there is an undefined attribute in the xml
    QDomNamedNodeMap xml_attributes = xmlDataNode.attributes();
    for (int i=0; i<xml_attributes.count(); i++)
    {
        QString a_name = xml_attributes.item(i).nodeName();
        if ( ! attributes.contains(a_name) ) {

            MorphModelEdit e;
            e.edit_type = AttribRemove;
            e.info = QString("Undefined Attribute '%1' removed from Node '%2'.").arg(a_name,name);
            e.xml_parent = xmlDataNode;
            e.name = a_name;
            e.value = a_name + "=\""  + xml_attributes.namedItem(a_name).nodeValue() + "\"";
            xml_attributes.removeNamedItem( a_name);
            model_descr->auto_fixes.append(e);
            i--;

        }

    }
}

//------------------------------------------------------------------------------

nodeController::~nodeController()
{
	for (int i=0; i< adapters.size(); i++) {
		delete adapters[i];
	}
	adapters.clear();

	auto i = attributes.begin();
	while (i != attributes.end()) {
		if (i.value()->getType()->name == "cpmGnuplotTerminalEnum") {
			model_descr->terminal_names.removeAll(i.value());
// 			qDebug() << "found terminal " << i.value()->get();
		}
		if (i.value()->getType()->name == "cpmSystemFile") {
			model_descr->sys_file_refs.removeAll(i.value());
// 			qDebug() << "found System File Reference to " << i.value()->get();
		}
		if (i.key() == "symbol" && attributes.find("name") != attributes.end()) {
			model_descr->symbolNames.remove( *i );
		}
		++i;
	}
	childs.clear();
	QDomNode tmp = xmlNode.parentNode();
	tmp.removeChild(xmlNode);
}

//------------------------------------------------------------------------------

QDomNode nodeController::getSubNode(QDomNode subNode)
{
    if(subNode.nodeName() != "xs:simpleType")
    {
        for(int i = 0; i < subNode.childNodes().size(); i++)
        {
            if(subNode.childNodes().at(i).nodeName() == "xs:all" || subNode.childNodes().at(i).nodeName() == "xs:choice")
            {
                QDomNode node = subNode.childNodes().at(i);
                return node;
            }
        }
        QDomNode node;
        return node;
    }
    else
    {
        QDomNode node;
        return node;
    }
}

//------------------------------------------------------------------------------

QList<QString> nodeController::getAddableChilds(bool unfiltered)
{
	QList<QString> addableChilds;
	QMap<QString, NodeInfo>::iterator it;
	if (unfiltered) {
		for (it = this->childInfo.begin(); it != this->childInfo.end(); it++)
			addableChilds.push_back(it.value().name);
	}
	else {
		QMap<QString, int> child_count;
		for(int h = 0; h < this->childs.size(); h++)
		{
			if (! childs[h]->isDisabled())
				child_count[childs[h]->name] ++ ;
		}
		for (it = this->childInfo.begin(); it != this->childInfo.end(); it++)
		{
			if (it.value().maxOccure == "unbounded") {
				addableChilds.push_back(it.value().name);
			}
			else {
				int count = 0;
				int max = it.value().maxOccure.toInt();
				QMap<QString, int>::const_iterator r = child_count.find(it.value().name);
				if ( r != child_count.end() ) {
					count = r.value();
				}
				if (count < max) {
					addableChilds.push_back(it.value().name);
				}
			}
		}
	}
	return addableChilds;
}

//------------------------------------------------------------------------------

nodeController* nodeController::find(QDomNode node) {

    if (this->xmlDataNode == node)
        return this;
    nodeController* contr=NULL;
    for (int i=0; i<childs.size();i++)  {
        contr = childs[i]->find(node);
        if (contr) break;
    }
    return contr;
}

//------------------------------------------------------------------------------

int nodeController::childIndex(nodeController *node) const {
    for (int c=0; c<childs.size(); c++) {
        if (childs[c] == node) return c;
    }
    return -1;
}

//------------------------------------------------------------------------------

nodeController::NodeInfo nodeController::childInformation(QString n) {
    return childInfo.value(n);
}


//------------------------------------------------------------------------------

AbstractAttribute* nodeController::attribute(QString name) {
   if ( ! attributes.contains(name) ) {
       return NULL;
   }
   return attributes[name];
}

//------------------------------------------------------------------------------ 

AbstractAttribute* nodeController::getAttributeByPath(QStringList path) {
	QString attrib_name = path.takeLast();
	nodeController* node = find(path);
	if (node) {
		// look for the attribute#
		attrib_name = attrib_name.replace("@","");
		
		if ( (attrib_name == "text" || attrib_name == "#text") && node->hasText()) {
			qDebug() << "getAttributeByPath[" << name << "]:" << "Attribute " << attrib_name << " found!"; 
			return node->textAttribute();
		}
		else {
			AbstractAttribute *a =  node->attribute(attrib_name);
			if (a)
				qDebug() << "getAttributeByPath[" << name << "]:" << "Attribute " << attrib_name << " found!";
			else 
				qDebug() << "getAttributeByPath[" << name << "]:" << "Attribute " << attrib_name << " not found!";
			return a;
		}
	}
	return nullptr;
// 	else {
// 		
// 		QString child_name = path.takeFirst();
// 		if (name=="MorpheusModel" && child_name=="MorpheusModel") {
// 			child_name = path.takeFirst();
// 		}
// 		if (child_name.startsWith("Contact[")) {
// 			QRegExp child_info("Contact\\[(\\w+),(\\w+)\\]");
// 			child_info.exactMatch(child_name);
// 			QString type1 = child_info.cap(1);
// 			QString type2 = child_info.cap(2);
// 			for (int i=0; i<childs.size();i++) {
// 				if (childs[i]->getName()=="Contact"){
// 					if (childs[i]->attribute("type1")->get() == type1 && childs[i]->attribute("type2")->get() == type2)
// 						return childs[i]->getAttributeByPath(path);
// 				}
// 			}
// 			qDebug() << "getAttributeByPath[" << name << "]: Unable to find Contact with type1=" <<type1 << " & type2=" << type2;
// 			return NULL;
// 			
// 		}
// 		else {
// 			QRegExp child_info("(\\w+)(?:\\[(?:(\\d+)|(\\w+)=(\\w+))\\])?");
// 			child_info.exactMatch(child_name);
// // 			qDebug() << "getAttributeByPath[" << name << "]: " << child_info.capturedTexts();
// 			
// 			child_name = child_info.cap(1);
// 			QString child_id_str = child_info.cap(2);
// 			QString attrib_name = child_info.cap(3);
// 			QString attrib_value = child_info.cap(4);
// 			
// 			if (!child_id_str.isEmpty()) {
// 				// have to look for a particular child
// 				int n_matching=0;
// 				int child_id = child_id_str.toInt();
// 				for (int i=0; i<childs.size();i++) {
// 					if (childs[i]->getName()==child_name) {
// 						if (child_id == n_matching)
// 							return childs[i]->getAttributeByPath(path);
// 						else
// 							n_matching++;
// 					}
// 				}
// 				qDebug() << "getAttributeByPath[" << name << "]: Unable to find " << child_id << "th node named " << child_name;
// 				return NULL;
// 			} 
// 			else if ( !attrib_name.isEmpty()) {
// 				for (int i=0; i<childs.size();i++) {
// 					if (childs[i]->getName()==child_name) {
// 						if (childs[i]->attribute(attrib_name) && (childs[i]->attribute(attrib_name)->get() == attrib_value))
// 							return childs[i]->getAttributeByPath(path);
// 					}
// 				}
// 				qDebug() << "getAttributeByPath[" << name << "]: Unable to find child " << child_name << " with Attribute " << attrib_name << "=" << attrib_value;
// 				return NULL;
// 			}
// 			else {
// 				nodeController* child = firstChild(child_name);
// 				if (child) {
// 					return child->getAttributeByPath(path);
// 				}
// 				else {
// 					qDebug() << "getAttributeByPath[" << name << "]: No child " << child_name;
// 					return NULL;
// 				}
// 			}
// 		}
// 	}
}


nodeController* nodeController::find(QStringList path) {
	if (path.empty())
		return this;
	
	QString child_name = path.takeFirst();
	if (name=="MorpheusModel" && child_name=="MorpheusModel") {
		child_name = path.takeFirst();
		if (child_name.isEmpty())
			return this;
	}
	if (child_name.startsWith("Contact[")) {
		QRegExp child_info("Contact\\[(\\w+),(\\w+)\\]");
		child_info.exactMatch(child_name);
		QString type1 = child_info.cap(1);
		QString type2 = child_info.cap(2);
		for (int i=0; i<childs.size();i++) {
			if (childs[i]->getName()=="Contact"){
				if (childs[i]->attribute("type1")->get() == type1 && childs[i]->attribute("type2")->get() == type2) {
					return childs[i]->find(path);
				}
			}
		}
		qDebug() << "find[" << name << "]: Unable to find Contact with type1=" <<type1 << " & type2=" << type2;
		return NULL;
	}
	else {
		QRegExp child_info("(\\w+)(?:\\[(?:(\\d+)|(\\w+)=(\\w+))\\])?");
		child_info.exactMatch(child_name);
// 			qDebug() << "find[" << name << "]: " << child_info.capturedTexts();
		
		child_name = child_info.cap(1);
		QString child_id_str = child_info.cap(2);
		QString attrib_name = child_info.cap(3);
		QString attrib_value = child_info.cap(4);
		
		if (!child_id_str.isEmpty()) {
			// have to look for a particular child
			int n_matching=0;
			int child_id = child_id_str.toInt();
			for (int i=0; i<childs.size();i++) {
				if (childs[i]->getName()==child_name) {
					if (child_id == n_matching)
						return childs[i]->find(path);
					else
						n_matching++;
				}
			}
			qDebug() << "find[" << name << "]: Unable to find " << child_id << "th node named " << child_name;
			return NULL;
		} 
		else if ( !attrib_name.isEmpty()) {
			for (int i=0; i<childs.size();i++) {
				if (childs[i]->getName()==child_name) {
					if (childs[i]->attribute(attrib_name) && (childs[i]->attribute(attrib_name)->get() == attrib_value)) {
						return childs[i]->find(path);
					}
				}
			}
			qDebug() << "find[" << name << "]: Unable to find child " << child_name << " with Attribute " << attrib_name << "=" << attrib_value;
			return NULL;
		}
		else {
			nodeController* child = firstChild(child_name);
			if (child) {
				return child->find(path);
			}
			else {
				qDebug() << "find[" << name << "]: No child " << child_name;
				return NULL;
			}
		}
	}
}

//------------------------------------------------------------------------------

QList<AbstractAttribute*> nodeController::getRequiredAttributes() {
    QList<AbstractAttribute*> l;
    for (auto it = attributes.begin(); it != attributes.end(); it++) {
        AbstractAttribute *attr = it.value();
        if(attr->isRequired()) {
            l.push_back(attr);
        }
    }
    return l;
}

//------------------------------------------------------------------------------

QList<AbstractAttribute*> nodeController::getOptionalAttributes() {
    QList<AbstractAttribute*> l;
    for ( auto it = attributes.begin(); it != attributes.end(); it++) {
        AbstractAttribute *attr = it.value();
        if(!attr->isRequired()) {
            l.push_back(attr);
        }
    }
    return l;
}

//------------------------------------------------------------------------------

QString nodeController::getInfo()
{return this->parent->childInfo[this->name].info;}

//------------------------------------------------------------------------------

QString nodeController::getText()
{
    if (text)
        return text->get();
    else
        return "";
}

//------------------------------------------------------------------------------

QSharedPointer<XSD::SimpleTypeInfo> nodeController::textType()
{
    if (text)
        return text->getType();
    else
        return model_descr->xsd.getSimpleTypes()["cpmText"];
}

//------------------------------------------------------------------------------

AbstractAttribute* nodeController::textAttribute() {
    return text;
}

//------------------------------------------------------------------------------

bool nodeController::isDeletable()
{
    int count = 0;
    if (!parent) return false;
	return ( ! parent->isChildRequired(this));
		return false;
//     for(int i = 0; i < this->parent->childs.size(); i++)
//     {
//         if(this->parent->childs[i]->name == this->name)
//         {count++;}
//     }
// 
//     int min = parent->childInfo[this->name].minOccure.toInt();
//     if((count - 1) < min)
//         {return false;}
//     else
//         {return true;}
}

//------------------------------------------------------------------------------

bool nodeController::hasText()
{
    return text;
}

//------------------------------------------------------------------------------

nodeController* nodeController::firstChild(QString childName) {
    for(int i = 0; i < this->childs.size(); i++)
    {
        if(this->childs[i]->name == childName && ! this->childs[i]->isDisabled())
        {
            return this->childs[i];
        }
    }
    return NULL;
}

//------------------------------------------------------------------------------

nodeController* nodeController::findGroupChild(QString group) {
	for (int i=0; i<childs.size(); i++)
	{
		QString childName = childs[i]->name;
		if (this->childInfo[childName].group == group &&  ! childs[i]->isDisabled() )
		{
			return childs[i];
		}
	}
	return NULL;
}
//------------------------------------------------------------------------------

bool nodeController::setText(QString txt)
{
    if(text) {
		bool b = text->set(txt);
		if (b && this->name == "Title")
			model_descr->title = txt; 
		if (b && this->name == "Details")
			model_descr->details = txt; 
		return true;
	}
    else
        return false;
}


//------------------------------------------------------------------------------

nodeController* nodeController::insertChild(QString childName, int pos)
{
    if ( ! getAddableChilds(true).contains(childName) )
        throw(QString("Unable to create NodeController for \"%1\". Child is undefined.").arg(childName));
	QDomElement elem = this->xmlNode.ownerDocument().createElement(childName);
	return insertChild(elem,pos);
}

//------------------------------------------------------------------------------

nodeController* nodeController::insertChild(QDomNode xml_node, int pos)
{
	QString childName = xml_node.nodeName();

	if (xml_node.nodeType() == QDomNode::CommentNode) {
		if (xml_node.toComment().data().contains("<Disabled>")) {
			QDomDocument doc;
			doc.setContent(xml_node.toComment().data());
			if (doc.documentElement().nodeName() == "Disabled")
				childName = doc.documentElement().firstChild().nodeName();
		}
	}
	
	if ( ! getAddableChilds(true).contains(childName) )
		throw(QString("Unable to create NodeController for \"%1\". Child is undefined.").arg(childName) );

	if (pos < 0 || pos >= childs.size()) {
		pos = childs.size();
		xmlDataNode.appendChild(xml_node);
	}
	else {
		xmlDataNode.insertBefore(xml_node,childs[pos]->xmlNode);
	}
	
	nodeController* contr(new nodeController(this, xml_node));
	
	if (pos == childs.size()) {
		childs.append(contr);
	}
	else {
		childs.insert(pos,contr);
	}
	
	model_descr->edits++;
	model_descr->change_count++;
	return contr;
}

//------------------------------------------------------------------------------

void nodeController::setRequiredElements()
{
    QMap<QString, NodeInfo>::iterator it;

    for (auto it_attr = this->attributes.begin(); it_attr != this->attributes.end(); it_attr++)
    {
        if ( it_attr.value()->isRequired() && ! it_attr.value()->isActive())
        {
            it_attr.value()->setActive(true);
            MorphModelEdit e;
            e.edit_type = AttribAdd;
            e.info = QString("Required Attribute '%1' added to Node '%2'.").arg(it_attr.value()->getName(), name);
            e.xml_parent = xmlDataNode;
            e.name = it_attr.value()->getName();
            model_descr->auto_fixes.append(e);
        }
    }

    for (it = this->childInfo.begin(); it != this->childInfo.end(); it++)
    {
        int min = it.value().minOccure.toInt();

        if (min == 1)
        {
            bool isSet = false;
            for(int i = 0; i < childs.size(); i++)
            {
                if(childs[i]->name == it.value().name)
                {
                    isSet = true;
                    break;
                }
            }

            if (!isSet)
            {
                if (it.value().is_exchanging)
                {
                    if (it.value().is_default_choice)
                    {
                        bool hasGroupMember = false;
                        for(int i = 0; i < childs.size(); i++)
                        {
                            QString tmp = childInfo[childs[i]->name].group;
                            if(tmp == it.value().group)
                            {
                                hasGroupMember = true;
                                break;
                            }
                        }

                        if (!hasGroupMember)
                        {
                            MorphModelEdit e;
                            e.edit_type = NodeAdd;
                            e.info = QString("Required Node '%1' added to Node '%2'.").arg(it.value().name, name);
                            e.xml_parent = xmlDataNode;
                            e.name = it.value().name;
                            model_descr->auto_fixes.append(e);

                            int message_pos = this->model_descr->auto_fixes.size();

                            this->insertChild(it.value().name,childs.size());

                            while (this->model_descr->auto_fixes.size() > message_pos)
                                this->model_descr->auto_fixes.pop_back();
                        }
                    }
                }
                else
                {
                    MorphModelEdit e;
                    e.edit_type = NodeAdd;
                    e.info = QString("Required Node '%1' added to Node '%2'.").arg(it.value().name, name);
                    e.xml_parent = xmlDataNode;
                    e.name = it.value().name;
                    model_descr->auto_fixes.append(e);

                    int message_pos = this->model_descr->auto_fixes.size();

                    this->insertChild(it.value().name,childs.size());

                    while (this->model_descr->auto_fixes.size() > message_pos)
                        this->model_descr->auto_fixes.pop_back();
                }
            }
        }
    }
}

//------------------------------------------------------------------------------

void nodeController::moveChild(int from, int to) {
    // Here we need the index of the neighboring child before the move ...
    if (to==0) {
        xmlDataNode.insertBefore(childs[from]->xmlNode,QDomNode());
    } else {
        xmlDataNode.insertAfter(childs[from]->xmlNode, childs[(from<to) ? to : to-1]->xmlNode);
    }

    // Here we use the index the child will have after move ...
    childs.move(from,to);

    model_descr->edits++;
	model_descr->change_count++;
}

//------------------------------------------------------------------------------

bool nodeController::canInsertChild(QString child, int dest_pos) {
	if (dest_pos > childs.size())
		return false;
	if ( ! childInfo.contains(child) )
		return false;
	
	return true;
}

//------------------------------------------------------------------------------

bool nodeController::canInsertChild(nodeController* source, int dest_pos) {
	if (dest_pos > childs.size())
		return false;
	if ( ! childInfo.contains(source->name))
		return false;
	if (childInfo[source->name].xsdChildType != source->xsdTypeNode) 
		return false;
	
	return true;
}

//------------------------------------------------------------------------------

bool nodeController::insertChild(nodeController* source, int dest_pos)
{
	// At first make sure that we can take the child
	if ( ! canInsertChild(source, dest_pos))
		return false;
	nodeController* child = source ->parent->removeChild(source -> parent -> childIndex(source));
	child->parent = this;
	child->setParent(this);
	
	if ( dest_pos==0) {
		xmlDataNode.insertBefore(child->xmlNode, QDomNode());
	}
	else {
		xmlDataNode.insertAfter(child->xmlNode, childs[dest_pos-1]->xmlNode);
	}
	childs.insert(dest_pos, child);
	return true;
}


//------------------------------------------------------------------------------

nodeController* nodeController::removeChild(int pos)
{
    Q_ASSERT(pos>=0 && pos < childs.size());
	
	nodeController* child  = childs[pos];
	xmlDataNode.removeChild(child->xmlNode);
	childs.removeAt(pos);
	child->parent = NULL;
	child->setParent(NULL);
	return child;
}

//------------------------------------------------------------------------------

bool nodeController::isChildRequired(nodeController* node)
{
	if (node->isDisabled())
		return false;
	
	QStringList search_mask;
	if (childInfo[node->getName()].is_exchanging) {
		QString group_name = childInfo[node->getName()].group;
		QMapIterator<QString, NodeInfo> node_info(childInfo);
		while (node_info.hasNext()) {
			node_info.next();
			if (node_info.value().group == group_name) {
				search_mask.append(node_info.value().name);
			}
		}
	}
	else {
		search_mask.append(node->getName());
	}
	
	int count=0;
	for (int i=0; i<childs.size(); i++) {
		if ( ! childs[i]->isDisabled() and search_mask.contains(childs[i]->getName()) ) {
			count++;
		}
	}
	return count <= childInfo[node->getName()].minOccure.toInt();
}

//------------------------------------------------------------------------------

bool nodeController::isDisabled() {
    return disabled /*|| inherited_disabled*/;
}

//------------------------------------------------------------------------------

bool nodeController::isInheritedDisabled() {
	return inherited_disabled;
}

//------------------------------------------------------------------------------

bool nodeController::setDisabled(bool b) {
    if ( b == disabled ) return true;
    if ( b ) {
//         if ( ! isDeletable() ) return false;
        // remove the node from the xml
        disabled = true;

        for (int c=0; c<childs.size(); c++) {
            childs[c]->inheritDisabled(true);
        }
        for ( auto a=attributes.begin(); a!=attributes.end(); a++) {
			a.value()->inheritDisabled(true);
		}
		
		// create a brand new disabled node
		QDomDocument doc;
		xmlDisabledNode = xmlNode.ownerDocument().createElement("Disabled");
		// create the comment to replace the data node in the xml
		xmlNode = xmlDataNode.ownerDocument().createComment("");
		parent->xmlDataNode.replaceChild(xmlNode, xmlDataNode);
		// reparent the data node and store it into the comment
		xmlDisabledNode.appendChild(xmlDataNode);
		QString dis_node_text;
		QTextStream s(&dis_node_text);
        xmlDisabledNode.save(s,4);
		xmlNode.setNodeValue(dis_node_text);

        if (orig_disabled)
			model_descr->change_count--;
		else
			model_descr->change_count++;

		model_descr->edits++;
    }
    else {
        disabled = false;
        parent->xmlDataNode.replaceChild(xmlDataNode, xmlNode);
		xmlNode = xmlDataNode;
        for (int c=0; c<childs.size(); c++) {
            childs[c]->inheritDisabled(false);
        }
		for (auto a=attributes.begin(); a!=attributes.end(); a++) {
			a.value()->inheritDisabled(false);
		}
		if (orig_disabled)
			model_descr->change_count++;
		else
			model_descr->change_count--;

		model_descr->edits++;
    }
    return true;
}

//------------------------------------------------------------------------------

void nodeController::inheritDisabled(bool inherit) {
	if (inherited_disabled == inherit)
		return;
    if (disabled) {
		if (inherit) {
			// replace the comment by the plain <Disabled> node
			parent->xmlNode.replaceChild( xmlDisabledNode, xmlNode);
			xmlNode = xmlDisabledNode;
		}
		else {
			// replace the disabled node by a comment node
			QString dis_node_text;
			QTextStream s(&dis_node_text);
			xmlDisabledNode.save(s,4);
			xmlNode.setNodeValue(dis_node_text);
			xmlNode = xmlNode.ownerDocument().createComment(dis_node_text);
			parent->xmlNode.replaceChild(xmlNode, xmlDisabledNode);
		}
	}
	else {
		for (int c=0; c<childs.size(); c++) {
			childs[c]->inheritDisabled(inherit);
		}
	}
	inherited_disabled = inherit;
}

//------------------------------------------------------------------------------

void nodeController::clearTrackedChanges()
{
	model_descr->auto_fixes.clear();
	model_descr->track_next_change = false;
}


void nodeController::trackNextChange()
{
	model_descr->track_next_change = true;
}

void nodeController::trackInformation(QString info)
{
	MorphModelEdit e;
	e.info = info;
	model_descr->auto_fixes.append(e);
}

void nodeController::saved() {
    orig_disabled = disabled;

    for ( auto it = attributes.begin(); it != attributes.end(); it++) {
        it.value()->saved();
    }
    for (int c=0; c<childs.size(); c++) {
        childs[c]->saved();
    }
	model_descr->change_count = 0;
}
