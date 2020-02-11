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
#include "vector"
/**
 * @brief Generic online Data statistics collector
 * 
 * use the creator method to generate a statistics collector of your choice.
 * Per OMP thread private containers are used. Use getCollapsed to obtain overall statistics
 */

class DataMapper {
public:
	enum Mode { AVERAGE, SUM, VARIANCE, MAXIMUM, MINIMUM };
	virtual void addVal(double value) = 0;
	virtual void addVal(double value, double weight) = 0;
	virtual double get() = 0;
	virtual double getCollapsed() = 0;
	virtual void reset() = 0;
	[[deprecated]] virtual DataMapper*  clone()  =0;
	static shared_ptr<DataMapper> create(Mode mode);
	static std::map<std::string, DataMapper::Mode> getModeNames();
	DataMapper::Mode getMode() const { return mode; }
	/// Weights-are-buckets means that weight mean how often a value was observed. This has implications on the computation of min/max. 
	void setWeightsAreBuckets(bool enabled) { weightsAreBuckets = enabled; };
	virtual ~DataMapper() {}
protected:
	DataMapper() :weightsAreBuckets(false) {};
	bool weightsAreBuckets;
	DataMapper::Mode mode;
};


class DataMapperSum : public DataMapper{
public:
	DataMapperSum() { reset(); mode = DataMapper::SUM; sum.resize(omp_get_max_threads(),0); };
	void addVal(double value) override { auto t=thread(); sum[t]+=value; }
	void addVal(double value, double weight) override {  auto t=thread(); sum[t]+=value*weight; };
	double get() override { auto t=thread(); return sum[t]; }
	void reset() override { auto t=thread(); sum[t]=0;};
	DataMapper*  clone() override { return new DataMapperSum(*this); };
	double getCollapsed() override { 
		double csum=0; 
		for (auto s: sum) { csum += s; }
		return csum;
	}
private:
	inline int thread() { return omp_get_thread_num(); }
	vector<double> sum;
};

class DataMapperAverage : public DataMapper{
public:
	DataMapperAverage() {
		mode = DataMapper::AVERAGE;
		sum.resize(omp_get_max_threads(),0);
		count.resize(omp_get_max_threads(),0);
	};
	void addVal(double value) override {  auto t=thread(); sum[t]+=value; count[t]++; } 
	void addVal(double value, double weight) override {  auto t=thread(); sum[t]+=value*weight; count[t]+=weight; };
	double get() override { auto t=thread(); if (count[t]<=0) return 0;  return sum[t] / count[t]; }
	void reset() override { auto t=thread(); sum[t]=0; count[t]=0; };
	DataMapper*  clone() override { return new DataMapperAverage(*this); };
	double getCollapsed() override { 
		double csum=0; double ccount=0; 
		for (auto s: sum) { csum += s;}
		for (auto c: count) { ccount += c;}
		return csum/ccount;
	}
private: 
	inline int thread() { return omp_get_thread_num(); }
	vector<double> sum; vector<double> count;
};

class DataMapperVariance : public DataMapper{
public:
	DataMapperVariance() {
		mode = DataMapper::VARIANCE;
		sum.resize(omp_get_max_threads(),0);
		count.resize(omp_get_max_threads(),0);
		sum_of_squares.resize(omp_get_max_threads(),0);
	};
	void addVal(double value) override { auto t=thread(); sum[t]+=value; sum_of_squares[t]+=value*value; count[t]++;   } 
	void addVal(double value, double weight) override { auto t=thread(); sum[t]+=value * weight; sum_of_squares[t]+=value*value * weight; count[t]+=weight; }
	double get() override { auto t=thread(); if (count[t]<=0) return 0; return (sum_of_squares[t]  - sum[t]*(sum[t]/count[t])) / count[t]; }
	void reset() override { auto t=thread(); sum[t]=0; sum_of_squares[t]=0; count[t]=0; };
	DataMapper* clone() override { return new DataMapperVariance(*this); };
	double getCollapsed() override { 
		double csum=0; double ccount=0; double csum_of_squares=0;
		for (auto s : sum) { csum += s;}
		for (auto s : sum_of_squares) { csum_of_squares += s;}
		for (auto c : count) { ccount += c;}
		return (csum_of_squares  - csum*(csum/ccount)) / ccount;
	}
private:
	inline int thread() { return omp_get_thread_num(); }
	vector<double> sum; vector<double> count; vector<double> sum_of_squares;
};

class DataMapperMin : public DataMapper{
public:
	DataMapperMin() { 
		mode = DataMapper::MINIMUM;
		min.resize(omp_get_max_threads(),std::numeric_limits< double >::max());
	};
	void addVal(double value) override { auto t=thread(); min[t] = value<min[t] ? value : min[t]; } 
	void addVal(double value, double weight) override { auto t=thread(); if (this->weightsAreBuckets) min[t] = value*weight<min[t] ? value*weight : min[t]; else min[t] = value<min[t] ? value : min[t]; }
	double get() override { auto t=thread(); return min[t]; }
    void reset() override { auto t=thread(); min[t] = std::numeric_limits< double >::max(); };
	DataMapper*  clone() override { return new DataMapperMin(*this); };
	double getCollapsed() override { 
		double cmin=std::numeric_limits< double >::max();
		for (auto m: min) {
			cmin = cmin<m ? cmin : m;
		}
		return cmin;
	}
private:
	inline int thread() { return omp_get_thread_num(); }
	vector<double> min;
};

class DataMapperMax : public DataMapper{
public:
	DataMapperMax() { 
		mode = DataMapper::MAXIMUM;
		max.resize(omp_get_max_threads(),std::numeric_limits< double >::min());
	};
	void addVal(double value) override { auto t=thread(); max[t] = value>max [t]? value : max[t]; } 
	void addVal(double value, double weight) override { auto t=thread(); if (this->weightsAreBuckets) max[t] = value*weight>max[t] ? value*weight : max[t]; else  max[t] = value>max[t] ? value : max[t]; } 
	double get() override { auto t=thread(); return max[t]; }
    void reset() override { auto t=thread(); max[t] = std::numeric_limits< double >::min(); };
	DataMapper*  clone() override { return new DataMapperMax(*this); };
	double getCollapsed() override { 
		double cmax = std::numeric_limits< double >::min();
		for (auto m : max) {
			cmax = cmax>m ? cmax : m;
		}
		return cmax;
	}
private:
	inline int thread() { return omp_get_thread_num(); }
	vector<double> max;
};

#endif // DATAMAPPER_H
