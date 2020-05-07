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

/** \defgroup NeighborhoodReporter
\ingroup ML_Global
\ingroup ML_CellType
\ingroup ReporterPlugins

\brief Reports data about a node's or a cell's neighborhood or 'microenvironment'. 

\section Description for Global

NeighborhoodReporter reports about the adjacent Neighborhood of a node, i.e. the node's 'microenvironment' and writes it to a Field.

The neighorhood size is retrieved from the \ref ML_Lattice definition of the default neighborhood.

\section Description for CellType

NeighborhoodReporter reports about the adjacent Neighborhood of a cell, i.e. the cell's 'microenvironment', or the Neighborhood of a node. 

Information can be retrieved from all contexts within the neighborhood (i.e. Property or MembraneProperty of neighboring cells, Field concentrations) and, if necessary, mapped to a single value.

The neighorhood size is retrieved from the \ref ML_CPM definition of the ShapeSurface neighborhood (CPM/). 

\section Parameters

A single \b Input element must be specified:
- \b value: input expression (e.g. Property, MembraneProperty or Field), which is evaluated at global scope in the  whole neighborhood. The local cell's/node's scope is available under namespace 'local', e.g. a cell's own id is 'local.cell.id' while the id of any of it's neighbors is 'cell.id'.
- \b scaling: setting scaling to \b per_cell will aquire information per neighboring cell (entity), \b per_length will scale the information with the interface length, i.e. the input value is considered to
be a rate per node length.
- \b noflux-cell-medium: if true, the cell-medium interfaces are treated as no-flux boundaries. That is, at these interfaces, the value will be taken from the cell itself instead of the (empty) neighborhood.

Note, accessing the local cell's/node's properties in the input expression is directly possible through the symbol namespace 'local'.

If input is a Vector, use \ref NeighborhoodVectorReporter.

Several Output tags can be specified, each referring to an individual property of the aquired information.
If the information is written to a MembraneProperty, no mapping is required, since their granularity is sufficient.

Multiple \b Output elements can be specified:
- \b mapping: statistic used for data mapping (if needed)
- \b symbol-ref: ouput variable (e.g. Property or MembraneProperty)


\subsection Examples
Average and Variance of a value defined by an expression in surrounding cells
(Assume 'A' and 'B' refer to CellProperties)
\verbatim
<NeighborhoodReporter>
	<Input value="c+a+b" scaling="cell"/>
	<Output symbol-ref="A" mapping="avg" />
	<Output symbol-ref="B" mapping="var" />
</NeighborhoodReporter>
\endverbatim

Spatially resolved distribution of a (membrane) concentration into a CellMembraneProperty 'B'
(Consider cMemProt to be a concentration of a membrane Protein)
\verbatim
<NeighborhoodReporter>
	<Input value="min(cMemProt,local.cMemProt)" />
	<Output symbol-ref="B" />
</NeighborhoodReporter>
\endverbatim

Count the number of cells of a particular celltype (ct2) in the cell's neighborhood
(Assume 'A' refers to a CellProperty)
\verbatim
<NeighborhoodReporter>
	<Input value="cell.type == celltype.ct2" scaling="cell"/>
	<Output symbol-ref="A" mapping="sum"/>
</NeighborhoodReporter>
\endverbatim

Surface length of a whole cell population (ct1) with other cells / medium 
(Assume 'A' refers to a Variable)
\verbatim
<NeighborhoodReporter>
	<Input value="cell.type != celltype.ct1" scaling="length"/>
	<Output symbol-ref="A" mapping="sum"/>
</NeighborhoodReporter>
\endverbatim

*/

#ifndef NEIGHBORHOOD_REPORTER_H
#define NEIGHBORHOOD_REPORTER_H

#include "core/interfaces.h"
#include "core/celltype.h"
#include "core/focusrange.h"
#include "core/data_mapper.h"
#include "core/membranemapper.h"

class NeighborhoodReporter : public ReporterPlugin
{

	private:
		enum InputModes{ INTERFACES, CELLS };
		
		CellType* celltype;
		
		PluginParameter2<double, XMLThreadsaveEvaluator, RequiredPolicy> input;
		PluginParameter2<InputModes, XMLNamedValueReader, OptionalPolicy> scaling;
		PluginParameter2<bool, XMLValueReader, DefaultValPolicy> exclude_medium;

		Granularity local_ns_granularity;
		uint local_ns_id;
		bool using_local_ns;
		
		struct OutputSpec {
			PluginParameter2<DataMapper::Mode, XMLNamedValueReader, OptionalPolicy> mapping;
			shared_ptr<DataMapper> mapper;
			PluginParameter2<double, XMLWritableSymbol> symbol;
			shared_ptr<const MembranePropertySymbol> membrane_acc;
		};
		vector< shared_ptr<OutputSpec> > output;
		vector< shared_ptr<OutputSpec> > halo_output;
		vector< shared_ptr<OutputSpec> > interf_output;
		vector< shared_ptr<OutputSpec> > interf_global_output;
		
		void reportCelltype(CellType* celltype);
		void reportGlobal();

	public:
		DECLARE_PLUGIN("NeighborhoodReporter");
		NeighborhoodReporter();
	
		void init(const Scope* scope) override;
		void loadFromXML(const XMLNode, Scope* scope) override;
		void report() override;
};

#endif
