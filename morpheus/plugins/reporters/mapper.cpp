#include "mapper.h"

REGISTER_PLUGIN(Mapper);


Mapper::Mapper() {
	input->setXMLPath("Input/value");
	this->registerPluginParameter(*input);
	
	polarity_output->setXMLPath("Polarity/symbol-ref");
	this->registerPluginParameter(polarity_output);
} 

void Mapper::loadFromXML(const XMLNode xNode, Scope* scope)
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
	
	ReporterPlugin::loadFromXML(xNode, scope);
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
#pragma omp parallel
				{
					auto thread = omp_get_thread_num();
#pragma omp for schedule(static)
					for (auto focus=range.begin(); focus<range.end(); ++focus) {
						multimap<FocusRangeAxis,int> restrictions;
						for (auto e : extends) {
							restrictions.insert(make_pair(e,focus->get(e)));
						}
						FocusRange inner_range(input->granularity(),restrictions);
						mapper->reset(thread);
						for (const auto& inner_focus : inner_range ){
							mapper->addVal(input(inner_focus),thread);
						}
						output.symbol->set(*focus,mapper->get(thread));
					}
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

		auto membrane_symbol = dynamic_pointer_cast<const MembranePropertySymbol>(output.symbol->accessor());
		if (!membrane_symbol) {
			throw string("Ooops: Bad cast in Mapper::report_output");
		}
#pragma omp parallel
		{
			// we need to iterate over the cells and use a membrane mapper to get data from nodes to the membrane
			MembraneMapper membrane_mapper(MembraneMapper::MAP_CONTINUOUS);
			FocusRange cell_range(Granularity::Cell, scope);

#pragma omp for schedule(static)
			for (auto focus=cell_range.begin(); focus<cell_range.end(); ++focus) {
				
				auto cell_surface = focus->cell().getSurface();
				membrane_mapper.attachToCell(focus->cellID());
				SymbolFocus surface_focus = *focus;
				for (auto node : cell_surface) {
					surface_focus.setPosition(node);
					membrane_mapper.map(node, input(*focus));
				}
				membrane_mapper.fillGaps();
				membrane_mapper.copyData(membrane_symbol->getField(*focus));
			}
		}
	}
	else {
		auto mapper =  DataMapper::create(output.mapping());
		if (output.symbol->granularity() == Granularity::Cell) {
			FocusRange out_range(output.symbol->accessor(), scope);
#pragma omp parallel
			{
				auto thread = omp_get_thread_num();
#pragma omp for schedule(static)
				for (auto out_focus=out_range.begin(); out_focus<out_range.end(); ++out_focus) {
					// Optimization for single node cells
					if (out_focus->cell().nNodes() ==1) {
						SymbolFocus node_focus(*out_focus);
						node_focus.setCell(node_focus.cellID(), *(node_focus.cell().getNodes().begin()));
						output.symbol->set(*out_focus,input(node_focus));
					}
					else {
						FocusRange input_range(input->granularity(),out_focus->cellID());
						for (auto focus: input_range) {
							mapper->addVal(input(focus),thread);
						}
						output.symbol->set(*out_focus,mapper->get());
						mapper->reset(thread);
					}
				}
			}
		}
		else if (output.symbol->granularity() == Granularity::Global) {
			FocusRange input_range(input->granularity(),scope);
			for (auto focus: input_range) {
				mapper->addVal(input(focus),0);
			}
			output.symbol->set(SymbolFocus::global, mapper->get(0));
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
#pragma omp parallel for
			for (auto out_focus=out_range.begin(); out_focus<out_range.end(); ++out_focus) {
				VDOUBLE polarisation;
				Granularity g = input->granularity();
				if (g == Granularity::MembraneNode ) g = Granularity::SurfaceNode;
				FocusRange range(g,out_focus->cellID());
				
				for (const auto& focus : range) {
					VDOUBLE orientation( (lattice.to_orth(focus.pos()) - focus.cell().getCenter()).norm());
					polarisation += input(focus) * orientation;
				}
				polarisation = polarisation / range.size();
				polarity_output->set(*out_focus, polarisation);
			}
		}
		else {
			throw string("Missing mapping implementation");
		}
	}
}

void Mapper::report() {
	
	for ( auto& out : outputs) {
		auto composite = dynamic_pointer_cast<const CompositeSymbol_I>(out.symbol->accessor());
		if ( composite ) {
			auto subscopes = composite->getSubScopes();
			for (auto subscope : subscopes) {
				report_output(out, subscope);
			}
		}
		else {
			report_output(out, scope);
		}
	}
	
	if (polarity_output->isDefined()) {
		auto composite = dynamic_pointer_cast<const CompositeSymbol_I>(polarity_output->accessor());
		if (composite) {
			auto subscopes = composite->getSubScopes();
			for (auto subscope : subscopes)
				report_polarity(subscope);
		}
		else {
			report_polarity(scope);
		}
	}
}
