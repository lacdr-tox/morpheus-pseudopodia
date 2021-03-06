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
#include "core/celltype.h"
#include "core/system.h" // TriggeredSystem

/** \defgroup ML_CellDivision CellDivision
\ingroup ML_CellType
\ingroup MiscellaneousPlugins InstantaneousProcessPlugins
\brief Divide cell based on condition

Triggers cell division when condition is satisfied.

First, a division plane is calculated, through the cell's center of mass (see orientation options below).
Then, the nodes of the mother cell on either side of this plane are allocated to two new daughter cells.

By default, all property values are copied from the mother to the two daughter cells. 
This can be overridden using \b Triggers to set or initialize the properties of daughter cells. See example below.

To specify daughter-specific properties (to model e.g. asymmetric cell division), you can use the daughterID option.
This defines a symbolic handle (value 1 or 2) for the two daughters that can be used in the Triggers. See example below.

- \b condition: Expression defining the condition under which a cell should divide
- \b division-plane: Plane of division, through cell's center of mass. 
  - major: longest axis in ellipsoid approximation of cell shape gives the normal vector of the division plane
  - minor: shortest axis in ellipsoid approximation of cell shape gives the normal vector of the division plane
  - random: randomly oriented division plane
  - oriented: user-specified division plane (must be given as normal vector in 'orientation')

- \b write_log (default none): Create log file about cell divisions in one of the following formats: CSV (Time, mother ID, daughter IDs), NEWICK (https://en.wikipedia.org/wiki/Newick_format), or DOT (https://en.wikipedia.org/wiki/DOT_(graph_description_language)) format.
- \b daughterID (optional): Local symbol that provides unique IDs (1 or 2) for the two daughter cells to be used in Triggers, e.g. to model asymmetric cell division.
- \b orientation (optional): Vector (or vectorexpression) giving the normal vector of the division plane. Only used (and required) if division-plane="oriented".
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

Using Triggers to specify properties after cell division (asymmetric division). Symbol 'Vt' is here set to a daughter-specific value with 'daughterID', 
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
	enum logmode{ NONE, DOT, CT_DOT, CSV, CT_CSV, NEWICK };

	PluginParameter2<double, XMLEvaluator, RequiredPolicy> condition;
	PluginParameter2<CellType::division, XMLNamedValueReader, RequiredPolicy> division_plane;
	PluginParameter2<VDOUBLE, XMLEvaluator, OptionalPolicy> orientation;
	PluginParameter2<logmode, XMLNamedValueReader, DefaultValPolicy> write_log;

	CellType* celltype;
	
	string log_file_name;
	vector<string> newicks;
	
	// Local symbol (inside TriggeredSystem) giving either 1 or 2
	// This enables one to distinguish between daughter cells , for asymmetric cell division
	string daughterID_symbol;
	SymbolRWAccessor<double> daughterID;
	shared_ptr<TriggeredSystem> trigger_system;
	
	static map<string,shared_ptr<ofstream>> log_files;
	static int instances;
	
public:
	CellDivision();
	~CellDivision();
	DECLARE_PLUGIN("CellDivision");
	void loadFromXML (const XMLNode, Scope* scop) override;
	void init(const Scope* scope) override;
	void executeTimeStep() override;
};

#endif // CELLDIVISION_H
