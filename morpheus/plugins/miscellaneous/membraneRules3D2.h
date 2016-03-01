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
#include <iostream>
#include <sstream>
#include <fstream>
#include <complex>
#include "Eigen/Eigenvalues"


using namespace SIM;

class MembraneRules3D : public TimeStepListener
{

private:
	
	CellType* celltype;
	shared_ptr<const CPM::LAYER> cpm_lattice;
	shared_ptr<const Lattice> lattice3D;
	VINT latticeSize;
	vector<VINT> neighbor_sites_halo;
	static const double no_distance;

/*
	<MembraneRules3D>
		<Input basal_celltype="[cellType]" apical_symbol="[symbol]" />
		<PositionInformation mode="[cytosolic/membrane]" method="[linear/exponential]" />
		<IntercellularCommunication mode="[none/geometric_mean]" />
		<Thresholding mode="[global/geometric_mean]" />
			<Global threshold="[double]" />
			<Local threshold="[double]" max_distance="[double]" weighted="[boolean]" />
		</Thresholding>
		<Analysis>
			<Cells tolerance="[double]" writeTIFF="[boolean]"  />
			<Tissue tolerance="[double]" writeTIFF="[boolean]" border="[double]" />
		</Analysis>
		<Output basal="[symbol]" apical_predicted="[symbol]" apical_segmented="[symbol]" posinfo="[symbol]" potential="[symbol]" lateral="[symbol]"/>
	<MembraneRules3D>*/

	struct Input{
		// registration
		string basal_celltype_str;
		shared_ptr<const CellType> basal_celltype;
		string apical_str;
		shared_ptr<PDE_Layer> apical_pde;		
	}; 
	Input input = Input();

	struct PositionInformation{
		PositionInformation() : mode(CYTOSOL), shape(LINEAR), exponent(1.0) {}
		enum { CYTOSOL, MEMBRANE } mode;
		enum { LINEAR, EXPONENTIAL} shape;
		double exponent;
	};
	PositionInformation posinfo = PositionInformation();
	vector<VINT> neighbors;
	vector<double> neighbor_distance;

	struct IntercellularCommunication{
		IntercellularCommunication() : mode(NONE) {}
		enum { NONE, GEOMETRIC_MEAN } mode;
	};
	IntercellularCommunication communication = IntercellularCommunication();
	
		
	struct Thresholding{
		Thresholding() : mode(NONE), threshold(0.25),  max_distance(5), weighted(false) {}
		enum { NONE, GLOBAL, LOCAL } mode;
		double threshold;
		double max_distance;
		double local_distance;
		bool weighted;
		
	};
	Thresholding thresholding = Thresholding();

	struct Pruning{
		Pruning() : unilateral(false), isolated(false) {}
		bool unilateral;
		bool isolated;
	};
	Pruning pruning = Pruning();

	struct AnalysisCells{
		AnalysisCells() : writeTIFF(false), writeMembraneMaps(false) {}
		vector< double > tolerances;
		bool writeTIFF;
		bool writeMembraneMaps;
	};
	AnalysisCells analysisCells = AnalysisCells();

	struct AnalysisTissue{
		AnalysisTissue() : border(20), writeTIFF(false) {}
		vector< double > tolerances;
		uint border;
		bool writeTIFF;
	};
	AnalysisTissue analysisTissue = AnalysisTissue();
	
	struct Fscore{
		Fscore() : tolerance(0), precision(0), sensitivity(0), fscore(0), fp(0), fn(0), tp(0) {}
		double tolerance;
		double precision;
		double sensitivity;
		double fscore;
		int fp;
		int fn;
		int tp;
	};
	Fscore fscore = Fscore();
	
	struct Node{
		Node(): length(1), area(1), volume(1) {}
		double length;
		double area;
		double volume;
	};
	Node node = Node();
	
// 	enum { CELLAUTONOMOUS, COMMUNICATION } communication_mode;
// 	double max_distance;
// 	double threshold;
	vector< shared_ptr< Lattice_Data_Layer<double> > > distanceMaps;
	vector< shared_ptr< Lattice_Data_Layer<bool> > > maskMaps;

	// output
	vector< string > output_symbol_strs;
	vector< CellMembraneAccessor > output_membranes;
	struct Output{
		shared_ptr< Lattice_Data_Layer<double> > cellids;
		shared_ptr< Lattice_Data_Layer<bool> > basal;
		shared_ptr< Lattice_Data_Layer<double> > lateral;
		shared_ptr< Lattice_Data_Layer<double> > posinfo;
		shared_ptr< Lattice_Data_Layer<double> > potential;
		//shared_ptr< Lattice_Data_Layer<double> > apical;
		shared_ptr< Lattice_Data_Layer<bool> > apical;
		shared_ptr< Lattice_Data_Layer<bool> > apical_segmented;
		shared_ptr< Lattice_Data_Layer<double> > fscore;
		shared_ptr< Lattice_Data_Layer<bool> > TP;
		shared_ptr< Lattice_Data_Layer<bool> > FP;
		shared_ptr< Lattice_Data_Layer<bool> > FN;
	};
	
