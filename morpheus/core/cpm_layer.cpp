#include "cpm_layer.h"
#include "lattice_data_layer.cpp"

// Explicit template instantiation
template class Lattice_Data_Layer<CPM::STATE>;


StatisticalLatticeStencil::StatisticalLatticeStencil(std::shared_ptr< const CPM::LAYER > data_layer, const vector< VINT >& neighbors)
{
	this->data_layer = data_layer;
	setStencil( data_layer->optimizeNeighborhood(neighbors) );
	
}

void StatisticalLatticeStencil::setStencil(const vector< VINT >& neighbors)
{
	stencil_neighbors = neighbors;
	stencil_states.resize(neighbors.size());
	stencil_offsets.resize(neighbors.size());
	int center_index = data_layer->get_data_index(VINT(0,0,0));
	int last_offset = -10000000;
	for (uint i=0; i<stencil_neighbors.size(); i++) {
		int index_offset = data_layer->get_data_index(stencil_neighbors[i]) - center_index;
		stencil_offsets[i] = index_offset;
		if ( abs(index_offset - last_offset) > 64/sizeof(CPM::STATE) ) {
			stencil_row_offsets.push_back(index_offset);
			last_offset = index_offset;
		}
	}
}

void StatisticalLatticeStencil::setPosition(VINT pos)
{
	stencil_statistics.clear();
	int center_index = data_layer->get_data_index(pos);
// #ifdef __GNUC__
// 		for (uint k=0; k<stencil_row_offsets.size(); ++k ) {
// 			__builtin_prefetch(&data_layer->data[ center_index + stencil_row_offsets[k]],0,1);
// 		}
// #endif
	const uint stencil_offsets_size = stencil_offsets.size();
	uint stencil_statistics_size = 0;
	
	for (uint k=0; k<stencil_offsets_size; ++k ) {
		stencil_states[k] = data_layer->data[ center_index + stencil_offsets[k] ].cell_id;
		uint stack_id = 0;
		while(1) {
			if (stack_id == stencil_statistics_size) {
				STATS s;
				s.cell  = stencil_states[k];
				s.count = 1;
				stencil_statistics.push_back( s );
				stencil_statistics_size++;
				break;
			}
			if (stencil_statistics[stack_id].cell == stencil_states[k]) {
				stencil_statistics[stack_id].count++;
				break;
			}
			stack_id++;
		}
	}
}

LatticeStencil::LatticeStencil(shared_ptr< const CPM::LAYER >  data_layer, const std::vector< VINT >& neighbors )
{
	this->data_layer = data_layer;
	setStencil(neighbors);
	
}

void LatticeStencil::setPosition(VINT pos)
{
	int center_index = data_layer->get_data_index(pos);

	for (uint i=0; i< stencil_neighbors.size(); i++) {
		stencil_states[i] = data_layer->data[center_index + stencil_offsets[i] ].cell_id;
	}
}


void LatticeStencil::setStencil(const vector< VINT >& neighbors)
{
	stencil_neighbors = neighbors;
	stencil_states.resize(neighbors.size());
	stencil_offsets.resize(neighbors.size());
	int center_index = data_layer->get_data_index(VINT(0,0,0));
	int last_offset = -10000000;
	for (uint i=0; i<stencil_neighbors.size(); i++) {
		int index_offset = data_layer->get_data_index(stencil_neighbors[i]) - center_index;
		stencil_offsets[i] = index_offset;
	}
}

