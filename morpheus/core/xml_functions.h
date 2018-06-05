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

#ifndef xml_functions_h
#define xml_functions_h


#include "config.h"
#include "string_functions.h"
#include "traits.h"
#include "xmlParser/xmlParser.h"

#include <zlib.h>
#include <fstream>
#include <iomanip>
#include <iterator>


/// Some methods making consistent handling for reading and writing values and XML structure

XMLNode parseXMLFile(string filename);
XMLNode getXMLNode(const XMLNode XML_base, const string& path);
string getXMLPath(const XMLNode node);

int getXMLnNodes(const XMLNode XML_base, string path);
/// getXMLAttribute template

/**Read an attribute from XMLNode and convert via operator>>(std:istream,T value) into target type T 
 * Adopts to value type via template instancing.
 * @param XML_base reference xml node
 * @param path path relative to XML_base
 * @param value reference to a value of type T
 * @param verbose defaults to true. Dropping a message when reading was successful.
 * @return false in case of any error while reading. Does not override value in that case.
 */
template <class T>
bool getXMLAttribute(const XMLNode XML_base, string path, T& value, bool verbose=false) {

	string attribute;
	attribute=strip_last_token(path,"/");
	if (attribute.empty()) return false;
	if (verbose) {
		cout << "getXMLAttribute: seeking for ";
		if (path.length()==0) cout << XML_base.getName();
		else cout << path;
		cout << "->" << attribute;
	}

	XMLNode xNode;
	if (! path.empty()) xNode = getXMLNode(XML_base,path);
	else xNode = XML_base;

	if (xNode.isEmpty()) { if (verbose) cout << " .. not found" << endl; return false; }

	string str_val;
	if (lower_case(attribute) == "text")  {
		if (xNode.nText()) {
			str_val = xNode.getText(); 
		}
		else {
			if (verbose) cout << " .. not found" << endl; 
			return false;
		}
	}
	else if (xNode.isAttributeSet(attribute.c_str()) ) str_val = xNode.getAttribute(attribute.c_str());
	else { if (verbose) cout << " .. not found" << endl; return false; }

	T tmp = TypeInfo<T>::fromString(str_val);

	if (verbose) cout << ": " << TypeInfo<T>::toString(tmp) << endl;
	value=tmp;
	return true;
}

// template <>
// bool getXMLAttribute<string>(const XMLNode XML_base, string path, string& value, bool verbose);
// 
// template <>
// bool getXMLAttribute<bool>(const XMLNode XML_base, string path, bool& value, bool verbose);

template <class T>
bool setXMLAttribute(XMLNode XML_base, string path, const T& value) {
	string attribute=strip_last_token(path,"/");
	if (attribute.empty()) return false;

	XMLNode xNode = getXMLNode(XML_base,path);
	if (xNode.isEmpty()) return false;
	XMLCSTR str_val;
	if (lower_case(attribute) == "text") {
		xNode.updateText(to_str(value).c_str(),0);
		str_val = xNode.getText();
	}
	else xNode.addAttribute( attribute.c_str(), to_str(value).c_str() );
	return true;
}

bool setXMLAttribute(XMLNode XML_base, string path, const string& value);

bool setXMLAttribute(XMLNode XML_base, string path, const char* value);


#endif
