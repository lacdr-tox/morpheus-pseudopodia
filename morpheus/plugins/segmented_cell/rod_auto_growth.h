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

#ifndef ROD_AUTO_GROWTH_H
#define ROD_AUTO_GROWTH_H

#include "core/simulation.h"
#include "core/interfaces.h"
#include "core/pluginparameter.h"
#include "core/super_celltype.h"

class Rod_Auto_Growth : public TimeStepListener
{
private:
	bool auto_growth_factor;
	double auto_growth_delay;
	double initial_time;
	PluginParameter<double> auto_growth_segments;
	uint segments_property_id;
	string segments_property_name;
	SuperCT* celltype;
public:
	DECLARE_PLUGIN("RodAutoGrowth");
	
	Rod_Auto_Growth();
	virtual void loadFromXML (const XMLNode);
	virtual void init ();
    virtual void executeTimeStep();
    virtual set< string > getDependSymbols();
    virtual set< string > getOutputSymbols();
};

#endif // ROD_AUTO_GROWTH_H
