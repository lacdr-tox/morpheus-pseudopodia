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

#ifndef DOMAIN_H
#define DOMAIN_H

#include "xml_functions.h"
#include "tiffio.h"
#include "vec.h"
#include <valarray>

struct Boundary {
	enum Type { none=1, periodic=-1, constant=-2, noflux=-3 };
	enum Codes { mx=0, px=1, my=2, py=3, mz=4, pz=5 };
	static const int nCodes = 6;
	static const int nTypes = 4;
	static string code_to_name(Codes c);
	static Codes name_to_code(string s);
	static string type_to_name(Type t);
	static Type name_to_type(string name);
};
// inline bool operator !(Boundary::Type t) { return t != Boundary::none; static_cast<>(0);}

// in- and output operators
std::ostream& operator << (std::ostream& os, const Boundary::Type& a) ;
std::istream& operator >> (std::istream& is, Boundary::Type& a);

class Lattice;
struct LatticeDesc;
class Scope;

class Domain {
public:
	Domain() : boundary_type(Boundary::Type::none), type(none), lattice(nullptr) {};
	void loadFromXML(const XMLNode xNode, Scope* scope, const LatticeDesc& desc);
	void init(Lattice* l);
	enum Type {none ,image, circle, hexagon};

	VINT size() const { return domain_size; };
	const vector<VINT>& enumerated() const {return domain_enumeration; };
	Boundary::Type boundaryType() const { return boundary_type; }
	bool inside(const VINT& a) const;
	Type domainType() const { return type; }

private:
	Boundary::Type boundary_type;
	Type type;
	Lattice* lattice;
	VINT domain_size;
    double diameter;
	VINT center;
	
	
	string image_path;
	VINT image_size;
	VINT image_offset;
	
	void createImageMap(string path, bool invert);
    void createEnumerationMap();
	bool insideImageDomain(const VINT& a) const;
	bool insideCircularDomain(const VINT& a) const;
	bool insideHexagonalDomain(const VINT& a) const;
	int image_index(const VINT& a) const;
	valarray<bool> image_map;
	vector<VINT> domain_enumeration;
};

#endif
