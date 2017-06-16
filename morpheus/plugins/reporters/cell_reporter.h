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

/** \defgroup CellReporter
\ingroup ML_CellType
\ingroup ReporterPlugins
\brief Reports data about the spatial area covered by a cell.

\section Description

CellReporter reports data about the spatial area covered by a cell. 

A single \b Input element must be specified:
- \b value: input variable (e.g. Property, MembraneProperty or Field). May contain expression.

That information can be written to an output symbol, if necessary, mapped to a single value by means of a statistic.
Writing to a Field or MembraneProperty does not require a mapping to be specified, since their granularity is sufficient.

Multiple \b Output elements can be specified:
- \b mapping: statistic used for data mapping (if needed)
- \b symbol-ref: ouput variable (e.g. Property or MembraneProperty)

Via the \b Polarisation tag, the degree of asymmetry in the spatial distribution can be mapped into a vector of polarisation, heading into the direction of maximum information.

\section Example

Average concentration of an agent in a Field in the range of a cell.
(Assume 'A' to refer to a Field and 'B' refer to a CellProperty)
\verbatim
<CellReporter>
	<Input value="A" scaling="cell"/>
	<Output symbol-ref="B" mapping="avg" />
</CellReporter>
\endverbatim

Polarisation and variance of a membrane property
(Assume B to refer to a CellProperty, C to refer to a MembraneProperty and D to refer to a VectorProperty)
\verbatim
<CellReporter>
	<Input value="C" scaling="cell"/>
	<Output symbol-ref="B" mapping="variance" />
	<Polarisation symbol-ref="D" />
</CellReporter>
\endverbatim

Determine the binding rate of a soluble substance to a membrane bound molecule
(Assume 'A' to refer to a Field  and 'C' and 'C_r' to refer to MembraneProperties)
\verbatim
<CellReporter>
	<Input value="C*A" scaling="cell"/>
	<Output symbol-ref="C_r" />
</CellReporter>
\endverbatim
**/

#ifndef CELLREPORTER_H
#define CELLREPORTER_H

#include "core/simulation.h"
#include "core/plugin_parameter.h"
#include "core/focusrange.h"
#include "core/celltype.h"
#include "core/cell_membrane_accessor.h"
#include "core/data_mapper.h"
#include "core/membranemapper.h"

class CellReporter : public ReporterPlugin
{
private:
	CellType* celltype;
	const Scope* scope;
	
	PluginParameter<double, XMLEvaluator, RequiredPolicy> input;
	
	struct OutputSpec {
		PluginParameter<DataMapper::Mode, XMLNamedValueReader, OptionalPolicy> mapping;
		PluginParameter<double, XMLWritableSymbol> symbol;
// 		bool need_cell_surface_granularity;
// 		Granularity effective_granularity;
// 		shared_ptr<DataMapper> mapper;
// 		shared_ptr<MembraneMapper> membrane_mapper;
// 		CellMembraneAccessor membrane_acc;
	};
	
	vector< OutputSpec > outputs;
	
	void report_output(const OutputSpec& output,  const Scope* scope);
	void report_output(const OutputSpec& output, const SymbolFocus& focus);
	
	void report_polarity(const Scope* scope);
	
	PluginParameter<VDOUBLE, XMLWritableSymbol, OptionalPolicy> polarity_output;

public:
	DECLARE_PLUGIN("CellReporter");
    CellReporter();

    void init(const Scope* scope) override;
    void loadFromXML(const XMLNode ) override;
    void report() override;
};

#endif // CELLREPORTER_H
