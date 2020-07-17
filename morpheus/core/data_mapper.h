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
#include <map>
#include <vector>
#include "random_functions.h"
/**
 * @brief Generic online Data statistics collector
 * 
 * use the creator method to generate a statistics collector of your choice.
 * Per OMP thread private containers are used. Use getCollapsed to obtain overall statistics
 */

class DataMapper {
public:
	enum Mode { AVERAGE, SUM, VARIANCE, MAXIMUM, MINIMUM, DISCRETE };
	virtual void addVal(double value, int slot=thread()) = 0;
	virtual void addValW(double value, double weight, int slot=thread()) = 0;
	virtual double get(int slot=thread()) = 0;
	virtual double getCollapsed() = 0;
	virtual void reset(int slot=thread()) = 0;
	[[deprecated]] virtual DataMapper*  clone()  =0;
	static shared_ptr<DataMapper> create(Mode mode);
	static std::map<std::string, DataMapper::Mode> getModeNames();
	DataMapper::Mode getMode() const { return mode; }
	/// Weights-are-buckets means that weight mean how often a value was observed. This has implications on the computation of min/max. 
	void setWeightsAreBuckets(bool enabled) { weightsAreBuckets = enabled; };
	virtual ~DataMapper() {}
protected:
	static inline int thread() { return omp_get_thread_num(); }
	DataMapper() :weightsAreBuckets(false) {};
	bool weightsAreBuckets;
	DataMapper::Mode mode;
};


class DataMapperSum : public DataMapper{
public:
	DataMapperSum() {
		mode = DataMapper::SUM;
		sum.resize(omp_get_max_threads(),0);
	};
	void addVal(double value, int slot=thread()) override { sum[slot]+=value; }
	void addValW(double value, double weight, int slot=thread()) override { sum[slot]+=value*weight; };
	double get(int slot=thread()) override { return sum[slot]; }
	void reset(int slot=thread()) override { sum[slot]=0;};
	DataMapper*  clone() override { return new DataMapperSum(*this); };
	double getCollapsed() override { 
		double csum=0; 
		for (auto s: sum) { csum += s; }
		return csum;
	}
private:
	vector<double> sum;
};

class DataMapperAverage : public DataMapper{
public:
	DataMapperAverage() {
		mode = DataMapper::AVERAGE;
		sum.resize(omp_get_max_threads(),0);
		count.resize(omp_get_max_threads(),0);
	};
	void addVal(double value, int slot=thread()) override {  sum[slot]+=value; count[slot]++; } 
	void addValW(double value, double weight, int slot=thread()) override {  sum[slot]+=value*weight; count[slot]+=weight; };
	double get(int slot=thread()) override { if (count[slot]<=0) return 0;  return sum[slot] / count[slot]; }
	void reset(int slot=thread()) override { sum[slot]=0; count[slot]=0; };
	DataMapper*  clone() override { return new DataMapperAverage(*this); };
	double getCollapsed() override {
		double csum=0; double ccount=0; 
		for (auto s: sum) { csum += s;}
		for (auto c: count) { ccount += c;}
		if (ccount==0) return 0;
		return csum/ccount;
	}
private: 
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
	void addVal(double value, int slot=thread()) override { sum[slot]+=value; sum_of_squares[slot]+=value*value; count[slot]++;   } 
	void addValW(double value, double weight, int slot=thread()) override { sum[slot]+=value * weight; sum_of_squares[slot]+=value*value * weight; count[slot]+=weight; }
	double get(int slot=thread()) override { if (count[slot]<=0) return 0; return (sum_of_squares[slot]  - sum[slot]*(sum[slot]/count[slot])) / count[slot]; }
	void reset(int slot=thread()) override { sum[slot]=0; sum_of_squares[slot]=0; count[slot]=0; };
	DataMapper* clone() override { return new DataMapperVariance(*this); };
	double getCollapsed() override { 
		double csum=0; double ccount=0; double csum_of_squares=0;
		for (auto s : sum) { csum += s;}
		for (auto s : sum_of_squares) { csum_of_squares += s;}
		for (auto c : count) { ccount += c;}
		if (ccount==0) return 0;
		return (csum_of_squares  - csum*(csum/ccount)) / ccount;
	}
private:
	vector<double> sum; vector<double> count; vector<double> sum_of_squares;
};

