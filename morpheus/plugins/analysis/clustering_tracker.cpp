/*
*/

#include "clustering_tracker.h"
#include <core/super_celltype.h>

REGISTER_PLUGIN(Clustering_Tracker);

Clustering_Tracker::Clustering_Tracker() {
	max_cluster_id=1;
	exclude.setXMLPath("exclude");
	exclude.setDefault("0");
	registerPluginParameter(exclude);
	
	celltype.setXMLPath("celltype");
	registerPluginParameter(celltype);
	
	cluster_id.setXMLPath("ClusterID/symbol-ref");
	registerPluginParameter(cluster_id);
}

void Clustering_Tracker::loadFromXML(const XMLNode xNode)
{
	AnalysisPlugin::loadFromXML(xNode);
}

void Clustering_Tracker::init(const Scope* scope)
{
	celltype.init();
	shared_ptr<const SuperCT> sct = dynamic_pointer_cast<const SuperCT>(celltype());
	cluster_id.setScope(celltype()->getScope());
	
	
	AnalysisPlugin::init(scope);
	
	if ( ! sct)
		is_supercelltype = false;
	else
		is_supercelltype = true;

	filename = celltype()->getName() + "_clustering" + ".dat";
	storage.open(filename.c_str(), fstream::out | fstream::trunc);
	if ( ! storage.is_open() ) { 
		cerr << "Clustering_Tracker: unable to create " << filename << endl;
		exit(-1); 
	}
	
}

bool Clustering_Tracker::touching(CPM::CELL_ID arg1,CPM::CELL_ID  arg2) {

	if (is_supercelltype) {
		const vector<CPM::CELL_ID>& segments1 = static_cast<const SuperCell &>(CPM::getCell(arg1)).getSubCells();
	// 	const vector<>& segments2 = static_cast<const SuperCell &>(CPM::getCell(arg2)).getSubCells();
	// 	double critical_dist = pow (volume.get(segments1[0]), 1.0/ SIM::getLattice()->getDimensions())/1.5 + pow (volume.get(segments2[0]), 1.0/SIM::getLattice()->getDimensions())/1.5;
		for (uint s1=0; s1<segments1.size();s1++) {
			const map<CPM::CELL_ID,uint>& interfaces = CPM::getCell(segments1[s1]).getInterfaces();
			map<CPM::CELL_ID,uint>::const_iterator it;
			for (it = interfaces.begin(); it != interfaces.end(); it++) {
				if (CPM::getCellIndex(it->first).super_cell_id ==  arg2) return true;
			}
		}
	}
	else {
		const map<CPM::CELL_ID,uint>& interfaces = CPM::getCell(arg1).getInterfaces();
		if ( interfaces.find(arg2) != interfaces.end())
			return true;
	}
	return false;
};

void Clustering_Tracker::analyse(double time)
{
	storage.open(filename.c_str(), fstream::out |fstream::app);
	if ( ! storage.is_open() ) {
		storage.open(filename.c_str(), fstream::out | fstream::trunc);
		if ( ! storage.is_open() ) { cerr << "Clustering_Tracker: unable to create " << filename << endl; exit(-1); }
	}
	
	storage << time;
	
	//assert( ! celltype()->getCellIDs().empty() );

//  put each cell in an individual cluster
	 
	list< Cluster > clustering;
	vector<CPM::CELL_ID> cells = celltype()->getCellIDs();
	for ( uint i = 0;  i < cells.size(); i++) {
		if (exclude(cells[i]))
			continue;
		Cluster InitCluster; 
		InitCluster.id=0;
		InitCluster.cell_ids.push_back(cells[i]);
		clustering.push_back(InitCluster);
	}
// 	cout << "Created initial state" << endl;
	list< Cluster >::reverse_iterator rcluster;
	list< Cluster >::iterator cluster = clustering.begin();
// perform cluster analysis by aggregating neighboring cells
	for ( cluster = clustering.begin(); cluster != clustering.end(); cluster++) {
		for ( uint cell_in_cluster = 0;  cell_in_cluster < cluster->cell_ids.size(); cell_in_cluster++ ) 
		{
			list< Cluster >::iterator other_cluster = cluster;
			other_cluster++;
			for ( ; other_cluster != clustering.end(); other_cluster++) {
				if ( touching(cluster->cell_ids[cell_in_cluster], other_cluster->cell_ids.front()  ) )
				{
					cluster->cell_ids.push_back(other_cluster->cell_ids.front());
					list< Cluster >::iterator remove_iter = other_cluster;
					--other_cluster;
					clustering.erase(remove_iter);
				}
			}
		}
	}
// 	cout << "sorting by size" << endl;
	clustering.sort();
	for ( cluster = clustering.begin(); cluster != clustering.end(); cluster++) {
		sort(cluster->cell_ids.begin(),cluster->cell_ids.end());
	}
// 	cout << "associating former clusters" << endl;
	// associating the former cluster ids with the current
	for (list<Cluster>::reverse_iterator old_cluster = old_clustering.rbegin(); old_cluster != old_clustering.rend(); old_cluster++) {
		// find the cluster with the largest intersection that has no assigned id yet; oldclusters are sorted!
		int max_intersection = 0;
		vector<uint> intersection(old_cluster->cell_ids.size());
		list<Cluster>::reverse_iterator max_inter_cluster = clustering.rend();
		for ( rcluster = clustering.rbegin(); rcluster != clustering.rend(); rcluster++) {
			if (rcluster->id == 0 && max_intersection < rcluster->cell_ids.size()) {
				vector<uint>::iterator last_it = set_intersection( old_cluster->cell_ids.begin(), old_cluster->cell_ids.end(), rcluster->cell_ids.begin(), rcluster->cell_ids.end(), intersection.begin() );
				if ( last_it - intersection.begin() > max_intersection ) {
					max_intersection = last_it - intersection.begin();
					max_inter_cluster = rcluster;
				}
			}
		}
		if (max_inter_cluster != clustering.rend()) {
			max_inter_cluster->id = old_cluster->id;
		}
	}
// 	cout << "assigning fresh cluster ids" << endl;
	// assign new cluster ids to freshly created clusters
	for ( cluster = clustering.begin(); cluster != clustering.end(); cluster++) {
		if (cluster->id == 0) { cluster->id=max_cluster_id++; }
		// assigning them to the cells.
		if( cluster_id.isDefined() ){
			for (uint i=0; i< cluster->cell_ids.size(); i++) {
				if (is_supercelltype) {
					vector<CPM::CELL_ID> subcells = static_cast<const SuperCell&> (CPM::getCell(cluster->cell_ids[i])).getSubCells();
					for (uint j=0; j<subcells.size(); j++) {
						cluster_id.set(subcells[j],cluster->id);
					}
				}
				else {
					cluster_id.set(cluster->cell_ids[i],cluster->id);
				}
			}
		}
	}
// 	cout << "writing to disk" << endl;
	// writing CSD to disk
	for ( rcluster = clustering.rbegin(); rcluster != clustering.rend(); rcluster++) {
		storage << "\t" <<  rcluster->cell_ids.size();
	}
// 	cout << "store the current clustering" << endl;
	// keep the remainder up to date
	old_clustering.swap(clustering);
	
	storage << endl;
	storage.close();
}

void Clustering_Tracker::finish(double time) { storage.close(); };

