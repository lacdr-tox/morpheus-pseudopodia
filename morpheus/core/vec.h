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

#ifndef VECTOR_TYPES
    #define VECTOR_TYPES
    
// #define HAVE_VEC_INDEX_OPRT

#include "config.h"
#include "traits.h"
#include <cmath>
#include <iostream>
#include <functional>
#include "cpp_future.h"

#ifndef M_PI
  const double M_PI = 3.1415926535897932384626433832795028841971;
#endif
#ifndef M_SQRT3
  const double M_SQRT3 = 1.7320508075688772935274463415059;
#endif
#ifndef M_HALF_SQRT3
  const double  M_HALF_SQRT3 = 0.86602540378443864676372317075294;
#endif


inline int pmod(int a, int b) { a%=b; return (a<0) ? a+b : a; }
inline double pmod(double a, double b) { double k=fmod(a,b); return (k<0) ? k+b : k; }
DEPRECATED inline int MOD(int a, int b) { a%=b; return (a<0) ? a+b : a; }
DEPRECATED inline double MOD(double a, double b) { double k=fmod(a,b); return (k<0) ? k+b : k;	}


constexpr int pdiv(const int a, const int b) { return   (a>=0) ? a/b : (a/b-(a%b<0)); } // should be the same as int(floor(double(a)/b))
inline double pdiv(double a, double b){ return floor(a/b);}
DEPRECATED constexpr int DIV(const int a, const int b) { return   (a>=0) ? a/b : (a/b-(a%b<0)); } // should be the same as int(floor(double(a)/b))
DEPRECATED inline double DIV(double a, double b) { return floor(a/b);}
// inline double DIV(double a, int b){ return floor(a/b);}

// 	int pow(int a, int b) { int r=1; for (; b>0; --b) r*=a; return r;}
constexpr double sqr(double a){return a*a;}




template <class T>
class _V {
	public:
		T x,y,z;
		
		// default constructor
		constexpr _V() : x(0),y(0),z(0) {};
		constexpr  _V(T a, T b, T c):x(a),y(b),z(c) {};
		constexpr _V(const _V<T> &a) = default;
		constexpr _V(_V<T> &&) = default;

		// Conversion to _V<double>, allows implicit conversion of _V<int> to _V<double>
		constexpr operator _V<double>() const { return _V<double>(x,y,z); } 
		// Rounding to nearest integer
		explicit operator _V<int>() const { return _V<int>(x+(x < 0 ? -0.5 : 0.5), y+(y < 0 ? -0.5 : 0.5), z+(z < 0 ? -0.5 : 0.5)); } 
		
		// Assignment operator
		_V<T>& operator=(const _V<T> &) = default;
		_V<T>& operator=(_V<T> &&) = default;

		// comparison operator
		friend constexpr bool operator ==(const _V<T> &a, const _V<T> &b) { return a.x==b.x && a.y==b.y && a.z==b.z; }
		friend constexpr bool operator !=(const _V<T> &a, const _V<T> &b) { return (b.x!=a.x || b.y!=a.y || b.z!=a.z); };
		
#ifdef HAVE_VEC_INDEX_OPRT
		// index operator
		T& operator [] (int i) { return *(&x + i); }
		const T& operator [] (int i) const { return *(&x + i); }
#endif

		const _V<T>& operator *=(const _V<T> &a);
		const _V<T>& operator +=(const _V<T> &a);
		const _V<T>& operator -=(const _V<T> &a);
		constexpr _V<T> operator -() const { return _V<T>(-x,-y,-z); }

		// value properties
		constexpr double abs() const;
		constexpr double abs_sqr() const;
		constexpr double angle_xy() const;
		/// Returns the radial representation of the vector by phi, theta, radius triplet
		_V<double> to_radial() const;
		static _V<T> from_radial(const _V<double>& a);
		// to/from std::array to allow working with iterators
		constexpr array<T,3> to_array() const;
		static constexpr _V<T> from_array(const array<T,3> &a);
		// string functions
		string to_string() const;
		string to_stringp() const;
		// might be defined in any
		_V<double> norm() const;

