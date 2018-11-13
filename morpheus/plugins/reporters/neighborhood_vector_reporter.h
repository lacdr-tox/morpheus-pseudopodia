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

/** \defgroup NeighborhoodVectorReporter
\ingroup ML_Global
\ingroup ML_CellType
\ingroup ReporterPlugins

\brief Reports data about the cell's neighborhood or 'microenvironment' using Vector input. 

\section Description

NeighborhoodVectorReporter reports about the adjacent Neighborhood of a cell, i.e. the cell's 'microenvironment' using Vector input.

Information can be retrieved from all contexts within the neighborhood and, if necessary, mapped to a single value.

The neighorhood size is retrieved from the \ref ML_Lattice definition of the boundary neighborhood (Space/Lattice/Neighborhood). 

A single \b Input element must be specified:
- \b value: input variable (e.g. VectorProperty). May contain expression.
- \b scaling: setting scaling to \b per_cell will aquire information per neighboring cell (entity), \b per_length will scale the information with the interface length, i.e. the input value is considered to
be a rate per node length.

Accessing the local cell's properties in the input expression is not directly possible, since it is evaluated in the context of the neighborhood. Use the \b ExposeLocal tag to make local symbols (\b symbol-ref) available in the input expression (\b symbol).

If input variable is a scalar, use \ref NeighborhoodReporter.

Several Output tags can be specified, each referring to an individual property of the aquired information.
If the information is written to a MembraneProperty, no mapping is required, since their granularity is sufficient.

Multiple \b Output elements can be specified:
- \b mapping: statistic used for data mapping (if needed)
- \b symbol-ref: ouput variable of Vector type (e.g. VectorProperty)

\section Examples
(Assume 'Aa' to represent a VectorProperty)
\verbatim
<NeighborhoodVectorReporter>
	<Input value="0.1*a.x, 0.1*a.y, 0" />
	<Output symbol-ref="Aa" />
</NeighborhoodVectorReporter>
\endverbatim
*/

#ifndef NEIGHBORHOOD_VECTOR_REPORTER_H
#define NEIGHBORHOOD_VECTOR_REPORTER_H

#include "core/interfaces.h"
#include "core/celltype.h"
#include "core/focusrange.h"

class NeighborhoodVectorReporter : public ReporterPlugin
{

	private:
		CellType* celltype;
		enum InputModes{ INTERFACES, CELLS };
		enum OutputMode{ AVERAGE, SUM };
		
		PluginParameter2<VDOUBLE,XMLEvaluator> input;
		PluginParameter2<InputModes, XMLNamedValueReader,DefaultValPolicy> scaling;
		PluginParameter2<bool, XMLValueReader, DefaultValPolicy> exclude_medium;
		
		struct ExposeSpec {
			PluginParameter_Shared<double, XMLReadableSymbol> local_symbol;
			PluginParameter_Shared<string, XMLValueReader> symbol;
		};
		vector< ExposeSpec > exposed_locals;
		Granularity exposed_locals_granularity;
		
		PluginParameter2<VDOUBLE,XMLWritableSymbol, RequiredPolicy> output;
		PluginParameter2<OutputMode,XMLNamedValueReader, RequiredPolicy> output_mode;

		void reportCelltype(CellType* celltype);
		void reportGlobal();
		
	public:
		DECLARE_PLUGIN("NeighborhoodVectorReporter");
		NeighborhoodVectorReporter();
	
		void init(const Scope* scope) override;
		void loadFromXML(const XMLNode, Scope* scope) override;
		void report() override;
};

#endif
