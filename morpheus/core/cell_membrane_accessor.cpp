#include "cell_membrane_accessor.h"
#include "celltype.h"


CellMembraneAccessor::CellMembraneAccessor(const CellType* celltype, uint property_id) :
	   fake_value(0),
	ct(celltype),
	pid(property_id)
{ 
	if (ct)
		assert(ct->default_membranes[pid]); 
};



// double& CellMembraneAccessor::getDefault(VINT pos) const {
// 	if (! ct)  { (*shit_value)=double(0.0); return *shit_value; }
// 	return (ct->default_membranePDEs[pid])->get_writable( pos ); 
// }

string CellMembraneAccessor::getSymbol() { if (ct) return ct->default_membranes[pid]->getSymbol(); else return "";};

string CellMembraneAccessor::getFullName() { if (ct) return ct->default_membranes[pid]->getName(); else return "";};

VINT CellMembraneAccessor::map_global ( CPM::CELL_ID cell_id, VINT pos ) const
{
	VDOUBLE from_center = SIM::lattice().orth_distance(SIM::lattice().to_orth(pos),CPM::getCell(cell_id).getCenter());
	VDOUBLE membrane_pos = MembraneProperty::orientationToMemPos(from_center);
// 	if( MembraneProperty::size.x <= 1 ){ // linear Membrane PDE
// 		double angle = (from_center).angle_xy();
// 		membrane_pos.x = (int) (angle * (((0.5*(double)MembraneProperty::size.x)/M_PI)))  ;
// 	}
// 	else{ // spherical Membrane PDE
// 		VDOUBLE radials = from_center.to_radial();
// 		membrane_pos.x = (int) (radials.x * (((0.5*(double)MembraneProperty::size.x)/M_PI)))  ;
// 		membrane_pos.y = (int) (radials.y * ((((double)MembraneProperty::size.y)/M_PI)))  ;
// 	}
	return membrane_pos;
}


TypeInfo<double>::Return CellMembraneAccessor::get(CPM::CELL_ID cell_id, uint theta, uint phi) const {
	assert ( ct );
	assert( ct->getID() == CellType::storage.index(cell_id).celltype );
    return (CPM::getCell(cell_id).membranes[pid]->get( VINT(theta,phi,0) ));
}


TypeInfo<double>::Return CellMembraneAccessor::get(CPM::CELL_ID cell_id, VINT pos) const {
	VINT membrane_pos = map_global(cell_id,pos);
	return get( cell_id, membrane_pos.x, membrane_pos.y );
}

TypeInfo<double>::Return CellMembraneAccessor::get(const SymbolFocus& focus) const {
	return (focus.cell().membranes[pid]->get(focus.membrane_pos()));
}

VINT CellMembraneAccessor::size(CPM::CELL_ID cell_id) const {
	return (CPM::getCell(cell_id).membranes[pid]->size());
}

PDE_Layer* CellMembraneAccessor::getMembrane(CPM::CELL_ID cell_id) const {
	return CPM::getCell(cell_id).membranes[pid].get();
}

bool CellMembraneAccessor::set(CPM::CELL_ID cell_id, TypeInfo<double>::Parameter value, VINT pos) const {
	VINT membrane_pos = map_global(cell_id,pos);
	return set(cell_id, membrane_pos.x, membrane_pos.y, value);
}

bool CellMembraneAccessor::set(CPM::CELL_ID cell_id, TypeInfo<double>::Parameter value, uint theta, uint phi) const {
	assert( ct );
	//assert( ct->getID() == CellType::storage.index(cell_id).celltype );
	if( SIM::getLattice()->getDimensions() == 2 ) // assume linear PDE
		phi = 0;
	
	return CPM::getCell(cell_id).membranes[pid]->set(VINT(theta,phi,0), value);
}

bool CellMembraneAccessor::set(const SymbolFocus& focus, TypeInfo<double>::Parameter value) const {
	assert(ct && focus.cell_index().celltype == ct->getID());
	return (focus.cell().membranes[pid]->set(focus.membrane_pos(),value));
}

bool CellMembraneAccessor::setBuffer(const SymbolFocus& focus, TypeInfo<double>::Parameter value) const {
	assert(ct && focus.cell_index().celltype == ct->getID());
	return (focus.cell().membranes[pid]->setBuffer(focus.membrane_pos(),value));
}

void  CellMembraneAccessor::swapBuffers(const SymbolFocus& f) const {
	assert(ct && f.cell_index().celltype == ct->getID());
	f.cell().membranes[pid]->applyBuffer(f.membrane_pos());
}

void CellMembraneAccessor::swapBuffers() const {
	if (ct==NULL) { return; }
	vector <CPM::CELL_ID > cells = ct->getCellIDs();
	for (uint i=0; i<cells.size(); i++) {
		CPM::getCell(cells[i]).membranes[pid]->swapBuffer();
	}
}

TypeInfo<double>::Return CellMembraneAccessor::operator()(CPM::CELL_ID cell_id, uint theta, uint phi) const {
	if (! ct) {
		return fake_value=0.0; 
	}
	assert( ct->getID() == CellType::storage.index(cell_id).celltype );
	return (CPM::getCell(cell_id).membranes[pid]->get( VINT(theta,phi,0) ));
};

TypeInfo<double>::Return CellMembraneAccessor::operator()(CPM::CELL_ID cell_id, VINT pos) const { 
	VINT membrane_pos = map_global(cell_id,pos);
	return this -> operator()( cell_id, membrane_pos.x, membrane_pos.y);
};

TypeInfo<double>::Return CellMembraneAccessor::operator()(const SymbolFocus& focus)  const {
	if (! ct) {
		return fake_value=0.0; 
	}
	assert(ct && focus.cell_index().celltype == ct->getID());
	return  (focus.cell().membranes[pid]->get(focus.membrane_pos()));
};





//double CellMembraneAccessor::angle2d(VDOUBLE v1, VDOUBLE v2) const{
//	return atan2(v1.x*v2.y-v2.x*v1.y, v1*v2) + M_PI; // [-PI, PI]
//}
