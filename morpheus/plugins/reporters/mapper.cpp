#include "mapper.h"

REGISTER_PLUGIN(Mapper);

// Plugin* Mapper::createInstance() { return new CellReporter(); }
// bool CellReporter::factory_registration =
// 	PluginFactory::RegisterCreatorFunction("CellReporter", CellReporter::createInstance ) &&
// 	PluginFactory::RegisterCreatorFunction("Mapping", CellReporter::createInstance );



Mapper::Mapper() {
	input->setXMLPath("Input/value");
	this->registerPluginParameter(*input);
	
	polarity_output->setXMLPath("Polarity/symbol-ref");
	this->registerPluginParameter(polarity_output);
} 

void Mapper::loadFromXML(const XMLNode xNode)
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

void Mapper::init(const Scope* scope)
{
	this->scope = scope;
	TimeStepListener::init(scope);
	// Reporter output value depends on cell position
	if (scope->getCellType())
		registerCellPositionDependency();
}

void Mapper::report_output(const OutputSpec& output, const SymbolFocus& focus) {
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

void Mapper::report_output(const OutputSpec& output, const Scope* scope) {

// 	
	
	if ( output.symbol->granularity() <= input->granularity()  || output.symbol->granularity() ==  Granularity::Node) {
		
		if ( input->granularity() == Granularity::MembraneNode ) {
			FocusRange range(Granularity::SurfaceNode, scope);
			cout << "Membrane node size " << range.size() << endl;
			// Just write input to the output
			for (const auto& focus : range) {
				output.symbol->set(focus,input(focus));
			}
		}
		else {
			FocusRange range(output.symbol->accessor(), scope);
			auto extends = range.spatialExtends();
			auto input_extends = FocusRange(input->granularity(), scope).spatialExtends();
			bool dimensions_lost = extends.size() < input_extends.size();
			if (  output.symbol->granularity() ==  input->granularity() &&  dimensions_lost) {
				auto mapper =  DataMapper::create(output.mapping());
				for (const auto& focus : range) {
					multimap<FocusRangeAxis,int> restrictions;
					for (auto e : extends) {
						restrictions.insert(make_pair(e,focus.get(e)));
					}
					FocusRange inner_range(input->granularity(),restrictions);
					mapper->reset();
					for (const auto& inner_focus : inner_range ){
						mapper->addVal(input(inner_focus));
					}
					output.symbol->set(focus,mapper->get());
				}
			}
			else {
				// Just write input to the output
				for (const auto& focus : range) {
					output.symbol->set(focus,input(focus));
				}
			}
		}
	}
	else if (output.symbol->granularity() == Granularity::MembraneNode ) {
		// Cross reporting Node <-> Membrane
		if (input->granularity() != Granularity::Node) {
			throw string("Missing mapping implementation");
		}

		// we need to iterate over the cells and use a membrane mapper to get data from nodes to the membrane
		MembraneMapper membrane_mapper(MembraneMapper::MAP_CONTINUOUS);

		FocusRange cell_range(Granularity::Cell, scope);
		for (auto focus : cell_range) {
			
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
			FocusRange range(output.symbol->accessor(), scope);
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
		else {
			throw string("Missing mapping implementation");
		}
	}
}

void Mapper::report_polarity(const Scope* scope) {
	if ( polarity_output->isDefined()) {
		const Lattice& lattice = SIM::lattice();
		
		if (polarity_output->granularity() <= input->granularity()) {
			throw string("Insufficient information for calculation of a polarity");
		}
		else if (polarity_output->granularity() == Granularity::Global) {
			Granularity g = input->granularity();
			if (g == Granularity::MembraneNode ) g = Granularity::SurfaceNode;
			FocusRange range(g,scope);
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
				Granularity g = input->granularity();
				if (g == Granularity::MembraneNode ) g = Granularity::SurfaceNode;
				FocusRange range(g,out_focus.cellID());
				
				for (const auto& focus : range) {
					VDOUBLE orientation( (lattice.to_orth(focus.pos()) - focus.cell().getCenter()).norm());
					polarisation += input(focus) * orientation;
				}
				polarisation = polarisation / range.size();
				polarity_output->set(out_focus, polarisation);
			}
		}
		else {
			throw string("Missing mapping implementation");
		}
	}
}

void Mapper::report() {
	
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
}
