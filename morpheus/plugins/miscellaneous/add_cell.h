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

#ifndef ADDCELL_H
#define ADDCELL_H

#include "core/interfaces.h"
#include "core/celltype.h"
#include "core/system.h"

/** \defgroup ML_AddCell AddCell
\ingroup ML_CellType
\ingroup MiscellaneousPlugins InstantaneousProcessPlugins
\brief Add a new cell based on a condition.

Create a new cell during simulation, depending on a condition in a position depending on a probability distribution.

- \b Count: Expression describing how many cells to be created.
- \b Distribution: Expression describing the spatial probability distribution (normalized to 1 internally).

- \b Overwrite (default=false): Whether or not a cell should be in a location occupied by another cell.
- \b Triggers (optional): System of Rules to be triggered after cell is added.

\section Example
// Adding cell with increasing probability along x axis, automatically scheduled
\verbatim
<AddCell>
	<Count> rand_uni(0,1) &lt; 0.001 </Count>
	<Distribution> l.x / size.x </Distribution>
</AddCell>
\endverbatim

// Adding cells with normal distribution centered in middle of lattice (stdev=25), with explicit time step
\verbatim
<AddCell time-step="1.0">
	<Count> rand_uni(0,1) &lt; 0.001 </Count>
	<Distribution> exp(-((l.x-size.x/2)^2 + (l.y-size.y/2)^2) / (2*25^2) )</Distribution>
</AddCell>
\endverbatim

// Adding cells with uniform spatial distribution, setting properties of new cell with Triggers
\verbatim
<AddCell>
	<Count> p > 1 </Count>
	<Distribution> 1 </Distribution>
	<Triggers>
		<Rule symbol-ref="Vt"> 50 </Rule>
	</Triggers>
</AddCell>
\endverbatim
*/


class AddCell : public InstantaneousProcessPlugin
{
private:
	PluginParameter2<double, XMLEvaluator, RequiredPolicy> count;
	PluginParameter2<double, XMLEvaluator, RequiredPolicy> probdist;
	PluginParameter2<bool, XMLValueReader, DefaultValPolicy> overwrite;
	
	
	// variable and functions declared here
	CellType* celltype;
	shared_ptr<TriggeredSystem> triggers;
	Lattice_Data_Layer<double>* cdf;

	//shared_ptr<Function> position_function;
	//string position_symbol_string;
	//SymbolAccessor<VDOUBLE> position_symbol;

	enum Mode{
		OVERWRITE,
		EXCLUDE
	};
	Mode mode;
	bool checkIfMedium(VINT pos);

	/// Choose a lattice site at random, according to a probability density function 
	VINT getRandomPos();
	VINT getPosFromIndex(int index, VINT size);

public:
	AddCell();
	DECLARE_PLUGIN("AddCell");
	void loadFromXML( const XMLNode, Scope* scope ) override;
	void init( const Scope* scope ) override;
	void executeTimeStep() override;
};

#endif // ADDCELL_H
