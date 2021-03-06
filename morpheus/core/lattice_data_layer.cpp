#include "lattice_data_layer.h"
#ifndef LATTICE_DATA_LAYER_CPP
#define LATTICE_DATA_LAYER_CPP

template <class T> uint Lattice_Data_Layer<T>::get_data_index(const VINT& a) const {
	return dot((a + shadow_width), shadow_offset);
}
// template <class T>
// uint Lattice_Data_Layer<T>::get_shadow_index(const VINT& a) const  { return a*shadow_offset; }

template <class T>
Lattice_Data_Layer<T>::Lattice_Data_Layer(shared_ptr<const Lattice> lattice,int shadow_width, T def_val, string layer_name) :
 shadow_base_width(shadow_width), _lattice(lattice), boundary_types(lattice->get_boundary_types())
{
// 	data = NULL;
	name = layer_name;
	dimensions = lattice->getDimensions();
	structure = lattice->getStructure();
	


	default_value = def_val;
	default_boundary_value = def_val;
	boundary_values.resize(Boundary::nCodes);
	for (auto & b_val : boundary_values) b_val = make_unique<DefaultValueReader>(default_value);
	domain_value  =  make_unique<DefaultValueReader>(default_value);
	
	// assume that the pde layer covers the whole lattice
	using_buffer = false;
	using_domain = false;
	has_reduction = false;
	l_size = lattice->size();
	
	this->shadow_width = VINT(shadow_base_width,dimensions>1 ? shadow_base_width:0, dimensions>2 ? shadow_base_width:0);
	allocate();
	if (lattice->getDomain().domainType() != Domain::none)
		setDomain();

	reset_boundaries();
}

template <class T> void  Lattice_Data_Layer<T>::allocate() {
	// precalculate some variables
	// TODO shadow width should be reset when using reduxtion !!
	shadow_size = l_size + 2 * shadow_width;
	shadow_size_size_xyz = shadow_size.x * shadow_size.y * shadow_size.z;
	shadow_offset =  VINT(1,shadow_size.x, shadow_size.x*shadow_size.y);
	
	// create a data lattice with a 2 node wide halo around
	// such that index = (pos + shadow_shift) * shadow_offset;
	
	// Remove particular dimension from access (does not account for the allocation yet)
	if (has_reduction) {
		if (reduction == Boundary::mz || reduction == Boundary::pz) shadow_offset.z=0;
		if (reduction == Boundary::my || reduction == Boundary::py) shadow_offset.y=0;
		if (reduction == Boundary::mx || reduction == Boundary::px) shadow_offset.x=0;
	}


	data.resize(shadow_size_size_xyz);
	data = default_value;
	if (using_buffer) {
		write_buffer.resize(data.size(), default_value);
	}
	
	if (using_domain) {
		domain.resize(data.size(),Boundary::none);
	}
	
	span xmb = span(0,shadow_width.x-1);
	s_xmb = xslice(xmb);
	s_xm  = xslice(xmb + shadow_width.x);
	s_xp  = xslice(xmb + l_size.x);
	s_xpb = xslice(xmb + l_size.x + shadow_width.x);
	
	if (dimensions>1) {
		span ymb = span(0,shadow_width.y-1);
		s_ymb = yslice(ymb);
		s_ym  = yslice(ymb + shadow_width.y);
		s_yp  = yslice(ymb + l_size.y);
		s_ypb = yslice(ymb + l_size.y + shadow_width.y);
	}
	if (dimensions>2) {
		span zmb = span(0,shadow_width.z-1);
		s_zmb = zslice(zmb);
		s_zm  = zslice(zmb + shadow_width.z);
		s_zp  = zslice(zmb + l_size.z);
		s_zpb = zslice(zmb + l_size.z + shadow_width.z);
	}
};

