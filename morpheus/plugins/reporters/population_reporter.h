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

#ifndef POPULATIONREPORTER_H
#define POPULATIONREPORTER_H

#include "core/simulation.h"
#include "core/celltype.h"
#include "core/symbol_accessor.h"
#include "core/cell_membrane_accessor.h"
#include "core/membranemapper.h"

/* \defgroup PopulationReporter
\ingroup reporters
*/

class PopulationReporter : public TimeStepListener //Celltype_MCS_Listener
{
private:

	string input_string, input_string_vec, output_string, mapping_string;
	SymbolAccessor<double> input_symbol;
	SymbolAccessor<VDOUBLE > input_symbol_vec;
	SymbolRWAccessor<double> output_symbol;

// 	SymbolAccessor<VDOUBLE> output_symbol_vec, input_symbol_vec;
	enum{ 	MAP_SUM, 
			MAP_AVERAGE, 
			MAP_VARIANCE, 
			MAP_MIN, 
			MAP_MAX, 
			MAP_POLARISATION, 
			MAP_AVERAGE_DIRECTION, 
			MAP_VARIANCE_DIRECTION,
			MAP_POSITIONAL_REGULARITY } mapping;
	CellType* celltype;
	shared_ptr<const Lattice> lattice;
	XMLNode stored_xml;

public:
	DECLARE_PLUGIN("PopulationReporter");
    PopulationReporter();

	void init();
    void loadFromXML(const XMLNode );
	
    virtual void computeTimeStep();
    virtual void executeTimeStep();
    virtual set< string > getDependSymbols();
    virtual set< string > getOutputSymbols();
	//virtual void loadFromXML(const XMLNode );
};

#endif // POPULATIONREPORTER_H
