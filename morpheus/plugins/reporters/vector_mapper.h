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
 * \defgroup ML_VectorMapper VectorMapper
\ingroup ML_Global
\ingroup ML_CellType
\ingroup ReporterPlugins

\brief Map vector data from a spatial context into another symbol, usually reducing spatial resolution by a mapping function.

\section Description

A \b VectorMapper defines how data within a spatial context, e.g. a cell, a cell population or at global scope, can be mapped to a symbol with a different spatial granularity (i.e. resolution). Note that defining a \n Mapper within a \ref ML_CellType also restricts the mapping to the spatial range occupied by the respective cell population.

A single \b Input element must be specified:
- \b value: input variable (e.g. \ref ML_PropertyVector or \ref ML_VectorField) or a respective expression.

That information can be written to an output symbol, if necessary, reduced in spatial granularity by means of the \b mapping statistics.
If the output granularity is sufficient, i.e. when writing to a \ref ML_Field, no \b mapping function needs to be specified.

Multiple \b Output elements can be specified:
- \b mapping: statistic used for data mapping (if needed)
- \b symbol-ref: ouput symbol (referencing e.g. a \ref ML_VariableVector or a \ref ML_PropertyVector

\section Example

Compute the average velocity of a cell population.
(Assume 'vel' refers to a \ref ML_PropertyVector containing the cell velocity (\ref MotilityReporter) and  'avg_vel' global \ref ML_VariableVector)
\verbatim
<VectorMapper>
	<Input value="vel" />
	<Output symbol-ref="avg_vel" mapping="average" />
</VectorMapper>
\endverbatim

**/

#ifndef MAPPER_H
#define MAPPER_H

#include "core/interfaces.h"
#include "core/celltype.h"
#include "core/focusrange.h"
#include "core/data_mapper.h"

class VectorMapper : public ReporterPlugin
{
private:
	CellType* celltype;
	const Scope* scope;
	
	PluginParameter_Shared<VDOUBLE, XMLEvaluator, RequiredPolicy> input;
	
	struct OutputSpec {
		PluginParameter_Shared<VectorDataMapper::Mode, XMLNamedValueReader, OptionalPolicy> mapping;
		PluginParameter_Shared<VDOUBLE, XMLWritableSymbol> symbol;
	};
	
	vector< OutputSpec > outputs;
	
	void report_output(const OutputSpec& output,  const Scope* scope);
	void report_output(const OutputSpec& output, const SymbolFocus& focus);

public:
	DECLARE_PLUGIN("VectorMapper");
    VectorMapper();

    void init(const Scope* scope) override;
    void loadFromXML(const XMLNode, Scope* scope ) override;
    void report() override;
};

#endif // MAPPER_H
