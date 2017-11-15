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

#ifndef CELL_PROPERTY_ACCESSOR_H
#define CELL_PROPERTY_ACCESSOR_H

class SuperCT;
#include "symbol.h"
#include "symbolfocus.h"
#include "simulation.h"
#include "property.h"
#include "celltype.h"
#include "super_celltype.h"


template <class T> 
class CellPropertyAccessor {
	public:
		CellPropertyAccessor();
		CellPropertyAccessor(const CellType* celltype, uint property_id);
		
		bool valid() const {return ct!=NULL;}
		const CellType * getScope() const { return ct; };
		typename TypeInfo<T>::Return getDefault() const;
		typename TypeInfo<T>::Return get(CPM::CELL_ID cell_id) const;
		typename TypeInfo<T>::Return get(VINT pos) const;
		typename TypeInfo<T>::Return get(const SymbolFocus& focus) const;
		bool set(CPM::CELL_ID cell_id, typename TypeInfo<T>::Parameter value) const;
		bool set(const SymbolFocus& focus, typename TypeInfo<T>::Parameter value) const;
		bool setBuffer(const SymbolFocus& focus, typename TypeInfo<T>::Parameter value) const;
		bool swapBuffer(const SymbolFocus& f) const;
		bool swapBuffer() const;
		typename TypeInfo<T>::Reference operator()(CPM::CELL_ID cell_id) const;
	
	private:
		mutable T fake_value;
		const CellType* ct;
		uint pid;
		bool is_super_cell_property;
};

template <class T>
CellPropertyAccessor<T>::CellPropertyAccessor() : ct(NULL), is_super_cell_property(false) {};

template <class T>
CellPropertyAccessor<T>::CellPropertyAccessor(const CellType* celltype, uint property_id) :
	ct(celltype),
	pid(property_id),
	is_super_cell_property(false)
{ 
	if (ct)
		 if (! dynamic_pointer_cast< Property<T> >(ct->default_properties[pid]) )
		 {
			 cerr << "Trying to create CellPropertyAccessor with wrong type ! " << endl;
			 cerr << "CT " << celltype->getName() << " property " << celltype->default_properties[pid]->getSymbol() << endl;
			 exit(-1);
		 }
			 
	if ( dynamic_cast<const SuperCT*>(ct) ) {
// 		cout << "CellPropertyAccessor: Property " << celltype->default_properties[pid]->getSymbol() << " is a property of a SuperCellType !!" << endl;
		is_super_cell_property  = true;
	}
};


template <class T>
typename TypeInfo<T>::Return CellPropertyAccessor<T>::getDefault() const {
	if (ct==NULL)  { return fake_value = T(); }
	return static_pointer_cast< Property<T> >(ct->default_properties[pid])->getRef(); 
}

template <class T>
typename TypeInfo<T>::Return CellPropertyAccessor<T>::get(CPM::CELL_ID cell_id) const {
	if (ct==NULL) { return fake_value = T(); }
	if (is_super_cell_property) {
		// upcast to the supercell
// 		cout <<  "CellPropertyAccessor: Property " << ct->default_properties[pid]->getSymbol() << " is a property of a SuperCellType !!" << endl;
		const CPM::INDEX& index = CellType::storage.index(cell_id);
		if (index.status != CPM::SUPER_CELL) {
// 			cout << "Wrapping SC property " << ct->default_properties[pid]->getSymbol() << "; routing from subcell " << cell_id << " to supercell " << index.super_cell_id <<  endl; 
			const Cell & sc = CPM::getCell(index.super_cell_id);
// 			cout << " to supercell " << cell_id << endl;
			assert(sc.getCellType() == ct);
			return static_cast< Property<T>* >( sc.properties[pid].get())->get();
			
		}
	}
	const Cell & c = CPM::getCell(cell_id);
	assert(c.getCellType() == ct);
	return static_cast< Property<T>* >(c.properties[pid].get())->get();
}

template <class T>
typename TypeInfo<T>::Return CellPropertyAccessor<T>::get(VINT pos) const {
	if (ct==NULL) { return fake_value = T(); }
	const CPM::STATE s = CPM::getNode(pos);
	if (is_super_cell_property) {
		const Cell & sc = CPM::getCell(s.super_cell_id);
		assert(sc.getCellType() == ct);
		return static_cast< Property<T>* >(sc.properties[pid].get())->get();
	} 
	else {
		const Cell & c = CPM::getCell(s.cell_id);
		assert(c.getCellType() == ct);
		static_cast< Property<T>* >(c.properties[pid].get())->get();
	}
}

