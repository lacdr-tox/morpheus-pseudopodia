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

#ifndef CONTACTLOGGER_H
#define CONTACTLOGGER_H

#include <core/interfaces.h>
#include "core/plugin_parameter.h"

/** 
\defgroup ContactLogger ContactLogger
\ingroup ML_Analysis
\ingroup AnalysisPlugin
\brief Write the contacts and contact lengths between cells to file

\section Description
ContactLogger write the cell-cell contacts, their contact lengths/areas and, optionally their duration, to file.

If no celltypes are specified, reports on cell-cell contacts between all celltypes. This includes the medium if ignore-medium is false.


\section Input
- \b celltype-from: Only report on contacts of cells of this celltype.
- \b celltype-to: Only report on contacts between this celltype and celltype-from (requires specification of celltype-from).
- \b ignore-medium: Do not report on contact to medium (default=true). 
- \b log-duration: Report on the duration (in time) of each cell-cell contact. Note that keeping track of this contact duration invokes a computational penalty.

\section Examples
Periodically write all contact length of cell-cell contacts between all cell types (excluding medium)
\verbatim
<ContactLogger time-step="100"/>
\endverbatim

Log all contacts of ct1 (including medium)
\verbatim
<ContactLogger time-step="100" celltype-from="ct1" ignore-medium="false"/>
\endverbatim

Log all contacts between ct1 and ct2 (ignoring medium)
\verbatim
<ContactLogger time-step="100" celltype-from="ct1" celltype-to="ct2" log-duration="true"/>
\endverbatim


*/

class ContactLogger : public AnalysisPlugin, InstantaneousProcessPlugin
{

	private:
		PluginParameterCellType<OptionalPolicy> celltype_from;
		PluginParameterCellType<OptionalPolicy> celltype_to;
		ofstream fout;
		
		PluginParameter2<bool, XMLValueReader, DefaultValPolicy > ignore_medium;
		// record duration
		PluginParameter2<bool, XMLValueReader, OptionalPolicy > log_duration;
		map< std::pair< CPM::CELL_ID, CPM::CELL_ID>, double> map_contact_duration;
		
	public:
		DECLARE_PLUGIN("ContactLogger");
		ContactLogger();
		void analyse(double time) override;
		void loadFromXML(const XMLNode ) override;
		void init(const Scope* scope) override;
 		void finish() override;
		void executeTimeStep() override;
};

#endif // CONTACTLOGGER_H
