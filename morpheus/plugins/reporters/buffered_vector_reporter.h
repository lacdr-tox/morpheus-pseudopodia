//
// Created by gerhard on 14-8-18.
//

#ifndef MORPHEUS_BUFFERED_REPORTER_H
#define MORPHEUS_BUFFERED_REPORTER_H

#include "core/interfaces.h"
#include "core/celltype.h"

#include <boost/circular_buffer.hpp>

class BufferedVectorReporter: public ReporterPlugin
{
private:
    CellType* cellType;
    enum class OutputMode { AVERAGE, SUM };
    OutputMode outputMode;

    PluginParameter2<double,XMLValueReader, RequiredPolicy> interval;
    PluginParameter2<size_t,XMLValueReader, RequiredPolicy> bufferSizeParam;
    PluginParameter2<VDOUBLE, XMLEvaluator> input;
    PluginParameter2<VDOUBLE, XMLWritableSymbol, RequiredPolicy> output;
    PluginParameter2<OutputMode, XMLNamedValueReader, RequiredPolicy> outputModeParam;

    typedef map<CPM::CELL_ID, std::unique_ptr<boost::circular_buffer<VDOUBLE>>> BufferType;
    BufferType buffer;
    size_t bufferSize;

public:
    DECLARE_PLUGIN("BufferedVectorReporter");
    BufferedVectorReporter();

    void init(const Scope* scope) override;
    void loadFromXML(const XMLNode, Scope* scope) override;
    void report() override;
};
#endif //MORPHEUS_BUFFERED_REPORTER_H
