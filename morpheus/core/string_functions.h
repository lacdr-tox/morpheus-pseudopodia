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

#ifndef string_functions_h
#define string_functions_h

#include <cstdio>
#include <iostream>
#include <vector>
#include <set>
#include <deque>
#include <map>
#include <iomanip>
#include <iterator>
#include <algorithm>
#include <sstream>
#include <cstring>
#include "config.h"


#define c_array_size( a )  (sizeof(a)/sizeof(a[0]))
#define c_array_begin( a ) (a)
#define c_array_end( a )   (a+c_array_size(a))

using namespace std;



/// additional output operators for loading and storing data
ostream& operator << (ostream& out, bool b);
istream& operator >> (istream& in, bool& b);

// output operators for Vectors
template <class T>
ostream& operator << (ostream& out, vector<T> v) {
	copy(v.begin(), v.end(), ostream_iterator<T>(out,";"));
	return out;
}

template <class T>
istream& operator >> (istream& in, vector<T>& v) {
	T tmp;
	while (1) {
		in >> tmp;
		if ( in.fail() ) break;
		v.push_back(tmp);
		char sep;
		in >> sep;
		if (sep != ';' or in.bad()) break;
	}
	return in;
}

/// additional output operators for loading and storing data
istream& operator >> (istream& stream, deque<double>& q);
ostream& operator << (ostream& stream, const deque<double>& q);

// output operators for MAPS
template <class T1, class T2>
ostream& operator << (ostream& out, map<T1,T2> m) {
	copy(m.begin(), m.end(), ostream_iterator<pair<T1,T2> >(out,";"));
	return out;
}
template <class T1, class T2>
istream& operator >> (istream& in, map<T1,T2>& m) {
	pair<T1,T2> tmp;
	while (1) {
		in >> tmp;
		if ( in.fail() ) break;
		m[tmp.first] = tmp.second;
		char sep;
		in >> sep;
		if (sep != ';' or in.fail()) break;
	}
	return in;
}


template <class T1, class T2>
ostream& operator << (ostream& out, pair<T1,T2> p) {
	out << p.first << " => " << p.second;
	return out;
}

template <class T1, class T2>
istream& operator >> (istream& in, pair<T1,T2>& p) {
	string sep;
	in >> p.first >> sep >> p.second;
	return in;
}

string strip_last_token(string& str, const string& del="/");
string remove_spaces(string& str);
string replace_spaces(string& str, char replacement='_');
bool replace_substring(std::string& str, const std::string& from, const std::string& to);
vector<string> tokenize(const string& str, const string& delimiters = " ");
string join(const vector<string>& strings, string delim);
string join(const std::set<string>& strings, string delim);
string join(const std::multiset<string>& strings, string delim);

string& lower_case(string& a);
string lower_case(const char* a);

template <class T> 
string to_str(T value, int prec=-1) {
	static stringstream s("");
	static bool init=false;
	if (!init)  { s.precision(2); init=true; }
	else  { s.clear(); s.str("");}
	if (prec >= 0 ) s.precision(prec);
	s << value; 
	return s.str();
}


string to_str(int value, int width);
//string to_str(uint value, int width);
string to_str(bool value, int prec=-1);

template <class T>
const char* to_cstr(T value, int prec=-1) {
	static string tmp = "";
	tmp = to_str(value,prec);
	return tmp.c_str();
}
// template <class T>
// bool from_cstr(T value, const char* cstr) {
// 	stringstream s(cstr);
// 	s >> value;
// }

#endif
