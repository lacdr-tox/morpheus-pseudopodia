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

#ifndef NEMATICTENSORLOGGER_H
#define NEMATICTENSORLOGGER_H

#include <core/interfaces.h>
#include "core/simulation.h"
#include "core/celltype.h"
#include "core/symbol_accessor.h"
#include <fstream>
#include <sstream>
#include <Eigen/Eigenvalues>


class NematicTensorLogger : public Analysis_Listener
{
	private:
		vector<CPM::CELL_ID> cellids;
		string celltype_str;
		shared_ptr<const CellType> celltype;
        SymbolAccessor<double> symbol;

        string symbol_str;
        double threshold;

        struct nematicTensor{
            vector< VDOUBLE > axes;
            vector< double> eigenvalues;
            vector< double> lengths;
            VDOUBLE ave_direction;
        };

        nematicTensor tensor;
        static const int LC_XX=0;
        static const int LC_XY=1;
        static const int LC_YY=2;
        static const int LC_XZ=3;
        static const int LC_YZ=4;
        static const int LC_ZZ=5;
        nematicTensor calcLengthHelper3D(const std::vector<double> &I, int N) const;

    public:
        NematicTensorLogger(){}; // default values
        ~NematicTensorLogger(){}; // default destructor for cleanup
        DECLARE_PLUGIN("NematicTensorLogger");
		virtual void notify(double time);
		virtual void loadFromXML(const XMLNode );
		virtual set< string > getDependSymbols();

		virtual void init(double time);
		virtual void finish(double time);
		void log(double time);
};

#endif // NEMATICTENSORLOGGER_H
