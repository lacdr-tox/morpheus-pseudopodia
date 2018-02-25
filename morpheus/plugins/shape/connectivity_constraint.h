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

#ifndef CONNECTIVITYCONSTRAINT_H
#define CONNECTIVITYCONSTRAINT_H

#include "core/interfaces.h"
#include "core/plugin_parameter.h"

/**
\defgroup ConnectivityConstraint
\ingroup ML_CellType
\ingroup CellShapePlugins
\brief Ensures cells remain connected components

Prevents updates that disrupt connectivity of cell domains. I.e. ensures that cells remain connected components.

\section Note
Currently the algorithm exclusively works in 2D lattices.

\section References
-  Merks, Roeland MH, Sergey V. Brodsky, Michael S. Goligorksy, Stuart A. Newman, and James A. Glazier. "Cell elongation is key to in silico replication of in vitro vasculogenesis and subsequent remodeling." Developmental biology 289, no. 1 (2006): 44-54.

\section Example

\verbatim
<Connectivity />
\endverbatim
*/

class ConnectivityConstraint : public Cell_Update_Checker
{
private:
	vector<VINT> neighbors;
	vector<bool> is_first_order;
	vector< vector<int> > neighbor_neighbors;
	int max_first_order;
	int dimensions;

public:
	ConnectivityConstraint();
	DECLARE_PLUGIN("ConnectivityConstraint");

    void init(const Scope* scope) override;
	bool update_check( CPM::CELL_ID cell_id, const CPM::Update& update) override;

};

#endif //CONNECTIVITYCONSTRAINT_H
