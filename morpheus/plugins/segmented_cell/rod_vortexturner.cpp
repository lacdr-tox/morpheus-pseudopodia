#include "rod_vortexturner.h"

REGISTER_PLUGIN(VortexTurner);

VortexTurner::VortexTurner()
{
	check_frequency = -1;
}


void VortexTurner::mcs_notify(uint mcs)
{
	int freq = 5000;
	if (check_frequency) freq = check_frequency;
	if (int(SIM::getTime()) % freq == 0) {
		VDOUBLE l_center = VDOUBLE( SIM::getLattice()->size() ) / 2.0;
		vector < CPM::CELL_ID> cell_ids = celltype->getCellIDs();
		for (uint cell=0; cell<cell_ids.size(); cell++ ) {
			const SuperCell& sc = dynamic_cast<const SuperCell&>(CPM::getCell(cell_ids[cell]));
			CPM::CELL_ID segment;
			bool is_reversed = reversed.get(cell_ids[cell])>0;
			if (is_reversed)
				segment = sc.getSubCells().back();
			else 
				segment= sc.getSubCells().front();
				
			if ( cross(orientation(segment), CPM::getCell(segment).getCenter()-l_center).z < 0 ) {
				cout << "reversing cell " <<  cell_ids[cell] << endl;
				reversed.set(cell_ids[cell], is_reversed ? 0.0 : 1.0);
			}
		}
	}
}

void VortexTurner::init(CellType* p)
{
	Plugin::init(p);
	celltype = dynamic_cast<SuperCT*>(p);
	if (! p)  throw string( "VortexTurner can only be used with SuperCellTypes");

	orientation = celltype->getSubCelltype()->findCellProperty<VDOUBLE>(orientation_name, true);
	reversed = celltype->getSubCelltype()->findCellProperty<double>(reversal_name, true);
}

void VortexTurner::loadFromXML(const XMLNode xNode)
{
    Plugin::loadFromXML(xNode);
	getXMLAttribute(xNode,"Reversal/symbol-ref",reversal_name);
	getXMLAttribute(xNode,"Orientation/symbol-ref",orientation_name);
	getXMLAttribute(xNode,"frequency",check_frequency);
}

