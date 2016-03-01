#include "scales.h"
#include "interfaces.h"
#include "property.h"

void Time_Scale::loadFromXML(const XMLNode xNode )
{
	if (xNode.isEmpty()) {
		cerr << "Empty node in Time_Scale" << endl;
		return;
	}
	getXMLAttribute(xNode, "value", xml_time);

	
	if (getXMLAttribute(xNode, "unit", unit)) {
		lower_case(unit);
		if (unit=="sec" || unit=="second"){  unit="sec"; unit_factor = 1;}
		else if (unit=="min" || unit=="minute" ) { unit="min"; unit_factor = 60;}
		else if (unit=="hour" ) { unit_factor = 3600;}
		else {
			cout << "'Unknown temporal unit " << unit << ". Defaulting to [atu]." << endl;
			unit = "atu";
			unit_factor = 1;
		}
		seconds_time = xml_time * unit_factor;
	}
	else {
		unit = "atu";
		seconds_time = xml_time;
		unit_factor = 1;
	}
	
	xml_tag_name = xNode.getName();
	
	if ( getXMLAttribute(xNode, "symbol", symbol_name) && ! symbol_name.empty() ) {
		shared_ptr< Property<double> > p = Property<double>::createConstantInstance(symbol_name, xml_tag_name);
		p->set(seconds_time);
		SIM::defineSymbol(p);
	}
	
}


XMLNode Time_Scale::saveToXML() const
{
	XMLNode xTime;
	if (xml_tag_name.empty())
		xTime = XMLNode::createXMLTopNode("Time");
	else
		xTime = XMLNode::createXMLTopNode(xml_tag_name.c_str());
	xTime.addAttribute("value",to_cstr(xml_time));
	if (unit != "atu")
		xTime.addAttribute("unit",unit.c_str());
	if (! symbol_name.empty())
		xTime.addAttribute("symbol",symbol_name.c_str());
	return xTime;
}

void Length_Scale::loadFromXML(const XMLNode xNode)
{
	if (xNode.isEmpty()) {
		cerr << "Empty node in Length_Scale" << endl;
		return;
	}
	getXMLAttribute(xNode, "value", xml_length);
	
	
	if (getXMLAttribute(xNode, "unit", unit) ) {
		lower_case(unit);
		
		if (unit=="m" || unit=="meter") { meter_length = xml_length; }
		else if (unit=="mm" || unit=="millimeter" ) { meter_length = xml_length * 1e-3;}
		else if (unit=="µm" || unit=="micron" || unit=="micrometer" ) { meter_length = xml_length * 1e-6;}
		else if (unit=="nanon" || unit=="nanometer" ) { meter_length = xml_length * 1e-9;}
		else {
			cout << "'Unknown spatial unit " << unit << ". Defaulting to [alu]." << endl;
			unit = "alu";
			meter_length = xml_length;
		}
	}
	else {
		unit = "alu";
		meter_length = xml_length;
	}
	
	xml_tag_name = xNode.getName();

	if ( getXMLAttribute(xNode, "symbol", symbol_name) && ! symbol_name.empty() ) {
		shared_ptr< Property<double> > p = Property<double>::createConstantInstance(symbol_name, xml_tag_name);
		p->set(meter_length);
		SIM::defineSymbol( p );
	}
}

XMLNode Length_Scale::saveToXML() const
{
	XMLNode xLength;
	if (xml_tag_name.empty())
		xLength = XMLNode::createXMLTopNode("Length");
	else
		xLength = XMLNode::createXMLTopNode(xml_tag_name.c_str());
	
	xLength.addAttribute("value",to_cstr(xml_length));
	if (unit != "alu")
		xLength.addAttribute("unit",unit.c_str());
	if (! symbol_name.empty())
		xLength.addAttribute("symbol",symbol_name.c_str());
	return xLength;
}

 
