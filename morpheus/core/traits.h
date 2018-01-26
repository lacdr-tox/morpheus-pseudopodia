#ifndef TRAITS_H
#define TRAITS_H


// Global type dependent switches and helper functions
#include "string_functions.h"

template <class T>
struct TypeInfo {
	typedef const T& Return;
	typedef T SReturn; 
	typedef const T& Parameter; 
	typedef T& Reference; 
	static SReturn fromString(const string& val) { stringstream s(val); T ret; s >> ret; if (s.fail()) { throw string("Unable to read value from string \'") + val + "'";} return ret; }
	static const string& name() { static const string n(""); return n; };
};


template <>
struct TypeInfo<double> {
	typedef double Return;
	typedef double SReturn;
	typedef double Parameter; 
	typedef double& Reference; 
	static SReturn fromString(const string& val) { stringstream s(val); double ret; s >> ret; if (s.fail()) { throw string("Unable to read value from string \'") + val + "'";} return ret; }
	static const string& name() { static const string n("Double"); return n;};
};

template <>
struct TypeInfo<float> {
	typedef float Return;
	typedef float SReturn;
	typedef float Parameter; 
	typedef float& Reference; 
	static SReturn fromString(const string& val) { stringstream s(val); double ret; s >> ret; if (s.fail()) { throw string("Unable to read value from string \'") + val + "'";} return ret; }
	static const string& name() { static const string n("Float"); return n;};
};

template <>
struct TypeInfo<bool> {
	typedef bool Return;
	typedef bool SReturn;
	typedef bool Parameter; 
	typedef bool& Reference; 
	static bool fromString(string val) { lower_case(val); if ( val == "true" ) return true; else if ( val == "false" ) return false; else throw string("Unable to read value from string \'") + val + "'"; }
	static const string& name() { static const string n("Boolean"); return n;};
};

template <>
struct TypeInfo<string> {
	typedef const string& Return;
	typedef string SReturn;
	typedef const string& Parameter; 
	typedef string& Reference; 
	static string fromString(string val) { return val; }
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
	static const string& name() { static const string n("Queue"); return n;};
};
#endif // TRAITS_H
