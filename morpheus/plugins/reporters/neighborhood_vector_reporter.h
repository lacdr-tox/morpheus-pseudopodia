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

\section Description for ML_Global

NeighborhoodVectorReporter reports about the adjacent Neighborhood of a node, i.e. the node's 'microenvironment' and writes it to a Field.

The neighorhood size is retrieved from the \ref ML_Lattice definition of the default neighborhood.

\section Description for CellType

NeighborhoodVectorReporter reports about the adjacent Neighborhood of a cell, i.e. the cell's 'microenvironment' using Vector input.

Information can be retrieved from all contexts within the neighborhood and, if necessary, mapped to a single value.

The neighorhood size is retrieved from the \ref ML_CPM definition of the ShapeSurface neighborhood (CPM/). 

\section Parameters

A single \b Input element must be specified:
- \b value: input expression (e.g. VectorProperty), which is evaluated at global scope in the whole neighborhood. The local cell's/node's scope is available under namespace 'local', e.g. a cell's own id is 'local.cell.id' while the id of any of it's neighbors is 'cell.id'.
- \b scaling: setting scaling to \b per_cell will aquire information per neighboring cell (entity), \b per_length will scale the information with the interface length, i.e. the input value is considered to
be a rate per node length.

Note, accessing the local cell's/node's properties in the input expression is directly possible through the symbol namespace 'local'.


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
#include "core/data_mapper.h"

class NeighborhoodVectorReporter : public ReporterPlugin
{

	private:
		CellType* celltype;
		enum InputModes{ INTERFACES, CELLS };
		
		PluginParameter2<VDOUBLE,XMLEvaluator> input;
		PluginParameter2<InputModes, XMLNamedValueReader,DefaultValPolicy> scaling;
		PluginParameter2<bool, XMLValueReader, DefaultValPolicy> exclude_medium;

		Granularity local_ns_granularity;
		uint local_ns_id;
		bool using_local_ns;
		
		
		PluginParameter2<VDOUBLE,XMLWritableSymbol, RequiredPolicy> output;
		PluginParameter2<VectorDataMapper::Mode, XMLNamedValueReader, RequiredPolicy> output_mode;

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