template <class T>
void Lattice_Data_Layer<T>::loadFromXML(const XMLNode xNode, shared_ptr<ValueReader> converter ) {
	// read the default value
	getXMLAttribute(xNode,"name",name);
// 	getXMLAttribute(xNode, "DefaultValue/text", default_value);

	string restr_name;
	
	if ( (xNode.nChildNode("Reduction") && getXMLAttribute(xNode,"Reduction/text", restr_name)) || (getXMLAttribute(xNode,"Reduction/value", restr_name)) ) {
		reduction = Boundary::name_to_code(restr_name);
		has_reduction = true;
		allocate();
	}
	
	for (int i=0; i< xNode.nChildNode("BoundaryValue"); i++) {
		string code_name;
		XMLNode xbv = xNode.getChildNode("BoundaryValue",i);
		if ( ! getXMLAttribute(xbv,"boundary",code_name) )
			throw(string("Missing boundary definition in BoundaryValue"));
		lower_case(code_name);
			
// 		value_type val = default_boundary_value;
		
		string str_val;
		if ( ! getXMLAttribute(xbv, "value", str_val) )
			throw(string("Missing or invalid value in BoundaryValue"));
		
		shared_ptr<ValueReader> val = converter->clone();
		val->set(str_val);
		
		if (code_name == "domain") {
			domain_value = val;
		}
		else {
			Boundary::Codes code = Boundary::name_to_code(code_name);
			boundary_values[code] = val;
		}
	}
	
	if (using_domain)
		setDomain();
	reset_boundaries();
	
	stored_node = xNode;
}

//save lattice to XML-file
template <class T> void Lattice_Data_Layer<T>::setDomain()
{
// 	cout << "Creating domain map" << endl;
	using_domain = true;
	if ( domain.size() != data.size())
		domain.resize(data.size(),Boundary::none);

	VINT pos;
	const Domain& d = _lattice->getDomain();
	for (pos.z=0; pos.z<l_size.z; pos.z++) {
		for (pos.y=0; pos.y<l_size.y; pos.y++) {
			for (pos.x=0; pos.x<l_size.x; pos.x++) {
				
				if (d.inside(pos)) {
					domain[get_data_index(pos)] = Boundary::none;
					data[get_data_index(pos)] = default_boundary_value;
				}
				else {
					domain[get_data_index(pos)] = d.boundaryType();
					data[get_data_index(pos)] = domain_value->get(pos);
				}
			}
		}
	}
	domain[s_xmb] = boundary_types[Boundary::mx];
	domain[s_xpb] = boundary_types[Boundary::px];
	domain[s_ymb] = boundary_types[Boundary::my];
	domain[s_ypb] = boundary_types[Boundary::py];
	domain[s_zmb] = boundary_types[Boundary::mz];
	domain[s_zpb] = boundary_types[Boundary::pz];
	if (boundary_types[Boundary::mx] == Boundary::periodic) {
		domain[s_xmb] = domain[s_xp];
		domain[s_xpb] = domain[s_xm];
	}
	if (boundary_types[Boundary::my] == Boundary::periodic) {
		domain[s_ymb] = domain[s_yp];
		domain[s_ypb] = domain[s_ym];
	}
	if (boundary_types[Boundary::mz] == Boundary::periodic) {
		domain[s_zmb] = domain[s_zp];
		domain[s_zpb] = domain[s_zm];
	}
}

//save lattice to XML-file
template <class T> XMLNode Lattice_Data_Layer<T>::saveToXML() const
{
	return stored_node;
}

template <class T>
void Lattice_Data_Layer<T>::storeData(ostream &out) const {
	const bool binary = false;
	
	if ( ! binary ) {
		string xsep(" "), ysep("\n"), zsep("\n");
	
		VINT pos;
		for (pos.z=0; pos.z<l_size.z; pos.z++) {
			for (pos.y=0; pos.y<l_size.y; pos.y++) {
				pos.x=0;
				out << this->get(pos);
				for (pos.x=1; pos.x<l_size.x; pos.x++) {
					out << xsep << this->get(pos);
				}
				if (pos.y+1<l_size.y) out << ysep;
			}
			if (pos.z+1<l_size.z) out << zsep;
		}
	}
}
	
template <class T>
bool Lattice_Data_Layer<T>::restoreData(istream& in,  T (* converter)(istream &)) {
	// iterate over the data points
	VINT pos;
	for (pos.z=0; pos.z<l_size.z; pos.z++) {
		for (pos.y=0; pos.y<l_size.y; pos.y++) {
			for (pos.x=0; pos.x<l_size.x; pos.x++) {
				data[get_data_index(pos)] = converter(in);
				if (in.fail()) {
					if (in.eof())
						throw (string("Lattice_Data_Layer<T>::restoreData: Not sufficient data in input stream"));
					else
						throw (string("Lattice_Data_Layer<T>::restoreData: Cannot convert values input stream"));
					return false;
				}
			}
		}
	}
	reset_boundaries();
	return true;
}


