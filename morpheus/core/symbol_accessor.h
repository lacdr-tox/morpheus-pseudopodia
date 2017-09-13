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

#ifndef SYMBOL_ACCESSOR_H
#define SYMBOL_ACCESSOR_H

#include "simulation.h"
#include "symbol.h"
#include "symbolfocus.h"
#include "scope.h"
#include "cell.h"
#include "celltype.h"


template <class T>
class ReadOnlyAccess {
public:
	typedef SymbolAccessor<T> Accessor;
	static Accessor findSymbol(const Scope* scope, string name) { return scope->findSymbol<T>(name); };
};

template <class T>
class ReadWriteAccess {
public:
	typedef SymbolRWAccessor<T> Accessor;
	static Accessor findSymbol(const Scope* scope, string name) { return scope->findRWSymbol<T>(name); };
};


template <class S, template <class> class AccessPolicy>
class SymbolAccessorBase {
	public:
		typedef S ValType;
		
		/** Default constructor for an UnLinked symbol */
		SymbolAccessorBase();
		
		/** Create a SymbolAccessor for the symbol specified by d within a scope. Partial availability in the scope can be allowed by @partial_spec.
		 */
		SymbolAccessorBase(SymbolData d, const Scope* scope, bool partial_spec = false);
		
		/** Create a read-only SymbolAccessor, that returns default_val for any scope wherein the symbol is not defined.
		 * 
		 * Main purpose is to allow output generators to have a simplified interface.
		 */
		SymbolAccessorBase(SymbolData d, const Scope* scope, const ValType& default_val);
		
		/// Get the scope of the symbol (wherein it is defined)
		const Scope* getScope() const { return scope; };
		
		
		Granularity getGranularity() const {
			// TODO Check this for virtual symbols and forwarding vector symbols
			if ( ! data.is_composite) 
				return data.granularity; 
			else {
				// TODO: Do something even more useful here !!
				// Minimal Granularity cell, since the symbol differs per celltype
				Granularity gran = Granularity::Cell;
				for (auto& subscope : component_accessors) {
					gran+= subscope.getGranularity();
				}
				return gran;
			}
		};
		
		multimap<FocusRangeAxis,int> getRestrictions() const {
			multimap<FocusRangeAxis,int> r;
			if (scope->getCellType()) {
				r.insert(make_pair(FocusRangeAxis::CellType,scope->getCellType()->getID()));
			}
			
			if (internal_link==SymbolData::PDELink) {
				auto field_size = pde_layer->getWritableSize();
				auto lattice_size = SIM::lattice().size();
				if (field_size.x == 1 && lattice_size.x>1) {
					r.insert(make_pair(FocusRangeAxis::X,1));
				}
				if (field_size.y == 1 && lattice_size.y>1) {
					r.insert(make_pair(FocusRangeAxis::Y,1));
				}
				if (field_size.z == 1 && lattice_size.z>1) {
					r.insert(make_pair(FocusRangeAxis::Z,1));
				}
			}
			
			return r;
		}
		
		typename TypeInfo<S>::SReturn get(CPM::CELL_ID cell_id) const;
		typename TypeInfo<S>::SReturn get(const VINT& pos) const;
		typename TypeInfo<S>::SReturn get(CPM::CELL_ID cell_id, const VINT& pos) const;
		typename TypeInfo<S>::SReturn get(const SymbolFocus& focus) const;
		typename TypeInfo<S>::SReturn operator ()(const SymbolFocus& focus) const { return get(focus); }

		const string& getName() const { return data.name; }  /// Returns the name of the symbol itself.
		const string& getBaseName() const; /// Returns the Base name of the symbol, if it's a derived symbol, or the symbol name in any other case.
		const string& getFullName() const; /// Returns the full name of the symbol. If this is empty, returns the name of the symbol itself.
		bool isInteger() const { return data.integer; };
		bool isConst() const { return data.invariant; }
		SymbolData::LinkType getLinkType() const  { return data.link; }
		bool isComposite() const { return data.is_composite; }
		bool valid() const;
		
		/// Tells whether the symbol is defined for CellType ct
		bool isDefined(CellType * ct);
		/// Tells whether the symbol is defined for Cell cell
		bool isDefined(CPM::CELL_ID cell);
		
		// valarray<double> getAll();
	protected:
		
