#include "super_cell.h"
#include "super_celltype.h"


SuperCell::SuperCell ( CPM::CELL_ID cell_name, SuperCT* celltype) : 
Cell ( cell_name, celltype )
{
	track_nodes = true;
	track_surface = false;
// 	super_celltype = celltype;
	
}
SuperCell::~SuperCell () {
};


// Cell::Nodes SuperCell::getNodes() const
// {
// 	Cell::Nodes nodes;
// 	for (uint i=0; i<sub_cells.size(); i++) {
// 		const Cell& sub_cell = CPM::getCell(sub_cells[i]);
// 		nodes.insert(sub_cell.getNodes().begin(),sub_cell.getNodes().end());
// 	}
// 	return nodes;
// }

CPM::CELL_ID SuperCell::addSubCell(CPM::CELL_ID cell_id) {
// 	if (segments.size()== 0) this->id_state.segment=0;
	const Cell& cell = CPM::getCell(cell_id);

	uint subcell_id = sub_cells.size();
	sub_cells.push_back(cell_id);
	sub_cell_pos[cell_id] = subcell_id;
	centers.push_back(cell.getCenter());
	updated_centers = centers;
	CellType::storage.index(cell_id).status = CPM::SUB_CELL;
	CellType::storage.index(cell_id).sub_cell_id =  subcell_id;
	CellType::storage.index(cell_id).super_cell_id = this->id;
	CellType::storage.index(cell_id).super_celltype = this->celltype->getID();
	
	if ( (cell.nNodes() != 0) && track_nodes) {
		const Cell::Nodes& new_nodes = cell.getNodes();
		cout << "Assimilating " << new_nodes.size() << " nodes of cell " << cell.getID() << endl;
		for (Cell::Nodes::const_iterator i=new_nodes.begin(); i!= new_nodes.end(); i++) {
			nodes.insert(*i);
			accumulated_nodes+=*i;
			const_cast<CPM::STATE&>( CPM::getNode(*i)).super_cell_id=id ;
		}
		if (track_surface) {
			map <CPM::CELL_ID, uint >::const_iterator ii;
			for (ii = cell.getInterfaces().begin(); ii!= cell.getInterfaces().end(); ii++) {
				// assimitlate all interfaces to cells that are not part of this supercell ...
				if (CellType::storage.index(ii->first).super_cell_id != id ) {
					interfaces[ii->first] += ii->second;
				}
			}
		}
		
	}
	return cell_id;
}

void SuperCell::removeSubCell(CPM::CELL_ID cell_id)
{
	map <CPM::CELL_ID, uint >::iterator sc_it = sub_cell_pos.find(cell_id);
// 	cout << "SuperCell::removeSubCell: removing " << cell_id << " from SuperCell " << this->id << endl;
	if (sc_it != sub_cell_pos.end() ) {
		CPM::setCellType(sub_cells[sc_it->second], CPM::getEmptyCelltypeID());
// 		CPM::getCell(  sub_cells[sc_it->second]).getCellType()->changeCellType(sub_cells[sc_it->second],CPM::getEmptyCelltype());
		sub_cells.erase(sub_cells.begin() + sc_it->second);
		centers.clear();
		sub_cell_pos.clear();
		// reconstruct local storages
		for (uint i=0; i<sub_cells.size(); i++) {
			sub_cell_pos[sub_cells[i]]=i;
			centers.push_back(CPM::getCell(sub_cells[i]).getCenter());
		}
		updated_centers = centers;
	} 
	else {
		cout << "Called removeSubCell with an ID that is not a subcell of the current cell!" << endl;
		exit(-1);
	}
}


