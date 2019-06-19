#ifndef TRAITS_H
#define TRAITS_H


// Global type dependent switches and helper functions
#include "config.h"
#include "string_functions.h"
#include <type_traits>
#include <sstream>

using std::to_string;

template <class T>
struct TypeInfoDefault {
	typedef const T& Return;
	typedef T SReturn; 
	typedef const T& Parameter; 
	typedef T& Reference;
	static SReturn fromString(const string& val) {
		stringstream s(val);
		T ret;
		s >> ret;
		if (s.fail()) { throw string("Unable to read value from string \'") + val + "'"; }
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

template <class T>
struct TypeInfoSmallDefault {
	typedef T Return;
	typedef T SReturn; 
	typedef T Parameter; 
	typedef T& Reference;
	static SReturn fromString(const string& val) {
		stringstream s(val);
		T ret;
		s >> ret;
		if (s.fail()) { throw string("Unable to read value from string \'") + val + "'"; }
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

template <class T>
class TypeInfo : public TypeInfoDefault<T> {};

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

template <>
struct TypeInfo<double> : public TypeInfoSmallDefault<double> {
	static string toString(Parameter val) {
		string s(20,' ');
		s.resize(snprintf(&s[0],20,"%.10g",val));
		return s;
	}
	static const string& name() { static const string n("Double"); return n;};
};

template <>
struct TypeInfo<float> : public TypeInfoSmallDefault<float> {
	static string toString(Parameter val) {
		string s(20,' ');
		s.resize(snprintf(&s[0],20,"%.6g",val));
		return s;
		
	}
	static const string& name() { static const string n("Float"); return n;};
};

template <>
struct TypeInfo<bool> : public TypeInfoSmallDefault<double> {
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
struct TypeInfo<string> : public TypeInfoDefault<string> {
	static string fromString(Parameter val) { return val; }
	static string toString(Parameter val) { return val; }
	static const string& name() { static const string n("String"); return n;};
};

template <>
struct TypeInfo<deque<double>> : public TypeInfoDefault<deque<double>> {
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

#ifdef HAVE_BOOST
#include <boost/circular_buffer.hpp>

template <class T>
struct TypeInfo<boost::circular_buffer<T>> : public TypeInfoDefault<boost::circular_buffer<T>> {
	static boost::circular_buffer<T> fromString(const string& val) {
		vector<T> buffer;
		T v;
		stringstream s(val);
		char c;
		s >> v;
		while (! s.fail()) {
			 buffer.push_back(v);
			 s >> c >> v;
		}
		boost::circular_buffer<T> ret(buffer.size()+1);
		for (const auto& val : buffer) {
			ret.push_back(val);
		}
		return ret;
	}
	static string toString(typename TypeInfoDefault<boost::circular_buffer<T>>::Parameter val) {
		string ret;
		auto it=val.begin();
		if (it==val.end())
			return ret;
		while(1) {
			ret += string("(") + to_str(it->time) + "," + to_str(it->value) + string(")");
			it++;
			if (it==val.end()) break;
			ret += ", ";
		}
		return ret;
	}
	static const string& name() { static const string n("CircularBuffer"); return n;};
};

#endif


#include "vec.h" 

#endif // TRAITS_H