	Output output;
	
	///////////////////////////
	
	struct MemPosData {
		// registration of basal contacts
		bool apical_segmented;

		// registration of basal contacts
		bool basal;
		
		// registration of lateral contacts
		int lateral;

		// on isolated lateral patch?
		bool isolated_lateral;
		
		// positional information: distance from basal
		double distance;

		// positional information: distance from basal | NORMALIZED
		double distance_norm;
		
		// intercellular potential: geometric mean 
		double potential;
		
		// apical prediction
		//bool apical;
		double apical_predicted;

	};
	
	struct BoundingBox{
		VINT minimum;
		VINT maximum;
		VINT size;
	};

	// NEMATIC TENSOR ////////
	static const int LC_XX=0;
	static const int LC_XY=1;
	static const int LC_YY=2;
	static const int LC_XZ=3;
	static const int LC_YZ=4;
	static const int LC_ZZ=5;
	struct Tensor{
		vector< VDOUBLE > axes;
		vector< double> lengths;
		VDOUBLE ave_vector;
		VDOUBLE center;
	};
	//Tensor tensor;
	/////////////////////////
	
	struct CellData{
		CPM::CELL_ID cell_id;
		
		int boundarycell;
		double dscore;
		int isolated_domains;
		set<CPM::CELL_ID> neighbors;
		
		// number of isolated basal/apical/lateral membrane domain
		int num_basal;
		int num_apical_seg;
		int num_apical_pre;
		int num_lateral;

		// bounding box of cell
		//VINT bbox;
		BoundingBox bbox;
		shared_ptr< Lattice_Data_Layer<double> > box;
		//shared_ptr< Lattice_Data_Layer<double> > box2;
		shared_ptr< Lattice_Data_Layer<bool> > box3;

		// map that stores data per membrane position
		map< VINT, MemPosData, less_VINT > membrane_data;
		
		MembraneRules3D::Tensor tensor_basal;
		MembraneRules3D::Tensor tensor_lateral;
		MembraneRules3D::Tensor tensor_apical_predicted;
		MembraneRules3D::Tensor tensor_apical_segmented;
		
	};
		
	vector< CellData > population_data;
	double gKernel[5];
	
	ofstream fout;

	//int boxnumber;
	
	bool cell_tiffs;
	
	void computeFscore( Fscore& scores );

	
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
 	shared_ptr<Lattice_Data_Layer< bool >> createBoxBoolean( VINT boxsize, bool default_value=false );
// 	shared_ptr<Lattice_Data_Layer< uint >> createBoxInteger( VINT boxsize );
	shared_ptr<Lattice_Data_Layer< double >> createBoxDouble( VINT boxsize, double default_value );
	Cell::Nodes getHalo( VINT mempos, CPM::CELL_ID id);
	Cell::Nodes getHaloOfSelf( VINT mempos, CPM::CELL_ID id);
	
	
	template <class T>
	void writeTIFF(string prepend, shared_ptr< Lattice_Data_Layer< T > >& box, CPM::CELL_ID id);
	
	
	void writeFNTPFPTIFF( string filename );
	void writeFNTPFPcell(string prepend, shared_ptr< Lattice_Data_Layer< bool > >& FN,
										shared_ptr< Lattice_Data_Layer< bool> >& TP,
										shared_ptr< Lattice_Data_Layer< bool > >& FP, 
										shared_ptr< Lattice_Data_Layer< bool > >& cell,
										CellData cd);
	void writeMultichannelTIFF(void);
	int euclideanDistanceTransform( shared_ptr< Lattice_Data_Layer< double > >& distanceMap, shared_ptr< Lattice_Data_Layer< bool > >& maskMap, VINT bottomleft, VINT topright);
	int euclideanDistanceTransform( shared_ptr< Lattice_Data_Layer< double > >& distanceMap, shared_ptr< Lattice_Data_Layer< bool > >& maskMap);
	//void mapToContainer( shared_ptr<PDE_Layer>& map, CPM::CELL_ID cell_id);
	bool localThresholding( VINT membrane_node, CellData& cd);
	double getMedian(vector<double>& values);
	
// 	void createFilter(double [], double);
// 	void gaussianBlur3D(CellData& cd);
	MembraneRules3D::CellData& getCellData( CPM::CELL_ID id );
	VINT maxVINT(VINT a, VINT b);
	VINT minVINT(VINT a, VINT b);	
	
	// Nematic tensor
	const MembraneRules3D::Tensor getTensor(Cell::Nodes nodes, VDOUBLE cell_center, bool inertiatensor=false);
	MembraneRules3D::Tensor calcLengthHelper3D(const std::vector<double> I, int N) const;
	inline double sign(double x);
	Eigen::Matrix3f getRotationMatrix(VDOUBLE a, VDOUBLE b);

};

#endif 
