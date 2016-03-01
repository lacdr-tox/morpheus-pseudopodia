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

/** \defgroup CellTracker
\ingroup AnalysisPlugins
\brief Writes cell tracks in XML format

\section Description
Writes cell tracks in XML format, as used in ISBI 2012 Particle Tracking Challenge.

These XML files can be read using the Icy plugin "ISBI Challenge Tracks Importer" (which requires the Icy "TrackManager" plugin).
Subsequently, the cell tracks can be compared to results from automatic cell tracking algorithms using the Icy plugin "Tracking Performance Measures".

- \b time-step: interval in which cell positions are recorded.

Note: The XML file is only written at the END of simulation (as specified in Time/StopTime).

\section Reference

- Chenouard et al., Objective comparison of particle tracking methods, Nature Methods, 2014.
http://dx.doi.org/10.1038/nmeth.2808

\section Links

- Icy platform for bioimage informatics: http://icy.bioimageanalysis.org
- Icy plugin "ISBI Challenge Tracks Importer": http://icy.bioimageanalysis.org/plugin/ISBI_Challenge_Tracks_Importer
- Icy plugin "Tracking Performance Measures": http://icy.bioimageanalysis.org/plugin/Tracking_Performance_Measures
- Description of Performance Measure: http://bioimageanalysis.org/track/PerformanceMeasures.pdf

\section Example
Write cell tracks to XML file, using intervals of 50 time-steps.
\verbatim
<CellTracker time-step="50">
</CellTracker>
\endverbatim

Note: The XML file is only written at the END of simulation (as specified in Time/StopTime).

*/

/**
    @author Walter de Back
*/

#ifndef CELLTRACKER_H
#define CELLTRACKER_H

#include <core/interfaces.h>
#include <core/super_celltype.h>
#include <core/simulation.h>
#include <core/plugin_parameter.h>
#include <vector>
#include <algorithm>

struct Spot {
    uint frame;
    VDOUBLE position;
};

class CellTracker : public AnalysisPlugin
{

    enum Format{ ISBI2012, MTrackJ };
public:
    DECLARE_PLUGIN("CellTracker");
    CellTracker();

    virtual void loadFromXML(const XMLNode );
    virtual void init(const Scope* scope);
    /// record cell positions
    virtual void analyse(double time);
    /// write cell tracks to XML file
    virtual void finish();

private:
    XMLNode stored_node;
    uint frame;
    map<CPM::CELL_ID, vector<Spot> > tracks;
    PluginParameter2<Format, XMLNamedValueReader, RequiredPolicy> format;

    void write_ISBI_XML(void);
//	PluginParameterCellType<RequiredPolicy> celltype;
};

#endif // CELL_TRACKER_H