class DataMapperMin : public DataMapper{
public:
	DataMapperMin() { 
		mode = DataMapper::MINIMUM;
		min.resize(omp_get_max_threads(),std::numeric_limits< double >::max());
	};
	void addVal(double value, int slot=thread()) override { min[slot] = value<min[slot] ? value : min[slot]; } 
	void addValW(double value, double weight, int slot=thread()) override { if (this->weightsAreBuckets) min[slot] = value*weight<min[slot] ? value*weight : min[slot]; else min[slot] = value<min[slot] ? value : min[slot]; }
	double get(int slot=thread()) override { return min[slot]; }
    void reset(int slot=thread()) override { min[slot] = std::numeric_limits< double >::max(); };
	DataMapper*  clone() override { return new DataMapperMin(*this); };
	double getCollapsed() override { 
		double cmin=std::numeric_limits< double >::max();
		for (auto m: min) {
			cmin = cmin<m ? cmin : m;
		}
		return cmin;
	}
private:
	vector<double> min;
};

class DataMapperMax : public DataMapper{
public:
	DataMapperMax() { 
		mode = DataMapper::MAXIMUM;
		max.resize(omp_get_max_threads(),std::numeric_limits< double >::min());
	};
	void addVal(double value, int slot=thread()) override { max[slot] = value>max [slot]? value : max[slot]; } 
	void addValW(double value, double weight, int slot=thread()) override { if (this->weightsAreBuckets) max[slot] = value*weight>max[slot] ? value*weight : max[slot]; else  max[slot] = value>max[slot] ? value : max[slot]; } 
	double get(int slot=thread()) override { return max[slot]; }
    void reset(int slot=thread()) override { max[slot] = std::numeric_limits< double >::min(); };
	DataMapper*  clone() override { return new DataMapperMax(*this); };
	double getCollapsed() override { 
		double cmax = std::numeric_limits< double >::min();
		for (auto m : max) {
			cmax = cmax>m ? cmax : m;
		}
		return cmax;
	}
private:
	vector<double> max;
};

class DataMapperDiscrete : public DataMapper {
public:
	DataMapperDiscrete() { 
		mode = DataMapper::DISCRETE;
		values.resize(omp_get_max_threads());
	};
	void addVal(double value, int slot=thread()) override { values[slot][value]++; } 
	void addValW(double value, double weight, int slot=thread()) override { 
		if (this->weightsAreBuckets) 
			values[slot][value*weight]++;
		else 
			values[slot][value] += weight;
	}
		
	double get(int slot=thread()) override { 
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
		
// 		if (max_occ_cnt<2)
		return max_occ_value;
		
// 		int selection = getRandomUint(max_occ_cnt);
// 		max_occ_cnt = 0;
// 		for (const auto& v : vmap) {
// 			if (v.second==max_occ) {
// 				if (max_occ_cnt == selection)
// 					return v.first;
// 				max_occ_cnt++;
// 			}
// 		}
// 		return 0;
		
	}
    void reset(int slot=thread()) override { values[slot].clear(); };
	DataMapper*  clone() override { return new DataMapperDiscrete(*this); };
	double getCollapsed() override {
		
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
		
// 		if (max_occ_cnt<2)
		// first occ dominates
		return max_occ_value;
		
// 		int selection = getRandomUint(max_occ_cnt);
// 		max_occ_cnt = 0;
// 		for (const auto vmap : values) {
// 			for (const auto& v : vmap) {
// 				if (v.second==max_occ) {
// 					if (max_occ_cnt == selection)
// 						return v.first;
// 					max_occ_cnt++;
// 				}
// 			}
// 		}
// 		return 0;
	}
private:
	vector< map<double,double> >values;
};

#endif // DATAMAPPER_H