		bool default_is_set;
		SymbolData set_default_val (SymbolData data , const ValType& default_val);
		
		// Initialisation for symbols available in all types
		// Forward initialsation to init_typed() if generic linking does not resolve
		 void init_all();
		
		// Initialisation for smybols available only for particular types
		bool init_special();
		
		SymbolData::LinkType internal_link;
		SymbolData data;
		const Scope* scope;
		
// typed properties, which do not fit into SymbolData()
		shared_ptr< Property<S> > global_value;
		CellPropertyAccessor<S> cell_property;
		CellMembraneAccessor cell_membrane;
		
		vector< typename AccessPolicy<S>::Accessor > component_accessors;
		bool allow_partial_spec;
		shared_ptr<PDE_Layer> pde_layer;
		shared_ptr<VectorField_Layer> vector_field_layer;

		const Lattice* lattice;
		VDOUBLE orth_size, mem_scale;
		struct { bool x; bool y; bool z; } periodic;
		
		/**	Link to a referred Vector property. 
		 *	Currently applies only to double type symbols */
// 		shared_ptr< typename vec_for_double<S>::type > vec;
		shared_ptr< SymbolAccessor<VDOUBLE> > vec;
		
		
		// allow the ODESystem access to all symbol interna
		template <SystemType s> friend class System;
		friend class Diffusion;
		friend class Mapper;
		friend class CellReporter;
		friend class FocusRange;
};

template <class T>
using SymbolAccessor = SymbolAccessorBase<T, ReadOnlyAccess> ;


template <class S>
class SymbolRWAccessor : public SymbolAccessorBase<S,ReadWriteAccess> {
	public: 
		SymbolRWAccessor() : SymbolAccessorBase<S,ReadWriteAccess>() {};
		SymbolRWAccessor(SymbolData d, const Scope* scope); // check that the referred symbol is writable;
		bool set(CPM::CELL_ID cell_id, typename TypeInfo<S>::Parameter v) const;
		bool set(const VINT& pos, typename TypeInfo<S>::Parameter v) const;
		bool set(CPM::CELL_ID cell_id, const VINT& pos, typename TypeInfo<S>::Parameter v) const;
		bool set(const SymbolFocus& focus, typename TypeInfo<S>::Parameter v) const;
		bool setBuffer(const SymbolFocus& focus, typename TypeInfo<S>::Parameter v) const;

		// swaps the data buffer in the underlying container
		bool swapBuffer() const;
		bool swapBuffer(const SymbolFocus& f) const;
};


// For the implementation, we need all the referred types fully specified
// Thus we import all their dependencies here
#include "function.h"

// this is the default constructor 
template <class S,template <class> class AccessPolicy>
SymbolAccessorBase<S,AccessPolicy>::SymbolAccessorBase() {
	data.link = SymbolData::UnLinked;
	scope = NULL;
	internal_link = SymbolData::UnLinked;
	allow_partial_spec = false;
}

template <class S,template <class> class AccessPolicy>
SymbolAccessorBase<S,AccessPolicy>::SymbolAccessorBase(SymbolData d, const Scope* scope, const ValType& default_val) : data(d), scope(scope) {
	if (data.link == SymbolData::PureCompositeLink) {
		// Create a global default value ...
		auto prop = Property<S>::createConstantInstance(data.name,data.fullname);
		prop->set(default_val);
		data.const_prop = prop;
		default_is_set = true;
		data.link = SymbolData::GlobalLink;
	}
	allow_partial_spec = false;
	init_all();
};

// this constructor is used for all non-double and non-VDOUBLE symbols
template <class S,template <class> class AccessPolicy>
SymbolAccessorBase<S,AccessPolicy>::SymbolAccessorBase(SymbolData d, const Scope* scope, bool partial_spec) : data(d), scope(scope)
{
	allow_partial_spec = partial_spec;
	init_all();
}

