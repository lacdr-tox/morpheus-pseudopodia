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

/** \ingroup ContactLogger
 *  \defgroup ContactLogger ContactLogger
 *  \brief Write the contacts and contact lengths between cells to file
 * 
\section Description
ContactLogger write the cell-cell contacts and their contact lengths/areas to file.

Example of format:

	Cell	Neighbor	Contact
	1	2	160
	1	21	209
	1	28	199
	1	57	194
	1	74	131
	1	99	169

	2	1	160
	2	3	160
	2	57	191
	2	58	196
	2	99	184
	2	100	189

	3	2	160
	3	4	159
	3	58	194
	3	59	200
	3	100	179
	3	101	184

\section Input
- celltype (required)

TODO: 
- configurable separator
- optionally add cell positions

\section Examples
Log cell-cell contacts at the end of simulation
\verbatim
<ContactLogger time-step="-1" celltype="cells"/>
\endverbatim

*/

class ContactLogger : public AnalysisPlugin
{
	private:
		PluginParameterCellType<RequiredPolicy> celltype;
		ofstream fout;
	public:
		DECLARE_PLUGIN("ContactLogger");
		ContactLogger();
		void analyse(double time);
		void loadFromXML(const XMLNode );
		void init(const Scope* scope);
 		void finish();
};

#endif // CONTACTLOGGER_H
