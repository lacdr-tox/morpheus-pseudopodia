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

#ifndef MEMBRANE_CONTACT_REPORTER_H
#define MEMBRANE_CONTACT_REPORTER_H

#include "core/celltype.h"
#include "core/pluginparameter.h"
#include <core/symbol_accessor.h>
#include <core/cell_membrane_accessor.h>
#include <core/lattice_data_layer.h>
#include "core/membranemapper.h"
#include "core/function.h"
#include "core/membranemapper.h"

using namespace SIM;

/** \defgroup MembraneRules
\ingroup miscellaneous
*/


class MembraneRules : public TimeStepListener
{
private:

	CellType* celltype;
	
	shared_ptr<const CPM::LAYER> cpm_layer;
	shared_ptr<const Lattice> lattice;
	vector<VINT> neighbor_sites;
	valarray<double> theta_y;
	valarray<uint> phi_coarsening;
	bool threeDim;
	vector <uint> neighbors;

	struct Neighbor{
		VINT pos;
		double distance;
		double area;
	};
	
	struct Reporter {
		enum  { IN_CELLTYPE, IN_PDE, IN_MEMBRANE } input;
		enum  { OUT_MEMBRANEPROPERTY, OUT_CELLPROPERTY } output;
		enum  { MAP_SUM, MAP_AVERAGE, MAP_DIFFERENCE } mapping;
		enum  { CELL_SHAPE, LOCAL_NBH_SIZE, DISTANCE_TRANSFORM, GAUSSIAN_FILTER, IMPUTATION, NEIGHBORS_REPORTER, LOCAL_THRESHOLD, GLOBAL_THRESHOLD } operation;
		enum  { THR_LOCAL_MEAN, THR_LOCAL_MIDPOINT, THR_LOCAL_MAX, THR_LOCAL_MEDIAN, THR_LOCAL_STDEV } local_threshold_mode;
		enum  { THR_ABSOLUTE, THR_RELATIVE_TO_MAX, THR_RELATIVE_TO_MIN } global_threshold_mode;
		
		// inputs
		string input_celltype_str;
		uint input_celltype_id;
		vector< string> input_symbol_strs;
		vector< CellMembraneAccessor > input_membranes;
		string input_symbol_str;// only used in neighbors_reporter
		SymbolAccessor<double> input_symbol; // only used in neighbors_reporter
		
		double threshold;
		bool binary;
		int nbh_size;
		double blurring;
		bool deformed_sphere;
		
		// outputs
		string output_symbol_str;
		CellMembraneAccessor output_membrane;
		SymbolAccessor<double> output_cellproperty;
		string output_mapping_str;
		
		Function function;
		
	};
		
	Reporter reporter;
	valarray< double > buffer;

	void print_valarray( Lattice_Data_Layer<double> lattice, uint precision=2, string filename="membranerule.log");
	
	double sum(Lattice_Data_Layer<double>);
	double mean(Lattice_Data_Layer<double>);
	double min_val(Lattice_Data_Layer<double>);
	double max_val(Lattice_Data_Layer<double>);
	//shared_ptr<MembraneMapper> mapper;
	map<VINT, vector<Neighbor>, less_VINT > nbh_cache;
	vector<Neighbor> getNeighborhoodSpherical(shared_ptr<MembraneMapper> mapper, VINT pos, double distance, CPM::CELL_ID cell_id);
	double getLocalMean(VINT pos, vector<Neighbor>& nbh, uint cell_id);
	double getLocalMidpoint(VINT pos, vector<Neighbor>& nbh, uint cell_id);
	double getLocalMax(VINT pos, vector<Neighbor>& nbh, uint cell_id);
	double getLocalMedian(VINT pos, vector<Neighbor>& nbh, uint cell_id);
	double getLocalStDev(VINT pos, vector<Neighbor>& nbh, uint cell_id, double stdev);
	
		
public:
	DECLARE_PLUGIN("MembraneRules");

    MembraneRules();
	virtual void init(CellType* p);
	virtual void computeTimeStep();
	virtual void executeTimeStep();
	virtual set< string > getDependSymbols();
	virtual set< string > getOutputSymbols();

	virtual void loadFromXML(const XMLNode );
	//double angle2d(VDOUBLE v1, VDOUBLE v2) const;
	
};

#endif 

