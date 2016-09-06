#include "cell_update.h"

namespace CPM {
Update::Update(UpdateData* data, shared_ptr<CPM::LAYER> layer) {
	d = data;
	this->layer = layer;
	operation = NONE;
}

void Update::set(VINT source, VINT dir, Update::Operation opx) {
	
	VINT pos = source;
	layer->lattice().resolve(pos);
	d->add_state = layer->get(pos);
	// In case we copy from a boundary state, we have to fix pos.
	if (!layer->lattice().equal_pos(pos,d->add_state.pos))
		d->add_state.pos = pos;
	d->source.setCell(d->add_state.cell_id, pos);
	
	d->add_state.pos += dir;
	pos+=dir;
	
	if ( ! layer->lattice().resolve(pos) )  {
		operation = NONE;
		return;
	}
	d->remove_state = layer->get(pos);
	d->focus.setCell(d->remove_state.cell_id, pos);
	d->focus_updated.setCell(d->add_state.cell_id, pos);
	
	
	if (d->boundary) d->boundary->setPosition(focus().pos());
	if (d->update) d->update->setPosition(focus().pos());
	if (d->surface) d->surface->setPosition(focus().pos());
	
	switch (opx) {
		case Operation::Extend :
			operation = ADD | REMOVE | NEIGHBORHOOD_UPDATE;
			break;
		case Operation::Move :
			operation = MOVE | NEIGHBORHOOD_UPDATE;
			break;
	}
}

void Update::set(VINT pos, CELL_ID cell_id) {
	
	d->add_state.cell_id = cell_id;
	d->add_state.pos = pos;
	if ( !layer->lattice().resolve(pos) ) {
		operation = NONE;
		return;
	}

	d->source.unset();
	
	d->remove_state = layer->get(pos);
	d->focus.setCell(d->remove_state.cell_id, pos);
	d->focus_updated.setCell(d->add_state.cell_id, pos);
	
	if (focusUpdated().cell_index().status == SUPER_CELL) {
		// cannot add to supercell
		operation = NONE;
		return;
	}
	
	if (d->boundary) d->boundary->setPosition(focus().pos());
	if (d->update) d->update->setPosition(focus().pos());
	if (d->surface) d->surface->setPosition(focus().pos());
	operation = ADD | REMOVE | NEIGHBORHOOD_UPDATE;
}

const Update Update::selectOp(Update::AtomicOperation op) const
{
	Update sel(*this);
	sel. operation = (operation & op) ? op : NONE;
	return sel;
}

const Update Update::removeOp(Update::AtomicOperation op) const
{
	Update sel(*this);
	sel.operation = operation & (~op);
	return sel;
}

}
