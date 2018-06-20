#include "focusrange.h"
#include "celltype.h"
#include "membrane_property.h"

FocusRangeIterator::FocusRangeIterator(shared_ptr<const FocusRangeDescriptor> data, uint index) : data(data) {
	setIndex(index);
};

void FocusRangeIterator::setIndex(int index) {
	if (!data) {
		if (idx!=0)
            throw std::out_of_range("out of FocusRange");
		return;
	}
	
    if ( index<0 || index>=data->size ) {
//         throw std::out_of_range("out of FocusRange");
		idx = data->size;
		focus.unset();
		return;
	}
	
	idx = index;
	switch (data->iter_mode) {
		case FocusRangeDescriptor::IT_CellNodes :
// 		case FocusRangeDescriptor::IT_CellSurfaceNodes : 
		{
			if (idx==0) {
				cell = 0;
				current_cell_node = data->cell_nodes[cell]->begin();
				pos = *current_cell_node;
				focus.setCell(data->cell_range[0],pos);
			}
			else {
				//find the cell the index belongs to
				uint fcs_left = idx;
				for (cell=0; 1; cell++) {
					if (data->cell_sizes[cell] > fcs_left) {
						current_cell_node = data->cell_nodes[cell]->begin();
						while (fcs_left>0)  { current_cell_node++; fcs_left--; }
						pos = *current_cell_node;
						focus.setCell(data->cell_range[cell], pos);
						break;
					}
					else {
						fcs_left-=data->cell_sizes[cell];
						if ( (cell+1)>=data->cell_sizes.size() ) {
							// throw string("FocusRange: Index aut of range");
							// don't throw on out of range index
							// just set beyond end state !! 
							focus.unset();
							break;
						}
					}
				}
			}
			break;
		}
		case FocusRangeDescriptor::IT_CellNodes_int:
		{
			if (idx==0) {
				cell = 0;
				current_cell_node = data->cell_nodes_int[cell].begin();
				pos = *current_cell_node;
				focus.setCell(data->cell_range[0],pos);
			}
			else {
				//find the cell the index belongs to
				uint fcs_left = idx;
				for (cell=0; 1; cell++) {
					if (data->cell_sizes[cell] > fcs_left) {
						current_cell_node = data->cell_nodes_int[cell].begin();
						while (fcs_left>0)  { current_cell_node++; fcs_left--; }
						pos = *current_cell_node;
						focus.setCell(data->cell_range[cell], pos);
						break;
					}
					else {
						fcs_left-=data->cell_sizes[cell];
						if ( (cell+1)>=data->cell_sizes.size() ) {
							// throw string("FocusRange: Index aut of range");
							// don't throw on out of range index
							// just set beyond end state !! 
							focus.unset();
							break;
						}
					}
				}
			}
			break;
		}
		case FocusRangeDescriptor::IT_Cell :
		{
			cell = idx /*/ data->c_div*/;
			focus.setCell(data->cell_range[cell]);
			break;
		}
		case FocusRangeDescriptor::IT_CellMembrane :
		{
			int remainder = idx;

			cell = remainder / data->c_div;
			remainder = remainder %data->c_div;
			
			pos.z = 0;
			
			pos.y = remainder / data->y_div;
			remainder = remainder % data->y_div;
			
			pos.x = remainder / data->x_div;
			remainder = remainder % data->x_div;
			
			focus.setMembrane(data->cell_range[cell], pos + data->pos_offset);
			
			break;
		}
			
		case FocusRangeDescriptor::IT_Space :
		{
			int remainder = idx;
			if (idx>=data->size || data->granularity == Granularity::Global) {
				focus.unset();
				break;
			}
			
			pos.z = remainder / data->z_div;
			remainder = remainder % data->z_div;
			
			pos.y = remainder / data->y_div;
			remainder = remainder % data->y_div;
			
			pos.x = remainder / data->x_div;
			remainder = remainder % data->x_div;
			
			focus.setPosition(pos + data->pos_offset);
			
			break;
		}

		case FocusRangeDescriptor::IT_Domain :
		{
			if (idx>=data->domain_enumeration->size()) {
				focus.unset();
				break;
			}
			pos = data->domain_enumeration->at(idx);
			
			focus.setPosition(pos);
			
			break;
		}
		
		case FocusRangeDescriptor::IT_Domain_int :
		{
			if (idx>=data->domain_nodes_int.size()) {
				focus.unset();
				break;
			}
			pos = data->domain_nodes_int.at(idx);
			
			focus.setPosition(pos);
			
			break;
		}
	}

}
FocusRangeIterator& FocusRangeIterator::operator++()
{
	idx++;
	
	if (idx>=data->size) {
		focus.unset();
		return *this;
	}
	
	switch (data->iter_mode) {
		case FocusRangeDescriptor::IT_Cell:
			cell++;
			focus.setCell(data->cell_range[cell]);
			break;
		case FocusRangeDescriptor::IT_CellNodes:
// 		case FocusRangeDescriptor::IT_CellSurfaceNodes:
			current_cell_node++;
			if ( current_cell_node == data->cell_nodes[cell]->end() ) {
				cell++;
				current_cell_node = data->cell_nodes[cell]->begin();
				pos = *current_cell_node;
				focus.setCell(data->cell_range[cell], pos);
			}
			else {
				pos = *current_cell_node;
				focus.setCell(data->cell_range[cell], pos);
			}
			break;
		case FocusRangeDescriptor::IT_CellNodes_int:
			current_cell_node++;
			if ( current_cell_node == data->cell_nodes_int[cell].end() ) {
				cell++;
				current_cell_node = data->cell_nodes_int[cell].begin();
				pos = *current_cell_node;
				focus.setCell(data->cell_range[cell], pos);
			}
			else {
				pos = *current_cell_node;
				focus.setCell(data->cell_range[cell], pos);
			}
			break;
		case FocusRangeDescriptor::IT_CellMembrane :
			pos.x++;
			if (pos.x >= data->pos_range.x) {
				pos.x=0;
				pos.y++;
				if (pos.y >= data->pos_range.y) {
					pos.y=0;
					cell++;
				}
			}
			focus.setMembrane(data->cell_range[cell], pos + data->pos_offset);
			break;
			
		case FocusRangeDescriptor::IT_Space :
			pos.x++;
			if (pos.x >= data->pos_range.x) {
				pos.x=0;
				pos.y++;
				if (pos.y >= data->pos_range.y) {
					pos.y=0;
					pos.z++;
				}
			}
			focus.setPosition(pos + data->pos_offset);
			break;
			
		case FocusRangeDescriptor::IT_Domain :
			focus.setPosition(data->domain_enumeration->at(idx));
			break;
		case FocusRangeDescriptor::IT_Domain_int :
			focus.setPosition(data->domain_nodes_int[idx]);
			break;
	}
	return *this;
}


