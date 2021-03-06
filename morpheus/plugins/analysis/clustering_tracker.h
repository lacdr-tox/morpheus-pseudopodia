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

/** \defgroup ML_ClusteringTracker ClusteringTracker
\ingroup ML_Analysis
\ingroup AnalysisPlugins
\brief Identifies and writes distribution of cell clusters within a single celltype

\section Description
ClusteringTracker identifies cell clusters within a single celltype and writes the size distribution of cell clusters to a file.

A cell is considered part of a cluster if it at shares least 1 shares interface (side of a lattice node) with it.

- \b time-step: interval in which analysis is executed
- \b celltype: comma separated list of cell types for which to detect cell clusters 
- \b exclude (optional): expression determining which cells to exclude

The cluster ID of each cell can be written to a symbol (preferrably a cell property) to use in other postprocessing/visualisation steps.

- \b ClusterID/symbol-ref: symbol (e.g. \ref ML_Property) to assign the current clusterID.


\section Example
Every 100 time-steps, write cluster size distribution to file:
\verbatim
	<ClusteringTracker time-step="100" celltype="CellType1">
	</ClusteringTracker>
\endverbatim

Write cluster size distribution to file AND assign cluster ID to each cell (assuming 'clusID' is a cell Property):
\verbatim
	<ClusteringTracker time-step="100" celltype="CellType1">
		<ClusterID symbol-ref="clusID" />
	</ClusteringTracker>
\endverbatim
*/

/**
	@author Jörn Starruß 
*/

#ifndef CLUSTERING_TRACKER_H
#define CLUSTERING_TRACKER_H

#include <core/interfaces.h>
#ifdef HAVE_SUPERCELLS
#include <core/super_celltype.h>
#endif
#include <core/simulation.h>
#include <core/plugin_parameter.h>
#include <vector>
#include <algorithm>

struct Cluster { uint id; vector< uint > cell_ids;} ;
bool operator <(const Cluster& a, const Cluster& b) { return (a.cell_ids.size() < b.cell_ids.size());}; 

class Clustering_Tracker : public AnalysisPlugin
{
public:
	DECLARE_PLUGIN("ClusteringTracker");
	Clustering_Tracker();
		
	virtual void finish() override;
	virtual void analyse(double time) override;
	virtual void init(const Scope* scope) override;
	virtual void loadFromXML(const XMLNode, Scope* scope ) override;
	
private:
	fstream storage;
	string filename;
// 	PluginParameter2<double,XMLEvaluator, DefaultValPolicy> exclude;
	PluginParameter<string, XMLValueReader, RequiredPolicy> celltype_list;
	struct CTdata { 
		PluginParameterCellType_Shared<RequiredPolicy> celltype; 
		PluginParameter_Shared<double, XMLWritableSymbol, OptionalPolicy> cluster_id;  
		PluginParameter_Shared<double, XMLEvaluator, DefaultValPolicy> exclude;
	};
	map<int, CTdata > celltypes;
// 	PluginParameter2<string,XMLValueReader,OptionalPolicy> cluster_id;
	string cluster_id_name;
	list<Cluster> old_clustering;
	uint max_cluster_id;
	bool touching(CPM::CELL_ID arg1,CPM::CELL_ID  arg2);
};

#endif // CLUSTERING_TRACKER_H
