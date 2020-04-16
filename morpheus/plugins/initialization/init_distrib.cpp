#include "init_distrib.h"
#include "core/lattice_data_layer.h"
#include "core/focusrange.h"

REGISTER_PLUGIN(InitDistribute);

InitDistribute::InitDistribute() {
	num_cells_eval.setXMLPath("number-of-cells");
	registerPluginParameter(num_cells_eval);

	probability.setXMLPath("probability");
	registerPluginParameter(probability);
	
	mode.setXMLPath("mode");
	mode.setConversionMap( {{"regular", Mode::REGULAR},{"random", Mode::RANDOM}} );
	mode.setDefault("regular");
	registerPluginParameter(mode);
};

vector<CPM::CELL_ID> InitDistribute::run(CellType *ct) {
	auto celltype = ct;

	// create a cdf field of the given probability
	// and an index for certain cdf levels
	
	auto lattice = SIM::getLattice();
	int cells_requested = num_cells_eval(SymbolFocus::global);
	
	Lattice_Data_Layer<double> cdf(lattice,0,-1.0);
	FocusRange range(Granularity::Node, SIM::getGlobalScope());
	double cum_sum =0;
	for (const auto& f : range) {
		auto p = probability(f);
		p = p<0 ? 0 : p;
		if (cdf.set(f.pos(), cum_sum+p)) {
			cum_sum += p;
		}
	}
	map<int, VINT> cdf_index;
	int levels = 50;
	double level_offset =0;
	if (mode()==Mode::REGULAR) {
		levels = cells_requested;
		level_offset = - 1.0/ levels / 2.0;
	}
	int level_index = 0;
	double level_barrier = 0.0;
	level_barrier = 1.0/levels * (level_index) + level_offset;
	
	for (const auto& f : range) {
		auto cdf_i = cdf.get(f.pos());
		if (cdf_i != -1 && cdf_i != 0) {
			cdf_i /= cum_sum;
			cdf.set(f.pos(), cdf_i);
			if (cdf_i >= level_barrier) {
				cdf_index[level_index] = f.pos();
				level_index++;
				level_barrier = 1.0/levels * (level_index) + level_offset;
			}
		}
	}
	
	vector<CPM::CELL_ID> cells;
	cout << this->XMLName() << " requests " << cells_requested << " to be created" << endl;
	
	if (mode()==Mode::REGULAR) {
		// just use the levels created by the cdf_index, which was triggered to creat as many levels as cells are requested
		auto i_pos = cdf_index.begin()++;
		while (i_pos != cdf_index.end()) {
			auto cell = createCell(celltype, i_pos->second);
			if (cell != CPM::NO_CELL) {
				cells.push_back(cell);
			}
			i_pos ++;
		}
	}
	else {
		assert(level_offset ==0);
		while (cells.size() < cells_requested) {
			double p = getRandom01();
			
			int index = floor(p * levels);
			VINT pos = cdf_index[index];
			cout << "Searching cdf " << p << " starting at " << index*1.0/levels << " at position "  << pos;
			while (cdf.get(pos) < p) {
				pos.x++; 
				if (pos.x == lattice->size().x) {
					pos.x=0; pos.y++;
					if (pos.y == lattice->size().y) {
						pos.y=0;
						pos.z++;
						if (pos.z == lattice->size().z) break;
					}
				}
			}
			cout << "Reached cdf " << p << " at position "  << pos << " cdf " << cdf.get(pos) << endl;
			auto cell = createCell(celltype, pos);
			if (cell != CPM::NO_CELL) {
				cells.push_back(cell);
			}
		}
	}
	
	cout << "successfully created " << cells.size() << " cells." << endl;
	if (cells.size() < cells_requested) {
		cout << "failed to create " << cells_requested - cells.size() << " cells." << endl;
	}
	return cells;
}


//============================================================================

CPM::CELL_ID InitDistribute::createCell(CellType* celltype, VINT newPos) {
	if (CPM::getLayer()->get(newPos) == CPM::getEmptyState() && CPM::getLayer()->writable(newPos)) {
		uint newID = celltype->createCell();
		CPM::setNode(newPos, newID);
		return newID;
	} else {
		// position is already occupied
		return CPM::NO_CELL;
	}
}
