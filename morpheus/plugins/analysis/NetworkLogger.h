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

#ifndef NETWORKLOGGER_H
#define NETWORKLOGGER_H

#include <core/interfaces.h>
#include "core/simulation.h"
#include "core/celltype.h"
#include <fstream>
#include <sstream>

class NetworkLogger : public Analysis_Listener
{
	private:
		string filename;
		fstream fout_nodes, fout_edges, fout_network, fout_linked_cells;

		string celltype_str;
		shared_ptr<const CellType> celltype;

	public:
		DECLARE_PLUGIN("NetworkLogger");
		NetworkLogger(){}; // default values
		~NetworkLogger(){}; // default destructor for cleanup
		virtual void notify(double time);
		virtual void loadFromXML(const XMLNode );
		virtual set< string > getDependSymbols();
		virtual void init(double time);
 		virtual void finish(double time){};
		string to_string(string a, int b, string c);
};

#endif // NETWORKLOGGER_H
