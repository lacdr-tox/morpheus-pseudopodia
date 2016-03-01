/*
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "rod_auto_growth.h"

REGISTER_PLUGIN(Rod_Auto_Growth);

Rod_Auto_Growth::Rod_Auto_Growth() : TimeStepListener(ScheduleType::MCSListener) {
	addPluginParameter(&auto_growth_segments,"Segments");
	auto_growth_factor = true;
	auto_growth_delay = 100;
}

void Rod_Auto_Growth::executeTimeStep()
{
	TimeStepListener::executeTimeStep();
	if (auto_growth_factor) {
		 // split the last segment if the number of target_segments is not reached
		bool full_grown = true;
		vector<CPM::CELL_ID> cells = celltype->getCellIDs();
		for (uint cid=0; cid < cells.size(); cid++) {
			SuperCell& sc = static_cast<SuperCell&>(celltype->storage.cell(cells[cid]));
			uint nSegments  = sc.getSubCells().size();
// 			cout <<  "Rod_Auto_Growth: checking cell " << cells[cid] << " has " << nSegments << " of " <<  auto_growth_segments(cells[cid]) << "segments!" <<  endl;
			if ( sc.getSubCells().size() < auto_growth_segments(cells[cid]) ) {
				full_grown = false;
				const Cell& subcell = CPM::getCell(sc.getSubCells().back());
				if ( (auto_growth_delay * nSegments ) <= SIM::getTime() - initial_time && subcell.nNodes() > 1) {
					VDOUBLE split_normal;
					if ( nSegments>1) {
						split_normal =  sc.getSubCenters()[nSegments-2] -sc.getSubCenters()[nSegments-1];
					} else {
						split_normal = subcell.getMajorAxis();/* long_cell_axis( subcell.getNodes() );*/
						if (getRandomBool()) split_normal = -1.0 * split_normal;
					}
					CPM::CELL_ID new_subcell = const_pointer_cast<CellType>( celltype->getSubCelltype()) -> divideCell(sc.getSubCells().back(), split_normal,subcell.getCenter());
					sc.getSubCenters().back();
// 					cout << "Rod_Auto_Growth: Split last segment into " << subcell.nNodes() << " and " << CPM::getCell(new_subcell).nNodes() << endl;
					sc.addSubCell(new_subcell);
				}
			}
		}
		if (full_grown) auto_growth_factor = false;
	}
}

void Rod_Auto_Growth::init()
{
	TimeStepListener::init();
	
	celltype = dynamic_cast< SuperCT* > (SIM::getScope()->getCellType()); 
	if (! celltype) {
		cerr << XMLName()+" can only be applied on a segmented celltype" << endl;
		exit(-1);
	}
	initial_time = double (SIM::getTime());
}


void Rod_Auto_Growth::loadFromXML(const XMLNode xNode)
{
	TimeStepListener::loadFromXML(xNode);
// 	segments_property_name="";
// 	getXMLAttribute(xNode,"segments", auto_growth_segments);
	auto_growth_factor = true;
	getXMLAttribute(xNode,"delay", auto_growth_delay);
}


set< string > Rod_Auto_Growth::getDependSymbols()
{
	return set<string>();
}

set< string > Rod_Auto_Growth::getOutputSymbols()
{
	set<string> symbols;
	symbols.insert("cell.center");
	return symbols;
}

