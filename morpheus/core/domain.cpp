#include "domain.h"
#include "lattice_data_layer.cpp"
#include "simulation.h"


std::string Boundary::type_to_name(Boundary::Type t)
{
	if (t == Boundary::noflux)
		return "noflux";
	if (t == Boundary::constant)
		return "constant";
	if (t == Boundary::periodic)
		return "periodic";
	return "none";
}

Boundary::Type Boundary::name_to_type(string s)
{
	lower_case(s);
	if (s=="noflux")
		return noflux;
	else if (s=="constant")
		return constant;
	else if (s=="periodic")
		return periodic;
	throw(string("Unknown boundary type ") + s);
}

std::string Boundary::code_to_name(Boundary::Codes code)
{
	switch (code) {
		case mx :
			return "-x";
		case px :
			return "x";
		case my : 
			return "-y";
		case py :
			return "y";
		case mz :
			return "-z";
		case pz :
			return "z";
	}
}


Boundary::Codes Boundary::name_to_code(std::string s)
{
	lower_case(s);
	if (s == "-x")
		return Boundary::mx; 
	else if (s == "-y")
		return Boundary::my;
	else if (s == "-z")
		return Boundary::mz;
	else if (s == "x")
		return Boundary::px; 
	else if (s == "y")
		return Boundary::py;
	else if (s == "z")
		return Boundary::pz;
	throw (string("Unknown boundary code ") + s);
	return Boundary::mx;
}

std::ostream& operator << (std::ostream& os, const Boundary::Type& a) { return (os << Boundary::type_to_name(a)); }
std::istream& operator >> (std::istream& is, Boundary::Type& a) { string s; is >> s; a = Boundary::name_to_type(s); return is; }

#include "lattice_data_layer.cpp"
#include "tiffio.h"
// explicite template instantiation ...
template class Lattice_Data_Layer<Boundary::Type>;


void Domain::loadFromXML(const XMLNode xNode,VINT size_hint)
{
	boundary_type = Boundary::noflux;
	getXMLAttribute(xNode, "boundary-type", boundary_type);
	if (xNode.nChildNode("Image")) {
		type = image;
		getXMLAttribute(xNode, "Image/path", image_path);
		uint32 width, height, slices;
		
// 		TIFFSetWarningHandler(0);
// 		TIFF* tif = TIFFOpen(image_path.c_str(), "r");
// 		if (tif) {
// 			TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
// 			TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
// 			slices = TIFFNumberOfDirectories(tif);
// 			VINT(width, height, slices);
// 		}
// 		else {
// 			throw(string("Unable to open Domain image ") + image_path + ".");
// 		}
		createImageMap(image_path);
		domain_size = max(size_hint, image_size);
		image_offset = (domain_size - image_size) /2;
		cout << "Domain Image size " << image_size << ", domain offset " << image_offset << endl;
	}
	else if (xNode.nChildNode("Circle")) {
		type = circular;
		getXMLAttribute(xNode, "Circle/diameter", circle_diameter);
		domain_size = max(VINT(circle_diameter,circle_diameter,1),size_hint);
		circle_center = domain_size / 2;
	}
	createEnumerationMap();

}

bool Domain::inside(const VINT& a) const
{
	switch (type) {
		case image :
			return insideImageDomain(a);
		case circular :
			return insideCircularDomain(a);
		case none :
			return true;
	}
}

bool Domain::insideCircularDomain(const VINT& a) const
{
	VINT radius(a-circle_center);
	radius.z=0;
	return (radius.abs() < circle_diameter/2);
}

int Domain::image_index(const VINT& a) const
{
	return (a.x + a.y * image_size.x + a.z * image_size.x*image_size.y);
}


bool Domain::insideImageDomain(const VINT& a) const
{
	VINT pos = a - image_offset;
	if (pos.x<0 || pos.x>=image_size.x || pos.y<0 || pos.y>=image_size.y || pos.z<0 || pos.z>=image_size.z ) {
		return false;
	}
	return image_map[image_index(pos)];
}

void Domain::createEnumerationMap() {
	VINT pos;
	for (pos.z=0; pos.z<this->domain_size.z; pos.z++)
		for (pos.y=0; pos.y<this->domain_size.y; pos.y++)
			for (pos.x=0; pos.x<this->domain_size.x; pos.x++) {
				if (inside(pos))
					domain_enumeration.push_back(pos);
			}
}

// bool BoundaryConstraint::circular_hex(const VINT& a) const {
// 	VDOUBLE b(lattice->to_orth(a-lattice_center));
// 	return (b.x*b.x + b.y*b.y < sqr_radius);
// };
// 
// bool BoundaryConstraint::circular(const VINT& a) const {
// 	VINT b(a-lattice_center);
// 	return b.x*b.x + b.y*b.y < sqr_radius;
// };


void Domain::createImageMap(string path) {
	
	TIFFSetWarningHandler(0);
	TIFF* tif = TIFFOpen(path.c_str(), "r");
	if (! tif)
		throw(string("Unable to open Domain image ") + path + ".");
	
	cout << "Loading Domain Image" << endl;
	uint32 width, height, slices;
	TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
	TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
	slices = TIFFNumberOfDirectories(tif);
	
	image_size = VINT(width,height,slices);
	image_map.resize(width * height * slices);
	
	uint32* raster;
	size_t npixels = width * height;
	raster = (uint32*) _TIFFmalloc( npixels * sizeof (uint32));
	if (!raster) 
		throw(string("imageDomain: out of memory"));
	
	VINT pos;
	do {
		uint32 w, h;
		TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
		TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
		if (width!=w || height!=h){
			throw (string("imageDomain: Require identical sizes for images in a stack!"));
		}
		
		unsigned short int numbits;
		TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &numbits);
		if( numbits == 8 ){
			if (TIFFReadRGBAImage(tif, w, h, raster, 0)) {
				pos.x=0; pos.y=0;
				for (uint i=0; i<npixels; i++) {
					uint8* channels = (uint8*) (raster+i);
					uint32 transparency = channels[3];
					uint32 grey_val = channels[0] + channels[1] + channels[2];
// 					cout  << grey_val << " ";
					
					image_map[i + w*h*pos.z] = (grey_val > 0);
				}
			}
		}
		else if( numbits == 16 ){
			cerr << "TIFF image is 16 bit. NOT IMPLEMENTED\n"; exit(-1);
		}
		pos.z++;
	} while( TIFFReadDirectory(tif) );
	
	_TIFFfree(raster);
	TIFFClose(tif);
}
