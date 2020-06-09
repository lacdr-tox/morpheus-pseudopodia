#include "symbolfocus.h"
#include "cell.h"
#include "membrane_property.h"


SymbolFocus::SymbolFocus () :
	has_pos(false), has_global_pos(false), has_membrane(false), has_cell(false), has_cell_index(false), d_cell(NULL)
	{};

SymbolFocus::SymbolFocus ( const VINT& pos ) :
	has_pos(true), has_global_pos(false),  has_membrane(false), has_cell(false), has_cell_index(false), d_pos(pos), d_cell(NULL)
	{};

SymbolFocus::SymbolFocus ( CPM::CELL_ID cell_id ) :
	has_pos(false), has_global_pos(false), has_membrane(false), has_cell(true),  has_cell_index(false), d_cell_id(cell_id), d_cell(& CPM::getCell(cell_id))
	{};

SymbolFocus::SymbolFocus ( CPM::CELL_ID cell_id, const VINT& pos ) :
	has_pos(true), has_global_pos(false),  has_membrane(false), has_cell(true),  has_cell_index(false), d_pos(pos), d_cell_id(cell_id), d_cell(& CPM::getCell(cell_id))
	{};

SymbolFocus::SymbolFocus ( CPM::CELL_ID cell_id, double phi, double theta) :
	has_pos(false), has_global_pos(false), has_membrane(true),  has_cell(true),  has_cell_index(false), d_membrane_pos(VINT(phi,theta,0)), d_cell_id(cell_id), d_cell(& CPM::getCell(cell_id))
	{};

const VINT&  SymbolFocus::membrane_pos() const {
	if (!has_membrane) {
		VDOUBLE from_center = SIM::lattice().orth_distance(SIM::lattice().to_orth(pos()),cell().getCenter());
		d_membrane_pos = MembraneProperty::orientationToMemPos(from_center);
	}
	return d_membrane_pos;
};

const VINT& SymbolFocus::pos() const {
	if (has_pos) return d_pos;
	else { 
		throw string("Requesting position from SymbolFocus, which is not available!");
		return d_pos;
	}
};

const VDOUBLE& SymbolFocus::global_pos() const
{
	if (has_global_pos) 
		return d_global_pos;
	
	d_global_pos = SIM::lattice().to_orth(pos());
	SIM::lattice().orth_resolve(d_global_pos);
	has_global_pos = true;
	return d_global_pos;
}


const Cell& SymbolFocus::cell() const {
	if (!has_cell) {
		if ( ! has_pos) {
			throw string("SymbolFocus cannot deduce cell.\nNo position or cell associated with the Focus.");
		}
		d_cell = & CPM::getCell(CPM::getNode(d_pos).cell_id);
		has_cell = true;
	}
	if (!d_cell)
		throw string("Cell ") + to_string(d_cell_id) + " does not exist (anymore).";
	return *d_cell;
}

const CPM::CELL_ID SymbolFocus::cellID() const {
	return d_cell_id;
}

const CPM::INDEX& SymbolFocus::cell_index() const {
	if (!has_cell_index) {
		d_cell_index = CPM::getCellIndex(cell().getID() );
		has_cell_index=true;
	}
	return d_cell_index;
};

void SymbolFocus::setCell(CPM::CELL_ID cell_id) {
	unset();
	d_cell_id = cell_id;
	d_cell = &CPM::getCell(cell_id);
	
	has_cell=true;
};

void SymbolFocus::setCell(CPM::CELL_ID cell_id, const VINT& pos) {
	if (! has_cell || d_cell->getID() != cell_id) {
		unset();
		d_cell_id = cell_id;
		d_cell = &CPM::getCell(cell_id);
		has_cell = true;
	}
	else {
		has_membrane=false;
	}
	d_pos = pos;
	has_pos = true;
	has_global_pos = false;
};

void SymbolFocus::setPosition(const VINT& pos) {
	unset();
	d_pos = pos;
	has_pos=true;
};
void SymbolFocus::setMembrane(CPM::CELL_ID cell_id, const VINT& pos ) {
	if (! has_cell || d_cell->getID() != cell_id) {
		unset();
		d_cell_id = cell_id;
		d_cell = &CPM::getCell(cell_id);
		has_cell=true;
	}
	d_membrane_pos = pos;
	has_membrane=true;
	has_pos=false;
	has_global_pos = false;
};

int SymbolFocus::get(FocusRangeAxis axis) const {
	switch (axis) {
		case FocusRangeAxis::X: return pos().x;
		case FocusRangeAxis::Y: return pos().y;
		case FocusRangeAxis::Z: return pos().z;
		case FocusRangeAxis::MEM_X: return membrane_pos().x;
		case FocusRangeAxis::MEM_Y: return membrane_pos().y;
		case FocusRangeAxis::CELL: return cellID();
		case FocusRangeAxis::CellType: return cell_index().celltype;
// 		case FocusRangeAxis::NODE: return 0;
// 		case FocusRangeAxis::MEM_NODE: return 0;
		default:
			return 0;
	}
}

void SymbolFocus::unset() {
	has_pos=false;
	has_global_pos=false;
	has_membrane=false;
	has_cell=false;
	has_cell_index=false;
};

bool SymbolFocus::operator<(const SymbolFocus& rhs) const {
	if (has_pos && rhs.has_pos) {
		const VINT& a=d_pos;const VINT& b=rhs.d_pos;
// 		return ( a.z < b.z || (a.z==b.z  &&  (a.y<b.y || (a.y==b.y && a.x<b.x))));
		return ( a.x < b.x || (a.x==b.x  &&  (a.y<b.y || (a.y==b.y && a.z<b.z))));
	}
	else if (has_pos || rhs.has_pos)
		return has_pos;
	else if (has_cell && rhs.has_cell) {
		if ( d_cell_id == rhs.d_cell_id){
			if (has_membrane && rhs.has_membrane) {
				const VINT& a=d_membrane_pos;const VINT& b=rhs.d_membrane_pos;
// 				return (a.y<b.y || (a.y==b.y && a.x<b.x));
				return (a.x<b.x || (a.x==b.x && a.y<b.y));
			}
			else if (has_membrane || rhs.has_membrane) {
				return has_membrane;
			}
			else {
				return false;
			}
		}
		else
			return d_cell_id < rhs.d_cell_id;
	}
	else if (has_cell || rhs.has_cell)
		return has_cell;
	else
		return false;
}

bool SymbolFocus::operator==(const SymbolFocus& rhs) const {
	if (has_pos && rhs.has_pos) {
		return d_pos == rhs.d_pos;
	}
	else if (has_pos || rhs.has_pos)
		return false;
	else if (has_cell && rhs.has_cell) {
		if (has_membrane && rhs.has_membrane) {
			return d_membrane_pos == rhs.d_membrane_pos;
		}
		else if (has_membrane || rhs.has_membrane) {
			return false;
		}
		else 
			return d_cell_id == rhs.d_cell_id;
	}
	else {
		return true; // Both are global
	}
}

const SymbolFocus SymbolFocus::global = SymbolFocus();





