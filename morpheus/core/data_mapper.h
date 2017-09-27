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

#ifndef DATAMAPPER_H
#define DATAMAPPER_H

#include "vec.h"
#include "map"

class DataMapper {
public:
	enum Mode { AVERAGE, SUM, VARIANCE, MAXIMUM, MINIMUM };
	virtual void addVal(double value) = 0;
	virtual void addVal(double value, double weight) = 0;
	virtual double get() = 0;
	virtual void reset() = 0;
	static shared_ptr<DataMapper> create(Mode mode);
	static std::map<std::string, DataMapper::Mode> getModeNames();
	DataMapper::Mode getMode() const { return mode; }
	void setWeightsAreBuckets(bool enabled) { weightsAreBuckets = enabled; };
protected:
	DataMapper() :weightsAreBuckets(false) {};
	bool weightsAreBuckets;
private:
	Mode mode;
};


class DataMapperSum : public DataMapper{
public:
	DataMapperSum() { reset(); };
	void addVal(double value) { sum+=value; }
	void addVal(double value, double weight) { sum+=value*weight; };
	double get() { return sum; }
	void reset() {sum=0;};
private: 
	double sum;
};

class DataMapperAverage : public DataMapper{
public:
	DataMapperAverage() { reset(); };
	void addVal(double value) { sum+=value; count++; } 
	void addVal(double value, double weight) { sum+=value*weight; count+=weight; };
	double get() { if (count<=0) return 0;  return sum / count; }
	void reset() {sum=0; count=0; };
private: 
	double sum; double count;
};

class DataMapperVariance : public DataMapper{
public:
	DataMapperVariance() { reset(); };
	void addVal(double value) { sum+=value; sum_of_squares+=value*value; count++;   } 
	void addVal(double value, double weight) { sum+=value * weight; sum_of_squares+=value*value * weight; count+=weight; }
	double get() { if (count<=0) return 0; return (sum_of_squares  - sum*(sum/count)) / count; }
	void reset() { sum=0; sum_of_squares=0; count=0; };
private: 
	double sum; double sum_of_squares; double count;
};

class DataMapperMin : public DataMapper{
public:
	DataMapperMin() { reset(); };
	void addVal(double value) { min = value<min ? value : min; } 
	void addVal(double value, double weight) { if (weightsAreBuckets) min = value*weight<min ? value*weight : min; else min = value<min ? value : min; }
	double get() { return min; }
    void reset() { min = std::numeric_limits< double >::max(); };
private: 
	double min;
};

class DataMapperMax : public DataMapper{
public:
	DataMapperMax() { reset(); };
	void addVal(double value) { max = value>max ? value : max; } 
	void addVal(double value, double weight) { if (weightsAreBuckets) max = value*weight>max ? value*weight : max; else  max = value>max ? value : max; } 
	double get() { return max; }
    void reset() { max = std::numeric_limits< double >::min(); };
private: 
	double max;
};

#endif // DATAMAPPER_H