		// Basic binary operations
		// Allow implicite type conversions by making them non-member friends with explicit template instantiation through definition in the header
		// ADD
		friend constexpr _V<T> operator +(const T a, const _V<T> &b) { return _V<T>(a + b.x, a + b.y, a + b.z); };
		friend constexpr _V<T> operator +(const _V<T> &b, const T a) { return _V<T>(a + b.x, a + b.y, a + b.z); };
		friend constexpr _V<T> operator +(const _V<T> &a, const _V<T> &b)  {return _V<T>(a.x+b.x,a.y+b.y,a.z+b.z);};
		// SUB
		friend constexpr _V<T> operator -(const T a, const _V<T> &b) { return _V<T>(a - b.x, a - b.y, a - b.z); };
		friend constexpr _V<T> operator -(const _V<T> &a, const T b) { return _V<T>(a.x - b, a.y - b, a.z - b); };
		friend constexpr _V<T> operator -(const _V<T> &a, const _V<T> &b) {return _V<T>(a.x-b.x,a.y-b.y,a.z-b.z);};
		// MULT
		// !! Product operator is elementwise !!
		friend constexpr _V<T> operator *(const T a, const _V<T> &b) {return _V<T>(a*b.x,a*b.y,a*b.z);};
		friend constexpr _V<T> operator *(const _V<T> &b, const T a) {return _V<T>(a*b.x,a*b.y,a*b.z);};
		friend constexpr _V<T> operator *(const _V<T> &a, const _V<T> &b) {return _V<T>(a.x*b.x,a.y*b.y,a.z*b.z);};
		
		//cross product
		friend constexpr _V<T> cross(const _V<T> &a, const _V<T> &b) { 
			return _V<T> ( (a.y * b.z) - (a.z * b.y), (a.z * b.x) - (a.x * b.z), (a.x * b.y) - (a.y * b.x) ); 
		}
		
		// dot / scalar product
		friend constexpr T dot(const _V<T> &a, const _V<T> &b) { 
			return (a.x*b.x+a.y*b.y+a.z*b.z);
		}

		// elementwise product
		friend constexpr _V<T> el_prod(const _V<T> &a, const _V<T> &b) { 
			return _V<T>(a.x*b.x,a.y*b.y,a.z*b.z);
		}

		// restrict point to within cube
		friend constexpr _V<T> clamp(const _V<T> &v, const _V<T> &lo, const _V<T> &hi) {
			return {cpp17::clamp(v.x, lo.x, hi.x),
					cpp17::clamp(v.y, lo.y, hi.y),
					cpp17::clamp(v.z, lo.z, hi.z)};
		}
		
		// Division
		// !! Division operator is elementwise !!
		friend constexpr _V<T> operator /(const _V<T> &a, const T b) {return _V<T>(a.x/b,a.y/b,a.z/b);}
		friend constexpr _V<T> operator /(const _V<T> &a, const _V<T> &b) {return _V<T>(a.x/b.x,a.y/b.y,a.z/b.z);}
		friend constexpr _V<T> pdiv(const _V<T> &a, const _V<T> &b) { return _V<T>(pdiv(a.x,b.x), pdiv(a.y,b.y), pdiv(a.z,b.z)); }
		// !! This is not the traditional modulo, but the periodic lattice modulo operation !!
		friend constexpr _V<T> operator %(const _V<T> &a, const _V<T> &b) { return pmod(a,b); };
		friend constexpr _V<T> pmod(const _V<T> &a, const _V<T> &b) { return _V<T>(b.x!=0 ? pmod(a.x,b.x) : 0, b.y!=0 ? pmod(a.y,b.y) : 0,b.z!=0 ? pmod(a.z,b.z) : 0); } 
		
		friend constexpr double dist(const _V<T> &a,const _V<T> &b) {return (b-a).abs();}
};

template <class T> const _V<T>& _V<T>::operator *=(const _V<T> &a) { this->x*=a.x; this->y*=a.y; this->z*=a.z; return *this; };
template <class T> const _V<T>& _V<T>::operator +=(const _V<T> &a) { this->x+=a.x; this->y+=a.y; this->z+=a.z; return *this; };
template <class T> const _V<T>& _V<T>::operator -=(const _V<T> &a) { this->x-=a.x; this->y-=a.y; this->z-=a.z; return *this; };
template <class T> constexpr double _V<T>::abs() const { return sqrt((this->x*this->x)+(this->y*this->y)+(this->z*this->z)); };
template <class T> constexpr double _V<T>::abs_sqr() const { return (this->x*this->x)+(this->y*this->y)+(this->z*this->z); };

template <class T>
constexpr double _V<T>::angle_xy() const {
	return (x==0 and y==0) ? 0 : atan2(y, x)  + (y<0 ? 2*M_PI : 0);
}

/** Convert a Vector into radial coordinates phi, theta, radius */
template <class T> 
_V<double> _V<T>::to_radial() const {
	double rho = this->abs();
	if(rho == 0)
		return _V<double>(0,0,0);
	double theta = M_PI - acos( this->z / rho );

	return _V<double>(angle_xy(), theta, rho);
}

