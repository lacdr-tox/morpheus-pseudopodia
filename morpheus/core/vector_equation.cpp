#include "vector_equation.h"

// MANUALLY REGISTER THE VectorEquation CLASS FOR KEYWORDS "VectorEquation" and "VectorRULE"
Plugin* VectorEquation ::createInstance() { return new VectorEquation(); }

bool VectorEquation ::factory_registration = PluginFactory::RegisterCreatorFunction("VectorEquation", VectorEquation::createInstance )
									   && PluginFactory::RegisterCreatorFunction("VectorRule", VectorEquation::createInstance );

VectorEquation::VectorEquation(): ReporterPlugin() {
	symbol.setXMLPath("symbol-ref");
	registerPluginParameter(symbol);
	
	spherical.setXMLPath("spherical");
	spherical.setDefault("false");
	registerPluginParameter(spherical);
	
	expression.setXMLPath("Expression/text");
	registerPluginParameter(expression);
}


void VectorEquation::loadFromXML(const XMLNode node )
{
    ReporterPlugin::loadFromXML(node);
}

void VectorEquation::init(const Scope* scope)
{
	this->scope = scope;
	ReporterPlugin::init(scope);
	evaluator = shared_ptr<ThreadedExpressionEvaluator<VDOUBLE> >( new ThreadedExpressionEvaluator<VDOUBLE>(expression()) );
	evaluator->init(scope);
	registerInputSymbols(evaluator->getDependSymbols());
}

void VectorEquation::report()
{
	switch(symbol.accessor().getGranularity()){
		case Granularity::Cell:
		{
			vector<CPM::CELL_ID> cells = scope->getCellType()->getCellIDs();
			for (uint c=0; c<cells.size(); ++c) {
				SymbolFocus f(cells[c]);
				symbol.set(cells[c], spherical() ? VDOUBLE::from_radial(evaluator->get(f)) : evaluator->get(f) ) ;
			}
			break;
		}
		case Granularity::Global:
			symbol.set(SymbolFocus::global, spherical() ? VDOUBLE::from_radial(evaluator->get(SymbolFocus::global)) : evaluator->get(SymbolFocus::global));
			break;
		// So far, no Vector Fields implemented
// 		case SymbolData::CellMembraneLink:
// 		{
// 			vector<CPM::CELL_ID> cells = celltype->getCellIDs();
// 			const CellMembraneAccessor& accessor = (symbol.internal_link == SymbolData::SingleCellMembraneLink) ? symbol.cell_membrane : symbol.celltype_membranes[celltype->getID()];
// 			for (uint c=0; c<cells.size(); ++c){
// 				const CPM::CELL_ID cell_id = cells[c];
// 				PDE_Layer* membrane = accessor.getMembrane(cell_id);
// 				// iterate through nodes of membrane-property lattice
// // 				uint mempdesize = MembraneProperty::size.x * (MembraneProperty::size.y==0 ? 1: MembraneProperty::size.y);
// // 				for(int node=0; node < mempdesize; node++) {
// 				VINT pos(0,0,0);
// 				const VINT size = accessor.size(cell_id);
// 				SymbolFocus f(cell_id,pos.x,pos.y);
// 				for (pos.y=0; pos.y < size.y; pos.y++) {
// 					for (pos.x=0; pos.x < size.x; pos.x++) {
// 						f.setMembrane(cell_id,pos);
// 						membrane->set(pos, function.get(f));
// 					}
// 				}
// 			}
// 			break;
// 		}
// 		case SymbolData::PDELink:
// 		{
// 			//shared_ptr<PDE_Layer> pde = SIM::findPDELayer(symbol.getName());
// 			
// 			VINT s = symbol.pde_layer->getWritableSize();
// 			for (int z=0; z<s.z; z++ ){
// //#pragma omp parallel for schedule(dynamic)
// 				for (int y=0; y<s.y; y++ ){
// 					for (int x=0; x<s.x; x++ ){
// 						VINT pos(x,y,z);
// 						double value = function.get(pos); 
// 						symbol.pde_layer->set(pos,value);
// 					}
// 				}
// 			}
// 			break;
// 		}		
// 		default:
// 		{
// 			cerr << "Equation::doTimeStep: cannot run Equation for symbol '" << symbol.getFullName()<< "', type '" << SymbolData::getLinkTypeName(symbol.getLinkType()) << "'." << endl;
// 			exit(-1);
// 			break;
// 		}
	}
	
}