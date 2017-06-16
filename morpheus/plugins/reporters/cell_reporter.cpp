#include "cell_reporter.h"

// REGISTER_PLUGIN(CellReporter);

Plugin* CellReporter::createInstance() { return new CellReporter(); }

bool CellReporter::factory_registration =
	PluginFactory::RegisterCreatorFunction("CellReporter", CellReporter::createInstance ) &&
	PluginFactory::RegisterCreatorFunction("Mapping", CellReporter::createInstance );



CellReporter::CellReporter() {
	input->setXMLPath("Input/value");
	this->registerPluginParameter(*input);
	
	polarity_output->setXMLPath("Polarity/symbol-ref");
	this->registerPluginParameter(polarity_output);
} 

void CellReporter::loadFromXML(const XMLNode xNode)
{
	map<string, DataMapper::Mode> output_mode_map = DataMapper::getModeNames();
	
	for (uint i=0; i<xNode.nChildNode("Output"); i++) {
		OutputSpec out;
		out.mapping->setXMLPath("Output["+to_str(i)+"]/mapping");
		out.mapping->setConversionMap(output_mode_map);
		out.symbol->setXMLPath("Output["+to_str(i)+"]/symbol-ref");
		registerPluginParameter(out.mapping);
		registerPluginParameter(out.symbol);
		outputs.push_back(out);
	}
	
	ReporterPlugin::loadFromXML(xNode);
}

void CellReporter::init(const Scope* scope)
{
	this->scope = scope;
	TimeStepListener::init(scope);
	// Reporter output value depends on cell position
	if (scope->getCellType())
		registerCellPositionDependency();
	
// 	celltype = scope->getCellType();
// 	assert(celltype);
// 	
// 	if (input->granularity() == Granularity::MembraneNode) {
// 		for (auto& out : outputs) { 
// 			if (out.symbol->granularity() == Granularity::Node) {
// 				out.effective_granularity = Granularity::Node;
// 				surface_outputs.push_back(out);
// 			}
// 			else {
// 				volume_outputs.push_back(out);
// 			}
// 		}
// 	}
// 	else {
// 		for (auto& out : outputs) { 
// 			if (out.symbol->granularity() == Granularity::MembraneNode && input->granularity() == Granularity::Node) {
// 				out.effective_granularity = Granularity::Node;
// 				out.membrane_mapper = shared_ptr<MembraneMapper>(new MembraneMapper(MembraneMapper::MAP_CONTINUOUS) );
// 				out.membrane_acc = celltype->findMembrane(out.symbol->name());
// 				surface_outputs.push_back(out);
// 			} else {
// 				volume_outputs.push_back(out);
// 			}
// 		}
// 	}
// 	
// 	for (auto& out : volume_outputs) { 
// 		if (out.symbol->granularity()==Granularity::MembraneNode || input->granularity()==Granularity::MembraneNode) {
// 			out.effective_granularity = Granularity::MembraneNode;
// 		}				
// 		else if (out.symbol->granularity()==Granularity::Node || input->granularity()==Granularity::Node)
// 			out.effective_granularity = Granularity::Node;
// 		else 
// 			out.effective_granularity = Granularity::Cell;
// 		
// 		if (out.mapping->isDefined()) {
// 			out.mapper = DataMapper::create(out.mapping());
// 		}
// 		else if (out.effective_granularity != out.symbol->granularity()) {
// 			throw MorpheusException(string("Missing mapping in CellReporter Output to symbol ") + out.symbol->name() + ".",this->stored_node);
// 		}
// 	}
// 	
// 	if (polarity_output->isDefined()) {
// 		if (polarity_output->granularity() != Granularity::Cell) {
// 			throw MorpheusException("Reporting Polarity in CellReporter requires a reference to a CellPropertyVector.",this->stored_node);
// 		}
// 	}
}

void CellReporter::report_output(const OutputSpec& output, const SymbolFocus& focus) {
	if ( input->granularity() <= output.symbol->granularity() || output.symbol->granularity() ==  Granularity::Node) {
		output.symbol->set(focus,input(focus));
	}
	else if (output.symbol->granularity() == Granularity::MembraneNode ) {
	}
	else {
		auto mapper =  DataMapper::create(output.mapping());
		
		if (output.symbol->granularity() == Granularity::Cell) {
			FocusRange input_range(input->granularity(),focus.cellID());
			for (auto focus: input_range) {
				mapper->addVal(input(focus));
			}
			output.symbol->set(focus,mapper->get());
		}
		else if (output.symbol->granularity() == Granularity::Global) {
			FocusRange input_range(input->granularity(),scope);
			for (auto focus: input_range) {
				mapper->addVal(input(focus));
			}
			output.symbol->set(SymbolFocus::global, mapper->get());
		}
	}
}

