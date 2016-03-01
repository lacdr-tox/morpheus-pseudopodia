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

#ifndef CELLTYPE_H
#define CELLTYPE_H

#include "simulation.h"
#include "interfaces.h"
#include "cell.h"
#include "ClassFactory.h"
#include "cell_membrane_accessor.h"
#include "membrane_pde.h"

class CellIndexStorage {
public:
	CellIndexStorage() : free_cell_name(0) {};
	bool isFree(CPM::CELL_ID cell_id) { return used_cell_names.count(cell_id)==0;};
	CPM::CELL_ID  getFreeID() { return free_cell_name; };
	shared_ptr<Cell> addCell(shared_ptr<Cell>, CPM::INDEX);
	shared_ptr<Cell> removeCell(CPM::CELL_ID);
	Cell& cell(CPM::CELL_ID  cell_id) { assert( cell_id < cell_by_id.size() ); assert( cell_by_id[cell_id] ); return *cell_by_id[cell_id]; };
	CPM::INDEX& index(CPM::CELL_ID cell_id) { assert( cell_id < cell_index_by_id.size()); return cell_index_by_id[cell_id]; }
	CPM::INDEX emptyIndex();
	
private:
	CPM::CELL_ID free_cell_name;
	unordered_set<CPM::CELL_ID> used_cell_names;
	vector< CPM::INDEX > cell_index_by_id;
	vector< shared_ptr<Cell> > cell_by_id;
};


class CellMembraneAccessor;
// class SuperCell;
// template <class T> class SymbolAccessor;

typedef  CClassFactoryP1 < string, CellType, uint > CellTypeFactory;
#define registerCellType(CTClass) bool CTClass::factory_registration = CellTypeFactory::RegisterCreatorFunction(CTClass(0).XMLClassName(),CTClass::createInstance);


class CellType
{
	// NOTE: Each CT has to register at the CTRegistry and is instanciated on demand using its static createInstance method

public:
// The interface registering at the cpm platform ::
	static bool factory_registration;
	static CellType* createInstance(uint ct_id);

	static CellIndexStorage storage;
	
	virtual string XMLClassName() const { return string("biological"); };
	virtual bool isMedium() const { return false; }

// and here comes some basic functionality for the celltype
	CellType(uint ct_id);
	virtual ~CellType() { /*for (auto cell : cell_ids) { storage.removeCell(cell); } cell_ids.clear();*/ };

	virtual XMLNode saveToXML() const;
	virtual void loadFromXML(const XMLNode Node);
	virtual XMLNode savePopulationToXML() const;
	virtual void loadPopulationFromXML(const XMLNode Node);
	virtual void init();

// inline methods to provide fast private member access
	const string& getName() const { return name; };  ///< celltype name
// 	void setName(const string& n)  __attribute__ ((deprecated)) { name =n; };
	uint getID() const { return id; };               ///< celltype cpm id
	const Scope* getScope() const { return local_scope; };
	set<SymbolDependency> cpmDependSymbols() const;

	vector< CPM::CELL_ID > getCellIDs() const { return cell_ids; }
	int getPopulationSize() { return cell_ids.size(); }

	const vector< shared_ptr<AbstractProperty> >& default_properties;
	const vector< shared_ptr<PDE_Layer> >& default_membranes;
	// template methods may not be virtual and overridden by inherited classes 
	// thus we must pack all the power needed for subcellular compartments into this function !
	template <class T>
	CellPropertyAccessor<T> findCellProperty(string symbol, bool required = true) const;
	template <class T>
	CellPropertyAccessor<T> findCellPropertyByName(string name, bool required = true) const;
	
	template <class T>
	CellPropertyAccessor<T> addProperty(string symbol, string name, T default_value = T());
	
	CellMembraneAccessor findMembrane(string symbol, bool required = true) const;
// 	CellMembraneAccessor findMembraneByName(string symbol) const;

	virtual CPM::CELL_ID  createCell(CPM::CELL_ID id  = storage.getFreeID() );     ///< Appends a new cell to the cell population
// 	virtual  createVirtualCell( id  = storage.getFreeID() );
	virtual CPM::CELL_ID  createRandomCell();           ///< Appends a new cell to the cell population and places it at random position
	const Cell& getCell(CPM::CELL_ID id ){ return storage.cell( id ); }
	