template <class T>
Boundary::Type Lattice_Data_Layer<T>::getBoundaryType(Boundary::Codes code) const
{
	return _lattice->get_boundary_type(code);
}

template <class T> 
typename TypeInfo<T>::Return Lattice_Data_Layer<T>::get(VINT a) const {
// 	if (using_domain) {
// 		lattice->resolve(a);
// 		assert(accessible(a));
// 		return data[get_data_index(a)];
// 	}
// 	else {
// 		Boundary::Codes b;
		if ( accessible(a) )
			return data[get_data_index(a)];
		else if ( _lattice->resolve(a))
			return data[get_data_index(a)];
		else
			throw string("Unable to access position ") + to_str(a);
// 		return boundary_values[b]->get(a);
// 	}
};

template <class T> 
bool Lattice_Data_Layer<T>::set(VINT a, typename TypeInfo<T>::Parameter b) {
// 	if ( lattice->accessible(a) ) {
// 		data[get_data_index(a)] = b;
// 	}
	VINT periodic_shifts(0,0,0);
	
	if ( this->writable_resolve(a)) {
		data[get_data_index(a)] = b;
		if (boundary_types[Boundary::px] == Boundary::periodic) {
			if (a.x<shadow_width.x) {
				periodic_shifts.x = l_size.x;
				data[get_data_index(a+periodic_shifts)] = b;
			} else if (a.x > l_size.x-shadow_width.x-1) {
				periodic_shifts.x =  -l_size.x;
				data[get_data_index(a+periodic_shifts)] = b;
			}
		}
		if (boundary_types[Boundary::py] == Boundary::periodic) {
			if (a.y<shadow_width.y) {
				periodic_shifts.y = l_size.y;
				data[get_data_index(a+periodic_shifts)] = b;
				if (periodic_shifts.x!=0)
					data[get_data_index(a+VINT(0,periodic_shifts.y,0))] = b;
			} else if (a.y > l_size.y-shadow_width.y-1) {
				periodic_shifts.y = -l_size.y;
				data[get_data_index(a+periodic_shifts)] = b;
				if (periodic_shifts.x!=0)
					data[get_data_index(a+VINT(0,periodic_shifts.y,0))] = b;
			}
		}
		if (boundary_types[Boundary::pz] == Boundary::periodic) {
			if (a.z<shadow_width.z) {
				periodic_shifts.z = l_size.z;
			} else if (a.z > l_size.z-shadow_width.z-1) {
				periodic_shifts.z = -l_size.z;
			}
			if (periodic_shifts.z!=0) {
				data[get_data_index(a+periodic_shifts)] = b;
				if (periodic_shifts.y!=0) {
					data[get_data_index(a+VINT(0,0,periodic_shifts.z))] = b;
					if (periodic_shifts.x!=0) {
						data[get_data_index(a+VINT(periodic_shifts.x,0,periodic_shifts.z))] = b;
						data[get_data_index(a+VINT(0,periodic_shifts.y,periodic_shifts.z))] = b;
						// now we need some more things to assign
					}
				}
				else if (periodic_shifts.x!=0) {
					data[get_data_index(a+VINT(0,0,periodic_shifts.z))] = b;
				}
			}
		}
		return true;
	}
	return false;
}

template <class T> 
bool Lattice_Data_Layer<T>::accessible (const VINT& a) const {
	return ( (a.z>=-shadow_width.z && a.z<l_size.z+shadow_width.z) && (a.y>=-shadow_width.y && a.y<l_size.y+shadow_width.y) && (a.x >=-shadow_width.x && a.x<l_size.x + shadow_width.x) );
}

template <class T> bool Lattice_Data_Layer<T>::writable(const VINT& a) const {
	if (using_domain) {
		VINT b(a);
		if ( !_lattice->resolve(b) )
			return false;
		return (domain[get_data_index(b)] == Boundary::none);
	}
	else
		return _lattice->inside(a);
}