template <class T>
typename TypeInfo<T>::Return CellPropertyAccessor<T>::get(const SymbolFocus& focus) const {
	if (ct==NULL) { return fake_value =T(); }
	if (is_super_cell_property && focus.cell_index().status != CPM::SUPER_CELL) {
// 		cout << "Wrapping SC property " << ct->default_properties[pid]->getSymbol() << "; routing from subcell " << cell_id << " to supercell " << index.super_cell_id <<  endl;
		const Cell & sc = CPM::getCell(focus.cell_index().super_cell_id);
// 		cout << " to supercell " << cell_id << endl;
		assert(sc.getCellType() == ct);
		static_cast< Property<T>* >( sc.properties[pid].get())->get();
	}
	assert(focus.cell_index().celltype == ct->getID());
	return static_cast< Property<T>* >(focus.cell().properties[pid].get())->get();
}



template <class T>
bool CellPropertyAccessor<T>::set(CPM::CELL_ID cell_id,  typename TypeInfo<T>::Parameter value) const {
	if (ct==NULL) { return false; }
	const Cell & c = CPM::getCell(cell_id);
// 	if (! (c.getCellType() == ct)) {
// 		cout << "CellPropertyAccessor[CT=" << ct->getName() << "]: requesting from cell of type " <<  c.getCellType()->getName() << endl;
// 	}
	assert(c.getCellType() == ct);
	if (is_super_cell_property) {
		const CPM::INDEX& index = CellType::storage.index(cell_id);
		if (index.status != CPM::SUPER_CELL) {
			const Cell & sc = CPM::getCell(index.super_cell_id);
			assert(sc.getCellType() == ct);
			static_cast< Property<T>* >( sc.properties[pid].get())->set(value);
			return true;
		}
	}

	static_cast< Property<T>* >(c.properties[pid].get())->set(value);
	return true;
}

template <class T>
bool CellPropertyAccessor<T>::set(const SymbolFocus& focus, typename TypeInfo<T>::Parameter value) const {
	if (ct==NULL) { return false; }
	if (is_super_cell_property && focus.cell_index().status != CPM::SUPER_CELL) {
		const Cell & sc = CPM::getCell(focus.cell_index().super_cell_id);
		assert(sc.getCellType() == ct);
		static_cast< Property<T>* >( sc.properties[pid].get())->set(value);
		return true;
	}
	assert(focus.cell_index().celltype == ct->getID());
	static_cast< Property<T>* >(focus.cell().properties[pid].get())->set(value);
	return true;
}

template <class T>
bool CellPropertyAccessor<T>::setBuffer(const SymbolFocus& focus, typename TypeInfo<T>::Parameter value) const {
	if (ct==NULL) { return false; }
	if (is_super_cell_property && focus.cell_index().status != CPM::SUPER_CELL) {
		const Cell & sc = CPM::getCell(focus.cell_index().super_cell_id);
		assert(sc.getCellType() == ct);
		static_cast< Property<T>* >( sc.properties[pid].get())->setBuffer(value);
		return true;
	}
	assert(focus.cell_index().celltype == ct->getID());
	static_cast< Property<T>* >(focus.cell().properties[pid].get())->setBuffer(value);
	return true;
}

template <class T>
bool CellPropertyAccessor<T>::swapBuffer(const SymbolFocus& f) const {
	assert(ct!=NULL && f.cell_index().celltype == ct->getID());
	static_cast< Property<T>* >(f.cell().properties[pid].get())->applyBuffer();
	return true;
}

template <class T>
bool CellPropertyAccessor<T>::swapBuffer() const {
	if (ct==NULL) { return false; }
	vector <CPM::CELL_ID > cells = ct->getCellIDs();
	for (uint i=0; i<cells.size(); i++) {
		static_cast< Property<T>* >(CPM::getCell(cells[i]).properties[pid].get())->applyBuffer();
	}
	return true;
}


template <class T>
typename TypeInfo<T>::Reference CellPropertyAccessor<T>::operator()(CPM::CELL_ID cell_id) const {
	if (ct==NULL) { return fake_value=T(); }

	if (is_super_cell_property) {
		const CPM::INDEX& index = CellType::storage.index(cell_id);
		if (index.status != CPM::SUPER_CELL) {
			return static_cast< Property<T>* >(CellType::storage.cell(index.super_cell_id).properties[pid].get())->getRef();
		}
	}
	const Cell & c = CPM::getCell(cell_id);
	assert(c.getCellType() == ct);
	return static_cast< Property<T>* >(c.properties[pid].get())->getRef();
}
#endif

