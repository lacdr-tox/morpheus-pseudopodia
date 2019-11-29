/*
*/

#include "clustering_tracker.h"
#include <core/super_celltype.h>

REGISTER_PLUGIN(Clustering_Tracker);

Clustering_Tracker::Clustering_Tracker() {
	max_cluster_id=1;
	celltype_list.setXMLPath("celltype");
	registerPluginParameter(celltype_list);
}

void Clustering_Tracker::loadFromXML(const XMLNode xNode, Scope* scope)
{
	celltype_list.loadFromXML(xNode,scope);
	auto celltype_names = tokenize(celltype_list(), ", ", true);
	
	auto celltype_map = CPM::getCellTypesMap();
	for (auto  name: celltype_names) {
		CTdata ct;
		ct.celltype->read(name);
		ct.celltype->init();
		ct.cluster_id->setXMLPath("ClusterID/symbol-ref");
		registerPluginParameter(ct.cluster_id);
		ct.exclude->setXMLPath("exclude");
		ct.exclude->setDefault("0");
		registerPluginParameter(ct.exclude);
		celltypes[ct.celltype()->getID()] = ct;
		
		filename += ct.celltype()->getName() + "_";
	}
	
	AnalysisPlugin::loadFromXML(xNode, scope);
	
	// Since loadFromXML propagates the local scope we have to override the Parameter scopes afterwards individually
	for (auto& ct: celltypes) {
		ct.second.cluster_id->setScope(ct.second.celltype()->getScope());
		ct.second.exclude->setScope(ct.second.celltype()->getScope());
	}
	
	
}

void Clustering_Tracker::init(const Scope* scope)
{
	
	AnalysisPlugin::init(scope);

	filename += "clustering.dat";
	storage.open(filename.c_str(), fstream::out | fstream::trunc);
	if ( ! storage.is_open() ) { 
		cerr << "Clustering_Tracker: unable to create " << filename << endl;
		exit(-1); 
	}
	
}

bool Clustering_Tracker::touching(CPM::CELL_ID arg1,CPM::CELL_ID  arg2) {

#ifdef HAVE_SUPERCELLS
	if (is_supercelltype) {
		const vector<CPM::CELL_ID>& segments1 = static_cast<const SuperCell &>(CPM::getCell(arg1)).getSubCells();
	// 	const vector<>& segments2 = static_cast<const SuperCell &>(CPM::getCell(arg2)).getSubCells();
	// 	double critical_dist = pow (volume.get(segments1[0]), 1.0/ SIM::getLattice()->getDimensions())/1.5 + pow (volume.get(segments2[0]), 1.0/SIM::getLattice()->getDimensions())/1.5;
		for (uint s1=0; s1<segments1.size();s1++) {
			const auto& interfaces = CPM::getCell(segments1[s1]).getInterfaceLengths();
			for (auto it = interfaces.begin(); it != interfaces.end(); it++) {
				if (CPM::getCellIndex(it->first).super_cell_id ==  arg2) return true;
			}
		}
	}
	else {
#endif
		const auto& interfaces = CPM::getCell(arg1).getInterfaceLengths();
		if ( interfaces.find(arg2) != interfaces.end())
			return true;
#ifdef HAVE_SUPERCELLS
	}
#endif
	return false;
};

void Clustering_Tracker::analyse(double time)
{
	if (celltypes.size() == 0 ) return;
	
	storage.open(filename.c_str(), fstream::out |fstream::app);
	if ( ! storage.is_open() ) {
		storage.open(filename.c_str(), fstream::out | fstream::trunc);
		if ( ! storage.is_open() ) { cerr << "Clustering_Tracker: unable to create " << filename << endl; exit(-1); }
	}
	
	storage << time;
	
	//assert( ! celltype()->getCellIDs().empty() );

//  put each cell in an individual cluster
	 
	list< Cluster > clustering;
	vector<CPM::CELL_ID> cells;
	for (auto ct : celltypes) { 
		auto ids=ct.second.celltype()->getCellIDs(); cells.insert(cells.end(), ids.begin(), ids.end()); 
	}
	for ( uint i = 0;  i < cells.size(); i++) {
		auto ct_id = CellType::storage.index(cells[i]).celltype;
		if (celltypes[ct_id].exclude(cells[i]))
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
		if( celltypes.begin()->second.cluster_id->isDefined() ){
			for (uint i=0; i< cluster->cell_ids.size(); i++) {
				auto cell_focus = SymbolFocus(cluster->cell_ids[i]);
				assert(celltypes[cell_focus.cell_index().celltype].cluster_id->isDefined());
				celltypes[cell_focus.cell_index().celltype].cluster_id->set(cell_focus,cluster->id);
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

void Clustering_Tracker::finish() { storage.close(); };

