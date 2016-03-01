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

#include <config.h>
#include <cmath>
#include <iostream>
#include <tr1/functional>

#ifndef M_PI
  const double M_PI = 3.1415926535897932384626433832795028841971;
#endif
#ifndef M_SQRT3
  const double M_SQRT3 = 1.7320508075688772935274463415059;
#endif
#ifndef M_HALF_SQRT3
  const double  M_HALF_SQRT3 = 0.86602540378443864676372317075294;
#endif



inline int MOD(int a, int b) { a%=b; return (a<0) ? a+b : a; }
inline double MOD(double a, double b) { double k=fmod(a,b); return (k<0) ? k+b : k;	}

inline int DIV(const int a, const int b) {	return  (a>=0) ? a/b : (a/b-(a%b<0));}
inline double DIV(double a, double b){ return floor(a/b);}
inline double DIV(double a, int b){ return floor(a/b);}

// 	int pow(int a, int b) { int r=1; for (; b>0; --b) r*=a; return r;}
inline double sqr(double a){return a*a;}

template <class T>
class _V {
	private:
// 		static void* member_offset[3];
	public:
		T x,y,z;
		
		// default constructor
		_V() : x(0),y(0),z(0) {};
		_V(T a, T b, T c):x(a),y(b),z(c) {};
		_V(const _V<T> &a) = default;
		_V(_V<T> &&) = default;
		
		_V<T>& operator=(const _V<T> &) = default;
		_V<T>& operator=(_V<T> &&) = default;

		// template for type conversion routines
		// restricted to _V deviations ...
		template <class S> _V(const _V<S> &a); 

		// comparison operator
		bool operator ==(const _V<T> &a) const;
		bool operator !=(const _V<T> &a) const;
// 		T& operator [] (int a);
// 		const T& operator [] (int a) const;
		// Assignment operator
// 		const _V<T>& operator =(const _V<T> &a);

		const _V<T>& operator +=(const _V<T> &a);
		const _V<T>& operator -=(const _V<T> &a);

		// value properties
		double abs() const;
		double abs_sqr() const;
		double angle_xy() const;
		/// Returns the radial representation of the vector by phi, theta, radius triplet
		_V<double> to_radial() const;
		static _V<T> from_radial(const _V<double>& a);
		// might be defined in any
		_V<T> norm() const;

};

// template <class T> void* _V<T>::member_offset[] = NULL;

// template <class T> T& _V<T>::operator[](int a) {
// // quick and dirty
// 	return ((T*)(this))[a]; 
// //reliable 
// 	switch (a) {
// 		case 0: return this->x;
// 		case 1: return this->y;
// 		case 2: return this->z;
// 	}
// 	assert(0);
// 	return x;
// }

// template <class T> const T& _V<T>::operator [] (int a) const {
// // quick and dirty
// 	return ((T*)(this))[a]; 
// //reliable 
// 	switch (a) {
// 		case 0: return this->x;
// 		case 1: return this->y;
// 		case 2: return this->z;
// 	}
// 	assert(0);
// 	return x;
// }

template <class T> bool _V<T>::operator ==(const _V<T> &a) const { return (this->x==a.x && this->y==a.y && this->z==a.z); };
template <class T> bool _V<T>::operator !=(const _V<T> &a) const { return (this->x!=a.x || this->y!=a.y || this->z!=a.z); };
// template <class T> const _V<T>& _V<T>::operator =(const _V<T> &a) { this->x=a.x; this->y=a.y; this->z=a.z; return *this; };
template <class T> const _V<T>& _V<T>::operator +=(const _V<T> &a) { this->x+=a.x; this->y+=a.y; this->z+=a.z; return *this; };
template <class T> const _V<T>& _V<T>::operator -=(const _V<T> &a) { this->x-=a.x; this->y-=a.y; this->z-=a.z; return *this; };
template <class T> double _V<T>::abs() const { return sqrt((this->x*this->x)+(this->y*this->y)+(this->z*this->z)); };
template <class T> double _V<T>::abs_sqr() const { return (this->x*this->x)+(this->y*this->y)+(this->z*this->z); };
template <class T> double _V<T>::angle_xy() const {
	
// 	double S = sqrt(sqr(this->x) + sqr(this->y));
// 	if (S==0)
// 		return 0;
// 	if (this->x >= 0)
// 		if (this->y >= 0) 
// 			return  asin( this->y / S);
// 		else
// 			return  asin( this->y / S) + 2*M_PI;
// 	else
// 		return M_PI - asin( this->y / S);
	
	if (x==0 and y==0) return 0;
	return atan2(y, x)  + (y<0 ? 2*M_PI : 0);
}