void SuperCell::setUpdate ( const CPM::Update& update) {
// 	updated_interfaces = interfaces;
// 	updated_centers = centers;

	
	if ( ! ( nodes.size() == 1 and update.opRemove()) ) {
		if (track_nodes) {
			for (uint i=0; i<centers.size(); ++i) {
				updated_centers[i] = centers[i];
			}
// 			updated_centers =  centers;
			if (update.opAdd() && update.opRemove()) {
				updated_centers[sub_cell_pos[update.focusStateAfter().cell_id]] = CellType::storage.cell(update.focusStateAfter().cell_id).getUpdatedCenter();
				updated_centers[sub_cell_pos[update.focusStateBefore().cell_id]] = CellType::storage.cell(update.focusStateBefore().cell_id).getUpdatedCenter();
			}
			else if (update.opAdd()) {
				updated_center = SIM::getLattice() -> to_orth(  (VDOUBLE)(accumulated_nodes + update.focusStateAfter().pos) / ( nodes.size() + 1 ) );
				updated_lattice_center = ( ( accumulated_nodes + update.focusStateAfter().pos) / ( nodes.size() + 1 ) )/* % SIM::getLattice() -> size()*/;
				updated_centers[sub_cell_pos[update.focusStateAfter().cell_id]] = CellType::storage.cell(update.focusStateAfter().cell_id).getUpdatedCenter();
			}
			else if (update.opRemove()) {
				updated_center = SIM::getLattice() -> to_orth(  (VDOUBLE)(accumulated_nodes - update.focusStateBefore().pos) / ( nodes.size() - 1 ) );
				updated_lattice_center = ( ( accumulated_nodes - update.focusStateBefore().pos) / ( nodes.size() - 1 ) ) /* % SIM::getLattice() -> size()*/;
				updated_centers[sub_cell_pos[update.focusStateBefore().cell_id]] = CellType::storage.cell(update.focusStateBefore().cell_id).getUpdatedCenter();
// 				assert(nodes.find(update.focusStateBefore().pos) != nodes.end());
			}

		}

		if (track_surface) {
			// reset the interfaces
// 			resetUpdatedInterfaces();
			
			if (update.opAdd()) {
			}

			if (update.opRemove()) {
			}
		}
	} else {
		updated_center = VDOUBLE(0.0,0.0,0.0);
		updated_lattice_center = VINT(0.0,0.0,0.0);
		updated_interfaces.clear();
		updated_surface.clear();
	}
}

void SuperCell::applyUpdate ( const CPM::Update& update)
{
	//  !! Updates are already applied to the subcells !!
	//  collect changes on the cell scale
	if (track_nodes) {
		if (update.opAdd() && update.opRemove()) {
			if (update.focusStateAfter().pos != update.focusStateBefore().pos) {
				if ( ! nodes.erase(update.focusStateBefore().pos) ) {
					cerr << "SuperCell::applyUpdate : Trying to remove node ["<< update.focusStateBefore().pos << "]  which is not in the storage! " << endl;
					cerr << CPM::getNode(update.focusStateBefore().pos) << endl;
					cerr << update.focusStateBefore() << " " << celltype->getName() << endl;
					copy(nodes.begin(), nodes.end(), ostream_iterator<VINT>(cerr,"|"));
					cerr << endl;
					assert(0);
					exit(-1);
				}
				nodes.insert(update.focusStateAfter().pos);
			}
		}
		else if (update.opAdd()) {
			accumulated_nodes += update.focusStateAfter().pos;
			nodes.insert(update.focusStateAfter().pos);
			orth_center = updated_center;
			lattice_center = updated_lattice_center;
		}
		else if (update.opRemove()) {
			accumulated_nodes -= update.focusStateBefore().pos;
			if ( ! nodes.erase(update.focusStateBefore().pos) ) {
					cerr << "SuperCell::applyUpdate : Trying to remove node ["<< update.focusStateBefore().pos << "]  which is not in the storage! " << endl;
					cerr << CPM::getNode(update.focusStateBefore().pos) << endl;
					cerr << update.focusStateBefore() << " " << celltype->getName() << endl;
					copy(nodes.begin(), nodes.end(), ostream_iterator<VINT>(cerr,"|"));
					cerr << endl;
					exit(-1);
			}
			orth_center = updated_center;
			lattice_center = updated_lattice_center;
		}
		// 	centers = updated_centers;
		uint n_centers = centers.size();
		for (uint i=0; i<n_centers; ++i) {
			centers[i] = updated_centers[i];
		}
	}

	
	if (track_surface && (update.opAdd() xor update.opRemove())) {
// 		map <CPM::CELL_ID, uint >::iterator ui, i;
// 		bool brute_force_copy = false;
// 		if (updated_interfaces.size() == interfaces.size())
// 			for (ui = updated_interfaces.begin(), i=interfaces.begin(); ui != updated_interfaces.end(); ++ui) {
// 				if (ui->first == i->first) {
// 					if ( ! ui->second ) {
// 						interfaces.erase(i++);
// 					} else {
// 						i->second = ui->second;
// 						++i;
// 					}
// 				}
// 				else {
// 					brute_force_copy = true;
// 					break;
// 				}
// 			}
// 		else brute_force_copy = true;
// 		if (brute_force_copy) { /*interfaces = updated_interfaces;*/
// 			interfaces.clear();
// 			map <CPM::CELL_ID, uint >::iterator i, ui;
// 			i = interfaces.begin();
// 			for (ui = updated_interfaces.begin();  ui != updated_interfaces.end(); ui++) {
// 				if (ui->second != 0) {
// 					i=interfaces.insert(i,*ui);
// 				}
// 			}
// 		}
	}
}

