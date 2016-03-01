#include "data_mapper.h"


shared_ptr<DataMapper> DataMapper::create(Mode mode) {
	switch(mode) {
		case SUM: return shared_ptr<DataMapper>(new DataMapperSum());
		case AVERAGE: return shared_ptr<DataMapper>(new DataMapperAverage());
		case VARIANCE: return shared_ptr<DataMapper>(new DataMapperVariance());
		case MINIMUM: return shared_ptr<DataMapper>(new DataMapperMin());
		case MAXIMUM: return shared_ptr<DataMapper>(new DataMapperMax());
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
	return mode_map;
}
