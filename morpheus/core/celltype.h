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
#include "symbol.h"
#include "ClassFactory.h"

class CellIndexStorage {
public:
	CellIndexStorage() : free_cell_name(0) {};
	bool isFree(CPM::CELL_ID cell_id) { return used_cell_names.count(cell_id)==0;};
	CPM::CELL_ID  getFreeID() { return free_cell_name; };
	shared_ptr<Cell> addCell(shared_ptr<Cell>, CPM::INDEX);
	shared_ptr<Cell> removeCell(CPM::CELL_ID);
	shared_ptr<Cell> replaceCell(shared_ptr<Cell>, CPM::INDEX);
	Cell& cell(CPM::CELL_ID  cell_id) { assert( cell_id < cell_by_id.size() ); assert( cell_by_id[cell_id] ); return *cell_by_id[cell_id]; };
	CPM::INDEX& index(CPM::CELL_ID cell_id) { assert( cell_id < cell_index_by_id.size()); return cell_index_by_id[cell_id]; }
	CPM::INDEX emptyIndex();
	
private:
	CPM::CELL_ID free_cell_name;
	unordered_set<CPM::CELL_ID> used_cell_names;
	vector< CPM::INDEX > cell_index_by_id;
	vector< shared_ptr<Cell> > cell_by_id;
};

class CellIDSymbol : public SymbolAccessorBase<double> {
public:
	CellIDSymbol(string name) : SymbolAccessorBase<double>(name) {
		this->flags().granularity = Granularity::Cell;
		this->flags().integer = true;
	}
// 	set<SymbolDependency> dependencies() const override  { return set<SymbolDependency>()};
	double get(const SymbolFocus& f) const override { return f.cell().getID();}
	const string& description() const override { static const string descr = "Cell ID"; return descr;}
	string linkType() const override { return "CellIDLink"; }
};

class CellVolumeSymbol : public SymbolAccessorBase<double> {
public:
	CellVolumeSymbol(string name) : SymbolAccessorBase<double>(name) { this->flags().granularity=Granularity::Cell; }
// 	set<SymbolDependency> dependencies() const override;
	double get(const SymbolFocus& f) const override { return f.cell().nNodes();}
	const string& description() const override { static const string descr = "Cell Volume"; return descr; }
	string linkType() const override { return "CellVolumeLink"; }
};

class CellLengthSymbol : public SymbolAccessorBase<double> {
public:
	CellLengthSymbol(string name) : SymbolAccessorBase<double>(name) { this->flags().granularity=Granularity::Cell; }
// 	set<SymbolDependency> dependencies() const override;
	double get(const SymbolFocus& f) const override { return f.cell().getCellLength();}
	const string& description() const override { static const string descr =  "Cell Length";  return descr;}
	string linkType() const override { return "CellLengthLink"; }
};

class CellInterfaceLengthSymbol : public SymbolAccessorBase<double> {
public:
	CellInterfaceLengthSymbol(string name) : SymbolAccessorBase<double>(name) { this->flags().granularity=Granularity::Cell; }
// 	set<SymbolDependency> dependencies() const override;
	double get(const SymbolFocus& f) const override { return f.cell().getInterfaceLength();}
	const string& description() const override { static const string descr =  "Cell Interface Length";  return descr;}
	string linkType() const override { return "CellInterfaceLengthLink"; }
};

class CellCenterSymbol : public SymbolAccessorBase<VDOUBLE> {
public:
	CellCenterSymbol(string name) : SymbolAccessorBase<VDOUBLE>(name) { this->flags().granularity=Granularity::Cell; }
	TypeInfo<VDOUBLE>::SReturn get(const SymbolFocus& f) const override { return f.cell().getCenter();}
	const string& description() const override { static const string descr =  "Cell Center";  return descr;}
	string linkType() const override { return "CellCenterLink"; }
};

class CellOrientationSymbol : public SymbolAccessorBase<VDOUBLE> {
public:
	CellOrientationSymbol(string name) : SymbolAccessorBase<VDOUBLE>(name) { this->flags().granularity=Granularity::Cell; }
// 	set<SymbolDependency> dependencies() const override;
	TypeInfo<VDOUBLE>::SReturn get(const SymbolFocus& f) const override { return f.cell().getOrientation();}
	const string& description() const override { static const string descr =  "Cell Orientation";  return descr;}
	string linkType() const override { return "CellOrientationLink"; }
};

class CellPopulationSizeSymbol : public SymbolAccessorBase<double> {
public:
	CellPopulationSizeSymbol(string name, const CellType* ct) : SymbolAccessorBase<double>(name), celltype(ct) {
		this->flags().integer = true;
		descr =  string("\'") + name + "\' population size";
	};
	double get(const SymbolFocus&) const override;
	const string& description() const override { return descr;}
	string linkType() const override { return "CellPopulationSizeLink"; }
private:
	string descr; 
	const  CellType* celltype;
};

