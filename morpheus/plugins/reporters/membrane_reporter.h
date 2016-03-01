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
// #include <core/lattice_data_layer.h>
#include "core/membranemapper.h"

using namespace SIM;

/* \defgroup MembraneReporter
\ingroup reporters
Reports the current surface of cells to the target symbol.

\verbatim
		<MembraneReporter>
			<Reporter>
				<Input>
					<Contact celltype="Medium"> | <Concentration symbol="Anything" />
				</Input>
				<Output>
					<MembraneProperty symbol=" " />  | <CellProperty symbol=" " mapping="SUM|AVERAGE|DIFFERENCE" />
				</Output>
			</Reporter>
		</MembraneReporter>
\endverbatim
*/


class MembraneReporter : public TimeStepListener
{
private:

	CellType* celltype;
	
	shared_ptr<const CPM::LAYER> cpm_layer;
	shared_ptr<const Lattice> lattice;
	vector<VINT> neighbor_sites;

	struct Reporter {
		enum  { IN_CELLTYPE, IN_PDE, IN_MEMBRANE, IN_CELLTYPE_DISTANCE, IN_CELLSHAPE } input;
		enum  { OUT_MEMBRANEPROPERTY, OUT_CELLPROPERTY } output;
		enum  { MAP_SUM, MAP_AVERAGE, MAP_DIFFERENCE, MAP_VARIANCE } mapping;
		enum  { DATA_BOOLEAN, DATA_CONTINUOUS, DATA_DISCRETE } datatype;
		
		bool deformed_sphere;
		
		// inputs
		string input_celltype_str;
		uint input_celltype_id;
		string input_symbol_str;
		bool debug_distance;
		bool debug_sources;
		SymbolAccessor<double> input_symbol;
		
		// outputs
		string output_symbol_str;
		CellMembraneAccessor output_membrane;
		SymbolRWAccessor<double> output_cellproperty;
		string output_mapping_str;
		
	};
		
	vector< Reporter > reporters;
	valarray< double > buffer;

public:
	DECLARE_PLUGIN("MembraneReporter");
    MembraneReporter();
	virtual void init();
    virtual void computeTimeStep();
    virtual void executeTimeStep();
    virtual set< string > getDependSymbols();
    virtual set< string > getOutputSymbols();
	virtual void loadFromXML(const XMLNode );
	//double angle2d(VDOUBLE v1, VDOUBLE v2) const;
};

#endif 