template <class S,template <class> class AccessPolicy>
void SymbolAccessorBase<S,AccessPolicy>::init_all()
{
	internal_link = data.link;
	
	if ( data.is_composite ) {
		
		data.invariant = false;
		component_accessors.resize(CPM::getCellTypes().size());
		data.component_subscopes.resize(component_accessors.size(), NULL);
		// Create a virtual accessor that forwards to the sub-scope specific accessor 
		if ( internal_link != SymbolData::PureCompositeLink ) {
			// A global value exists, but is overridden in some of the spatial subscopes
// 			assert(0); 
			// we create a default symbol accessor based on the current scope data
			SymbolData default_data = data;
			default_data.is_composite = false;
			default_data.component_subscopes.clear();
			typename AccessPolicy<S>::Accessor default_sym(default_data,scope);
			
			for (uint ct_id=0; ct_id<component_accessors.size(); ct_id++) {
				if (data.component_subscopes.size()>ct_id && data.component_subscopes[ct_id]) {
					component_accessors[ct_id] = AccessPolicy<S>::findSymbol(data.component_subscopes[ct_id],data.name);
				}
				else {
					component_accessors[ct_id] = default_sym;
				}
			}
			internal_link = SymbolData::PureCompositeLink;
			
		}
		else {
			for (uint ct_id=0; ct_id<component_accessors.size(); ct_id++) {
				if ( !data.component_subscopes[ct_id]) {
					if (allow_partial_spec)
						component_accessors[ct_id] = typename AccessPolicy<S>::Accessor();
					else 
						throw SymbolError(SymbolError::Type::InvalidPartialSpec, string("Global default for symbol \"") + data.name + "\" is missing." );
				}
				else 
					component_accessors[ct_id] = AccessPolicy<S>::findSymbol(data.component_subscopes[ct_id],data.name);
			}
		}
	}
	else {
		try {
			switch (data.link) {
				case SymbolData::GlobalLink:
					if ( ! dynamic_pointer_cast< Property<S> >(data.const_prop) )
						throw(string("Type of constant symbol value differs!"));
					global_value = dynamic_pointer_cast< Property<S> >(data.const_prop);
					break;
				case SymbolData::CellPropertyLink:
					cell_property = scope->getCellType()->findCellProperty<S>(data.name,true);
					break;
				default:
					if ( ! init_special() ) 
						throw(string("Symbol \"" + data.name + "\" does not refer to a "+data.getLinkTypeName()+". ") );
						//throw(string("SymbolAccessor: Not implemented for linktype = ") + data.getLinkTypeName());
			}
		} 
		catch(string e) {
			throw SymbolError(SymbolError::Type::InvalidLink, string("SymbolAccessor<") + data.type_name + ">:. Error while linking Symbol: '" + data.name + "'\n"  + e);
		}
	}
}

template <class S,template <class> class AccessPolicy>
bool SymbolAccessorBase<S,AccessPolicy>::init_special() { return false; };


template <class S,template <class> class AccessPolicy>
const string&SymbolAccessorBase<S,AccessPolicy>::getFullName() const{
	if (!data.fullname.empty()) return data.fullname;
	switch (internal_link) {
		case SymbolData::PureCompositeLink:
			for ( const auto&  ac : component_accessors) {
				if (!ac.data.fullname.empty()) {
					return ac.data.fullname;
				}
			}
			return data.name;
		case SymbolData::VecXLink:
		case SymbolData::VecYLink:
		case SymbolData::VecZLink:
		case SymbolData::VecPhiLink:
		case SymbolData::VecThetaLink:
		case SymbolData::VecAbsLink:
			return vec->getFullName();
		default:
			return data.name;
	}
}

template <class S,template <class> class AccessPolicy>
const string& SymbolAccessorBase<S,AccessPolicy>::getBaseName() const { return data.base_name; }

template <class S,template <class> class AccessPolicy>
bool SymbolAccessorBase<S,AccessPolicy>::valid () const
{
	return internal_link != SymbolData::UnLinked;
}

template <class S,template <class> class AccessPolicy>
bool SymbolAccessorBase<S,AccessPolicy>::isDefined(CellType* ct)
{
	if (internal_link == SymbolData::PureCompositeLink && default_is_set) {
		if ( ! data.component_subscopes[ct->getID()] ) 
			return false;
	}
	return true;
}

template <class S,template <class> class AccessPolicy>
bool SymbolAccessorBase<S,AccessPolicy>::isDefined(CPM::CELL_ID cell)
{
	if (internal_link == SymbolData::PureCompositeLink && default_is_set) {
		if ( ! data.component_subscopes[CPM::getCellIndex(cell).celltype] ) 
			return false;
	}
	return true;
}


