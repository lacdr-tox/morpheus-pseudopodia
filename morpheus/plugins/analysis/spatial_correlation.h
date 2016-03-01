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

#ifndef SPATIALCORRELATION_H
#define SPATIALCORRELATION_H

#include "core/interfaces.h"
#include "core/celltype.h"
#include "core/symbol_accessor.h"
#include <fstream>
//#include <math.h> // isnan



/**
	@author Walter de Back
	
*/
class SpatialCorrelation : public Analysis_Listener
{
public:
	DECLARE_PLUGIN("SpatialCorrelation");

	virtual void loadFromXML (const XMLNode);
    virtual set< string > getDependSymbols();

	virtual void init(double time);
	virtual void notify(double time);
	virtual void finish(double time);


private:

	struct{
		CPM::CELL_ID me;
		CPM::CELL_ID other;
		double distance;
		double property;
	} Link;

	string celltype_name;
	string symbolref_str;
	string mode_str;
	bool mode_classes;
	
	shared_ptr<const CellType> celltype;
	shared_ptr<const Lattice> lattice;
	ofstream storage;
	SymbolAccessor<double> property;
	int num_classes;
	double binsize;
	int lattice_max_size;
	
	double compute_C( bool mode_classes, double X_ave, double max_distance = 0.0);
};

#endif
