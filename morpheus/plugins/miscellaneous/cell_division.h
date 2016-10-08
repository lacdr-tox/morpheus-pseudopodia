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

#ifndef CELLDIVISION_H
#define CELLDIVISION_H

#include "core/interfaces.h"
#include "core/plugin_parameter.h"
#include "core/system.h" // TriggeredSystem

/** \defgroup CellDivision
\ingroup MiscellaneousPlugins
\brief Divide cell based on condition

Triggers cell division when condition is satisfied.

First, a division plane is calculated, through the cell center of mass.
- \b random: random orientation
- \b major: longest axis in elliptic approximation of cell shape
- \b minor: shortest axis in elliptic approximation of cell shape

Then, the nodes of the mother cell on either side of this plane are allocated to two new daughter cells.

By default, all property values are copied from the mother to the two daughter cells. 
This can be overridden using Triggers to set or initialize the properties of daughter cells. See example below.

To specify daughter-specific properties (to model e.g. asymmetric cell division), you can use the daughterID option.
This defines a symbolic handle (value 1 or 2) for the two daughters that can be used in the Triggers. See example below.

- \b condition: Expression describing condition under a cell should divide
- \b division-plane: Plane of division, through cell center of mass. 
  - major: longest axis in ellipsoid approximation of cell shape
  - minor: shortest axis in ellipsoid approximation of cell shape
  - random: randomly oriented division plane
  - oriented: user-specified division plane (must be given as vector in 'orientation')

- \b write_log (default none): Create log file about cell divisions in one of the following formats: CSV (Time, mother ID, daughter IDs), NEWICK (https://en.wikipedia.org/wiki/Newick_format), or DOT (https://en.wikipedia.org/wiki/DOT_(graph_description_language)) format.
- \b daughterID (optional): Local symbol that provides unique IDs (1 or 2) for the two daughter cells to be used in Triggers. E.g. to model asymmetric cell division
- \b orientation (optional): Vector (or vectorexpression) giving the division plane. Only used (and required) if division-plane="oriented".
- \b Triggers (optional): a System of Rules that are triggered for both daughter cells after cell division.

\section Examples
Divide with random orientation when cell volume doubles.
\verbatim
<CellDivision	condition="V >= (2.0 * V0)" 
				division_plane="random" />
\endverbatim

Divide every 1000 time steps along a user-specified orientation.
\verbatim
<CellDivision	condition="mod(time, 1000) == 0" 
				division_plane="oriented" 
				orientation="vector.x, vector.y, vector.z" />
\endverbatim

Using Triggers to specify properties after cell division (assymetric division). Symbol 'Vt' is here set to a daughter-specific value with 'daughterID', 
\verbatim
<CellDivision condition="V >= (2.0 * V0)" division_plane="major" daughterID="daughter">
	<Triggers>
		<Rule symbol-ref="Vt">
			<Expression>
				if( daughter == 1, 100, 50 )
			</Expression>
		</Rule>
		<Rule symbol-ref="divisions">
			<Expression>
				divisions + 1
			</Expression>
		</Rule>
	</Triggers>
</CellDivision>
\endverbatim
*/


class CellDivision : public InstantaneousProcessPlugin
{
private:
	enum logmode{ NONE, DOT, CSV, NEWICK };

	PluginParameter2<double, XMLEvaluator, RequiredPolicy> condition;
	PluginParameter2<CellType::division, XMLNamedValueReader, RequiredPolicy> division_plane;
	PluginParameter2<VDOUBLE, XMLEvaluator, OptionalPolicy> orientation;
	PluginParameter2<logmode, XMLNamedValueReader, DefaultValPolicy> write_log;

	CellType* celltype;
	
	ofstream fout; // output stream to log divisions
	vector<string> newicks;
	
	// Local symbol (inside TriggeredSystem) giving either 1 or 2
	// This enables one to distinguish between daughter cells , for asymmetric cell division
	string daughterID_symbol;
	SymbolRWAccessor<double> daughterID;
	shared_ptr<TriggeredSystem> trigger_system;
	
public:
	CellDivision();
	~CellDivision();
	DECLARE_PLUGIN("CellDivision");
	void loadFromXML (const XMLNode);
	void init(const Scope* scope);
	void executeTimeStep();
};

#endif // CELLDIVISION_H
