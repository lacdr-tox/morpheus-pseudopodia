#ifndef TRAITS_H
#define TRAITS_H


// Global type dependent switches and helper functions
#include "string_functions.h"
#include <type_traits>
#include <sstream>

using std::to_string;

template <class T>
struct TypeInfo {
	typedef const T& Return;
	typedef T SReturn; 
	typedef const T& Parameter; 
	typedef T& Reference; 
	static SReturn fromString(const string& val) {
		stringstream s(val);
		T ret;
		s >> ret;
		if (s.fail()) { throw string("Unable to read ") + name() + " value from string \'" + val + "'"; }
		return ret;
	}
	static string toString(Parameter val) {
		stringstream s;
		s.precision(6);
		s << val;
		return s.str();
	}
	static const string& name() { static const string n(typeid(T).name()); return n; };
};


template <>
struct TypeInfo<double> {
	typedef double Return;
	typedef double SReturn;
	typedef double Parameter; 
	typedef double& Reference; 
	static SReturn fromString(const string& val) {
		stringstream s(val); 
		double ret; 
		s >> ret; 
		if (s.fail()) { throw string("Unable to read ") + name() + " value from string \'" + val + "'";}
		return ret; 
	}
	static string toString(Parameter val) {
		string s(20,' ');
		s.resize(snprintf(&s[0],20,"%.10g",val));
		return s;
	}
	static const string& name() { static const string n("Double"); return n;};
};

template <>
struct TypeInfo<float> {
	typedef float Return;
	typedef float SReturn;
	typedef float Parameter; 
	typedef float& Reference; 
	static SReturn fromString(const string& val) { 
		stringstream s(val); 
		float ret; 
		s >> ret; 
		if (s.fail()) { throw string("Unable to read ") + name() + " value from string \'" + val + "'"; }
		return ret; 
	}
	static string toString(Parameter val) {
		string s(20,' ');
		s.resize(snprintf(&s[0],20,"%.6g",val));
		return s;
		
	}
	static const string& name() { static const string n("Float"); return n;};
};

template <>
struct TypeInfo<bool> {
	typedef bool Return;
	typedef bool SReturn;
	typedef bool Parameter; 
	typedef bool& Reference; 
	static bool fromString(string val) { 
		lower_case(val); 
		if ( val == "true"  || val == "1" ) return true;
		if ( val == "false" || val == "0" ) return false;
		throw string("Unable to read ") + name() + " value from string \'" + val + "'";
	}
	static string toString(Parameter val) { return val ? string("true"):string("false"); }
	static const string& name() { static const string n("Boolean"); return n;};
};

template <>
struct TypeInfo<string> {
	typedef const string& Return;
	typedef string SReturn;
	typedef const string& Parameter; 
	typedef string& Reference; 
	static string fromString(Parameter val) { return val; }
	static string toString(Parameter val) { return val; }
	static const string& name() { static const string n("String"); return n;};
};


template <>
struct TypeInfo<deque<double>> {
	typedef const deque<double>& Return;
	typedef deque<double> SReturn;
	typedef const deque<double>& Parameter; 
	typedef deque<double>& Reference; 
	static deque<double> fromString(string val) {
		 deque<double> ret;
		 double v;
		 stringstream s(val);
		 char c;
		 s >> v;
		 while (! s.fail()) {
			 ret.push_back(v);
			 s >> c >> v;
		}
		return ret;
	}
	static string toString(Parameter val) {
		string ret;
		auto it=val.begin();
		if (it==val.end())
			return ret;
		while(1) {
			ret += TypeInfo<double>::toString(*it);
			it++;
			if (it==val.end()) break;
			ret += ", ";
		}
		return ret;
	}
	static const string& name() { static const string n("Queue"); return n;};
};

template <class T>
string to_str(const T& value) {
	return TypeInfo<T>::toString(value);
}

template <class T>
const char* to_cstr(const T& value) {
	static string tmp = "";
	tmp = to_str(value);
	return tmp.c_str();
}
#endif // TRAITS_H