template <class T> bool Lattice_Data_Layer<T>::writable_resolve(VINT& a) const {
	if (using_domain) {
		if ( ! _lattice->resolve(a) )
			return false;
		assert(accessible(a));

		return (domain[get_data_index(a)] == Boundary::none);
	}
	else {
		return _lattice->resolve(a);
		
	}
}

template <class T> bool Lattice_Data_Layer<T>::writable_resolve(VINT& a, Boundary::Type& b) const {

	Boundary::Codes code;
	if ( ! _lattice->resolve(a,code) ) {
		b = boundary_types[code];
		return false;
	}
	assert(accessible(a));
	if (using_domain) {
		b = domain[get_data_index(a)];
		return b==Boundary::none;
	}
	else  {
		return true;
	}
}

template <class T> T& Lattice_Data_Layer<T>::get_writable(VINT a) {
	if ( writable_resolve(a)) {
		assert(accessible(a));
		return data[get_data_index(a)];
	}
	assert(0);
	return shit_value;
}


template <class T> VINT Lattice_Data_Layer<T>::getWritableSize(){
	if (has_reduction) {
		switch (reduction) {
			case Boundary::mz : 
			case Boundary::pz :
				return VINT(l_size.x,l_size.y,1);
			case Boundary::mx : 
			case Boundary::px :
				return VINT(1,l_size.y,l_size.z);
			case Boundary::my : 
			case Boundary::py :
				return VINT(l_size.x,1,l_size.z);
			default:
				return size();
		}
	}
	else {
		return size();
	}
};

template <class T> valarray<T> Lattice_Data_Layer<T>::getData() const {
	valarray<size_t> sizes(3);
	sizes[0] = l_size.z;
	sizes[1] = l_size.y;
	sizes[2] = l_size.x;
	valarray<size_t> strides(3);
	strides[0] = shadow_size.x * shadow_size.y;
	strides[1] = shadow_size.x;
	strides[2] = 1;
	return data[gslice(get_data_index(VINT(0,0,0)),sizes,strides)];
}

template <class T> 
vector<VINT> Lattice_Data_Layer<T>::optimizeNeighborhood(const vector<VINT>& a) const {
	vector<VINT> t(a);
	sort(t.begin(),t.end(), less_memory_position);
	return t;
}

template <class T> 
bool Lattice_Data_Layer<T>::less_memory_position(const VINT& a, const VINT& b) {
	return a.z < b.z || ( a.z == b.z && a.y < b.y ) || ( a.z == b.z && a.y == b.y && a.x < b.x );
	
}

template <class T> 
gslice Lattice_Data_Layer<T>::xslice(span a) {
	valarray<size_t> lenghts(dimensions);
	valarray<size_t> strides(dimensions);

	uint i_dim=0;
	
	if (a.length==1 && dimensions>1 ) {
		lenghts.resize(dimensions-1);
		strides.resize(dimensions-1);
		if (dimensions>2) {
		lenghts[i_dim] = shadow_size.z;
		strides[i_dim] = shadow_size.x * shadow_size.y;
		i_dim++;
		}
		if (dimensions>1) {
			lenghts[i_dim] = shadow_size.y;
			strides[i_dim] = shadow_size.x;
			i_dim++;
		}
	} else {
		
		if (dimensions>2) {
			lenghts[i_dim] = shadow_size.z;
			strides[i_dim] = shadow_size.x * shadow_size.y;
			i_dim++;
		}
		if (dimensions>1) {
			lenghts[i_dim] = shadow_size.y;
			strides[i_dim] = shadow_size.x;
			i_dim++;
		}
		lenghts[i_dim] = a.length;
		strides[i_dim] = 1;
	}
	
	return gslice(a.low,lenghts,strides);
}

template <class T>
gslice Lattice_Data_Layer<T>::yslice(span a) {
	valarray<size_t> lenghts(dimensions-1);
	valarray<size_t> strides(dimensions-1);

	uint i_dim=0;
	if (dimensions>2) {
		lenghts[i_dim] = shadow_size.z;
		strides[i_dim] = shadow_size.x * shadow_size.y;
		i_dim++;
	}
	if (dimensions>1) {
		lenghts[i_dim] = a.length*shadow_size.x;
		strides[i_dim] = 1;
		i_dim++;
	}
	
	return gslice(a.low * shadow_size.x ,lenghts,strides);
}

