#include "vector_mapper.h"

REGISTER_PLUGIN(VectorMapper);


VectorMapper::VectorMapper() {
	input->setXMLPath("Input/value");
	this->registerPluginParameter(*input);
} 

void VectorMapper::loadFromXML(const XMLNode xNode, Scope* scope)
{
	map<string, VectorDataMapper::Mode> out_map = VectorDataMapper::getModeNames();
	for (uint i=0; i<xNode.nChildNode("Output"); i++) {
		OutputSpec out;
		out.mapping->setXMLPath("Output["+to_str(i)+"]/mapping");
		out.mapping->setConversionMap(out_map);
		out.symbol->setXMLPath("Output["+to_str(i)+"]/symbol-ref");
		registerPluginParameter(out.mapping);
		registerPluginParameter(out.symbol);
		outputs.push_back(out);
	}
	
	ReporterPlugin::loadFromXML(xNode, scope);
}

void VectorMapper::init(const Scope* scope)
{
	this->scope = scope;
	TimeStepListener::init(scope);
	// Reporter output value depends on cell position
	if (scope->getCellType())
		registerCellPositionDependency();
}

void VectorMapper::report_output(const OutputSpec& output, const SymbolFocus& focus) {
	if ( input->granularity() <= output.symbol->granularity() || output.symbol->granularity() ==  Granularity::Node) {
		output.symbol->set(focus,input(focus));
	}
	else {
		auto mapper = VectorDataMapper::create(output.mapping());
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

void VectorMapper::report_output(const OutputSpec& output, const Scope* scope) {

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
				auto mapper =  VectorDataMapper::create(output.mapping());
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
	}
	else {
		auto mapper =  VectorDataMapper::create(output.mapping());
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
						output.symbol->set(*out_focus,mapper->get(thread));
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

void VectorMapper::report() {
	
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
}
