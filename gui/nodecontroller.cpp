#include "nodecontroller.h"

nodeController::nodeController(nodeController* parent,  XSD::ChildInfo info, QDomNode xml_node) :
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
	this->node_type = info.type;
	text = nullptr;

	// All child nodeControllers share the same symbolName map
	// Only the root node creates one
	if (!parent) {
				// This is an xml root element, and we create a descriptor,
		// and an XSD, and derive the node type from the XSD's root element ...
		try {
			model_descr = QSharedPointer<ModelDescriptor>::create();
			model_descr->change_count =0;
			model_descr->edits =0;
			model_descr->track_next_change = false;
			
			inherited_disabled = false;
			disabled = false;
			QDomNode xsd_element_node = model_descr->xsd.getXSDElement().firstChildElement("xs:element");
			if (xsd_element_node.attributes().namedItem("name").nodeValue() != xmlNode.nodeName())
				throw QString("XSD root element (%1) does not fit to the XML root element (%2)").arg(xsd_element_node.attributes().namedItem("name").nodeValue()).arg(xmlNode.nodeName());
			
			xmlDataNode = xmlNode;
			node_type = model_descr->xsd.getComplexType(xsd_element_node.attributes().namedItem("type").nodeValue());
		}
		catch (QString s) {
			qDebug() << "Error creating Root NodeController: " << s;
			throw s;
		}
	}
	else { // there is a parent node
		model_descr = parent->model_descr;
		inherited_disabled = parent->isDisabled();
		if (xmlNode.isComment()) {
			QDomDocument doc;
			doc.setContent(xmlNode.toComment().data());

			// DocumentElement must be <Disabled>
			// the first child below is the real disabled node ...
			// this is asured by the parent node ...
			xmlDisabledNode = doc.documentElement();
			if (doc.documentElement().nodeName() == "Disabled") {
				disabled = true;
				xmlDataNode = xmlDisabledNode.firstChild();
				if (inherited_disabled) {
					// throw away the comment and instead put the real data
					xmlNode.parentNode().replaceChild(xmlDataNode,xmlNode);
				}
			}
			else {
				throw QString("Comment does not contain disabled model fragments");
			}
				
		}
		else if(xmlNode.nodeName() == "Disabled" ) {
			disabled = true;
			xmlDisabledNode = xmlNode;
			xmlDataNode =  xmlNode.firstChild();
			
			if (! inherited_disabled) {
				// move the disabled model fragment into a comment
				QString dis_node_text;
				QTextStream s(&dis_node_text);
				xmlDisabledNode.save(s,4);
				xmlNode = xmlNode.ownerDocument().createComment(dis_node_text);
			}
		}
		else {
			disabled = false;
			xmlDataNode = xmlNode;
		}
	}
	
	name = xmlDataNode.nodeName();
	orig_disabled = disabled;
    

	// 1a is a complex type
	parseAttributes();
	for ( auto attr : attributes) {
		// register gnuplot terminal attributes
		if (attr->getType()->name == "cpmGnuplotTerminalEnum" || attr->getType()->name == "cpmLoggerTerminalEnum") {
			model_descr->terminal_names.append(attr);
// 				qDebug() << "found terminal " << attr->get();
		}
		// register system file references
		if (attr->getType()->name == "cpmSystemFile") {
			model_descr->sys_file_refs.append(attr);
// 				qDebug() << "found System File Reference to " << attr->get();
		}
		// register the symbol-name-pair as a lookup table to create a symbol - name descriptions.
			if (attr->getName() == "symbol" && attributes.find("name") != attributes.end() ) {
			model_descr->symbolNames.insert(attr, attributes["name"]);
		}
	}

	if ( node_type->content_type == XSD::ComplexTypeInfo::SimpleContent ) {
		text = new AbstractAttribute(this, node_type->text_type, info.default_val, xmlDataNode, model_descr);
	}

    // read xml data
	if( node_type->content_type == XSD::ComplexTypeInfo::ComplexContent )
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
				if ( node_type->child_info.children.contains(disabled_child.nodeName()) ) {
					childs.push_back(new nodeController(this,node_type->child_info.children[disabled_child.nodeName()], child));
				}
				else {
					MorphModelEdit e;
					e.edit_type = MorphModelEdit::NodeRemove;
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
						childs.push_back(new nodeController(this, node_type->child_info.children[child.nodeName()], child));
				}
				else {
					MorphModelEdit e;
					e.edit_type = MorphModelEdit::NodeRemove;
					e.info = QString("Undefined Node '%1' removed from Node '%2'.").arg(child.nodeName(),name);
					e.xml_parent = xmlDataNode;
					e.name = child.nodeName();
					QTextStream s(&e.value);
					child.save(s,4);
					if (!model_descr->stealth) model_descr->auto_fixes.append(e);
					xmlDataNode.removeChild(child);
					i--;
				}
			}
		}
	}
	setRequiredElements();

	// Add watchdogs and other ugly sniffers
	if ( name=="Title"  ) {
		model_descr->title = getText();
	}
	if ( name=="Details" ) {
		model_descr->details = getText() ;
	}
	if ( name=="TimeSymbol") {
		model_descr->time_symbol = attribute("symbol");
	}
	if ( name=="Lattice" ) {
		if (attributes.contains("class") && firstActiveChild("Size") && firstActiveChild("Size")->attributes.contains("value")) {
			QObject* adapter = new LatticeStructureAdapter(this, attributes["class"], firstActiveChild("Size")->attributes["value"]);
			adapters.append(adapter);
		}
		else
			qDebug() << "Cannot register LatticeStructureAdapter! Attributes not found!";
	}
	if ( name=="TimeSymbol" ) {
// 		if (attribute("symbol"))
			model_descr->time_symbol = attribute("symbol");
	}
	if (attributes.contains("tags")) {
		auto root = this;
		while (root->parent) root = root->parent;
		
		auto updater = 
			[this, root](AbstractAttribute* a) {
				if (a->isActive()) {
					this->tags = a->get().remove(QRegExp("\\s")).split(",", QString::SkipEmptyParts);
					emit root->dataChanged(this);
				}
			};
		connect(attributes["tags"], &AbstractAttribute::changed, updater);
		updater(attributes["tags"]);
	}
	
	if ( node_type->is_scheduled )
		model_descr->pluginNames[name]+=1;
}

