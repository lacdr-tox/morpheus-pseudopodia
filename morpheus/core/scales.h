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

#ifndef TIME_SCALE_H
#define TIME_SCALE_H

#include "xml_functions.h"
#include "scope.h"
class Time_Scale
{
	private:
		double seconds_time;
		double unit_factor;
		double xml_time;
		string xml_tag_name;
		string symbol_name;
		string unit;
	public:
		Time_Scale(string xml_tag) : seconds_time(1), xml_time(1),xml_tag_name(xml_tag), unit("sec") {};
		Time_Scale(string xml_tag, double a) : seconds_time(a), xml_time(a),xml_tag_name(xml_tag), unit("sec") {};
		void set(double seconds) { seconds_time = seconds; };
		double getSeconds() { return seconds_time; };
		double operator()() { return seconds_time; };
		string getTimeScaleUnit(){ return unit; };
		double getTimeScaleUnitFactor() { return unit_factor; };
		double getTimeScaleValue(){ return xml_time; };
		void loadFromXML(const XMLNode, Scope* scope);
		XMLNode saveToXML() const;
};

class Length_Scale
{
	private:
		double meter_length;
		double xml_length;
		string xml_tag_name;
		string symbol_name;
		string unit;
	public:
		Length_Scale() : meter_length(1e-6), xml_length(1),xml_tag_name("NodeLength"), unit("meter") {};
		Length_Scale(string xml_tag) : meter_length(1e-6), xml_length(1),xml_tag_name(xml_tag), unit("meter") {};
		Length_Scale(string xml_tag, double a) : meter_length(a), xml_length(a),xml_tag_name(xml_tag), unit("meter") {};
		double getMeters() { return meter_length; };
		double operator()() { return meter_length; };
		string getLengthScaleUnit(){ return unit; };
		double getLengthScaleValue(){ return xml_length; };

		void loadFromXML(const XMLNode, Scope* scope);
		XMLNode saveToXML() const;
};

#endif // TIME_SCALE_H