template <class T>
class PrimitiveProperty : public AbstractProperty {
public:
	PrimitiveProperty(string symbol, T value) : symbol_name(make_shared<string>(symbol)), value(value), buffer(value) {};
	PrimitiveProperty(const PrimitiveProperty<T>& other) : symbol_name(other.symbol_name), value(other.value), buffer(other.value) {};
	const string& symbol() const override { return *symbol_name; }
	const string& type() const override { return TypeInfo<T>::name(); }
	
	shared_ptr<AbstractProperty> clone() const override { return make_shared< PrimitiveProperty<T> >(*this); }
	void init(const SymbolFocus& ) override {};
	
	void assign(shared_ptr<AbstractProperty> other) override {
		auto derived = dynamic_pointer_cast<PrimitiveProperty<T>>(other);
		if (!derived)
			throw string("Type mismatch in property assignment");
		assign(derived);
	};
	void assign(shared_ptr< PrimitiveProperty<T> > other) {
		value = other->value;
		buffer = other->buffer;
	};
	
	string XMLDataName() const override { return type()+"PropertyData"; }
	void restoreData(XMLNode node) override {  
		if (XMLDataName() != string(node.getName())) {
			cout << "Warning: Property Data tagname mismatch: " << XMLDataName() << " != " << node.getName() << endl;
		}
		value = TypeInfo<T>::fromString(node.getAttribute("value"));
// 		value = TypeInfo<T>::fromString(node.getText());
	};
	XMLNode storeData() const override { 
		auto node = XMLNode::createXMLTopNode(XMLDataName().c_str());
		node.addAttribute("symbol-ref", symbol().c_str());
		node.addAttribute("value",TypeInfo<T>::toString(value).c_str());
// 		node.addText(to_cstr(value));
		return node;
	};
	
	shared_ptr<string> symbol_name;
	T value, buffer;
};

/** Primitive Symbol (accessor) for cell-attached Properties
 * 
 * The PrimitivePropertySymbol attaches cell Properties to a scope.
 * It provides all the necessary meta-information (type, constness, symbol name)
 * and the mediates access to the cell-attached Properties. 
 * 
 * XML snapshoting is not supported. See PropertySymbol for XML interfaced properties.
 */

template <class T>
class PrimitivePropertySymbol : public SymbolRWAccessorBase<T> {
public:
	PrimitivePropertySymbol(string symbol, const CellType* ct,  uint pid) : SymbolRWAccessorBase<T>(symbol), celltype(ct), property_id(pid) {
		this->flags().granularity = Granularity::Cell;
	}
	std::string linkType() const override { return "CellPropertyLink"; }
	const string& description() const override { return this->name(); }
	typename TypeInfo<T>::SReturn get(const SymbolFocus& f) const override { return getCellProperty(f)->value; }
	typename TypeInfo<T>::Reference getRef(const SymbolFocus& f) const { return getCellProperty(f)->value; }
// 		void init(const SymbolFocus&) const override {};
	void set(const SymbolFocus& f, typename TypeInfo<T>::Parameter value) const override { getCellProperty(f)->value = value; };
	void setBuffer(const SymbolFocus& f, typename TypeInfo<T>::Parameter value) const override { getCellProperty(f)->buffer = value; };
	void applyBuffer() const override;
	void applyBuffer(const SymbolFocus& f) const override {  getCellProperty(f)->value =  getCellProperty(f)->buffer; };
protected:
	PrimitiveProperty<T>* getCellProperty(const SymbolFocus& f) const;
	const CellType* celltype;
	uint property_id;
};


typedef  StaticClassFactoryP1 < string, CellType, uint > CellTypeFactory;
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
	virtual void loadFromXML(const XMLNode Node, Scope* scope);
	virtual void loadPlugins();
	virtual XMLNode savePopulationToXML() const;
	virtual void loadPopulationFromXML(const XMLNode Node);
	virtual void init();

// inline methods to provide fast private member access
	const string& getName() const { return name; };  ///< celltype name
// 	void setName(const string& n)  __attribute__ ((deprecated)) { name =n; };
	uint getID() const { return id; };               ///< celltype cpm id
	const Scope* getScope() const { return local_scope; };
	std::multimap< Plugin*, SymbolDependency > cpmDependSymbols() const;

	const vector< CPM::CELL_ID >& getCellIDs() const { return cell_ids; }

	const vector< shared_ptr<AbstractProperty> >& default_properties;