//------------------------------------------------------------------------------

void nodeController::parseAttributes()
{
	QList<QDomNode> attr_xsd_nodes = node_type->attributes;
	
	for(int  i = 0;  i < attr_xsd_nodes.size(); i++) {
		AbstractAttribute* xml_attribute = new AbstractAttribute(this, attr_xsd_nodes[i], xmlDataNode, model_descr);
		attributes[xml_attribute->getName()] = xml_attribute;
		if (isDisabled()) { xml_attribute->inheritDisabled(true); }
	}

	// reverse checking, whether there is an undefined attribute in the xml
	QDomNamedNodeMap xml_attributes = xmlDataNode.attributes();
	for (int i=0; i<xml_attributes.count(); i++)
	{
		QString a_name = xml_attributes.item(i).nodeName();
		if ( ! attributes.contains(a_name) ) {
			MorphModelEdit e;
			e.edit_type = MorphModelEdit::AttribRemove;
			e.info = QString("Undefined Attribute '%1' removed from Node '%2'.").arg(a_name,name);
			e.xml_parent = xmlDataNode;
			e.name = a_name;
			e.value = a_name + "=\""  + xml_attributes.namedItem(a_name).nodeValue() + "\"";
			xml_attributes.removeNamedItem( a_name);
			if (!model_descr->stealth) model_descr->auto_fixes.append(e);
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
	
	if ( node_type->is_scheduled ) {
		model_descr->pluginNames[name]-=1;
		if (model_descr->pluginNames[name]<=0) {
			model_descr->pluginNames.remove(name);
		}
	}

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

QDomNode nodeController::cloneXML() const {
	QDomNode tmp = xmlDataNode.cloneNode(true);
	return tmp;
}

//------------------------------------------------------------------------------

QList<QString> nodeController::getAddableChilds(bool unfiltered)
{
	QList<QString> addableChilds;
	if (unfiltered) {
		for (const auto& child : node_type->child_info.children )
			addableChilds.push_back(child.name);
	}
	else {
		QMap<QString, int> child_count;
		for ( auto child : this->childs) {
			if (! child->isDisabled()) child_count[child->name] ++ ;
		}
		if (node_type->child_info.is_choice) { // xs::choice
			if (node_type->child_info.max_occurs == "unbounded" || activeChilds().size() < node_type->child_info.max_occurs.toInt() ) {
				for (const auto& child : node_type->child_info.children )
					addableChilds.push_back(child.name);
			}
		}
		else { // xs:all
			for (const auto& child : node_type->child_info.children )
			{
				if (child.max_occurs == "unbounded" || node_type->child_info.max_occurs == "unbounded") {
					addableChilds.push_back(child.name);
				}
				else {
					int count;
					int max = child.max_occurs.toInt() * node_type->child_info.max_occurs.toInt();
					QMap<QString, int>::const_iterator r = child_count.find(child.name);
					count = ( r != child_count.end() ) ? r.value() : 0;
					
					if (count < max) {
						addableChilds.push_back(child.name);
					}
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

XSD::ChildInfo nodeController::childInformation(QString n) {
	if (node_type->child_info.children.contains(n))
		return node_type->child_info.children[n];
	else
		return XSD::ChildInfo();
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


nodeController* nodeController::find(QStringList path, bool create) {
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
					return childs[i]->find(path, create);
				}
			}
		}
		if (create && getAddableChilds(true).contains("Contact") ) {
			auto contact = insertChild("Contact");
			contact->attribute("type1")->set(type1);
			contact->attribute("type2")->set(type2);
			return contact->find(path, create);
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
						return childs[i]->find(path, create);
					else
						n_matching++;
				}
			}
			
			if (create && getAddableChilds(true).contains(child_name)) {
				nodeController* child;
				for (int i=n_matching; i<=child_id; i++) {
					child = insertChild(child_name);
				}
				if (child) return child->find(path, create);
			}
			qDebug() << "find[" << name << "]: Unable to find " << child_id << "th node named " << child_name;
			return NULL;
		} 
		else if ( !attrib_name.isEmpty()) {
			for (int i=0; i<childs.size();i++) {
				if (childs[i]->getName()==child_name) {
					if (childs[i]->attribute(attrib_name) && (childs[i]->attribute(attrib_name)->get() == attrib_value)) {
						return childs[i]->find(path, create);
					}
				}
			}
			
			if (create && getAddableChilds(true).contains(child_name)) {
				nodeController* child = insertChild(child_name);
				if (child->attribute(attrib_name)) {
					child->attribute(attrib_name)->set(attrib_value);
					child->attribute(attrib_name)->setActive(true);
					return child->find(path, create);
				}
			}
			qDebug() << "find[" << name << "]: Unable to find child " << child_name << " with Attribute " << attrib_name << "=" << attrib_value;
			return NULL;
		}
		else {
			nodeController* child = firstActiveChild(child_name);
			if (child) {
				return child->find(path, create);
			}
			if (create && getAddableChilds(true).contains(child_name)) {
				nodeController* child = insertChild(child_name);
				return child->find(path, create);
			}
			qDebug() << "find[" << name << "]: No child " << child_name;
			return NULL;
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

QString nodeController::getNodeInfo()
{ return node_type->name; }

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
        return model_descr->xsd.getSimpleType("cpmText");
}

//------------------------------------------------------------------------------

AbstractAttribute* nodeController::textAttribute() {
    return text;
}

//------------------------------------------------------------------------------

bool nodeController::isDeletable()
{
	if (!parent) return false;
	return ( ! parent->isChildRequired(this));
}

//------------------------------------------------------------------------------

bool nodeController::hasText()
{
    return text;
}

//------------------------------------------------------------------------------
QStringList nodeController::inheritedTags() const {
	QStringList inh_tags;
	if (parent) {
		inh_tags = parent->inheritedTags();
		if (parent->allowsTags()) {
			inh_tags += parent->getTags();
		}
	}
	return inh_tags;
}

//------------------------------------------------------------------------------

QStringList nodeController::subtreeTags() const {
	
	if (childs.isEmpty())
		return tags;
	
	QStringList subtree_tags = tags;
	for (const auto& child : childs) {
		if ( ! child->isDisabled() )  {
			subtree_tags += child->subtreeTags();
		}
	}
	return subtree_tags;
}

//------------------------------------------------------------------------------

QStringList nodeController::getEffectiveTags() const
{
	QStringList effective_tags =  inheritedTags() + tags;
	effective_tags.removeDuplicates();
	return effective_tags;
}

//------------------------------------------------------------------------------

QList<nodeController*> nodeController::activeChilds(QString childName) {
	QList<nodeController*> active_childs;
	for (const auto& child : childs) {
		if ( (! child->isDisabled()) && (childName.isEmpty() || childName == child->getName()) )  {
			active_childs.append(child);
		}
	}
	return active_childs;
}

//------------------------------------------------------------------------------

nodeController* nodeController::firstActiveChild(QString childName) {
	for(int i = 0; i < this->childs.size(); i++)
	{
		if ( ! childs[i]->isDisabled() && (childName.isEmpty() || this->childs[i]->name == childName )) {
			return this->childs[i];
		}
	}
	return nullptr;
}

//------------------------------------------------------------------------------

// nodeController* nodeController::findGroupChild(QString group) {
// 	for (int i=0; i<childs.size(); i++)
// 	{
// 		QString childName = childs[i]->name;
// 		if (this->childInfo[childName].group == group &&  ! childs[i]->isDisabled() )
// 		{
// 			return childs[i];
// 		}
// 	}
// 	return NULL;
// }

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
// 	qDebug() << "Inserting Node " << childName;
	QDomNode disabled_content;

	if (xml_node.nodeType() == QDomNode::CommentNode) {
		qDebug() << "Comment: " << xml_node.toComment().data();
		if (xml_node.toComment().data().contains("<Disabled>")) {
			QDomDocument doc;
			doc.setContent(xml_node.toComment().data());
			if (doc.documentElement().nodeName() == "Disabled") {
				childName = doc.documentElement().firstChild().nodeName();
			}
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
	
	nodeController* contr = new nodeController(this, node_type->child_info.children[childName], xml_node);
	
	if (pos == childs.size()) {
		childs.append(contr);
	}
	else {
		childs.insert(pos,contr);
	}
	
	if (!model_descr->stealth) model_descr->edits++;
	if (!model_descr->stealth) model_descr->change_count++;
	return contr;
}

//------------------------------------------------------------------------------

void nodeController::setRequiredElements()
{
	for ( auto& attr : attributes)
	{
		if ( attr->isRequired() && ! attr->isActive())
		{
			attr->setActive(true);
			MorphModelEdit e;
			e.edit_type = MorphModelEdit::AttribAdd;
			e.info = QString("Required Attribute '%1' added to Node '%2'.").arg(attr->getName(), name);
			e.xml_parent = xmlDataNode;
			e.name = attr->getName();
			if (!model_descr->stealth) model_descr->auto_fixes.append(e);
		}
	}

	for (const auto &child_def : node_type->child_info.children)
	{
		int min = 0;
		if (node_type->child_info.is_choice && node_type->child_info.default_choice == child_def.name) 
			min = node_type->child_info.min_occurs.toInt();
		else
			min = child_def.min_occurs.toInt() * node_type->child_info.min_occurs.toInt();

		if (min>0)
		{
			int child_count = 0;
			if ( node_type->child_info.is_choice)
				child_count = childs.size();
			else {
				for(int i = 0; i < childs.size(); i++)
				{
					child_count += (childs[i]->name == child_def.name);
				}
			}

			if (child_count<min) {
				MorphModelEdit e;
				e.edit_type = MorphModelEdit::NodeAdd;
				e.info = QString("Required Node '%1' added to Node '%2'.").arg(child_def.name, name);
				e.xml_parent = xmlDataNode;
				e.name = child_def.name;
				if (!model_descr->stealth) model_descr->auto_fixes.append(e);

				int message_pos = this->model_descr->auto_fixes.size();
				for (int i=child_count; i<min; i++)
					this->insertChild(child_def.name,childs.size());

				while (this->model_descr->auto_fixes.size() > message_pos)
					this->model_descr->auto_fixes.pop_back();
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

	if (!model_descr->stealth) model_descr->edits++;
	if (!model_descr->stealth) model_descr->change_count++;
}

//------------------------------------------------------------------------------

bool nodeController::canInsertChild(QString child, int dest_pos) {
	if (dest_pos > childs.size())
		return false;
	if ( ! node_type->child_info.children.contains(child) )
		return false;
	
	return true;
}

//------------------------------------------------------------------------------

bool nodeController::canInsertChild(nodeController* source, int dest_pos) {
	if (dest_pos > childs.size())
		return false;
	if ( ! node_type->child_info.children.contains(source->name))
		return false;
	if ( node_type->child_info.children[source->name].type != source->node_type ) 
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
	
	if (node_type->child_info.is_choice) {
		return (node_type->child_info.min_occurs.toInt() >= activeChilds().size());
	}
	else {
		return (node_type->child_info.min_occurs.toInt()*node_type->child_info.children[node->name].min_occurs.toInt() >= activeChilds(node->name).size());
	}
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

		if (!model_descr->stealth) {
			if (orig_disabled)
				model_descr->change_count--;
			else
				model_descr->change_count++;

			model_descr->edits++;
		}
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
		if (!model_descr->stealth) {
			if (orig_disabled)
				model_descr->change_count++;
			else
				model_descr->change_count--;
			model_descr->edits++;
		}
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

void nodeController::synchDOM() {
	if (disabled) {
		// synch xml comment !
		QString dis_node_text;
		QTextStream s(&dis_node_text);
		xmlDisabledNode.save(s,4);
		xmlNode.setNodeValue(dis_node_text);
	}
	else {
		for (auto& child : childs ) {
			child -> synchDOM();
		}
	}
}

void nodeController::setStealth(bool enabled) {
	model_descr->stealth = enabled;
}

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
	if (!model_descr->stealth) model_descr->auto_fixes.append(e);
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
