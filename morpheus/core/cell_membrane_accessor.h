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

#ifndef CELL_MEMBRANE_ACCESSOR_H
#define CELL_MEMBRANE_ACCESSOR_H

template <class T> class CellPropertyAccessor;
#include "simulation.h"
#include "celltype.h"
#include "symbolfocus.h"
 
class CellMembraneAccessor {
	public:
		CellMembraneAccessor() : ct(NULL), fake_value(0) {};
		CellMembraneAccessor(const CellMembraneAccessor& cma) : ct(cma.ct), fake_value(0), pid(cma.pid) {};
		CellMembraneAccessor(const CellType* celltype, uint property_id);
		bool valid() const {return ct!=NULL;}
		const CellType * getCellType() {return ct; };
// 		double& getDefault() const;
		TypeInfo<double>::Return get(CPM::CELL_ID c, VINT pos) const;
		TypeInfo<double>::Return get(CPM::CELL_ID c, uint theta, uint phi=0) const;
		TypeInfo<double>::Return get(const SymbolFocus& focus) const;
		PDE_Layer* getMembrane(CPM::CELL_ID cell_id) const;
		const CellType* getScope() const {return ct;};
		
		VINT size(CPM::CELL_ID cell_id) const;
		bool set(CPM::CELL_ID c, TypeInfo<double>::Parameter value, VINT pos) const;
		bool set(CPM::CELL_ID c, TypeInfo<double>::Parameter value, uint theta,  uint phi=0) const;
		bool set(const SymbolFocus& focus, TypeInfo<double>::Parameter value) const;
		bool setBuffer(const SymbolFocus& focus, TypeInfo<double>::Parameter value) const;
		void swapBuffers() const;
		void swapBuffers(const SymbolFocus& f) const;
		TypeInfo<double>::Return operator()(CPM::CELL_ID c, VINT pos) const;
		TypeInfo<double>::Return operator()(CPM::CELL_ID c, uint theta, uint phi=0) const;
		TypeInfo<double>::Return operator()(const SymbolFocus& focus)  const;
		
		string getSymbol();
		string getFullName();
	private:

		VINT map_global( CPM::CELL_ID cell_id, VINT pos ) const;
		const CellType* ct;
		mutable double fake_value;
		uint pid;
};


#endif