FocusRangeIterator operator+(int n, const FocusRangeIterator& iter) {
	return FocusRangeIterator(iter) + n;
}

FocusRangeIterator operator-(int n, const FocusRangeIterator& iter){
	return FocusRangeIterator(iter) - n;
}


multimap<FocusRangeAxis,int> FocusRange::getBiologicalCellTypesRestriction()
{
	multimap<FocusRangeAxis,int> restriction;
	auto celltypes = CPM::getCellTypes();
	for (auto wct : celltypes) {
		auto ct = wct.lock();
		if (ct->isMedium())
			continue;
		restriction.insert( {FocusRangeAxis::CellType,ct->getID()} );
	}
	return restriction;
}

FocusRange::FocusRange(Granularity granularity, const Scope* scope) {
	const CellType* ct = nullptr;
    multimap<FocusRangeAxis, int> restrictions;
	// Extract the default range for the Granularity
	if (scope && scope->getCellType()) {
		restrictions.insert( make_pair(FocusRangeAxis::CellType,scope->getCellType()->getID()) );
	}
	
	init_range(granularity, restrictions, true);
}

FocusRange::FocusRange(Granularity granularity, multimap<FocusRangeAxis, int> restrictions, bool writable_only)
{
	init_range(granularity, restrictions, writable_only);
};

FocusRange::FocusRange(Granularity granularity, CPM::CELL_ID cell_id )
{
    multimap<FocusRangeAxis, int> restrictions;
	restrictions.insert(make_pair(FocusRangeAxis::CELL, cell_id));
	init_range(granularity, restrictions, true);
};