template <class T>
gslice Lattice_Data_Layer<T>::zslice(span a) {
	valarray<size_t> lenghts(1);
	valarray<size_t> strides(1);

	if (dimensions>2) {
		lenghts[0]= a.length * shadow_size.x * shadow_size.y;
		strides[0] = 1;
	}
	
	return gslice(a.low * shadow_size.x * shadow_size.y ,lenghts,strides);
}


template <class T>
void Lattice_Data_Layer<T>::reset_boundaries() {
	// set no-flux boundaries to the neighboring site values
	VINT pos;
	if (using_domain && ! domain_value->isTimeConst()) {
		if (domain_value->isSpaceConst()) {
			T val = domain_value->get(VINT(0,0,0));
			for (pos.z=0; pos.z<l_size.z; pos.z++) {
				for (pos.y=0; pos.y<l_size.y; pos.y++) {
					pos.x=0;
					auto idx = get_data_index(pos);
					for (; pos.x<l_size.x; pos.x++, idx++) {
						if (domain[idx] != Boundary::none)
							data[idx] = val;
					}
				}
			}
		}
		else {
			for (pos.z=0; pos.z<l_size.z; pos.z++) {
				for (pos.y=0; pos.y<l_size.y; pos.y++) {
					pos.x=0;
					auto idx = get_data_index(pos);
					for (; pos.x<l_size.x; pos.x++, idx++) {
						if (domain[idx] != Boundary::none)
							data[idx] = domain_value->get(pos);
					}
				}
			}
		}
	}
	if (boundary_types[Boundary::mx] == Boundary::periodic) {
		data[s_xmb] = data[s_xp];
		data[s_xpb] = data[s_xm];
	}
	else {
		if (boundary_values[Boundary::mx]->isSpaceConst())
			data[s_xmb] = boundary_values[Boundary::mx]->get(VINT(0,0,0));
		else {
			if (cache_xmb.size() == 0) {
				cache_xmb.resize(shadow_width.x*shadow_size.y*shadow_size.z);
			}

			VINT start = -shadow_width;
			VINT stop = l_size+shadow_width;
			stop.x = 0;
			int idx=0; 
			for (pos.z = start.z; pos.z<stop.z; pos.z++)
				for (pos.y=start.y; pos.y<stop.y; pos.y++)
					for (pos.x=start.x; pos.x<stop.x; pos.x++) {
						cache_xmb[idx] = boundary_values[Boundary::mx]->get(pos);
						idx++;
					}
			data[s_xmb] = cache_xmb;
		}
			
		if (boundary_values[Boundary::px]->isSpaceConst())
			data[s_xpb] = boundary_values[Boundary::px]->get(VINT(0,0,0));
		else {
			if (cache_xpb.size() == 0) {
				cache_xpb.resize(shadow_width.x*shadow_size.y*shadow_size.z);
			}

			VINT start = -shadow_width;
			VINT stop = l_size+shadow_width;
			start.x = l_size.x;
			int idx=0; pos = start;
			for (pos.z = start.z; pos.z<stop.z; pos.z++)
				for (pos.y=start.y; pos.y<stop.y; pos.y++)
					for (pos.x=start.x; pos.x<stop.x; pos.x++) {
						cache_xpb[idx] = boundary_values[Boundary::px]->get(pos);
						idx++;
					}
			data[s_xpb] = cache_xpb;
		}
	}
	if (dimensions>1) {
		if (boundary_types[Boundary::my] == Boundary::periodic) {
			data[s_ymb] = data[s_yp];
			data[s_ypb] = data[s_ym];
		}
		else {
			
			if (boundary_values[Boundary::my]->isSpaceConst())
				data[s_ymb] = boundary_values[Boundary::my]->get(VINT(0,0,0));
			else {
				if (cache_ymb.size() == 0) {
					cache_ymb.resize(shadow_size.x*shadow_width.y*shadow_size.z);
				}

				VINT start = -shadow_width;
				VINT stop = l_size+shadow_width;
				stop.y = 0;
				int idx=0;
				for (pos.z = start.z; pos.z<stop.z; pos.z++)
					for (pos.y=start.y; pos.y<stop.y; pos.y++)
						for (pos.x=start.x; pos.x<stop.x; pos.x++) {
							cache_ymb[idx] = boundary_values[Boundary::my]->get(pos);
							idx++;
						}
				data[s_ymb] = cache_ymb;
			}
			
			if (boundary_values[Boundary::py]->isSpaceConst())
				data[s_ypb] = boundary_values[Boundary::py]->get(VINT(0,0,0));
			else {
				if (cache_ypb.size() == 0) {
					cache_ypb.resize(shadow_size.x*shadow_width.y*shadow_size.z);
				}

				VINT start = -shadow_width;
				VINT stop = l_size+shadow_width;
				start.y = l_size.y;
				int idx=0;
				for (pos.z = start.z; pos.z<stop.z; pos.z++)
					for (pos.y=start.y; pos.y<stop.y; pos.y++)
						for (pos.x=start.x; pos.x<stop.x; pos.x++) {
							cache_ypb[idx] = boundary_values[Boundary::py]->get(pos);
							idx++;
						}
				data[s_ypb] = cache_ypb;
			}
		}
	}
	if (dimensions>2) {
		if (boundary_types[Boundary::mz] == Boundary::periodic) {
			data[s_zmb] = data[s_zp];
			data[s_zpb] = data[s_zm];
		}
		else {
			if (boundary_values[Boundary::mz]->isSpaceConst())
				data[s_zpb] = boundary_values[Boundary::mz]->get(VINT(0,0,0));
			else {
				if (cache_zmb.size() == 0) {
					cache_zmb.resize(shadow_size.x*shadow_size.y*shadow_width.z);
				}

				VINT start = -shadow_width;
				VINT stop = l_size+shadow_width;
				stop.z = 0;
				int idx=0;
				for (pos.z = start.z; pos.z<stop.z; pos.z++)
					for (pos.y=start.y; pos.y<stop.y; pos.y++)
						for (pos.x=start.x; pos.x<stop.x; pos.x++) {
							cache_zmb[idx] = boundary_values[Boundary::mz]->get(pos);
							idx++;
						}
				data[s_zmb] = cache_zmb;
			}
			
			if (boundary_values[Boundary::pz]->isSpaceConst())
				data[s_zpb] = boundary_values[Boundary::pz]->get(VINT(0,0,0));
			else {
				
				if (cache_zpb.size() == 0) {
					cache_zpb.resize(shadow_size.x*shadow_size.y*shadow_width.z);
				}

				VINT start = -shadow_width;
				VINT stop = l_size+shadow_width;
				start.z = l_size.z;
				int idx=0;
				for (pos.z = start.z; pos.z<stop.z; pos.z++)
					for (pos.y=start.y; pos.y<stop.y; pos.y++)
						for (pos.x=start.x; pos.x<stop.x; pos.x++) {
							cache_zpb[idx] = boundary_values[Boundary::pz]->get(pos);
							idx++;
						}
				data[s_zpb] = cache_zpb;
			}
		}
	}
}

