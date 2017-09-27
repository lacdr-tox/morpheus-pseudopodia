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

/** \defgroup MotilityReporter
\ingroup ML_CellType
\ingroup ReporterPlugins
\brief Reports statistics about cell motility.

\section Description
MotilityReporter reports statistics about the cell motility. 

- \b Velocity: estimates cell velocity within time intervals determined by time-step. 
- \b Displacement: measures the displacement of a cell relative to it's original position can be determined.

\section Example
Report every 10 [simulation time] the velocity and displacement of cells into properties A and B.
(Assume both 'A' and 'B' refer to a VectorProperty)
\verbatim
<MotilityReporter time-step="10.0">
	<Velocity symbol-ref="A" />
	<Displacement symbol-ref="B" />
</MotilityReporter>
\endverbatim
*/

#ifndef MOTILITYREPORTER_H
#define MOTILITYREPORTER_H

#include "core/interfaces.h"
#include "core/celltype.h"
#include "core/plugin_parameter.h"

class MotilityReporter : public ReporterPlugin
{

public:
	DECLARE_PLUGIN("MotilityReporter");
    MotilityReporter();
    virtual void loadFromXML ( const XMLNode ) override;
    virtual void init (const Scope* scope) override;
    virtual void report() override;
	
private:
	const CellType* celltype;
	PluginParameter2<double,XMLValueReader, RequiredPolicy> interval;
	PluginParameter2<VDOUBLE, XMLWritableSymbol, OptionalPolicy> velocity;
	PluginParameter2<VDOUBLE, XMLWritableSymbol, OptionalPolicy> displacement;
	
	map<uint,VDOUBLE> origin;
	map<uint,VDOUBLE> position;
	
};

#endif // MOTILITYREPORTER_H