/** Create a Vector from radial coordinates phi,theta, radius */
template <>
inline _V<double> _V<double>::from_radial(const _V< double >& a) {
	if (a.y!=0.0) 
		return _V<double>(cos(a.y) * cos(a.x) * a.z, cos(a.y) *  sin(a.x) * a.z, sin(a.y) * a.z);
	else
		return _V<double>(cos(a.x) * a.z, sin(a.x) * a.z, 0.0);
}

template <class T>
constexpr array<T,3> _V<T>::to_array() const {
	return {this->x, this->y, this->z};
}
template <class T>
constexpr _V<T> _V<T>::from_array(const array<T,3> &a) {
	return {a[0], a[1], a[2]};
}

template <class T>
string _V<T>::to_string() const {
	return std::to_string(this->x) + ","
		   + std::to_string(this->y) + ","
		   + std::to_string(this->z);
}

template <class T>
string to_string(const _V<T> &a) {
	return a.to_string();
}

template <class T>
string _V<T>::to_stringp() const {
	return "(" + this->to_string() + ")";
}

//template output operators
template <class T>
std::ostream& operator << (std::ostream& os, const _V<T>& a) { return os << to_string(a); }

template <class T>
std::istream& operator >> (std::istream& is, _V<T>& a) { char s; is >> a.x; is >> s; if (s!=',') is.putback(s);  is >> a.y; is >> s; if (s!=',') is.putback(s); is >> a.z; return (is); }

template <class T>
constexpr _V<T> max(const _V<T> &a,const _V<T> &b) { return _V<T>(max(a.x,b.x),max(a.y,b.y),max(a.z,b.z));}

// Integer division, rounded to the closest matching integer
constexpr _V<int> round_div(const _V<int> &a, const int b) {
	return _V<int>(int(double(a.x)/b + (a.x < 0 ? -0.5 : 0.5)),int(double(a.y)/b + (a.y < 0 ? -0.5 : 0.5)),int(double(a.z)/b + (a.z < 0 ? -0.5 : 0.5)));
};

// Normalisation
template <class T>
inline _V<double> _V<T>::norm() const {
		double b=this->abs();
		return (b>0) ? _V<double>(x/b,y/b,z/b) : _V<double>();
};

typedef _V<int> VINT;

typedef _V<double> VDOUBLE;

///  Computes the  angle between two vectors in2D [-PI,PI]
inline double angle_signed_2D(const VDOUBLE &a, const VDOUBLE &b){ 
	VDOUBLE v1(a.norm());
	VDOUBLE v2(b.norm());
	double dotp = dot(v1, v2);
	double perpDot = v1.x * v2.y - v1.y * v2.x;
	return atan2( perpDot, dotp);
}

///  Computes the  angle between two vectors [-PI,PI]
inline double angle_unsigned_3D(const VDOUBLE &v1, const VDOUBLE &v2){	// returns angle between two vectors [0, PI]
	return acos( dot(v1,v2) / (v1.abs() * v2.abs()));
}

constexpr double distance_plane_point(const VDOUBLE& plane_normal, const VDOUBLE& plane_point, const VDOUBLE& point) {
	return dot(plane_normal, ( plane_point - point));
}

typedef int64_t VINT_HASHTYPE;

struct VINThash {
	size_t operator()(const VINT& b) const
	{
		return std::hash<VINT_HASHTYPE>()((b.x)+((VINT_HASHTYPE)(b.y)<<16)+((VINT_HASHTYPE)(b.z)<<32));
	}
};


// struct that helps to build sorted vint container
struct less_VINT {  // we use a struct to prevent constructors
	bool operator() (const VINT& a, const VINT& b) const {
		return ( a.z < b.z || (a.z==b.z  &&  (a.y<b.y || (a.y==b.y && a.x<b.x))));
	}
	
};

struct CompareAngle {
	bool operator ()(const VINT &a, const VINT &b) const { 
		if (a.z != b.z)
			return a.z<b.z;
		else
			return a.angle_xy() < b.angle_xy(); 
	}
};


template <>
struct TypeInfo<VDOUBLE > : public TypeInfoDefault<VDOUBLE>{
	static string toString(Parameter val) {
		return TypeInfo<double>::toString(val.x) + "," + TypeInfo<double>::toString(val.y) + "," + TypeInfo<double>::toString(val.z);
	}
	static const string& name() { static const string n("VDouble"); return n;};
};


template <>
struct TypeInfo<VINT > : public TypeInfoDefault<VINT>{
	static string toString(Parameter val) {
		return TypeInfo<int>::toString(val.x) + "," + TypeInfo<int>::toString(val.y) + "," + TypeInfo<int>::toString(val.z);
	}
	static const string& name() { static const string n("VInt"); return n;};
};


#endif //VECTOR_Types

