#include "symbol_accessor.h"


// Add INITIALISATION for all double only symbols
// This is called after standard initializer did not succeed
template <>
bool SymbolAccessorBase<double,ReadOnlyAccess>::init_special() {
	switch (data.link) {
		case SymbolData::VecXLink:
		case SymbolData::VecYLink:
		case SymbolData::VecZLink:
		case SymbolData::VecAbsLink:
		case SymbolData::VecPhiLink:
		case SymbolData::VecThetaLink: 
		{
			// remove trailing [.x|.y|.z|.abs]
			string vec_name = data.name.substr(0,data.name.find_last_of("."));
			vec = shared_ptr< SymbolAccessor<VDOUBLE> >(new SymbolAccessor<VDOUBLE>(scope->findSymbol<VDOUBLE>(vec_name)));
			// properly refer to the underlying container
			data.link = vec->getLinkType();
			data.granularity = vec->getGranularity();
			break;
		}
			
		case SymbolData::PDELink:
			pde_layer = SIM::findPDELayer(data.name);
			if (!pde_layer) throw(string("Unknown pdelayer " + data.name));
			break;
			
		case SymbolData::CellMembraneLink:
			cell_membrane =  scope->getCellType()->findMembrane(data.name);
			break;
		case SymbolData::FunctionLink:
			if (!data.func) throw(string("Missing function pointer"));
			break;
		case SymbolData::Time:
		case SymbolData::CellIDLink:
		case SymbolData::SubCellIDLink:
		case SymbolData::SuperCellIDLink:
		case SymbolData::CellTypeLink:
		case SymbolData::CellVolumeLink:
		case SymbolData::CellSurfaceLink:
		case SymbolData::CellLengthLink:
		case SymbolData::PopulationSizeLink:
			break;
		default:
			return false;
	}
	return true;
// 	} 
}

template <>
bool SymbolAccessorBase<double,ReadWriteAccess>::init_special() {
	switch (data.link) {
		case SymbolData::PDELink:
			pde_layer = SIM::findPDELayer(data.name);
			if (!pde_layer) throw(string("Unknown pdelayer " + data.name));
			break;
			
		case SymbolData::CellMembraneLink:
			cell_membrane =  scope->getCellType()->findMembrane(data.name);
			break;
		case SymbolData::Time:
		case SymbolData::CellIDLink:
		case SymbolData::SubCellIDLink:
		case SymbolData::SuperCellIDLink:
		case SymbolData::CellTypeLink:
		case SymbolData::CellVolumeLink:
		case SymbolData::CellSurfaceLink:
		case SymbolData::CellLengthLink:
		case SymbolData::PopulationSizeLink:
			throw string("Cannot create writable SymbolAccessor for Linktype ") + SymbolData::getLinkTypeName(data.link) + ",";
		default:
			return false;
	}
	return true;
}


template <>
TypeInfo<double>::SReturn SymbolAccessorBase<double,ReadOnlyAccess>::get(const SymbolFocus& focus) const {
	switch (this->internal_link) {
		case SymbolData::PureCompositeLink: 
			return component_accessors[focus.cell_index().celltype].get(focus);
		case SymbolData::GlobalLink:
			return global_value->get();
		case SymbolData::CellPropertyLink:
			return cell_property.get(focus);
		case SymbolData::PDELink:
			return pde_layer->get(focus.pos());
		case SymbolData::CellMembraneLink:
			return cell_membrane.get(focus);
		case SymbolData::FunctionLink:
			return data.func->get(focus);
		case SymbolData::VecXLink:
			return vec->get(focus).x;
		case SymbolData::VecYLink:
			return vec->get(focus).y;
		case SymbolData::VecZLink:
			return vec->get(focus).z;
		case SymbolData::VecAbsLink:
			return vec->get(focus).abs();
		case SymbolData::VecPhiLink:
			return vec->get(focus).angle_xy();
		case SymbolData::VecThetaLink:
			return vec->get(focus).to_radial().y;
		case SymbolData::Time:
			return SIM::getTime();
		case SymbolData::CellIDLink:
			return focus.cell().getID();
		case SymbolData::SubCellIDLink:
			return focus.cell_index().sub_cell_id;
		case SymbolData::SuperCellIDLink:
			return focus.cell_index().super_cell_id;
		case SymbolData::CellTypeLink:
			return focus.cell_index().celltype;
		case SymbolData::CellLengthLink:
				return focus.cell().getCellLength();
		case SymbolData::CellVolumeLink:
				return focus.cell().nNodes();
		case SymbolData::CellSurfaceLink:
				return focus.cell().getInterfaceLength();
		case SymbolData::PopulationSizeLink:
			return data.celltype.lock()->getPopulationSize();
		case SymbolData::UnLinked:
			throw SymbolError(SymbolError::Type::Undefined, "Access to unlinked symbol");
		default:
			throw SymbolError(SymbolError::Type::InvalidLink, string("SymbolAccessor: Link type '") + this->data.getLinkTypeName() + "' is not defined for type "  + TypeInfo<ValType>::name());
	}
}


