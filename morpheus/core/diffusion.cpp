#include "diffusion.h"


Diffusion::Diffusion(Symbol field): ContinuousProcessPlugin(ContinuousProcessPlugin::INDEPEND, XMLSpec::XML_NONE)
{
	pde_field = dynamic_pointer_cast<const Field::Symbol>(field);
	mem_field = dynamic_pointer_cast<const MembranePropertySymbol>(field);
}

void Diffusion::init(const Scope* scope)
{
	ContinuousProcessPlugin::init(scope);
	
	if (SIM::lattice().getDimensions()==3)
		membrane_length = &membrane_length_3d;
	else if (SIM::lattice().getDimensions()==2)
		membrane_length = &membrane_length_2d;
	else
		membrane_length = NULL;
	
	
	if (pde_field) {
			registerInputSymbol(pde_field);
			registerOutputSymbol(pde_field);
			setTimeStep(pde_field->getField()->getMaxTimeStep());
			cout << "Max diffusion step is " << pde_field->getField()->getMaxTimeStep() << endl;
	}
	else if (mem_field) {
			registerInputSymbol(mem_field);
			registerOutputSymbol(mem_field);
	}
	is_adjustable = true;
}

double Diffusion::membrane_length_2d(double area)
{
	const double prefactor_volume_to_circumference = 2 * sqrt(M_PI);
	return prefactor_volume_to_circumference * sqrt(area);
}

double Diffusion::membrane_length_3d(double volume)
{
	const double prefactor_volume_to_circumference = pow(6.0*sqr(M_PI), (1.0/3.0));
	return prefactor_volume_to_circumference * pow(volume, (1.0/3.0));
}


void Diffusion::executeTimeStep()
{
	if (pde_field) {
		pde_field->getField()->doDiffusion(timeStep());
	}
	else if (mem_field) {
		
		double physical_node_length = SIM::getNodeLength();
		FocusRange range(Granularity::Cell, mem_field->scope());
#pragma omp parallel for schedule(dynamic)
		for ( auto f=range.begin(); f < range.end(); ++f) {
				uint cell_volume = f->cell().nNodes();
				double spherical_circumference = membrane_length(cell_volume);
				double node_length_along_equator = (spherical_circumference * physical_node_length) / MembraneProperty::getResolution();
				if (node_length_along_equator == 0)
					continue;
				auto field = mem_field->getField(*f);
				field->updateNodeLength( node_length_along_equator );
				field->doDiffusion(timeStep());
		}
	}
}

string Diffusion::XMLName() const
{
	return "Diffusion";
}

XMLNode Diffusion::saveToXML() const
{
    return XMLNode::emptyNode();
}



