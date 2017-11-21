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

#ifndef PROTRUSION_H
#define PROTRUSION_H

#include "core/interfaces.h"
#include "core/focusrange.h"
#include "core/plugin_parameter.h"

/** \defgroup Protrusion
\ingroup ML_CellType
\ingroup CPM_EnergyPlugins CellMotilityPlugins 
\brief Energetically favors updates in region of high protusive activity (actin-inspired)

Implements the 'Act model' (Niculescu et al, 2015) which extends the CPM with a local feedback mechanism resulting in cell protrusions and, as a consequence, in cell motility. The mechanism amplifies the inherent membrane fluctuations of CPM cells in a manner depending on the size of the fluctuations and their recent protrusive activity.

The protrusive activity is tracked by keeping an activity value for every lattice 
site. The empty lattice sites that form the medium have a zero activity value, 
while sites that are freshly incorporated by a cell get the maximum activity value 
(\f$ Max_{Act} \f$). The activity value of a site decreases by one after every MCS, 
until it reaches zero, creating a memory of MaxAct MCSs in which the site 
“remembers” that it was active. The combination of the memory of a site u with 
the activity in its neighborhood forms the basis for a local positive feedback 
mechanism that biases the copy attempt from the active site u to a less active
site v.

The energy difference is calculated as follows:

\f$ \Delta E(u \rightarrow v) = \frac{ \lambda_{Act}}{Max_{Act}} (GM_{Act}(u) - GM_{Act}(v)) \f$

where \f$ GM_{Act}(u) = (  \prod_{y \in N(u)} Act(y)^{1/|N(u)|} )  \f$ 

with \f$  N(u) \f$ defined as the direct 2nd order or Moore neighborhood of \f$ u \f$ that belongs to the same cell.

- \b field must refer to a \ref ML_Field that store the Activity values.
- \b strength specifies the strength \f$ \lambda_{Act} \f$ of this energy term.
- \b maxmimum specifies the maximum activity value \f$ Max_{Act} \f$, related to the length of the 'memory'.

\section Reference
- I. Niculescu, J. Textor, R. de Boer, Crawling and Gliding: A Computational Model for Shape-Driven Cell Migration, PLoS Comp Biol, 2015.
http://dx.doi.org/10.1371/journal.pcbi.1004280
\section Examples
Assuming a Field 'act' exist to store the activity values:
\verbatim
<Protrusion field="act" strength="80" maximum="200" />
\endverbatim
*/

class Protrusion : public CPM_Energy, Cell_Update_Listener, InstantaneousProcessPlugin
{
private:
	// required
	PluginParameter2<double, XMLReadWriteSymbol, RequiredPolicy> field;
	// optional
	PluginParameter2<double, XMLEvaluator, RequiredPolicy> strength;
	PluginParameter2<double, XMLEvaluator, RequiredPolicy> maxact;

	const Scope* scope;
	shared_ptr <const Lattice > lattice;
	shared_ptr <const CPM::LAYER > cpm_lattice;
	uint nbh_order;
	vector<VINT> nbh;

public:
	DECLARE_PLUGIN("Protrusion");
	Protrusion();

	double delta(const SymbolFocus& cell_focus, const CPM::Update& update) const override;
	void init(const Scope* scope) override;
	double hamiltonian(CPM::CELL_ID cell_id) const override;
	void update_notify(CPM::CELL_ID cell_id, const CPM::Update& update) override;
	double local_activity(SymbolFocus focus) const;
	void executeTimeStep() override;
	
};

#endif
