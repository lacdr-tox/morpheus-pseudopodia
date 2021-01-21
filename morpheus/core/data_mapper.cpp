#include "data_mapper.h"
#include "simulation.h"


shared_ptr<DataMapper> DataMapper::create(Mode mode) {
	switch(mode) {
		case SUM: return shared_ptr<DataMapper>(new DataMapperSum());
		case AVERAGE: return shared_ptr<DataMapper>(new DataMapperAverage());
		case VARIANCE: return shared_ptr<DataMapper>(new DataMapperVariance());
		case MINIMUM: return shared_ptr<DataMapper>(new DataMapperMin());
		case MAXIMUM: return shared_ptr<DataMapper>(new DataMapperMax());
		case DISCRETE: return shared_ptr<DataMapper>(new DataMapperDiscrete());
		default:
			throw std::string("Unknown DataMapper mode");
			return shared_ptr<DataMapper>(nullptr);
	}
};

std::map< std::string, DataMapper::Mode > DataMapper::getModeNames()
{
	std::map< std::string, DataMapper::Mode > mode_map;
	mode_map["average"] = AVERAGE;
	mode_map["sum"] = SUM;
	mode_map["variance"] = VARIANCE;
	mode_map["minimum"] = MINIMUM;
	mode_map["maximum"] = MAXIMUM;
	mode_map["discrete"] = DISCRETE;
	return mode_map;
}

double DataMapperDiscrete::get(int slot) { 
	const auto& vmap = values[slot];

	if (vmap.size()==1)
		return vmap.begin()->first;
	
	int max_occ=0; int max_occ_cnt=0; double max_occ_value=0;
	for (const auto& v : vmap) {
		if (v.second>max_occ) {
			max_occ = v.second;
			max_occ_cnt = 1;
			max_occ_value= v.first;
		}
		else if (v.second == max_occ)
			max_occ_cnt++;
	}
	
	if (max_occ_cnt<2)
		return max_occ_value;
	
	int max_occ_id = std::hash<double>()( SIM::getTime()) % max_occ_cnt;
	max_occ_cnt = 0;
	for (const auto& v : vmap) {
		if (v.second==max_occ) {
			if (max_occ_cnt == max_occ_id)
				return v.first;
			max_occ_cnt++;
		}
	}
	return 0;
	
}

double DataMapperDiscrete::getCollapsed() {
	
	int max_occ=0; int max_occ_cnt=0; double max_occ_value=0;
	for (const auto vmap : values) {
		for (const auto& v : vmap) {
			if (v.second>max_occ) {
				max_occ = v.second;
				max_occ_cnt = 1;
				max_occ_value = v.first;
			}
			else if (v.second == max_occ)
				max_occ_cnt++;
		}
	}
	
	if (max_occ_cnt<2)
		return max_occ_value;
	
	int max_occ_id = std::hash<double>()( SIM::getTime()) % max_occ_cnt;
	max_occ_cnt = 0;
	for (const auto vmap : values) {
		for (const auto& v : vmap) {
			if (v.second==max_occ) {
				if (max_occ_cnt == max_occ_id)
					return v.first;
				max_occ_cnt++;
			}
		}
	}
	return 0;
}


shared_ptr<VectorDataMapper> VectorDataMapper::create(Mode mode) {
	switch(mode) {
		case SUM: return make_shared<VectorSumMapper>();
		case AVERAGE: return make_shared<VectorAverageMapper>();
		case DISCRETE: return make_shared<VectorDiscreteMapper>();
	}
}

std::map<std::string, VectorDataMapper::Mode> VectorDataMapper::getModeNames() {
	map<string, Mode> out_map;
	out_map["sum"] =  SUM;
	out_map["average"] = AVERAGE;
	out_map["discrete"] = DISCRETE;
	return out_map;
}

VDOUBLE VectorDiscreteMapper::get(int slot) { 
	const auto& vmap = values[slot];

	if (vmap.size()==1)
		return vmap.begin()->first;
	
	int max_occ=0; int max_occ_cnt=0; VDOUBLE max_occ_value;
	for (const auto& v : vmap) {
		if (v.second>max_occ) {
			max_occ = v.second;
			max_occ_cnt = 1;
			max_occ_value= v.first;
		}
		else if (v.second == max_occ)
			max_occ_cnt++;
	}
	
	if (max_occ_cnt < 2) return max_occ_value;
	// somehow the discrete mapping choices should be distinct per time and position for different mappers
	// e.g. hash( time * (pos.x+1) * (pos.y+1) (pos.z+1) ) % max ;
	int max_occ_id = std::hash<double>()( SIM::getTime() ) % max_occ_cnt;
	int id = -1;
	for (const auto& v : vmap ) {
		if (v.second == max_occ_cnt) {
			if (++id == max_occ_id) 
				return v.first;
		}
	}
	return max_occ_value;
}

VDOUBLE VectorDiscreteMapper::getCollapsed() {
	int max_occ=0; int max_occ_cnt=0; VDOUBLE max_occ_value;
	for (const auto vmap : values) {
		for (const auto& v : vmap) {
			if (v.second>max_occ) {
				max_occ = v.second;
				max_occ_cnt = 1;
				max_occ_value = v.first;
			}
			else if (v.second == max_occ)
				max_occ_cnt++;
		}
	}
	// somehow the discrete mapping choices should be distinct per time and position for different mappers
	// e.g. hash( time * (pos.x+1) * (pos.y+1) (pos.z+1) ) % max ;
	
	if (max_occ_cnt < 2) return max_occ_value;
	int max_occ_id = std::hash<double>()( SIM::getTime() ) % max_occ_cnt;
	int id = -1;
	for (const auto vmap : values) {
		for (const auto& v : vmap ) {
			if (v.second == max_occ_cnt) {
				if (++id == max_occ_id) 
					return v.first;
			}
		}
	}
	return max_occ_value;
}