XMLNode SuperCell::saveToXML ( ) const 
{
	XMLNode xCNode = XMLNode::createXMLTopNode("Cell");
	xCNode.addAttribute("name",to_cstr(id));
	
	// save properties to XMLNode
	for (uint prop=0; prop < properties.size(); prop++) {
		xCNode.addChild(properties[prop]->storeData());
	}

	// save membraneProperties to XML
	for (uint mem=0; mem < membranes.size(); mem++) {
		XMLNode node = membranes[mem]->storeData();
		node.updateName("MembranePropertyData");
		xCNode.addChild(node);
	}
	
	for ( vector<CPM::CELL_ID>::const_iterator subcell = sub_cells.begin(); subcell != sub_cells.end(); subcell++ )
	{
		xCNode.addChild("SubCell").addAttribute("cell-id", to_cstr(*subcell));
	}
	return xCNode;
}

void SuperCell::loadFromXML ( const XMLNode xNode ) 
{
	if (xNode.nChildNode("Nodes")) {
		XMLNode xSCNode = xNode.deepCopy();
		xSCNode.getChildNode("Nodes").deleteNodeContent();
		Cell::loadFromXML(xSCNode);
	}
	else 
		Cell::loadFromXML(xNode);
	
	
	// try to fill in the referenced SubCells
	uint n_subcells = xNode.nChildNode("SubCell");
	if ( ! n_subcells ) {
		cout << "Cannot find SubCells to load for SuperCell " << this->id << endl;
		return;
	}
	// remove all existing subcells, i.e. turn them into EmptyState
	while ( ! sub_cells.empty())
		removeSubCell(sub_cells.back());
	
	cout << "SuperCell::loadFromXML: loading " << n_subcells << " SubCells into SuperCell " << this->id << endl;
	for ( uint subcell=0; subcell<n_subcells;subcell++) {
		CPM::CELL_ID subcell_id;
		getXMLAttribute(xNode.getChildNode("SubCell",subcell),"cell-id",subcell_id);
		addSubCell(subcell_id);
	}
	
}

// a pretty ugly class that is used as comparison functor to sort the nodes in a container along the arbitrary axis "normal"
typedef struct  totally_unnamed {
	static VDOUBLE normal;
	bool operator () ( const VDOUBLE& lhs, const VDOUBLE& rhs ) const
	{ return (distance_plane_point(this->normal, VDOUBLE(), lhs) <  distance_plane_point(this->normal, VDOUBLE(), rhs)); }
} plane_dist_comp;
VDOUBLE plane_dist_comp::normal = VDOUBLE();


/// Splits the last segment into two equal sized segments. Uses a vector "normal_of_split_plane" to sort the nodes previous to splitting. SegmentedCell contains afterwards one more segment, but the  occupies nodes remain exactly the same.
// void SuperCell::splitLastSegment(VDOUBLE normal_of_split_plane)
// {
// 	const bool dbg=false;
// 	assert( ! sub_cells.empty() );
// 	assert( sub_cells.back() -> nNodes() >= 2);
// 	if (dbg) cout << "Cell "<< id_state.cell << ": Splitting last Segment";
// 
// 	plane_dist_comp::normal  = normal_of_split_plane;
// 	set<VINT, plane_dist_comp> occupied_nodes(sub_cells.back() -> getNodes().begin(), sub_cells.back() -> getNodes().end() );
// 
// // 	sort(occupied_nodes.begin(), occupied_nodes.end(), compare);
// 
// 	CPM::STATE seg_state = id_state;
// 	seg_state.segment = addSubCell();
// 	if (dbg) cout << ": transfering " << occupied_nodes.size() / 2 << " nodes to " << seg_state;
// 	set<VINT,plane_dist_comp>::const_iterator inode = occupied_nodes.begin();
// 	for (uint i =0; i < ((occupied_nodes.size() / 2)); i++) { inode++; }
// 	for (; inode != occupied_nodes.end(); inode++) {
// 		if (dbg) cout << ".";
// 		CPM::setNode(*inode, seg_state);
// 	}
// 	if (dbg) cout << " done\n";
// }