template <>
TypeInfo<double>::SReturn SymbolAccessorBase<double,ReadWriteAccess>::get(const SymbolFocus& focus) const {
	switch (this->internal_link) {
		case SymbolData::PureCompositeLink: 
			return component_accessors[focus.cell_index().celltype].get(focus);
		case SymbolData::GlobalLink:
			return global_value->get();
		case SymbolData::CellPropertyLink:
			return cell_property.get(focus);
		case SymbolData::PDELink:
			return pde_layer->get(focus.pos());
		case SymbolData::CellMembraneLink:
			return cell_membrane.get(focus);
		case SymbolData::UnLinked:
			throw SymbolError(SymbolError::Type::Undefined, "Access to unlinked symbol");
		default:
			throw SymbolError(SymbolError::Type::InvalidLink, string("SymbolAccessor: Link type '") + this->data.getLinkTypeName() + "' is not defined for type " + TypeInfo<ValType>::name() );
	}
}

template <>
bool SymbolRWAccessor<double>::set(const SymbolFocus& focus, TypeInfo<double>::Parameter  value) const
{
	switch (internal_link) {
		case SymbolData::PureCompositeLink :
			return component_accessors[focus.cell_index().celltype].set(focus,value);
		case SymbolData::GlobalLink:
			global_value->getRef() = value;
			return true;
		case SymbolData::CellPropertyLink: 
		{
			return cell_property.set(focus, value);
		}
		case SymbolData::PDELink:
		{
			return pde_layer->set(focus.pos(), value);
		}
		case SymbolData::CellMembraneLink: 
		{
			return cell_membrane.set(focus, value);
		}
		case SymbolData::UnLinked:
			throw SymbolError(SymbolError::Type::Undefined, "Access to unlinked symbol");
		default:
			throw SymbolError(SymbolError::Type::InvalidLink, string("SymbolAccessor: Link type '") + this->data.getLinkTypeName() + "' is not defined for type " + TypeInfo<ValType>::name() );
		
	}
}

template <>
bool SymbolRWAccessor<double>::setBuffer(const SymbolFocus& focus, TypeInfo<double>::Parameter value) const {

	switch (internal_link) {
		case SymbolData::PureCompositeLink :
			return component_accessors[focus.cell_index().celltype].set(focus,value);
		case SymbolData::CellPropertyLink:
			return cell_property.setBuffer(focus,value);
		case SymbolData::PDELink:
		{
			pde_layer->setBuffer(focus.pos(), value);
			return true;
		}
		case SymbolData::CellMembraneLink:
			return cell_membrane.setBuffer(focus, value);

		case SymbolData::GlobalLink:
			global_value->setBuffer(value);
			return true;
		case SymbolData::UnLinked:
			throw SymbolError(SymbolError::Type::Undefined, "Write access to unlinked symbol");
		default:
			throw SymbolError(SymbolError::Type::InvalidLink, string("SymbolAccessor: Link type '") + this->data.getLinkTypeName() + "' is not defined for type " + TypeInfo<ValType>::name() );
	}
}

