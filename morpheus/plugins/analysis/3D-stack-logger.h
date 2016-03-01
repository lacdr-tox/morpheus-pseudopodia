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

#ifndef TIFFPLOTTER_H
#define TIFFPLOTTER_H

#include "core/simulation.h"
#include "core/celltype.h"
#include "core/symbol_accessor.h"
#include <sstream>
#include <fstream>
#include <limits>
#include <tiffio.h>


using namespace SIM;

class Stack3D : virtual public Analysis_Listener
{
	public:
		DECLARE_PLUGIN("Stack3D");

		virtual void init(double time);
		virtual void notify(double time);
		virtual void finish(double time);

		void loadFromXML(const XMLNode);

		Stack3D();
		~Stack3D();

	private:
		VINT latticeDim;
		//stringstream vtkText;


        enum outputType { CELLID, CELLTYPE, CELLPROPERTY };
        outputType ot;

        enum colorMap { JET, HOT, COLD, BLUE, POSITIVE, NEGATIVE, GREY, CYCLIC, RANDOM };
        colorMap clrmp;
        void ColorMap(unsigned char *rgb, double value,double min,double max);

        string symbol_str;
		string celltype_str;
        bool single_celltype;
        string colormap_str;
        string exclude_medium_str;
        bool exclude_medium;
        shared_ptr<const CellType> celltype;
		vector< shared_ptr<const CellType> > vec_ct;
		shared_ptr<const CPM::LAYER> cpm_layer;
        SymbolAccessor<double> symbol;

		//void writePDELayer(uint mcs);
		void writeCPMLayer(double time);
  
  
};

#endif //TIFFPLOTTER_H
