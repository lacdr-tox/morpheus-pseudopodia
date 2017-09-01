#include "symbolfocus.h"
#include "cell.h"
#include "membrane_property.h"


SymbolFocus::SymbolFocus () :
	has_pos(false), has_membrane(false), has_cell(false), has_cell_index(false), d_cell(NULL)
	{};

SymbolFocus::SymbolFocus ( const VINT& pos ) :
	has_pos(true),  has_membrane(false), has_cell(false), has_cell_index(false), d_cell(NULL), d_pos(pos)
	{};

SymbolFocus::SymbolFocus ( CPM::CELL_ID cell_id ) :
	has_pos(false), has_membrane(false), has_cell(true),  has_cell_index(false), d_cell(& CPM::getCell(cell_id))
	{};

SymbolFocus::SymbolFocus ( CPM::CELL_ID cell_id, const VINT& pos ) :
	has_pos(true),  has_membrane(false), has_cell(true),  has_cell_index(false), d_cell(& CPM::getCell(cell_id)), d_pos(pos)
	{};

SymbolFocus::SymbolFocus ( CPM::CELL_ID cell_id, double phi, double theta) :
	has_pos(false), has_membrane(true),  has_cell(true),  has_cell_index(false), d_cell(& CPM::getCell(cell_id)), d_membrane_pos(VINT(phi,theta,0))
	{};

const VINT&  SymbolFocus::membrane_pos() const {
	if (!has_membrane) {
		VDOUBLE from_center = SIM::lattice().orth_distance(SIM::lattice().to_orth(pos()),cell().getCenter());
		d_membrane_pos = MembraneProperty::orientationToMemPos(from_center);
// 		if( MembraneProperty::size.y <= 1){ // assume linear PDE
// 			double angle = (from_center).angle_xy();
// 			d_membrane_pos.x = (int) (angle * (((0.5*(double)MembraneProperty::size.x)/M_PI)))  ;
// 		}
// 		else{ // 3D simulation: assume spherical PDE
// 			VDOUBLE radials = from_center.to_radial();
// 			d_membrane_pos.x = (int) (radials.x * (((0.5*(double)MembraneProperty::size.x)/M_PI)))  ;
// 			d_membrane_pos.y    = (int) (radials.y * ((((double)MembraneProperty::size.y)/M_PI)))  ;
// 		}
	}
	return d_membrane_pos;
};
const VINT& SymbolFocus::pos() const {
	if (has_pos) return d_pos; else { throw string("Requesting position from SymbolFocus, which is not available!"); return d_pos; }
};

const Cell& SymbolFocus::cell() const {
	if (!has_cell) {
		if ( ! has_pos) {
			throw string("SymbolFocus cannot deduce cell.\nNo position or cell associated with the Focus.");
		}
		d_cell = & CPM::getCell(CPM::getNode(d_pos).cell_id);
	}
	return *d_cell;
}

const CPM::CELL_ID SymbolFocus::cellID() const {
	return cell().getID();
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
	d_cell = &CPM::getCell(cell_id);
	has_cell=true;
};

void SymbolFocus::setCell(CPM::CELL_ID cell_id, const VINT& pos) {
	unset(); d_cell = &CPM::getCell(cell_id);
	has_cell=true;
	d_pos = pos;
	has_pos=true;
};

void SymbolFocus::setPosition(const VINT& pos) {
	unset();
	d_pos = pos;
	has_pos=true;
};
void SymbolFocus::setMembrane(CPM::CELL_ID cell_id, const VINT& pos ) {
	if (! has_cell || d_cell->getID() != cell_id) {
		unset();
		d_cell = &CPM::getCell(cell_id);
		has_cell=true;
	}
	d_membrane_pos = pos;
	has_membrane=true;
	has_pos=false;
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
	has_membrane=false;
	has_cell=false;
	has_cell_index=false;
};

const SymbolFocus SymbolFocus::global = SymbolFocus();