void FocusRange::init_range(Granularity granularity, multimap< FocusRangeAxis, int > restrictions, bool writable_only)
{
	shared_ptr<FocusRangeDescriptor> range = make_shared<FocusRangeDescriptor>();
	
	shared_ptr<const CellType> ct;
	range->spatial_restriction = FocusRangeDescriptor::RESTR_GLOBAL;
	range->granularity = granularity;
	
	if (!restrictions.empty()) {
		
		if (restrictions.count(FocusRangeAxis::CELL)) {
			
			range->spatial_restriction = FocusRangeDescriptor::RESTR_CELL;
// 			cout << "Read Restriction to cell ";
// 			for (auto i =restrictions.lower_bound(FocusRangeAxis::CELL); i!=restrictions.upper_bound(FocusRangeAxis::CELL); i++) {
// 				cout << i->second << ", ";
// 			}
// 			cout << endl;
		}
		else if ( restrictions.count(FocusRangeAxis::CellType) == 1) {
			ct = CPM::getCellTypes()[restrictions.find(FocusRangeAxis::CellType)->second].lock();
			range->spatial_restriction = FocusRangeDescriptor::RESTR_CELLPOP;
// 			if (ct && !ct->isMedium()) {
// // 				cout << "Read Restriction to celltype " << ct->getName() << endl;
// 			}
// 			else {
// 				throw string( "Invalid CellType for FocusRangeLimits provided.");
// 			}
		}
	}
	
	switch (range->granularity) {
		case  Granularity::Cell :
			range->iter_mode = FocusRangeDescriptor::IT_Cell;
			if (range->spatial_restriction == FocusRangeDescriptor::RESTR_CELL) {
				auto cell_range = restrictions.equal_range(FocusRangeAxis::CELL);
				for (auto cell  = cell_range.first; cell != cell_range.second; cell++ ) {
					range->cell_range.push_back(cell->second);
				}
				if (range->cell_range.size()>1)
					range->data_axis.push_back(FocusRangeAxis::CELL);
				range->pos_range = VINT(1,1,1);
			}
			else if (range->spatial_restriction == FocusRangeDescriptor::RESTR_CELLPOP) {
				range->data_axis.push_back(FocusRangeAxis::CELL);
				range->cell_range = ct->getCellIDs();
				range->pos_range = VINT(1,1,1);
			}
			else {
				range->data_axis.push_back(FocusRangeAxis::CELL);
				auto celltypes = CPM::getCellTypes();
				auto ct_restr = restrictions.equal_range(FocusRangeAxis::CellType);
				if (ct_restr.first != restrictions.end()) {
					for (auto ct_id=ct_restr.first; ct_id!= ct_restr.second; ct_id++) {
						auto ct = celltypes[ct_id->second].lock();
						auto cell_ids = ct->getCellIDs();
						range->cell_range.insert(range->cell_range.end(),cell_ids.begin(), cell_ids.end());
					}
				}
				else {
					for (auto wct : celltypes) {
						auto ct = wct.lock();
// 						if (ct->isMedium())
// 							continue;
						auto cell_ids = ct->getCellIDs();
						range->cell_range.insert(range->cell_range.end(), cell_ids.begin(), cell_ids.end());
					}
				}
			}
			break;
		case Granularity::MembraneNode :
			range->iter_mode = FocusRangeDescriptor::IT_CellMembrane;
			range->pos_range = MembraneProperty::getSize();
			range->spatial_dimensions.insert(FocusRangeAxis::MEM_X);
			if (range->pos_range.y>1)
				range->spatial_dimensions.insert(FocusRangeAxis::MEM_Y);
			
			if (range->spatial_restriction == FocusRangeDescriptor::RESTR_CELL) {
				auto cell_range = restrictions.equal_range(FocusRangeAxis::CELL);
				for (auto cell  = cell_range.first; cell != cell_range.second; cell++ ) {
					range->cell_range.push_back(cell->second);
				}
				if (range->cell_range.size()>1)
					range->data_axis.push_back(FocusRangeAxis::CELL);
				range->data_axis.push_back(FocusRangeAxis::MEM_X);
				if (range->pos_range.y>1)
					range->data_axis.push_back(FocusRangeAxis::MEM_Y);
			}
			else if (range->spatial_restriction == FocusRangeDescriptor::RESTR_CELLPOP) {
				range->cell_range = ct->getCellIDs();
				range->data_axis.push_back(FocusRangeAxis::CELL);
				range->data_axis.push_back(FocusRangeAxis::MEM_X);
				if (range->pos_range.y>1)
					range->data_axis.push_back(FocusRangeAxis::MEM_Y);
			}
			else {
				throw string("Can not iterate with membrane node granularity over global range in FocusRange");
			}
			break;
		case Granularity::SurfaceNode:
		case Granularity::Node : {
			bool surface_only = range->granularity ==  Granularity::SurfaceNode;
			surface_only = false;
			VINT l_size = SIM::lattice().size();
			range->spatial_dimensions.insert(FocusRangeAxis::X);
			if (l_size.y>1) range->spatial_dimensions.insert(FocusRangeAxis::Y);
			if (l_size.z>1) range->spatial_dimensions.insert(FocusRangeAxis::Z);
			
			if (range->spatial_restriction == FocusRangeDescriptor::RESTR_CELL) {
				auto cell_range = restrictions.equal_range(FocusRangeAxis::CELL);
				for (auto cell  = cell_range.first; cell != cell_range.second; cell++ ) {
					range->cell_range.push_back(cell->second);
					if (surface_only)
						range->cell_nodes.push_back( &(CPM::getCell(cell->second).getSurface()) );
					else
						range->cell_nodes.push_back( &(CPM::getCell(cell->second).getNodes()) );
					range->cell_sizes.push_back(range->cell_nodes.back()->size());
				}
				range->iter_mode = FocusRangeDescriptor::IT_CellNodes;
				if (range->cell_range.size()>1)
					range->data_axis.push_back(FocusRangeAxis::CELL);
				range->data_axis.push_back(FocusRangeAxis::NODE);
				range->pos_range = VINT(1,1,1);
			}
			else if (range->spatial_restriction == FocusRangeDescriptor::RESTR_CELLPOP) {
				range->cell_range = ct->getCellIDs();
				for (auto it : range->cell_range) {
					if (surface_only)
						range->cell_nodes.push_back( &(CPM::getCell(it).getSurface()) );
					else
						range->cell_nodes.push_back( &(CPM::getCell(it).getNodes()) );
					range->cell_sizes.push_back(range->cell_nodes.back()->size());
				}
				range->iter_mode = FocusRangeDescriptor::IT_CellNodes;
				range->data_axis.push_back(FocusRangeAxis::NODE);
				range->data_axis.push_back(FocusRangeAxis::CELL);
				range->pos_range = VINT(1,1,1);
			}
			else if (range->spatial_restriction == FocusRangeDescriptor::RESTR_GLOBAL) {

				if (SIM::lattice().getDomain().domainType() != Domain::none && writable_only) {
					range->spatial_restriction = FocusRangeDescriptor::RESTR_DOMAIN;
					range->domain_enumeration = & SIM::lattice().getDomain().enumerated();
					range->iter_mode = FocusRangeDescriptor::IT_Domain;
					range->data_axis.push_back(FocusRangeAxis::NODE);
				}
				else {
					range->data_axis.push_back(FocusRangeAxis::X);
					if (l_size.y>1) range->data_axis.push_back(FocusRangeAxis::Y);
					if (l_size.z>1) range->data_axis.push_back(FocusRangeAxis::Z);
					
					range->spatial_restriction = FocusRangeDescriptor::RESTR_GLOBAL;
					range->pos_range = l_size;
					range->iter_mode = FocusRangeDescriptor::IT_Space;
				}
			}
			break;
		}
		case Granularity::Global :
			range->pos_range = VINT(1,1,1);
			range->iter_mode = FocusRangeDescriptor::IT_Space;
			break;
	}
	
	if (!restrictions.empty()) {
		// Now apply all spatial limits
		switch (range->iter_mode) {
			case FocusRangeDescriptor::IT_CellMembrane :
				if (restrictions.count(FocusRangeAxis::MEM_X) == 1) {
					range->pos_range.x=1;
					range->pos_offset.x = restrictions.find(FocusRangeAxis::MEM_X)->second;
					range->spatial_dimensions.erase(FocusRangeAxis::MEM_X);
					auto& dim = range->data_axis;
					dim.erase(find(dim.begin(), dim.end(), FocusRangeAxis::MEM_X) );
				}
				if (restrictions.count(FocusRangeAxis::MEM_Y) == 1) {
					range->pos_range.y=1;
					range->pos_offset.y = restrictions.find(FocusRangeAxis::MEM_Y)->second;
					range->spatial_dimensions.erase(FocusRangeAxis::MEM_X);
					auto& dim = range->data_axis;
					dim.erase(find(dim.begin(), dim.end(), FocusRangeAxis::MEM_Y) );
				}
			break;
			case FocusRangeDescriptor::IT_CellNodes :
			case FocusRangeDescriptor::IT_CellNodes_int :
			{
				VINT filter_enabled(restrictions.count(FocusRangeAxis::X) == 1, restrictions.count(FocusRangeAxis::Y) == 1, restrictions.count(FocusRangeAxis::Z) == 1);
				if (filter_enabled.abs()==0)
					break;
				VINT filter_value(filter_enabled.x == 1 ? restrictions.find(FocusRangeAxis::X)->second : 0,
								filter_enabled.y == 1 ? restrictions.find(FocusRangeAxis::Y)->second : 0,
								filter_enabled.z == 1 ? restrictions.find(FocusRangeAxis::Z)->second : 0);
				vector< const set <VINT, less_VINT >* > cell_nodes;
				const vector<CPM::CELL_ID>& cell_range = range->cell_range;
				if (range->iter_mode == FocusRangeDescriptor::IT_CellNodes) {
					cell_nodes = range->cell_nodes;
				}
				else if (range->iter_mode == FocusRangeDescriptor::IT_CellNodes_int) {
					for (auto& nodes : range->cell_nodes_int) {
						cell_nodes.push_back(&nodes);
					}
				}
				vector< set <VINT, less_VINT > > new_cell_nodes;
				vector< CPM::CELL_ID > new_cell_range;
				
				for (int i = 0; i<cell_nodes.size(); i++) {
					set <VINT, less_VINT > new_nodes;
					for (auto node : *cell_nodes[i]) {
						if (   (filter_enabled.x ? node.x == filter_value.x : true) 
						    && (filter_enabled.y ? node.y == filter_value.y : true)
						    && (filter_enabled.z ? node.z == filter_value.z : true) )
						{
							new_nodes.insert(new_nodes.end(), node);
						}
					}
					if (!new_nodes.empty()) {
						new_cell_nodes.emplace_back(new_nodes);
						new_cell_range.push_back(cell_range[i]);
					}
				}
				swap(range->cell_nodes_int, new_cell_nodes);
				swap(range->cell_range, new_cell_range);
				range->iter_mode = FocusRangeDescriptor::IT_CellNodes_int;
				
				if (filter_enabled.x) {
					range->pos_range.x=1;
					range->spatial_dimensions.erase(FocusRangeAxis::X);
				}
				if (filter_enabled.y) {
					range->pos_range.y=1;
					range->spatial_dimensions.erase(FocusRangeAxis::Y);
				}
				if (filter_enabled.z) {
					range->pos_range.z=1;
					range->spatial_dimensions.erase(FocusRangeAxis::Z);
				}
				break;
			}
			case FocusRangeDescriptor::IT_Domain :
			case FocusRangeDescriptor::IT_Domain_int :
			{
				VINT filter_enabled(restrictions.count(FocusRangeAxis::X) == 1, restrictions.count(FocusRangeAxis::Y) == 1, restrictions.count(FocusRangeAxis::Z) == 1);
				if (filter_enabled.abs()==0)
					break;
				VINT filter_value(filter_enabled.x == 1 ? restrictions.find(FocusRangeAxis::X)->second : 0,
								filter_enabled.y == 1 ? restrictions.find(FocusRangeAxis::Y)->second : 0,
								filter_enabled.z == 1 ? restrictions.find(FocusRangeAxis::Z)->second : 0);
				
				const vector <VINT >* domain_nodes;
				if (range->iter_mode == FocusRangeDescriptor::IT_Domain) {
					domain_nodes = range->domain_enumeration;
				}
				else /*if (range->iter_mode == FocusRangeDescriptor::IT_Domain_int)*/ {
					domain_nodes = & range->domain_nodes_int;
				}
				vector<VINT> new_domain_nodes;
				for (const auto& node : *domain_nodes) {
					if (   (filter_enabled.x ? node.x == filter_value.x : true) 
						&& (filter_enabled.y ? node.y == filter_value.y : true)
						&& (filter_enabled.z ? node.z == filter_value.z : true) )
					{
						new_domain_nodes.push_back(node);
					}
				}
				
				swap(range->domain_nodes_int,new_domain_nodes);
				range->iter_mode = FocusRangeDescriptor::IT_Domain_int;
				
				if (filter_enabled.x) {
					range->pos_range.x=1;
					range->spatial_dimensions.erase(FocusRangeAxis::X);
				}
				if (filter_enabled.y) {
					range->pos_range.y=1;
					range->spatial_dimensions.erase(FocusRangeAxis::Y);
				}
				if (filter_enabled.z) {
					range->pos_range.z=1;
					range->spatial_dimensions.erase(FocusRangeAxis::Z);
				}
				break;
			}
			case FocusRangeDescriptor::IT_Space:
				if (restrictions.count(FocusRangeAxis::X) == 1) {
					range->pos_range.x=1;
					range->pos_offset.x = restrictions.find(FocusRangeAxis::X)->second;
					range->spatial_dimensions.erase(FocusRangeAxis::X);
					auto& dim = range->data_axis;
					dim.erase(find(dim.begin(), dim.end(), FocusRangeAxis::X) );
				}
				if (restrictions.count(FocusRangeAxis::Y) == 1) {
					range->pos_range.y=1;
					range->pos_offset.y = restrictions.find(FocusRangeAxis::Y)->second;
					range->spatial_dimensions.erase(FocusRangeAxis::Y);
					auto& dim = range->data_axis;
					dim.erase(find(dim.begin(), dim.end(), FocusRangeAxis::Y) );
				}
				if (restrictions.count(FocusRangeAxis::Z) == 1) {
					range->pos_range.z=1;
					range->pos_offset.z = restrictions.find(FocusRangeAxis::Z)->second;
					range->spatial_dimensions.erase(FocusRangeAxis::Z);
					auto& dim = range->data_axis;
					dim.erase(find(dim.begin(), dim.end(), FocusRangeAxis::Z) );
				}
			break;
			case FocusRangeDescriptor::IT_Cell : 
				if (restrictions.count(FocusRangeAxis::X) || restrictions.count(FocusRangeAxis::Y) || restrictions.count(FocusRangeAxis::Z)) {
					// just collect those cells which occupy some nodes in the restricted area
					cerr << "Missing implementation" << endl;
					assert(0);
				}
				break;
		}
	}
	
	// Pre-compute the sizes of dimensions for irregular data sets
	switch(range->iter_mode) {
		case FocusRangeDescriptor::IT_Domain :
			range->size = range->domain_enumeration->size();
			range->sizes.push_back(range->domain_enumeration->size());
			break;
		case FocusRangeDescriptor::IT_Domain_int :
			range->size = range->domain_nodes_int.size();
			range->sizes.push_back(range->domain_nodes_int.size());
			break;
		case FocusRangeDescriptor::IT_CellNodes : 
			range->size = 0;
			for (auto count : range->cell_sizes) {
				range->size += count;
			}
			range->sizes.push_back(-1);
			range->sizes.push_back(range->cell_sizes.size());
			break;
		case FocusRangeDescriptor::IT_CellNodes_int:
			range->size=0;
			for (uint i=0; i< range->cell_nodes_int.size(); i++) {
				range->cell_sizes[i] = range->cell_nodes_int[i].size();
				range->size += range->cell_sizes[i];
			}
			range->sizes.push_back(-1);
			range->sizes.push_back(range->cell_nodes_int.size());
			break;
		default:
			 // IT_Space, IT_Cell, IT_CellMembrane
			if (range->pos_range.x == 0) range->pos_range.x = 1;
			if (range->pos_range.y == 0) range->pos_range.y = 1;
			if (range->pos_range.z == 0) range->pos_range.z = 1;
			range->x_div = 1;
			range->y_div = range->x_div * range->pos_range.x;
			range->z_div = range->y_div * range->pos_range.y;
			range->c_div = range->z_div * range->pos_range.z;
			if (range->iter_mode == FocusRangeDescriptor::IT_Cell || range->iter_mode == FocusRangeDescriptor::IT_CellMembrane) {
				range->size = range->c_div * range->cell_range.size();
			}
			else 
				range->size = range->c_div;
			
			if (range->cell_range.size()>1) range->sizes.push_back(range->cell_range.size());
			if (range->pos_range.z>1) range->sizes.push_back(range->pos_range.z);
			if (range->pos_range.y>1) range->sizes.push_back(range->pos_range.y);
			if (range->pos_range.x>1) range->sizes.push_back(range->pos_range.x);
			// if no dimension with multiple entries exists, just report a single element
			if (range->sizes.empty()) range->sizes.push_back(1);
			
			break;
	}
	
	data = range;
	
// 	cout << "FocusRange: C" << data->cell_range.size() << " P"<< data->pos_range << " S" << data->size << endl;
}