template <class T>
void Lattice_Data_Layer<T>::useBuffer(bool b) {
	if (b) {
		using_buffer = true;
		write_buffer.resize(data.size());
		write_buffer = default_value;
	}
	else {
		using_buffer = false;
		write_buffer.resize(0);
	}
}

template <class T>
bool Lattice_Data_Layer<T>::setBuffer(const VINT& pos, typename TypeInfo<T>::Parameter value)
{
	assert(using_buffer);
	if (!writable(pos) ) return false;
	write_buffer[get_data_index(pos)] = value;
	return true;
}

template <class T>
typename TypeInfo<T>::Return Lattice_Data_Layer<T>::getBuffer(const VINT& pos) const {
	assert(using_buffer); return write_buffer[get_data_index(pos)];
}

template <class T>
void Lattice_Data_Layer<T>::applyBuffer(const VINT& pos) {
	 data[get_data_index(pos)] = write_buffer[get_data_index(pos)];
}

template <class T>
void Lattice_Data_Layer<T>::copyDataToBuffer() {
	write_buffer = data;
}

template <class T>
void Lattice_Data_Layer<T>::swapBuffer()
{
	assert(using_buffer);
	swap(data, write_buffer);
	reset_boundaries();
}



#endif // LATTICE_DATA_LAYER_CPP
