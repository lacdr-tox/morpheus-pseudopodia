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

#ifndef MEMBRANERULES3D_H
#define MEMBRANERULES3D_H

#include "core/celltype.h"
#include "core/pluginparameter.h"
#include <core/symbol_accessor.h>
#include <core/cell_membrane_accessor.h>
#include <core/lattice_data_layer.h>
#include <sys/time.h>

using namespace SIM;

class MembraneRules3D : public TimeStepListener
{

private:

	CellType* celltype;
	
	shared_ptr<const CPM::LAYER> cpm_lattice;
	shared_ptr<const Lattice> lattice3D;

	
	VINT latticeSize;
	vector<VINT> neighbor_sites;
	static const double no_distance;
	
	// registration
	string basal_celltype_str;
	shared_ptr<const CellType> basal_celltype;
	
	// distance transform
	enum { CYTOSOL, MEMBRANE, NONE } distance_mode;
	vector<VINT> neighbors;
	vector<double> neighbor_distance;
	
	// local thresholding
	enum { CELLAUTONOMOUS, COMMUNICATION } communication_mode;
	double max_distance;
	double threshold;
	vector< shared_ptr< Lattice_Data_Layer<double> > > distanceMaps;
	vector< shared_ptr< Lattice_Data_Layer<double> > > maskMaps;

	// output
	vector< string > output_symbol_strs;
	vector< CellMembraneAccessor > output_membranes;
	struct Output{
		shared_ptr< Lattice_Data_Layer<double> > cellids;
		shared_ptr< Lattice_Data_Layer<double> > basal;
		shared_ptr< Lattice_Data_Layer<double> > posinfo;
		shared_ptr< Lattice_Data_Layer<double> > potential;
		shared_ptr< Lattice_Data_Layer<double> > apical;
	};
	
	Output output;
	
	///////////////////////////
	
	struct MemPosData {
		// registration of basa contact
		bool basal;
		
		// positional information: distance from basal
		double distance;
		
		// intercellular potential: geometric mean 
		double potential;
		
		// apical prediction
		double apical;

		//////////// technical variables ////////////////
		
		// set of lattice sites around the membrane position that belong to a different cell
		Cell::Nodes halo;

		// local thresholding
		//vector< VINT > local;
		
		
	};
	
	struct BoundingBox{
		VINT minimum;
		VINT maximum;
		VINT size;
	};


	struct CellData{
		CPM::CELL_ID cell_id;

		// bounding box of cell
		//VINT bbox;
		BoundingBox bbox;
		shared_ptr< Lattice_Data_Layer<double> > box;
		shared_ptr< Lattice_Data_Layer<double> > box2;
		shared_ptr< Lattice_Data_Layer<double> > box3;

		// map that stores data per membrane position
		map< VINT, MemPosData, less_VINT > membrane_data;
		
	};
		
	vector< CellData > population_data;

public:
	DECLARE_PLUGIN("MembraneRules3D");
	
    MembraneRules3D();

	virtual void init(CellType* ct);
	virtual void computeTimeStep();
	virtual void executeTimeStep();
	virtual set< string > getDependSymbols();
	virtual set< string > getOutputSymbols();
// 	virtual TimeStepListener::ScheduleType::type type();
	virtual void loadFromXML(const XMLNode );
	
	MembraneRules3D::BoundingBox getBoundingBoxCell(CPM::CELL_ID cell_id);
	MembraneRules3D::BoundingBox getBoundingBoxNode( VINT membrane_node, CPM::CELL_ID cell_id);
// 	shared_ptr<Lattice_Data_Layer< bool >> createBoxBool( VINT boxsize );
// 	shared_ptr<Lattice_Data_Layer< uint >> createBoxInteger( VINT boxsize );
	shared_ptr<Lattice_Data_Layer< double >> createBoxDouble( VINT boxsize, double default_value );
	Cell::Nodes getHalo( VINT mempos, CPM::CELL_ID id);
	
	template <class T>
	void writeTIFF(string prepend, shared_ptr< Lattice_Data_Layer< T > >& box, CPM::CELL_ID id);
	
	
	void writeMultichannelTIFF(void);
	void euclideanDistanceTransform( shared_ptr< Lattice_Data_Layer< double > >& distanceMap, shared_ptr< Lattice_Data_Layer< double > >& maskMap, VINT bottomleft, VINT topright);
	void euclideanDistanceTransform( shared_ptr< Lattice_Data_Layer< double > >& distanceMap, shared_ptr< Lattice_Data_Layer< double > >& maskMap);
	//void mapToContainer( shared_ptr<PDE_Layer>& map, CPM::CELL_ID cell_id);
	bool localThresholding1( VINT membrane_node, CPM::CELL_ID cell_id );
	bool localThresholding2( VINT membrane_node, CPM::CELL_ID cell_id );
	bool localThresholding3( VINT membrane_node, CPM::CELL_ID cell_id, CellData& cd, int thread);
	double localThresholding4( VINT membrane_node, CPM::CELL_ID cell_id, CellData& cd);
	MembraneRules3D::CellData& getCellData( CPM::CELL_ID id );
	VINT maxVINT(VINT a, VINT b);
	VINT minVINT(VINT a, VINT b);	
};

#endif 
