#include "cell_reporter.h"

REGISTER_PLUGIN(CellReporter);

CellReporter::CellReporter() {
	input.setXMLPath("Input/value");
	this->registerPluginParameter(input);
	
	polarity_output.setXMLPath("Polarity/symbol-ref");
	this->registerPluginParameter(polarity_output);
} 

void CellReporter::loadFromXML(const XMLNode xNode)
{
	map<string, DataMapper::Mode> output_mode_map = DataMapper::getModeNames();

	
	for (uint i=0; i<xNode.nChildNode("Output"); i++) {
		shared_ptr<OutputSpec> out(new OutputSpec());
		out->mapping.setXMLPath("Output["+to_str(i)+"]/mapping");
		out->mapping.setConversionMap(output_mode_map);
		out->symbol.setXMLPath("Output["+to_str(i)+"]/symbol-ref");
		registerPluginParameter(out->mapping);
		registerPluginParameter(out->symbol);
		outputs.push_back(out);
	}
	
	ReporterPlugin::loadFromXML(xNode);
}

void CellReporter::init(const Scope* scope)
{
	TimeStepListener::init(scope);
	// Reporter output value depends on cell position
	registerCellPositionDependency();
	
	celltype = scope->getCellType();
	assert(celltype);
	
	if (input.granularity() == Granularity::MembraneNode) {
		for (auto out : outputs) { 
			if (out->symbol.granularity() == Granularity::Node) {
				out->effective_granularity = Granularity::Node;
				surface_outputs.push_back(out);
			}
			else {
				volume_outputs.push_back(out);
			}
		}
	}
	else {
		for (auto out : outputs) { 
			if (out->symbol.granularity() == Granularity::MembraneNode && input.granularity() == Granularity::Node) {
				out->effective_granularity = Granularity::Node;
				out->membrane_mapper = shared_ptr<MembraneMapper>(new MembraneMapper(MembraneMapper::MAP_CONTINUOUS) );
				out->membrane_acc = celltype->findMembrane(out->symbol.name());
				surface_outputs.push_back(out);
			} else {
				volume_outputs.push_back(out);
			}
		}
	}
	
	for (auto out : volume_outputs) { 
		if (out->symbol.granularity()==Granularity::MembraneNode || input.granularity()==Granularity::MembraneNode) {
			out->effective_granularity = Granularity::MembraneNode;
		}				
		else if (out->symbol.granularity()==Granularity::Node || input.granularity()==Granularity::Node)
			out->effective_granularity = Granularity::Node;
		else 
			out->effective_granularity = Granularity::Cell;
		
		if (out->mapping.isDefined()) {
			out->mapper = DataMapper::create(out->mapping());
		}
		else if (out->effective_granularity != out->symbol.granularity()) {
			throw MorpheusException(string("Missing mapping in CellReporter Output to symbol ") + out->symbol.name() + ".",this->stored_node);
		}
	}
	
	if (polarity_output.isDefined()) {
		if (polarity_output.granularity() != Granularity::Cell) {
			throw MorpheusException("Reporting Polarity in CellReporter requires a reference to a CellPropertyVector.",this->stored_node);
		}
	}
}


void CellReporter::report() {
	
	vector<CPM::CELL_ID> cells = celltype->getCellIDs();

	if (!surface_outputs.empty()) {
//#pragma omp parallel for schedule(dynamic)
		for (const auto cell_id : cells) {
			SymbolFocus f(cell_id);
			auto cell_surface = f.cell().getSurface();
			
			for (const auto& out : surface_outputs) {
				if (out->membrane_mapper) {
					out->membrane_mapper->attachToCell(cell_id);
					for (auto node : cell_surface) {
						f.setPosition(node);
						out->membrane_mapper->map(node, input(f));
					}
					out->membrane_mapper->fillGaps();
					out->membrane_mapper->copyData(out->membrane_acc.getMembrane(cell_id));
				}
				else {
					for (auto node : cell_surface) {
						f.setPosition(node);
						out->symbol.set(f,input(f));
					}
				}
			}
		}
	}
	
	if (!volume_outputs.empty()) {
		// Initialize the global outputs
		for (const auto& out : volume_outputs) {
			// We need to collapse input data via mapping
			if (out->mapper) {
				valarray<double> data;
				for (const auto cell_id : cells) {
					FocusRange range(out->effective_granularity, cell_id);
					for (auto focus : range) {
						out->mapper->addVal(input(focus));
					}
					
					// Write mapped data to cell
					if (out->symbol.granularity() == Granularity::Cell) {
						out->symbol.set(SymbolFocus(cell_id), out->mapper->get());
						out->mapper->reset();
					}
				}
				// Write mapped data to global
				if (out->symbol.granularity() == Granularity::Global) {
					out->symbol.set(SymbolFocus(), out->mapper->get());
					out->mapper->reset();
				}
			} 
			// Sufficient output granularity. Output without data mapping.
			else {
				FocusRange range(out->effective_granularity, this->scope());
				for (auto focus : range) {
					out->symbol.set(focus,input(focus));
				}
			}
		}
		
	}
	if ( polarity_output.isDefined()) {
		if (input.granularity() == Granularity::Node) {
			const Lattice& lattice = SIM::lattice();
			for (const auto cell_id: cells)  {
				VDOUBLE polarisation;
				FocusRange range(Granularity::Node, cell_id);
				for (const auto& focus : range) {
					VDOUBLE orientation( (lattice.to_orth(focus.pos()) - focus.cell().getCenter()).norm());
					polarisation += input(focus) * orientation;
				}
				polarisation = polarisation / range.size();
				polarity_output.set(SymbolFocus(cell_id), polarisation);
			}
		}
		else if (input.granularity() == Granularity::MembraneNode) {
			for (const auto cell_id: cells)  {
				VDOUBLE polarisation;
// 				double theta_scale = 2.0 * M_PI / MembraneProperty::getSize().x;
// 				double phi_scale = 1.0 * M_PI / MembraneProperty::getSize().y;
				FocusRange range(Granularity::MembraneNode, cell_id);
				double surface = 0;
				for (const auto& focus : range) {
					VDOUBLE orientation = MembraneProperty::memPosToOrientation(focus.membrane_pos());
					
					double d_surf = sqrt(1-abs(orientation.z));
					surface += d_surf /* * 2 * M_PI / MembraneProperty::size.x */;
					
// 					VDOUBLE orientation( cos( focus.membrane_pos().x * theta_scale ), sin( focus.membrane_pos().x * theta_scale ),0);
					polarisation += input(focus) * orientation * d_surf;
				}
				polarisation = polarisation / surface;
				polarity_output.set(SymbolFocus(cell_id), polarisation);
			}
		}
		
	}
}
