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

#ifndef DISPLACEMENT_TRACKER_H
#define DISPLACEMENT_TRACKER_H

#include "core/interfaces.h"
#include "core/plugin_parameter.h"
#include "core/celltype.h"
#include <fstream>


/** 
 * \defgroup ML_DisplacementTracker Displacement Tracker
 * \ingroup ML_Analysis
 * \ingroup AnalysisPlugins
 * \brief Track the cell displacement of a population.

\section Description
Displacement_Tracker extracts the cell displacements within one cellpopulation (might be extended upon necessity). The data is stored in a text file. In addition, the mean square displacement and the total displacement of the population is calculated and stored in the first columns of the text file.

\section Example
\verbatim
	<DisplacementTracker time-step="100" celltype="CellType1" cluster-id="cluster">
\endverbatim
*/

/**
	@author Jörn Starruß 
	
*/
class DisplacementTracker : public AnalysisPlugin
{
public:
	DECLARE_PLUGIN("DisplacementTracker");
	DisplacementTracker();

	virtual void init(const Scope* scope) override;
	virtual void analyse(double time) override;

private:
	map<uint,VDOUBLE> origins;
	PluginParameterCellType<RequiredPolicy> celltype; 
	string  filename;

};

#endif