// GETTERS
template <class S,template <class> class AccessPolicy>
typename TypeInfo<S>::SReturn SymbolAccessorBase<S,AccessPolicy>::get(CPM::CELL_ID cell_id) const {
	return get(SymbolFocus(cell_id));
}


template <class S,template <class> class AccessPolicy>
typename TypeInfo<S>::SReturn SymbolAccessorBase<S,AccessPolicy>::get(CPM::CELL_ID cell_id, const VINT& pos) const 
{
	return get(SymbolFocus(cell_id,pos));
}

template <class S,template <class> class AccessPolicy>
typename TypeInfo<S>::SReturn SymbolAccessorBase<S,AccessPolicy>::get(const VINT& pos) const
{
	return get(SymbolFocus(pos));
}

template <class S,template <class> class AccessPolicy>
typename TypeInfo<S>::SReturn SymbolAccessorBase<S,AccessPolicy>::get(const SymbolFocus& focus) const
{
	switch (internal_link) {
		
		case SymbolData::PureCompositeLink:
			// forwarding
			return component_accessors[focus.cell_index().celltype].get(focus);
			
		case SymbolData::GlobalLink:
			return global_value->getRef();
			
		case SymbolData::CellPropertyLink:
			return cell_property.get(focus);
			
		case SymbolData::UnLinked:
			throw SymbolError(SymbolError::Type::Undefined, "Access to unlinked symbol");
			
		default:
			throw SymbolError(SymbolError::Type::InvalidLink, string("SymbolAccessor: Link type '") + data.getLinkTypeName() + "' is not defined for type " + TypeInfo<S>::name() );
	}
}


template <class S>
SymbolRWAccessor<S>::SymbolRWAccessor(SymbolData d, const Scope* scope) : SymbolAccessorBase<S,ReadWriteAccess>(d, scope)
{
	if ( ! d.writable ) {
		throw string("SymbolRWAccsessor: Cannot create writable accessor to non-writable Symbol ") + d.name + " of link type " + d.getLinkTypeName();
		exit(-1);
	}
	this->init_all();
}

// SETTERS
template <class S>
bool SymbolRWAccessor<S>::set(CPM::CELL_ID cell_id, typename TypeInfo<S>::Parameter value) const 
{
	return set(SymbolFocus(cell_id),value);
}

template <class S>
bool SymbolRWAccessor<S>::set(CPM::CELL_ID cell_id, const VINT& pos, typename TypeInfo<S>::Parameter value)  const
{
	return set(SymbolFocus(cell_id,pos),value);
}

template <class S>
bool SymbolRWAccessor<S>::set(const VINT& pos, typename TypeInfo<S>::Parameter value)  const
{
	return set(SymbolFocus(pos),value);
}

template <class S>
bool SymbolRWAccessor<S>::set(const SymbolFocus& f, typename TypeInfo<S>::Parameter  value)  const
{
	switch (this->internal_link) {
		case SymbolData::PureCompositeLink :
			return this->component_accessors[f.cell_index().celltype].set(f,value);
			
		case SymbolData::GlobalLink:
			this->global_value->getRef() = value;
			return true;
			
		case SymbolData::CellPropertyLink: 
			return this->cell_property.set(f, value);
		
		case SymbolData::UnLinked:
			throw SymbolError(SymbolError::Type::Undefined, "Write access to unlinked symbol");
			
		default:
			throw SymbolError(SymbolError::Type::InvalidLink, string("SymbolAccessor: Link type '") + this->data.getLinkTypeName() + "' is not defined for type " + TypeInfo<S>::name() );
	}
}




template <class S>
bool SymbolRWAccessor<S>::setBuffer(const SymbolFocus& f, typename TypeInfo<S>::Parameter  value) const
{
	switch (this->internal_link) {
		case SymbolData::PureCompositeLink :
			return this->component_accessors[f.cell_index().celltype].setBuffer(f,value);
			
		case SymbolData::GlobalLink:
			this->global_value->setBuffer(value);
			return true;
			
		case SymbolData::CellPropertyLink:
		{
			return this->cell_property.setBuffer(f, value);
		}
		
		case SymbolData::UnLinked:
			throw SymbolError(SymbolError::Type::Undefined, "Write access to unlinked symbol");
			
		default:
			throw SymbolError(SymbolError::Type::InvalidLink, string("SymbolAccessor: Link type '") + this->data.getLinkTypeName() + "' is not defined for type " + TypeInfo<S>::name() );
	}
}

