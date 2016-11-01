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

#ifndef INITCELLLATICE_H
#define INITCELLLATICE_H

#include "core/interfaces.h"
#include "core/plugin_parameter.h"
#include "core/focusrange.h"

/** \defgroup InitCellLattice
\ingroup InitializerPlugins
\brief Initializes lattice of cell for CA-like models

Initializes a population of cells in which each lattice site is occupied with exactly 1 cell. Useful to populate CA-like models.

Consequently, each cell has area (2D) or volume (3D) \f$ v_{\sigma}=1 \f$. 

Basically a shorthand for the same functionality of InitRectangle.

\section Example
Populate a lattice with cells:
\verbatim
<InitCellLattice />
\endverbatim

Equivalent to InitRectangle for a 20x20 lattice:
\verbatim
<InitRectangle mode="regular" numberOfCells="400">
	<Dimensions size="20,20,0" origin="0.0, 0.0, 0.0"/>
</InitRectangle>
\endverbatim
*/


class InitCellLattice : public Population_Initializer
{
private:

public:
	InitCellLattice(){};
	DECLARE_PLUGIN("InitCellLattice");
	bool run(CellType* ct);

};

#endif
