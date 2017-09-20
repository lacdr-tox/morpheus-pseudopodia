#include "xml_functions.h"

XMLNode parseXMLFile(string filename) {
	gzFile file=gzopen(filename.c_str(), "rb");
	if (file==NULL) { cerr << "unable to open file " << filename << endl; exit(-1); }
	cout << "Initializing from file " << filename << endl;
	string stringbuff;	int error;
	void* buff= malloc(16384*sizeof(char));
	char* c=(char*)buff;	int i;
	while ((i=gzread(file,buff,sizeof(buff)))>0) {
		c[i]='\0';
		stringbuff+=c;
	}
	free(buff);
	gzerror(file,&error);
	gzclose(file); 
	if (!(error==Z_STREAM_END or error==Z_OK)) {
		cerr << "error in " << error << " " << filename << ": "<< gzerror(file,&error); getchar();exit(-1);
	}
	
// and parse the file
	cout << "parsing ..." << endl;

	XMLNode xml_root=XMLNode::parseString(stringbuff.c_str(),"MorpheusModel");
	if( xml_root.isEmpty() )
		xml_root=XMLNode::parseString(stringbuff.c_str(),"CellularPottsModel");
	
	
	return xml_root;
}

XMLNode getXMLNode(const XMLNode XML_base, const string& path) {
	vector<string> tags=tokenize(path,"/");
	
	XMLNode xNode = XML_base;
	for (vector<string>::iterator itag=tags.begin(); itag<tags.end();itag++) {
		if ( itag->empty() ) continue;
		int ntag=0;
		string::size_type r_bracket, l_bracket;
		// select from multiple identical tags via indexing --> [i]
		if ((r_bracket=itag->find_last_of("]")) != string::npos && (l_bracket = itag->find_last_of("[")) != string::npos && r_bracket > l_bracket)  {
			stringstream tmp(itag->substr(l_bracket+1, r_bracket-1));
			itag->resize(l_bracket);
			tmp >> ntag;
//			cout << "[" << *itag << " "<< ntag+1 << "of"<< xNode.nChildNode(itag->c_str()) << "]"; 
		}
		if ( xNode.nChildNode(itag->c_str()) > ntag ) xNode = xNode.getChildNode(itag->c_str(),ntag); 
		else
			return XMLNode::emptyXMLNode;
	}
	return xNode;
}

string getXMLPath(const XMLNode node) {
	const XMLNode parent = node.getParentNode();
	if (parent.isEmpty())
		return  node.getName();

	string path;
	path = getXMLPath(parent);
	
	int n_childnodes = parent.nChildNode(node.getName());
	
	if (n_childnodes>1) {
		// let's find the position of the current node
		int pos;
		for (pos=0; pos<n_childnodes; pos++) {
			if ( parent.positionOfChildNode(parent.getChildNode(node.getName(),pos)) == parent.positionOfChildNode(node)) 
				break;
		}
		if (pos == n_childnodes) {
			std::cerr << "getXMLPath Ooops: Cannot find XML node in the parent" << endl;
		}
		path += string("/") + node.getName() + "[" + to_str(pos) + "]";
	}
	else {
		path += string("/") + node.getName();
	}
	return path;
}

int getXMLnNodes(const XMLNode XML_base, string path) {
	string last_tag = strip_last_token(path,"/");
	XMLNode xNode=getXMLNode(XML_base, path);
	return xNode.nChildNode(last_tag.c_str());
}

bool setXMLAttribute(XMLNode XML_base, string path, const string& value) {
	string attribute=strip_last_token(path,"/");
	if (attribute.empty()) return false;

	XMLNode xNode = getXMLNode(XML_base,path);
	if (xNode.isEmpty()) return false;
	if (lower_case(attribute) == "text") {
		xNode.updateText(value.c_str(),0);
	}
	else xNode.addAttribute( attribute.c_str(), value.c_str());
	return true;
}

template<>
bool getXMLAttribute<string>(const XMLNode XML_base, string path, string& value, bool verbose) {

	string attribute;
	attribute=strip_last_token(path,"/");
	if (attribute.empty()) return false;
	if (verbose) {
		cout << "getXMLAttribute: seeking for ";
		if (path.empty()) cout << XML_base.getName();
		else cout << path;
		cout << "->" << attribute;
	}

	XMLNode xNode;
	if (path.length()>0) xNode = getXMLNode(XML_base, path);
	else xNode = XML_base;
// 	if (xNode.isEmpty()) { if (verbose) cout << endl; return false; }

	XMLCSTR str_val;
	if (lower_case(attribute) == "text") {
		if (xNode.nText()) str_val = xNode.getText(); 
		else { if (verbose) cout << " .. not found" << endl; return false;}
	}
	else if (xNode.isAttributeSet(attribute.c_str()) ) str_val = xNode.getAttribute(attribute.c_str());
	else { if (verbose) cout << " .. not found" << endl; return false; }
	value = string(str_val);
	if (path.length()==0) path=XML_base.getName();
	if (verbose) cout << ": " << value << endl;
	return true;
}

template <>
bool getXMLAttribute<bool>(const XMLNode XML_base, string path, bool& value, bool verbose) {
	
	string attribute = strip_last_token(path,"/");
	if (attribute.empty()) return false;
	if (verbose) {
		cout << "getXMLAttribute: seeking for ";
		if (path.empty()) cout << XML_base.getName();
		else cout << path;
		cout << "->" << attribute;
	}

	XMLNode xNode;
	if (path.length()>0) xNode = getXMLNode(XML_base, path);
	else xNode = XML_base;
	
	XMLCSTR str_val;
	if (lower_case(attribute) == "text") {
		if (xNode.nText()) str_val = xNode.getText(); 
		else { if (verbose) cout << " .. not found" << endl; return false;}
	}
	else if (xNode.isAttributeSet(attribute.c_str()) ) str_val = xNode.getAttribute(attribute.c_str());
	else { if (verbose) cout << " .. not found" << endl; return false; }
	// TODO does not allow any spaces in front or after the value
	if ( ! strcmp(str_val,"1") || ! strcmp(str_val,"true") )  { value = true; if (verbose) { cout << ": true" << endl;}  return true;};
	if ( ! strcmp(str_val,"0") || ! strcmp(str_val,"false") ) { value= false; if (verbose) { cout << ": false" << endl;} return true;};
	cout << " error converting value" << endl;
	return false;
}

bool setXMLAttribute(XMLNode XML_base, string path, const char* value) {
	string attribute=strip_last_token(path,"/");
	if (attribute.empty()) return false;

	XMLNode xNode = getXMLNode(XML_base,path);
	if (xNode.isEmpty()) return false;
	XMLCSTR str_val;
	if (lower_case(attribute) == "text") {
		xNode.updateText(value,0);
		str_val = xNode.getText();
	}
	else xNode.updateAttribute( attribute.c_str(), value);
	return true;
}

namespace xml {
	

}