void CellReporter::report_output(const OutputSpec& output, const Scope* scope) {

	FocusRange range(output.symbol->granularity(), scope);
	
	if ( output.symbol->granularity() <= input->granularity()  || output.symbol->granularity() ==  Granularity::Node) {
		// Just write input to the output
		FocusRange range(output.symbol->granularity(), scope);
		for (auto focus : range) {
				output.symbol->set(focus,input(focus));
		}
	}
	else if (output.symbol->granularity() == Granularity::MembraneNode ) {
		// Cross reporting Node <-> Membrane
		if (input->granularity() != Granularity::Node) {
			throw string("Missing mapping implementation");
		}

		// we need to iterate over the cells and use a membrane mapper to get data from nodes to the membrane
		MembraneMapper membrane_mapper(MembraneMapper::MAP_CONTINUOUS);

		FocusRange range(Granularity::Cell, scope);
		for (auto focus : range) {
			auto cell_surface = focus.cell().getSurface();
			
			membrane_mapper.attachToCell(focus.cellID());
			for (auto node : cell_surface) {
				focus.setPosition(node);
				membrane_mapper.map(node, input(focus));
			}
			membrane_mapper.fillGaps();
			membrane_mapper.copyData(output.symbol->accessor().cell_membrane.getMembrane(focus.cellID()));
			
		}
		
	}
	else {
		auto mapper =  DataMapper::create(output.mapping());
		
		if (output.symbol->granularity() == Granularity::Cell) {
			for (auto out_focus : range) {
				FocusRange input_range(input->granularity(),out_focus.cellID());
				for (auto focus: input_range) {
					mapper->addVal(input(focus));
				}
				output.symbol->set(out_focus,mapper->get());
				mapper->reset();
			}
		}
		else if (output.symbol->granularity() == Granularity::Global) {
			FocusRange input_range(input->granularity(),scope);
			for (auto focus: input_range) {
				mapper->addVal(input(focus));
			}
			output.symbol->set(SymbolFocus::global, mapper->get());
		}
	}
}

void CellReporter::report_polarity(const Scope* scope) {
	if ( polarity_output->isDefined()) {
		const Lattice& lattice = SIM::lattice();
		
		if (polarity_output->granularity() <= input->granularity()) {
			throw string("Insufficient information for calculation of a polarity");
		}
		else if (polarity_output->granularity() == Granularity::Global) {
			FocusRange range(input->granularity(),scope);
			VDOUBLE center;
			for (const auto& focus : range) {
				center+=focus.pos();
			}
			center = lattice.to_orth(center/range.size());
			
			VDOUBLE polarisation;
			for (const auto& focus : range) {
				VDOUBLE orientation( (lattice.to_orth(focus.pos()) - center).norm());
				polarisation += input(focus) * orientation;
			}
			polarisation = polarisation / range.size();
			polarity_output->set(SymbolFocus::global, polarisation);
		}
		else if (polarity_output->granularity() == Granularity::Cell) {
			FocusRange out_range(Granularity::Cell, scope);
			for (auto out_focus : out_range) {
				VDOUBLE polarisation;
				if (input->granularity() == Granularity::MembraneNode) {
					auto surface = out_focus.cell().getSurface();
					for (const auto& node : surface) {
						out_focus.setPosition(node);
						VDOUBLE orientation( (lattice.to_orth(node) - out_focus.cell().getCenter()).norm());
						polarisation += input(out_focus) * orientation;
					}
					polarisation = polarisation / surface.size();
				}
				else {
					FocusRange range(input->granularity(),out_focus.cellID());
					for (const auto& focus : range) {
						VDOUBLE orientation( (lattice.to_orth(focus.pos()) - focus.cell().getCenter()).norm());
						polarisation += input(focus) * orientation;
					}
					polarisation = polarisation / range.size();
				}
				polarity_output->set(out_focus, polarisation);
			}
		}
	}
}

