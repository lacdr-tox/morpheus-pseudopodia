#ifndef CSVREADER_H
#define CSVREADER_H

#include "core/interfaces.h"
#include "core/celltype.h"

class CSVReader : public Population_Initializer
{
private:
	PluginParameter2<string, XMLValueReader, RequiredPolicy> filename;
    PluginParameter2<VDOUBLE, XMLEvaluator, DefaultValPolicy > scaling;

	CellType* cell_type;

	string readTextFile(string filename);
	
	CPM::CELL_ID empty_state;
	vector<CPM::CELL_ID> cells_created;

public:
	CSVReader();
	DECLARE_PLUGIN("CSVReader");

    vector<CPM::CELL_ID> run(CellType *ct) override;
};

#endif // CSVREADER_H