// 	
	uint addProperty(shared_ptr<AbstractProperty> property) {
		_default_properties.push_back(property);
		return _default_properties.size()-1;
	}
	
	template <class T>
	using PropertyAccessor = shared_ptr<PrimitivePropertySymbol<T>>;
	
	template <class T>
	PropertyAccessor<T> addProperty(string symbol, T value)  {
		uint pid = _default_properties.size();
		_default_properties.push_back( make_shared< PrimitiveProperty<T> >(symbol,value));
		return make_shared<PrimitivePropertySymbol<T> >(symbol,this,pid);
	}

	virtual CPM::CELL_ID  createCell(CPM::CELL_ID id  = storage.getFreeID() );     ///< Appends a new cell to the cell population
	virtual CPM::CELL_ID  createRandomCell();           ///< Appends a new cell to the cell population and places it at random position
	const Cell& getCell(CPM::CELL_ID id ){ return storage.cell( id ); }
	

	virtual CPM::CELL_ID addCell(CPM::CELL_ID  cell_id );               ///< Appends an existing cell to the cell population. Takes care of transfering all occupied nodes
	virtual void removeCell(CPM::CELL_ID cell_id);                      ///< Just unregisters the cell from the celltype
// 	virtual void changeCellType(uint cell_id,  CellType* new_cell_type);     ///< Removes cell cell_id from the cell population. The cell may not occupy and nodes.

	/** Splits the specified cell @p id along a given split plane and returns the id of the newly created daughter cell. 
	    By default, the cell is split perpendicular to its long axis, right trough the center of gravity;
	  */
	enum division{ MAJOR, MINOR, RANDOM, ORIENTED };
	pair<CPM::CELL_ID, CPM::CELL_ID> divideCell2(CPM::CELL_ID id, division d = CellType::MAJOR, VDOUBLE orientation=VDOUBLE(1,0,0) );
	pair<CPM::CELL_ID, CPM::CELL_ID> divideCell2(CPM::CELL_ID mother_id, VDOUBLE split_plane_normal,  VDOUBLE split_plane_center );
	

	virtual bool check_update(const CPM::Update& update) const;  ///<  the method is called previous to an cpm update and may prevent it by returning false.
	virtual double delta(const CPM::Update& update) const;               ///<  the method is called in the cpm, update to determine the energy change contributed by the celltype
	virtual double hamiltonian() const ;   ///< Calculates the Hamiltonian energy for the whole cellpopulation
	virtual void set_update(const CPM::Update& update);
	virtual void apply_update(const CPM::Update& update);        ///<  the method is called in the cpm update to apply an update to the celltype structure. Note that the lattice at that time still holds the old state.


	friend class SuperCell;
	friend class Cell;

private:
	uint id;
	string name;
	XMLNode stored_node;
	Scope* local_scope;

protected:
	// Plugins sorted by interface
	vector< shared_ptr<Plugin> > plugins;
	vector< shared_ptr<CPM_Energy> > energies;
	vector< shared_ptr<Cell_Update_Checker> > check_update_listener;
	vector< shared_ptr<Cell_Update_Listener> > update_listener;

	// Cell specific properties
	vector< shared_ptr<AbstractProperty> > _default_properties;

	// Cell populations
	vector< CPM::CELL_ID > cell_ids;
	struct InitPropertyDesc { string symbol; string expression; VecNotation notation=VecNotation::ORTH; };
	struct CellPopDesc {
		int pop_size;
		XMLNode xPopNode;
		vector<CPM::CELL_ID> cells;
		vector<InitPropertyDesc> property_initializers;
		vector< shared_ptr<Population_Initializer> > pop_initializers;
	};
	vector<CellPopDesc> cell_populations;
	
};



/// Medium Celltype :: that celltype contains always one single (medium)cell
class MediumCellType : virtual public CellType {

public:
	string XMLClassName() const override { return string("medium"); };
	
	bool isMedium() const override { return true; }
	static CellType* createInstance(uint ct_id);
	static bool factory_registration;


	MediumCellType(uint id);
	// Here we can make sure that the Medium has only one singe cell and disable node tracking
	CPM::CELL_ID createCell(CPM::CELL_ID name) override;
	virtual CPM::CELL_ID addCell(CPM::CELL_ID  cell_id) override;
	virtual void removeCell(CPM::CELL_ID  cell_id) override;
};




// /////////////////////////////////////////////////////////////////////
// // Implemntation of template functions

template <class T>
void PrimitivePropertySymbol<T>::applyBuffer() const  {
	auto cells = celltype->getCellIDs();
	for (auto cell : cells) {
		auto p = getCellProperty(SymbolFocus(cell));
		p->value = p->buffer;
	}
}

template <class T>
PrimitiveProperty<T>* PrimitivePropertySymbol<T>::getCellProperty(const SymbolFocus& f) const {
	return static_cast< PrimitiveProperty<T>* >(f.cell().properties[property_id].get());
}
#endif
