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
Periodically write contact length and duration of cell-cell contacts between all cell types 
\verbatim
<ContactLogger time-step="010" log-duration="True" celltype="ct1"/>
\endverbatim

\verbatim
time	cell.id	cell.type	neighbor.id	neighbor.type	length	duration
0	1	0	0	0	1.55556	0
0	2	0	0	0	1.55556	0
0	3	0	0	0	1.55556	0
...
100	1	0	19	0	8.77778	188
100	1	0	27	0	2.16667	106
100	1	0	46	0	15.2778	166
\endverbatim


Log cell-cell contacts at the end of simulation
\verbatim
<ContactLogger time-step="100" log-duration="true"/>
\endverbatim

time	cell.id	cell.type	neighbor.id	neighbor.type	length	duration
0	1	0	0	2	1.55556	0
0	2	0	0	2	1.55556	0
0	3	0	0	2	1.55556	0
...
0	51	1	0	2	1.55556	0
0	52	1	0	2	1.55556	0
0	53	1	0	2	1.55556	0
...
100	1	0	19	0	8.77778	188
100	1	0	27	0	2.16667	106
100	1	0	46	0	15.2778	166
...
*/

class ContactLogger : public AnalysisPlugin, InstantaneousProcessPlugin
{

	private:
		PluginParameterCellType<OptionalPolicy> celltype;
		ofstream fout;
		
		PluginParameter2<bool, XMLValueReader, DefaultValPolicy > ignore_medium;
		// record duration
		PluginParameter2<bool, XMLValueReader, OptionalPolicy > log_duration;
		map< std::pair< CPM::CELL_ID, CPM::CELL_ID>, double> map_contact_duration;
		
	public:
		DECLARE_PLUGIN("ContactLogger");
		ContactLogger();
		void analyse(double time);
		void loadFromXML(const XMLNode );
		void init(const Scope* scope);
 		void finish();
		void executeTimeStep();
};

#endif // CONTACTLOGGER_H