	virtual CPM::CELL_ID addCell(CPM::CELL_ID  cell_id );               ///< Appends an existing cell to the cell population. Also takes care of transfering all occupied nodes
	virtual void removeCell(CPM::CELL_ID cell_id);
// 	virtual void changeCellType(uint cell_id,  CellType* new_cell_type);     ///< Removes cell cell_id from the cell population. The cell may not occupy and nodes.

	/** Splits the specified cell @p id along a given split plane and returns the id of the newly created daughter cell. 
	    By default, the cell is split perpendicular to its long axis, right trough the center of gravity;
	  */
	enum division{ MAJOR, MINOR, RANDOM };
	pair<CPM::CELL_ID, CPM::CELL_ID> divideCell2(CPM::CELL_ID id, division d = CellType::MAJOR);
	pair<CPM::CELL_ID, CPM::CELL_ID> divideCell2(CPM::CELL_ID mother_id, VDOUBLE split_plane_normal,  VDOUBLE split_plane_center );
	

	virtual bool check_update(const CPM::UPDATE& update, CPM::UPDATE_TODO todo) const;  ///<  the method is called previous to an cpm update and may prevent it by returning false.
	virtual double delta(const CPM::UPDATE& update, CPM::UPDATE_TODO todo) const;               ///<  the method is called in the cpm, update to determine the energy change contributed by the celltype
	virtual double hamiltonian() const ;   ///< Calculates the Hamiltonian energy for the whole cellpopulation
	virtual void set_update(const CPM::UPDATE& update, CPM::UPDATE_TODO todo);
	virtual void apply_update(const CPM::UPDATE& update, CPM::UPDATE_TODO todo);        ///<  the method is called in the cpm update to apply an update to the celltype structure. Note that the lattice at that time still holds the old state.

	/// Interface for time continuous processes
	/// TODO Those actions should somehow move to a central time sscheduler
// 	void doTimeStep(double current_time, double time_span);
// 	virtual void execute_once_each_mcs(int mcs);  ///<  called once each mcs ;-)

// 	friend ostream& operator<< (ostream& os, CellType& ct);
	friend class SuperCell;
	friend class Cell;

private:
	uint id;
	string name;
	
// 	void CreateCells(uint n);
protected:
	/** Creates a cell of the native cell class. Should be simply overridden by a subclass
	  * @param name unique name (number) assigned to the cell in the simulation
	  */
// 	void addCellProperty(Cell_Property* cp);
// 	static uint max_cell_name;
// 	static vector<  > cell_index_by_id;
// 	static vector< shared_ptr<Cell> > cell_by_id;
	
// 	vector< shared_ptr<Cell> > cells;
	vector< CPM::CELL_ID > cell_ids;

	Scope* local_scope;
	
	struct IntitPropertyDesc {string symbol; string expression;  } ;
	vector<IntitPropertyDesc> init_properties;
	vector< shared_ptr<Population_Initializer> > pop_initializers;
	int xml_pop_size;
	vector< shared_ptr<Plugin> > plugins;
	vector< shared_ptr<CPM_Energy> > energies;
	vector< shared_ptr<CPM_Check_Update> > check_update_listener;
	vector< shared_ptr<CPM_Update_Listener> > update_listener;
	vector< shared_ptr<TimeStepListener> > timestep_listener;
// 	vector< shared_ptr<Celltype_MCS_Listener> > mcs_listener;
	
	vector< shared_ptr<AbstractProperty> > _default_properties;
	map<string, uint > property_by_name;
	map<string, uint > property_by_symbol;

	vector< shared_ptr<PDE_Layer> > _default_membranePDEs;
// 	map<string, uint > membrane_by_name;
	map<string, uint > membrane_by_symbol;

	
	Linear_Lattice* membrane_lattice;
	
	// allow cellpropertyaccessors to access defaultproperties
// 	template <class S>
// 	friend class CellPropertyAccessor;
// 	friend class CellMembraneAccessor;

// 	Cell& getCell(uint i)  { return *(cells[i]); };
};



/// Medium Celltype :: that celltype contains always one single (medium)cell
class MediumCellType : virtual public CellType {

public:
	virtual string XMLClassName() const { return string("medium"); };
	virtual bool isMedium() const { return true; }
	static CellType* createInstance(uint ct_id);
	static bool factory_registration;


	MediumCellType(uint id);
	// Here we can make sure that the Medium has only one singe cell and disable node tracking
	virtual CPM::CELL_ID createCell(CPM::CELL_ID name);
	
