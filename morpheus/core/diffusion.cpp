#include "diffusion.h"


Diffusion::Diffusion(shared_ptr<PDE_Layer> pde): ContinuousProcessPlugin(ContinuousProcessPlugin::INDEPEND, XMLSpec::XML_NONE)
{
	container_type = SymbolData::PDELink;
	pde_layer = pde;
	membrane_length = NULL;
}

Diffusion::Diffusion(CellMembraneAccessor mem_acc): ContinuousProcessPlugin(ContinuousProcessPlugin::INDEPEND, XMLSpec::XML_NONE)
{
	container_type = SymbolData::CellMembraneLink;
	membrane_accessor = mem_acc;
	membrane_length = NULL;
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
	
	
	switch (container_type) {
		case SymbolData::PDELink:
			registerInputSymbol(pde_layer->getSymbol(),SIM::getGlobalScope());
			registerOutputSymbol(pde_layer->getSymbol(),SIM::getGlobalScope());
			setTimeStep(pde_layer->getMaxTimeStep());
			cout << "Max diffusion step is " << pde_layer->getMaxTimeStep() << endl;
			break;
		case SymbolData::CellMembraneLink:
			registerInputSymbol(membrane_accessor.getSymbol(), membrane_accessor.getCellType()->getScope());
			registerOutputSymbol(membrane_accessor.getSymbol(), membrane_accessor.getCellType()->getScope());
			break;
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
	switch (container_type) {
		case SymbolData::PDELink:
			pde_layer->doDiffusion(timeStep());
			break;
		case SymbolData::CellMembraneLink: 
		{
			const CellType* ct = membrane_accessor.getCellType();
			vector <CPM::CELL_ID> cells = ct->getCellIDs();

			static double physical_node_length = SIM::getNodeLength();
#pragma omp parallel for schedule(dynamic)
			for ( int ic=0; ic<cells.size(); ic++) {
					uint cell_volume = CPM::getCell( cells[ic] ).nNodes();
					double spherical_circumference = membrane_length(cell_volume);
					double node_length_along_equator = (spherical_circumference * physical_node_length) / MembraneProperty::resolution;
					
					membrane_accessor.getMembrane(cells[ic])->updateNodeLength( node_length_along_equator );
					// membrane_accessor.getMembrane(cells[ic])->setNodeLength( some funciton( cell_volume_accessor(cells[ic])));
					membrane_accessor.getMembrane(cells[ic])->doDiffusion(timeStep());
			}
			break;
		}
		default:
			cerr << "Invalid symbol type in Diffusion::computeTimeStep!" << endl; assert(0); exit(-1);
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



