#include "buffered_vector_reporter.h"

using namespace SIM;

REGISTER_PLUGIN(BufferedVectorReporter)

BufferedVectorReporter::BufferedVectorReporter() : ReporterPlugin() {
    bufferSizeParam.setXMLPath("buffer-size");
    registerPluginParameter(bufferSizeParam);

    interval.setXMLPath("time-step");
    registerPluginParameter(interval);

    input.setXMLPath("Input/value");
    input.setGlobalScope();
    registerPluginParameter(input);

    output.setXMLPath("Output/symbol-ref");
    registerPluginParameter(output);

    map<string, OutputMode> outMap;
    outMap["sum"] =  BufferedVectorReporter::OutputMode::SUM;
    outMap["average"] = BufferedVectorReporter::OutputMode::AVERAGE;
    outputModeParam.setConversionMap(outMap);
    outputModeParam.setXMLPath("Output/mapping");
    registerPluginParameter(outputModeParam);

}

void BufferedVectorReporter::init(const Scope *scope) {
    ReporterPlugin::init(scope);

    cellType = scope->getCellType();
    assert(cellType);

    this->setTimeStep(interval());

    outputMode = outputModeParam();
    bufferSize = bufferSizeParam();
}

void BufferedVectorReporter::loadFromXML(const XMLNode xNode, Scope *scope) {
    ReporterPlugin::loadFromXML(xNode, scope);
}

void BufferedVectorReporter::report() {
    auto cells = cellType->getCellIDs();
    for(auto cell: cells) {
        SymbolFocus focus(cell);

        auto value = input.get(focus);

        // Effective STL Item 24 (efficient find or add)
        // TODO cleanup disappearing cells from map
        auto lb = buffer.lower_bound(cell);
        auto it(lb);
        if(lb != buffer.end() && !(buffer.key_comp()(cell, lb->first))) {
            // key already exists, iterator it already correct, nothing to do
        } else {
            it = buffer.insert(lb, BufferType::value_type(cell,
                    make_unique<boost::circular_buffer<VDOUBLE>>(bufferSize)));
        }
        auto cb = (it->second).get();
        cb->push_back(value);

        auto result = std::accumulate(cb->begin(), cb->end(), VDOUBLE(0, 0, 0));
        if(outputMode == OutputMode::AVERAGE) {
            result *= 1.0 / cb->size();
        }
        output.set(focus, result);
    }


};