	virtual CPM::CELL_ID addCell(CPM::CELL_ID  cell_id);
	virtual void removeCell(CPM::CELL_ID  cell_id);
};


// /////////////////////////////////////////////////////////////////////
// // Implemntation of template functions

#ifndef SEGMENTED_CT_H //skip that part of the declaration 
#include "cell_property_accessor.h"
#include "super_celltype.h"

// This is a template function and thus does cannot be virtual. Therefore findCellProperty
// has to deal with all cases, also if the celltype is a super_celltype;

template <class S>
CellPropertyAccessor<S> CellType::findCellProperty(string prop_symbol, bool required) const {
// 	cout << "findCellProperty: " << this->name << "  symbol " << prop_symbol << endl;
	bool found_wrong_type_matching = false;
	map<string,uint>::const_iterator prop_idx = property_by_symbol.find(prop_symbol);
	if (prop_idx != property_by_symbol.end() ) {
		if (TypeInfo<S>::name() == default_properties[prop_idx->second]->getTypeName()) {
			return CellPropertyAccessor<S>(this,prop_idx->second);
		}
		else {
			found_wrong_type_matching = true;
		}
	}
	else {
		auto cts = CPM::getCellTypes();
		// Find the connected SuperCellType ..., i.e. it's subcelltype is this
		for (uint i=0; i<cts.size(); i++) {
// 			cout << "	checking ct " << cts[i]->getName() << endl;
			shared_ptr<const SuperCT> sct = dynamic_pointer_cast<const SuperCT>(cts[i].lock());
			if ( sct) { 
// 				cout << "sct is a super_celltype" << endl;
				if (sct->getSubCelltype().get() == this) {
// 					cout << "Trying to obtain property " << prop_symbol << " from a SuperCellType " << sct->getName() << endl;
					return sct->findCellProperty<S>(prop_symbol, required);
				}
			}
		}
	}

	if (required && found_wrong_type_matching)
		throw(string("CellType[\"") + name + string("\"].findCellProperty: requested cellproperty [")+prop_symbol+string("] has different Type"));
	if (required && ! found_wrong_type_matching)
		throw (string("CellType[\"") + name + string("\"].findCellProperty: requested cellproperty [")+prop_symbol+string("] not found"));
	return CellPropertyAccessor<S>();
}

template <class S>
CellPropertyAccessor<S> CellType::findCellPropertyByName(string name, bool required) const {
	bool found_wrong_type_matching = false;
	map<string,uint>::const_iterator prop_idx = property_by_name.find(name);
	if (prop_idx != property_by_symbol.end() ) {
		if (Property<S>().getTypeName() == default_properties[prop_idx->second]->getTypeName()) {
			return CellPropertyAccessor<S>(this,prop_idx->second);
		}
		else {
			found_wrong_type_matching = true;
		}
	}

	if (required && found_wrong_type_matching)
		throw(string("CellType[\"") + name + string("\"].findCellProperty: requested cellproperty ")+name+string(" has different Type"));
	if (required && ! found_wrong_type_matching)
		throw (string("CellType[\"") + name + string("\"].findCellProperty: requested cellproperty ")+name+string(" not found"));
	return CellPropertyAccessor<S>();
}


template <class T>
CellPropertyAccessor<T> CellType::addProperty(string symbol, string name, T default_value) {
	
	// Assert that the symbol is unique
	// Since this method is unsed only internally, it should not matter too much if we rename the symbol ...
	string orig_symbol = symbol;
	uint count=0;
	while (property_by_symbol.find(symbol) != property_by_symbol.end()) {
		count++;
		symbol = orig_symbol+to_str(count);
		cout << count << endl;
	}
	
	shared_ptr<Property<T> > prop = Property<T>::createPropertyInstance(symbol, name);
	prop->set(default_value);
	
	uint prop_id = _default_properties.size();
	_default_properties.push_back(prop);
	
	property_by_name[name] = prop_id;
	

	property_by_symbol[symbol] = prop_id;
	
	for (uint i=0; i<cell_ids.size(); i++) {
		storage.cell(cell_ids[i]).p_properties.push_back(shared_ptr<AbstractProperty>(prop->clone()));
	}
	
	return CellPropertyAccessor<T>(this, prop_id);
}
	
#endif

#endif