template <>
bool SymbolRWAccessor<double>::swapBuffer(const SymbolFocus& f) const {
	
	switch (internal_link) {
		case SymbolData::PureCompositeLink:
		{
			// TODO: the default value reference should only be considered once, not for any component it is registered
			// This is now done, but somewhat ugly ...
			bool default_swapped = false;
			for (auto it : component_accessors) {
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
		
		case SymbolData::CellPropertyLink:
			cell_property.swapBuffer(f);
			return true;

		case SymbolData::PDELink:
		{
			pde_layer->applyBuffer(f.pos());
			return true;
		}
		case SymbolData::CellMembraneLink:
			cell_membrane.swapBuffers(f);
			return true;

		case SymbolData::GlobalLink:
			global_value->applyBuffer();
			return true;
		case SymbolData::UnLinked:
			throw SymbolError(SymbolError::Type::Undefined, "Write access to unlinked symbol");
		default:
			throw SymbolError(SymbolError::Type::InvalidLink, string("SymbolAccessor: Link type '") + this->data.getLinkTypeName() + "' is not defined for type " + TypeInfo<ValType>::name() );

	}
}

template <>
bool SymbolRWAccessor<double>::swapBuffer() const {
	if (!data.writable) {
		cerr << "Cannot swap buffers of a non-writable symbol " << data.name << endl;
		return false;
	}
	
	switch (internal_link) {
		case SymbolData::PureCompositeLink:
		{
			// TODO: the default value reference should only be considered once, not for any component it is registered
			// This is now done, but somewhat ugly ...
			bool default_swapped = false;
			for (auto it : component_accessors) {
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
		case SymbolData::CellPropertyLink:
			cell_property.swapBuffer();
			return true;

		case SymbolData::PDELink:
		{
			pde_layer->swapBuffer();
			return true;
		}
		case SymbolData::CellMembraneLink:
			cell_membrane.swapBuffers();
			return true;

		case SymbolData::GlobalLink:
			global_value->applyBuffer();
			return true;

		case SymbolData::UnLinked:
			throw SymbolError(SymbolError::Type::Undefined, "Write access to unlinked symbol");
		default:
			throw SymbolError(SymbolError::Type::InvalidLink, string("SymbolAccessor: Link type '") + this->data.getLinkTypeName() + "' is not defined for type " + TypeInfo<ValType>::name() );

	}
}

// Add INITIALISATION for all VDOUBLE only symbols
// This is called after standard initializer did not succeed
template <>
bool SymbolAccessorBase<VDOUBLE, ReadOnlyAccess>::init_special() {
	switch (data.link) {
		case SymbolData::Space:
			// some lattice geometry properties ...
			lattice = SIM::getLattice().get();
			periodic.x = (lattice->get_boundary_type(Boundary::mx) == Boundary::periodic);
			periodic.y = (lattice->get_boundary_type(Boundary::my) == Boundary::periodic);
			periodic.z = (lattice->get_boundary_type(Boundary::mz) == Boundary::periodic);
			orth_size = lattice->size();
			orth_size.y = lattice->to_orth( VDOUBLE(0, lattice->size().y, 0) ).y;
			orth_size.z = lattice->to_orth( VDOUBLE(0, 0, lattice->size().z) ).z;
			break;
		case SymbolData::MembraneSpace:
			lattice = MembraneProperty::lattice().get();
			mem_scale.x = 2.0 * M_PI / lattice->size().x;
			mem_scale.y = 1.0 * M_PI / lattice->size().y;
			mem_scale.z = 1.0;
			break;
		case SymbolData::VectorFunctionLink:
			if (data.vec_func) throw ("Missing VectorFunction in Symbol");
			break;
		case SymbolData::CellCenterLink:
			break;
		case SymbolData::CellOrientationLink:
			break;
		default:
			return false;
	}
	return true;
}



// GETTERS
template <>
TypeInfo<VDOUBLE>::SReturn SymbolAccessorBase<VDOUBLE,ReadOnlyAccess>::get(const SymbolFocus& f) const {
	switch (internal_link) {
		case SymbolData::PureCompositeLink :
			return component_accessors[f.cell_index().celltype].get(f);
		case SymbolData::GlobalLink:
			return global_value->getRef();
		case SymbolData::CellPropertyLink:
			return cell_property.get(f);
		case SymbolData::VectorFunctionLink:
			return data.vec_func->get(f);
		case SymbolData::Space:
		{
			VDOUBLE orth_pos = lattice->to_orth(f.pos());
			if (periodic.x) orth_pos.x = MOD(orth_pos.x,orth_size.x);
			if (periodic.y) orth_pos.y = MOD(orth_pos.y,orth_size.y);
			if (periodic.z) orth_pos.z = MOD(orth_pos.z,orth_size.z);
			return orth_pos;
		}
		case SymbolData::MembraneSpace: {
			return MembraneProperty::memPosToOrientation(f.membrane_pos());;
		}
		case SymbolData::CellCenterLink:
			return f.cell().getCenter();
		case SymbolData::CellOrientationLink:
			return f.cell().getMajorAxis().norm();
		case SymbolData::UnLinked:
			throw SymbolError(SymbolError::Type::Undefined, "Access to unlinked symbol");
		default:
			throw SymbolError(SymbolError::Type::InvalidLink, string("SymbolAccessor: Link type '") + this->data.getLinkTypeName() + "' is not defined for type " + TypeInfo<ValType>::name() );

	}
}
