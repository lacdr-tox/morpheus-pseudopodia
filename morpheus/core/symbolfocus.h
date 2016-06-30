//////
//
// This file is part of the modelling and simulation framework 'Morpheus',
// and is made available under the terms of the BSD 3-clause license (see LICENSE
// file that comes with the distribution or https://opensource.org/licenses/BSD-3-Clause).
//
// Authors:  Joern Starruss and Walter de Back
// Copyright 2009-2016, Technische Universität Dresden, Germany
//
//////

#ifndef SYMBOLFOCUS_H
#define SYMBOLFOCUS_H
#include "cpm_layer.h"

class Symbol;

/** @brief Represents a spatial cursor for the retrieval of dependent symbolic information
// 	 *
// 	 * Resolving symbols during a simulation often demands to determine the context -- a cell or a position or whatsoever
// 	 * Finally, it is more efficient to just use a single object to fill that gap for all required symbols, which also slenderizes the interface for symbol retrieval.
// 	*/
class SymbolFocus {
public:
	SymbolFocus();
	SymbolFocus(const VINT& pos);
	SymbolFocus(CPM::CELL_ID cell_id);
	SymbolFocus(CPM::CELL_ID cell_id, const VINT& pos);
	SymbolFocus(CPM::CELL_ID cell_id, double phi, double theta);
	/// Retrieve the cell at the current position
	const Cell& cell() const;
	int celltype() const { return cell_index().celltype; };
	/// Retrieve id of the cell at the current position
	const CPM::CELL_ID cellID() const;
	/// Retrive a membrane position in spherical coordinates. Throws an error if membrane position is not set. 
	const VINT& membrane_pos() const;
	const VINT& pos() const;
	const CPM::INDEX& cell_index() const;
	void setCell(CPM::CELL_ID cell_id);
	void setCell(CPM::CELL_ID cell_id, const VINT& pos);
	void setPosition(const VINT& pos);
	void setMembrane( CPM::CELL_ID cell_id, const VINT& pos );
	void unset();
	bool valid() const { return has_pos || has_cell; }
	static const SymbolFocus global;
	
private:
	mutable bool has_pos, has_membrane, has_cell, has_cell_index;
	mutable VINT d_pos, d_membrane_pos;
	mutable const Cell* d_cell;
	mutable CPM::INDEX d_cell_index;
};
#endif // SYMBOLFOCUS_H
