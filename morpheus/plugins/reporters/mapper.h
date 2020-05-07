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

/** 
 * \defgroup ML_Mapper Mapper
\ingroup ML_Global
\ingroup ML_CellType
\ingroup ReporterPlugins

\brief Map data from a spatial context into another symbol, usually reducing spatial resolution by a mapping function.

(formerly known as CellReporter)

\section Description

A \b Mapper defines how data within a spatial context, e.g. a cell, a cell population or at global scope, can be mapped to a symbol with a different spatial granularity (i.e. resolution). Note that defining a \n Mapper within a \ref ML_CellType also restricts the mapping to the spatial range occupied by the respectiv cell population.

A single \b Input element must be specified:
- \b value: input variable (e.g. \ref ML_Property, \ref ML_MembraneProperty or \ref ML_Field) or a respective expression.

That information can be written to an output symbol, if necessary, reduced in spatial granularity by means of the \b mapping statistics.
If the output granularity is sufficient, i.e. when writing to a \ref ML_Field or \ref ML_MembraneProperty, no \b mapping function needs to be specified.

Multiple \b Output elements can be specified:
- \b mapping: statistic used for data mapping (if needed)
- \b symbol-ref: ouput symbol (referencing e.g. a \ref ML_Variable, a \ref ML_Property or a \ref ML_MembraneProperty)

Via the \b Polarisation tag, the degree of polarization in a spatial distribution of a quantity wrt. the center of the spatial range can be mapped into a vector, heading into the direction of the maximum quantity.

\section Example

Count number of cells within a celltype or globally.
(Assume 'A' refers to Variable)
\verbatim
<Mapper>
	<Input value="cell.id>0" />
	<Output symbol-ref="A" mapping="sum" />
</Mapper>
\endverbatim

Compute the sum and the variance of concentrations in a Field.
(Assume 'A' refers to a Field and  'B1' and 'B2' to global Variables)
\verbatim
<Mapper>
	<Input value="A" />
	<Output symbol-ref="B1" mapping="sum" />
	<Output symbol-ref="B2" mapping="variance" />
</Mapper>
\endverbatim

Average concentration of an agent in a Field in the range of a cell.
(Assume 'A' to refer to a Field and 'B' refer to a CellProperty)
\verbatim
<Mapper>
	<Input value="A" />
	<Output symbol-ref="B" mapping="avg" />
</Mapper>
\endverbatim

Polarisation and variance of a membrane property
(Assume B to refer to a CellProperty, C to refer to a MembraneProperty and D to refer to a VectorProperty)
\verbatim
<Mapper>
	<Input value="C" />
	<Output symbol-ref="B" mapping="variance" />
	<Polarisation symbol-ref="D" />
</Mapper>
\endverbatim

Compute the binding rate of a soluble substance (A) to a membrane bound molecule (C).
(Assume 'A' to refer to a Field  and 'C' and 'C_r' to refer to MembraneProperties)
\verbatim
<Mapper>
	<Input value="C*A"/>
	<Output symbol-ref="C_r" />
</Mapper>
\endverbatim
**/

#ifndef MAPPER_H
#define MAPPER_H

#include "core/interfaces.h"
#include "core/celltype.h"
#include "core/focusrange.h"
#include "core/data_mapper.h"
#include "core/membranemapper.h"

class Mapper : public ReporterPlugin
{
private:
	CellType* celltype;
	const Scope* scope;
	
	PluginParameter_Shared<double, XMLEvaluator, RequiredPolicy> input;
	
	struct OutputSpec {
		PluginParameter_Shared<DataMapper::Mode, XMLNamedValueReader, OptionalPolicy> mapping;
		PluginParameter_Shared<double, XMLWritableSymbol> symbol;
	};
	
	vector< OutputSpec > outputs;
	
	void report_output(const OutputSpec& output,  const Scope* scope);
	void report_output(const OutputSpec& output, const SymbolFocus& focus);
	
	void report_polarity(const Scope* scope);
	
	PluginParameter_Shared<VDOUBLE, XMLWritableSymbol, OptionalPolicy> polarity_output;

public:
	DECLARE_PLUGIN("Mapper");
    Mapper();

    void init(const Scope* scope) override;
    void loadFromXML(const XMLNode, Scope* scope ) override;
    void report() override;
};

#endif // MAPPER_H