template <class S>
bool SymbolRWAccessor<S>::swapBuffer(const SymbolFocus& f) const
{
	switch (this->internal_link) {
		case SymbolData::PureCompositeLink :
		{
			// TODO: the default value reference should only be considered once, not for any component it is registered
			// This is now done, but somewhat ugly ...
			bool default_swapped = false;
			for (auto it : this->component_accessors) {
				if (it.getScope() == this->getScope() ) {
					if (!default_swapped) {
						it.swapBuffer(f);
						default_swapped = true;
					}
				}
				else {
					it.swapBuffer(f);
				}
			}
		}
		case SymbolData::GlobalLink:
			this->global_value->applyBuffer();
			return true;
			
		case SymbolData::CellPropertyLink:
			return this->cell_property.swapBuffer(f);
		
		case SymbolData::UnLinked:
			throw SymbolError(SymbolError::Type::Undefined, "Write access to unlinked symbol");
			
		default:
			throw SymbolError(SymbolError::Type::InvalidLink, string("SymbolAccessor: Link type '") + this->data.getLinkTypeName() + "' is not defined for type " + TypeInfo<S>::name() );
	}
}

template <class S>
bool SymbolRWAccessor<S>::swapBuffer() const 
{
	switch (this->internal_link) {
		
		case SymbolData::PureCompositeLink :
		{
			// TODO: the default value reference should only be considered once, not for any component it is registered
			// This is now done, but somewhat ugly ...
			bool default_swapped = false;
			for (auto it : this->component_accessors) {
				if (it.getScope() == this->getScope() ) {
					if (!default_swapped) {
						it.swapBuffer();
						default_swapped = true;
					}
				}
				else {
					it.swapBuffer();
				}
			}
		}
		
		case SymbolData::GlobalLink:
			this->global_value->applyBuffer();
			return true;
			
		case SymbolData::CellPropertyLink:
			this->cell_property.swapBuffer();
			return true;
			
		case SymbolData::UnLinked:
			throw SymbolError(SymbolError::Type::Undefined, "Write access to unlinked symbol");
			
		default:
			throw SymbolError(SymbolError::Type::InvalidLink, string("SymbolAccessor: Link type '") + this->data.getLinkTypeName() + "' is not defined for type " + TypeInfo<S>::name() );
	}
}

///////////////////////////////////////////777
// 			SPECIAL CASE VDOUBLE
///////////////////////////////////////////777

// INITIALISATION
// template<>
template <>
bool SymbolAccessorBase<VDOUBLE, ReadOnlyAccess>::init_special();

template <>
bool SymbolAccessorBase<VDOUBLE, ReadWriteAccess>::init_special();

// GETTERS
template <>
TypeInfo<VDOUBLE>::SReturn SymbolAccessorBase<VDOUBLE,ReadOnlyAccess>::get(const SymbolFocus& f) const;

// SETTERS
template <>
bool SymbolRWAccessor<VDOUBLE>::set(const SymbolFocus& f, TypeInfo<VDOUBLE>::Parameter value) const;

///////////////////////////////////////////777
// 			SPECIAL CASE double
///////////////////////////////////////////777

// INITIALISATION
template <>
bool SymbolAccessorBase<double,ReadOnlyAccess>::init_special();

template <>
bool SymbolAccessorBase<double,ReadWriteAccess>::init_special();

// GETTERS
template <>
TypeInfo<double>::SReturn SymbolAccessorBase<double,ReadOnlyAccess>::get(const SymbolFocus& f) const;

template <>
TypeInfo<double>::SReturn SymbolAccessorBase<double,ReadWriteAccess>::get(const SymbolFocus& f) const;

// SETTERS

template <>
bool SymbolRWAccessor<double>::set(const SymbolFocus& f, TypeInfo<double>::Parameter value) const;

template <>
bool SymbolRWAccessor<double>::setBuffer(const SymbolFocus& focus, TypeInfo<double>::Parameter v) const;

template <>
bool SymbolRWAccessor<double>::swapBuffer(const SymbolFocus& f) const;

template <>
bool SymbolRWAccessor<double>::swapBuffer() const;

#endif // SYMBOL_ACCESSOR_H

