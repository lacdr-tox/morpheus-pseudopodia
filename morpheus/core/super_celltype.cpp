#include "super_celltype.h"
using namespace SIM;

CellType* SuperCT::createInstance(uint ct_id) {
	return new SuperCT(ct_id);
}

registerCellType(SuperCT);

SuperCT::SuperCT(uint ct_id) : CellType(ct_id) { 
// 	sub_celltype = NULL;
	
}

SuperCT::~SuperCT() {
// 	CellType::~CellType() ;
}

CPM::CELL_ID SuperCT::createCell(CPM::CELL_ID cell_id) {
	cout << "SuperCT::createCell " << cell_id << endl;
	if ( ! storage.isFree(cell_id) ) {
		cout << "!! Warning !! Given cell id " << cell_id << " is already used" << endl;
		cell_id = storage.getFreeID();
		cout << "!! Warning !! Overriding id with " << cell_id << "." << endl;
	}

	// 	cout << "creating real cell "<< cell_id << endl;
	shared_ptr<SuperCell> c(new SuperCell(cell_id, this) );
	if ( ! c ) 
		throw string("unable to create cell");/*, SuperCT* celltype*/
		
	
	//	maintain local associations
	cell_ids.push_back(cell_id);
	

	// maintain global associations 
	// turn super_cell segment_id into subcell
	
	// register index for supercell
	CPM::INDEX t = storage.emptyIndex();
	t.celltype = this->getID();
	t.status = CPM::SUPER_CELL;
	storage.addCell(c,t);
	
	CPM::CELL_ID sub_cell_id = sub_celltype->createCell();
	c->addSubCell(sub_cell_id);
	
	return cell_id;
};

XMLNode SuperCT::saveToXML ( ) const 
{
	XMLNode node = CellType::saveToXML();
	node.addAttribute("subtype", sub_celltype->getName().c_str());
	return node;
}

void SuperCT::loadFromXML ( const XMLNode node ) 
{
	CellType::loadFromXML( node );
	// somehow we have to provide an opportunity to grep supercell plugins
	
	if (! getXMLAttribute(node, "subtype", sub_celltype_name)) {
		cerr << "SuperCellTypes need a SubType defined !" << endl;
		exit(-1);
	}

}

void SuperCT::bindSubCelltype()
{
	auto celltypes = CPM::getCellTypes();
	for (uint i=0; i< celltypes.size(); i++) {
		if (celltypes[i].lock()->getName() == sub_celltype_name ) {
			sub_celltype = const_pointer_cast<CellType>( celltypes[i].lock() );
		}
	}
	if (! sub_celltype) {
		cerr << "SuperCT: unable to connect to subcelltype " << sub_celltype_name << endl;
		exit(-1);
	}
}


void SuperCT::init() 
{
	CellType::init();
};


bool SuperCT::check_update(const CPM::Update& update) const 
{
	if (! sub_celltype->check_update(update) ) return false;
	static uint rejectoins=0;
	if (update.opAdd() && update.opRemove() && update.focusStateAfter().super_cell_id == update.focusStateBefore().super_cell_id) {
		for (uint i=0; i<check_update_listener.size(); i++) {
			if ( ! check_update_listener[i]->update_check (update.focusStateAfter().super_cell_id,update) ){
				if (rejectoins < 200) {
					cout << "Update prevented by " << check_update_listener[i]->XMLName() << endl;
					rejectoins++;
				}
				return false;
			}
		}
	}
	else {
		if (update.opAdd()) {
			auto update_add = update.selectOp(CPM::Update::ADD);
			for (uint i=0; i<check_update_listener.size(); i++) {
				if ( ! check_update_listener[i]->update_check(update_add.focusStateAfter().super_cell_id, update_add) ) {
					if (rejectoins < 200) {
						cout << "Update prevented by " << check_update_listener[i]->XMLName() << endl;
						rejectoins++;
					}
					return false;
				}
			}
		}
		if (update.opRemove()) {
			auto update_remove = update.selectOp(CPM::Update::REMOVE);
			for (uint i=0; i<check_update_listener.size(); i++) {
				if ( ! check_update_listener[i]->update_check(update_remove.focusStateBefore().super_cell_id, update_remove) ) {
					if (rejectoins < 200) {
						cout << "Update prevented by " << check_update_listener[i]->XMLName() << endl;
						rejectoins++;
					}
					return false;
				}
			}
		}
	}
	return true;
}

double SuperCT::delta(const  CPM::Update& update) const {
	double d = sub_celltype->delta(update);
	
	if (update.opAdd() && update.opRemove() && update.focusStateAfter().super_cell_id == update.focusStateBefore().super_cell_id) {
		for (uint i=0; i<energies.size(); i++) {
			d+= energies[i]->delta (update.focusStateAfter().super_cell_id, update);
		}
	} 
	else {
		if (update.opAdd()) {
			auto update_add = update.selectOp(CPM::Update::ADD);
			for (uint i=0; i<energies.size(); i++) {
				d+= energies[i]->delta (update_add.focusStateAfter().super_cell_id, update_add);
			}
		}
		if (update.opRemove()) {
			auto update_remove = update.selectOp(CPM::Update::REMOVE);
			for (uint i=0; i<energies.size(); i++) {
				d+= energies[i]->delta (update_remove.focusStateBefore().super_cell_id, update_remove);
			}
		}
	}
	return d;
}

void SuperCT::set_update(const  CPM::Update& update) {
	
	sub_celltype->set_update(update);
	
	if (update.opAdd() && update.opRemove() && update.focusStateAfter().super_cell_id == update.focusStateBefore().super_cell_id) {
		storage.cell(update.focusStateAfter().super_cell_id) . setUpdate(update);
	} else {
		if (update.opAdd())
			storage.cell(update.focusStateAfter().super_cell_id) . setUpdate(update.selectOp(CPM::Update::ADD));
		if (update.opRemove())
			storage.cell(update.focusStateBefore().super_cell_id) . setUpdate(update.selectOp(CPM::Update::REMOVE));
	}
}

void SuperCT::apply_update(const CPM::Update& update) {
	
	sub_celltype->apply_update(update);
	
	if (update.opAdd() && update.opRemove() && update.focusStateAfter().super_cell_id == update.focusStateBefore().super_cell_id) {
		storage.cell(update.focusStateAfter().super_cell_id) . applyUpdate(update);
		for (uint i=0; i<update_listener.size(); i++) {
			update_listener[i]->update_notify(update.focusStateAfter().super_cell_id,update);
		}
	} 
	else {
		if (update.opAdd()) {
			auto update_add = update.selectOp(CPM::Update::ADD);
			storage.cell(update_add.focusStateAfter().super_cell_id) . applyUpdate(update_add);
			for (uint i=0; i<update_listener.size(); i++) {
				update_listener[i]->update_notify(update_add.focusStateAfter().super_cell_id, update_add);
			}
		}
		if (update.opRemove()) {
			auto update_remove = update.selectOp(CPM::Update::REMOVE);
			storage.cell(update_remove.focusStateBefore().super_cell_id) . applyUpdate(update_remove);
			for (uint i=0; i<update_listener.size(); i++) {
				update_listener[i]->update_notify(update_remove.focusStateBefore().super_cell_id, update_remove);
			}
		}
	}

}

double SuperCT::hamiltonian() const {
	double h = CellType::hamiltonian();
// 	foreach cell; foreach segment
// 	Cell* cell = static_cast<SegmentedCell* >(cells[cell_i])->getSegments()[segment_i];
// 	for (uint i=0; i<segment_energies.size(); i++) {
// 		segment_energies[i]->hamiltonian(cell;
// 	}
	return h;
}
