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

#ifndef SEGMENTED_CT_H
#define SEGMENTED_CT_H

#include "celltype.h"
#include "super_cell.h"



class SuperCT : public CellType 
{
public:
	// platform reqs
	static CellType* createInstance(uint ct_id);
	static bool factory_registration;
	string XMLClassName() const override { return string("supercell"); };

	SuperCT(uint id);
	~SuperCT();
	XMLNode saveToXML() const override;
	void loadFromXML( const XMLNode Node, Scope* scope ) override;
// 	virtual XMLNode savePopulationToXML() const;
// 	virtual void loadPopulationFromXML(const XMLNode Node);
// 	
	void init() override;
	void bindSubCelltype();
	shared_ptr<const CellType> getSubCelltype() const { return sub_celltype;};
// 	template <class S>
// 	CellPropertyAccessor<S> findCellProperty(string prop_symbol, bool required = true) const;
		
// 	The interface methods should propagate all calls to the subcelltype
	bool check_update(const CPM::Update& update) const override;
	void set_update(const CPM::Update& update) override; 
	void apply_update(const CPM::Update& update) override; 
	double delta(const CPM::Update& update) const override;
	double hamiltonian() const override;  

protected:

	CPM::CELL_ID createCell(CPM::CELL_ID name = storage.getFreeID()) override;
	
// 	struct Segment_Filter {
// 		enum Filter_Type  { all, single, min, max};
// 		Filter_Type type;
// 		int value;
// 	};
	
	string sub_celltype_name;
	shared_ptr<CellType> sub_celltype;
};


// /////////////////////////////////////////////////////////////////////
// // Implementation of template functions

// #include "cell_property_accessor.h"

// template <class S>
// CellPropertyAccessor<S> SuperCT::findCellProperty(string prop_symbol, bool required) const {
// 	bool found_wrong_type_matching = false;
// 	map<string,uint>::const_iterator prop_idx = property_by_symbol.find(prop_symbol);
// 	if (prop_idx != property_by_symbol.end() ) {
// 		if (Property<S>().getTypeName() == default_properties[prop_idx->second]->getTypeName()) {
// 			return CellPropertyAccessor<S>(this,prop_idx->second);
// 		}
// 		else {
// 			found_wrong_type_matching = true;
// 		}
// 	}
// 
// 	if (required && found_wrong_type_matching)
// 		throw(string("CellType[\"") + getName() + string("\"].findCellProperty: requested cellproperty [")+prop_symbol+string("] has different Type"));
// 	if (required && ! found_wrong_type_matching)
// 		return sub_celltype->findCellProperty<S>(prop_symbol, required);
// 		throw (string("CellType[\"") + getName() + string("\"].findCellProperty: requested cellproperty [")+prop_symbol+string("] not found"));
// 	return CellPropertyAccessor<S>();
// }

#endif