void CellReporter::report() {
	
	for ( auto& out : outputs) {
		if (out.symbol->accessor().isComposite() ) {
			for (auto subscope : scope->getSubScopes()) {
				report_output(out, subscope.get());
			}
		}
		else {
			report_output(out, scope);
		}
	}
	
	if (polarity_output->isDefined()) {
		if (polarity_output->accessor().isComposite() ) {
			for (auto subscope : scope->getSubScopes())
				report_polarity(subscope.get());
		}
		else {
			report_polarity(scope);
		}
	}
// 	vector<CPM::CELL_ID> cells = celltype->getCellIDs();
// 
// 	if (!surface_outputs.empty()) {
// //#pragma omp parallel for schedule(dynamic)
// 		for (const auto cell_id : cells) {
// 			SymbolFocus f(cell_id);
// 			auto cell_surface = f.cell().getSurface();
// 			
// 			for (const auto& out : surface_outputs) {
// 				if (out.membrane_mapper) {
// 					out.membrane_mapper->attachToCell(cell_id);
// 					for (auto node : cell_surface) {
// 						f.setPosition(node);
// 						out.membrane_mapper->map(node, (*input)(f));
// 					}
// 					out.membrane_mapper->fillGaps();
// 					out.membrane_mapper->copyData(out.membrane_acc.getMembrane(cell_id));
// 				}
// 				else {
// 					for (auto node : cell_surface) {
// 						f.setPosition(node);
// 						out.symbol->set(f,(*input)(f));
// 					}
// 				}
// 			}
// 		}
// 	}
// 	
// 	if (!volume_outputs.empty()) {
// 		// Initialize the global outputs
// 		for (const auto& out : volume_outputs) {
// 			// We need to collapse input data via mapping
// 			if (out.mapper) {
// 				valarray<double> data;
// 				for (const auto cell_id : cells) {
// 					FocusRange range(out.effective_granularity, cell_id);
// 					for (auto focus : range) {
// 						out.mapper->addVal(input(focus));
// 					}
// 					
// 					// Write mapped data to cell
// 					if (out.symbol->granularity() == Granularity::Cell) {
// 						out.symbol->set(SymbolFocus(cell_id), out.mapper->get());
// 						out.mapper->reset();
// 					}
// 				}
// 				// Write mapped data to global
// 				if (out.symbol->granularity() == Granularity::Global) {
// 					out.symbol->set(SymbolFocus(), out.mapper->get());
// 					out.mapper->reset();
// 				}
// 			} 
// 			// Sufficient output granularity. Output without data mapping.
// 			else {
// 				FocusRange range(out.effective_granularity, this->scope);
// 				for (auto focus : range) {
// 					out.symbol->set(focus, input(focus));
// 				}
// 			}
// 		}
// 		
// 	}
// 	if ( polarity_output->isDefined()) {
// 		if (input->granularity() == Granularity::Node) {
// 			const Lattice& lattice = SIM::lattice();
// 			for (const auto cell_id: cells)  {
// 				VDOUBLE polarisation;
// 				FocusRange range(Granularity::Node, cell_id);
// 				for (const auto& focus : range) {
// 					VDOUBLE orientation( (lattice.to_orth(focus.pos()) - focus.cell().getCenter()).norm());
// 					polarisation += input(focus) * orientation;
// 				}
// 				polarisation = polarisation / range.size();
// 				polarity_output->set(SymbolFocus(cell_id), polarisation);
// 			}
// 		}
// 		else if (input->granularity() == Granularity::MembraneNode) {
// 			for (const auto cell_id: cells)  {
// 				VDOUBLE polarisation;
// // 				double theta_scale = 2.0 * M_PI / MembraneProperty::getSize().x;
// // 				double phi_scale = 1.0 * M_PI / MembraneProperty::getSize().y;
// 				FocusRange range(Granularity::MembraneNode, cell_id);
// 				double surface = 0;
// 				for (const auto& focus : range) {
// 					VDOUBLE orientation = MembraneProperty::memPosToOrientation(focus.membrane_pos());
// 					double d_surf = MembraneProperty::nodeSize(focus.membrane_pos());
// 					surface += d_surf /* * 2 * M_PI / MembraneProperty::size.x */;
// 					polarisation += input(focus) * orientation * d_surf;
// 				}
// 				polarisation = polarisation / surface;
// 				polarity_output->set(SymbolFocus(cell_id), polarisation);
// 			}
// 		}
// 		
// 	}
}
