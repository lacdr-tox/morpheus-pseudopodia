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
\brief Add new cells based during simulation.

Create new cells during simulation at positions given by a probability distribution. AddCell runs every MCS and places cells given by \b Count. Fractional counts are realized statistically.

- \b Count: Expression describing how many cells to be created. Fractional counts are realized statistically.
- \b Distribution: Expression describing the spatial probability distribution (normalized to 1 internally).
- \b overwrite (default=false): Whether or not a cell should be created in a location occupied by another cell.

- \b Triggers (optional): System of Rules to be triggered after cell is added.

\section Example
// Adding cell every MCS with increasing probability along x axis, automatically scheduled
\verbatim
<AddCell>
	<Count> 1 </Count>
	<Distribution> l.x / size.x </Distribution>
</AddCell>
\endverbatim

// Adding cells with normal distribution centered in middle of lattice (stdev=25), with explicit time step
// Fractional count 0.01 is realized such that on average the every 100 MCS a cell is created. 
\verbatim
<AddCell>
	<Count> 0.01 </Count>
	<Distribution> exp(-((l.x-size.x/2)^2 + (l.y-size.y/2)^2) / (2*25^2) )</Distribution>
</AddCell>
\endverbatim

// Adding cells with uniform spatial distribution, setting properties of new cell with Triggers
\verbatim
<AddCell>
	<Count> p > 1 </Count>
	<Distribution> 1 </Distribution>
	<Triggers>
		<Rule symbol-ref="birth_time"> time </Rule>
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
	struct cdf_data { double val; VINT pos; };
	valarray<cdf_data> cdf;

	void createCDF();
	/// Choose a lattice site at random, according to a probability density function 
	VINT getRandomPos();

public:
	AddCell();
	DECLARE_PLUGIN("AddCell");
	void loadFromXML( const XMLNode, Scope* scope ) override;
	void init( const Scope* scope ) override;
	void executeTimeStep() override;
};

#endif // ADDCELL_H