template <class T> _V<double> _V<T>::to_radial() const {
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


// Basic binary operations
// to allow all possible type conversions one should make them non-member friends
template <class T> _V<T> operator +(const _V<T> &a, const _V<T> &b)  {return _V<T>(a.x+b.x,a.y+b.y,a.z+b.z);};
template <class T> _V<T> operator -(const _V<T> &a, const _V<T> &b) {return _V<T>(a.x-b.x,a.y-b.y,a.z-b.z);};
template <class T> _V<T> operator *(const T a, const _V<T> &b) {return _V<T>(a*b.x,a*b.y,a*b.z);};
template <class T> _V<T> operator *(const _V<T> &b, const T a) {return _V<T>(a*b.x,a*b.y,a*b.z);};
template <class T> T operator *(const _V<T> &a, const _V<T> &b) {return (a.x*b.x+a.y*b.y+a.z*b.z);};
template <class T> _V<T> operator %(const _V<T> &a, const _V<T> &b) {return _V<T>(b.x!=0 ? MOD(a.x,b.x) : 0, b.y!=0 ? MOD(a.y,b.y) : 0,b.z!=0 ? MOD(a.z,b.z) : 0);};
// template <class T> const _V<T> operator /(const _V<T> &a, const T b) {return _V<T>(a.x/b, a.y/b, a.z/b);};


//template output operators
template <class T>
std::ostream& operator << (std::ostream& os, const _V<T>& a) { return (os << a.x << ',' << a.y << ',' << a.z); }

template <class T>
std::istream& operator >> (std::istream& is, _V<T>& a) { char s; is >> a.x; is >> s; if (s!=',') is.putback(s);  is >> a.y; is >> s; if (s!=',') is.putback(s); is >> a.z; return (is); }

template <class T>
inline double dist(const _V<T> &a,const _V<T> &b) {return (b-a).abs();}

template <class T>
inline _V<T> max(const _V<T> &a,const _V<T> &b) { return _V<T>(max(a.x,b.x),max(a.y,b.y),max(a.z,b.z));}

// Integer specific operations


inline _V<int> operator /(const _V<int> &a, const int b) {
	return _V<int>(int(double(a.x)/b + (a.x < 0 ? -0.5 : 0.5)),int(double(a.y)/b + (a.y < 0 ? -0.5 : 0.5)),int(double(a.z)/b + (a.z < 0 ? -0.5 : 0.5)));
};

typedef _V<int> VINT;


// Double specific operations
// Type Cast
template<> template<>
inline _V<double>::_V(const _V<int> &a) :x(a.x),y(a.y),z(a.z) {};

template<> template<>
inline _V<int>::_V(const _V<double> &a) :x(a.x),y(a.y),z(a.z) {};


// Division
inline _V<double> operator /(const _V<double> &a, const double b) {return _V<double>(a.x/b,a.y/b,a.z/b);}

// inline const _V<double> operator /(const _V<double> &a, const int b) {return _V<double>(a.x/b,a.y/b,a.z/b);}


// Normalisation
template<>
inline _V<double> _V<double>::norm() const {
		double b=this->abs();
		return (b>0) ? _V<double>(x/b,y/b,z/b) : _V<double>();
};

typedef _V<double> VDOUBLE;

inline VDOUBLE operator -(const VDOUBLE &a, const VINT &b) { return VDOUBLE(a.x-b.x,a.y-b.y,a.z-b.z); }
inline VDOUBLE operator -(const VINT &a, const VDOUBLE &b) { return VDOUBLE(a.x-b.x,a.y-b.y,a.z-b.z); }
inline VDOUBLE operator +(const VDOUBLE &a, const VINT &b) { return VDOUBLE(a.x+b.x,a.y+b.y,a.z+b.z); }
inline VDOUBLE operator +(const VINT &a, const VDOUBLE &b) { return VDOUBLE(a.x+b.x,a.y+b.y,a.z+b.z); }
inline VDOUBLE operator *(const VDOUBLE &a, const VINT &b) { return VDOUBLE(a.x*b.x,a.y*b.y,a.z*b.z); }
inline VDOUBLE operator *(const VINT &a, const VDOUBLE &b) { return VDOUBLE(a.x*b.x,a.y*b.y,a.z*b.z); }

inline VDOUBLE cross(const VDOUBLE &a, const VDOUBLE &b) { // cross (vector) product
	VDOUBLE cross;
	cross.x = (a.y * b.z) - (a.z * b.y);
	cross.y = (a.z * b.x) - (a.x * b.z);
	cross.z = (a.x * b.y) - (a.y * b.x);
	return cross;
}

inline double dot(const VDOUBLE &a, const VDOUBLE &b) { // dot product
	return (a.x*b.x+a.y*b.y+a.z*b.z);
}
 
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

inline double distance_plane_point(const VDOUBLE& plane_normal, const VDOUBLE& plane_point, const VDOUBLE& point) {
	return (plane_normal * ( plane_point - point));
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



#endif //VECTOR_Types

