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

//
// C++ Interface: pde_layer
//
// Description:
//
//
// Author:  <>, (C) 2009
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef PDE_LAYER_H
#define PDE_LAYER_H

#include "lattice_data_layer.h"
class PDE_Layer;
#include "config.h"
#include "interfaces.h"
#include <iostream>
#include <fstream>
#include <iterator>

/**
        @author
*/

class PDE_Layer : public Lattice_Data_Layer<double>
{
public:
	static const float NO_VALUE;

	PDE_Layer(shared_ptr<const Lattice> l, double p_node_length, bool surface=false);
	~PDE_Layer();


	void loadFromXML(const XMLNode xnode);
	XMLNode saveToXML() const;
	
	bool restoreData(const XMLNode xnode);
	XMLNode storeData(string fn="") const;

	void init(const Scope* scope, const SymbolFocus& focus = SymbolFocus::global);

// 	void execute_once_each_mcs(uint mcs);
	/// Calculate the values of time + delta_t  without changing the current values of the layer.
	void doDiffusion(double delta_t );
	/// The maximal time step to proceed without loosing too much precision.
	double getMaxTimeStep();
	double getDiffusionRate();
	void setDiffusionRate(double diff_rate);
	void updateNodeLength(double nl); /// Update the physical length the lattice discretization. Used in MembraneProperties of CPM cells that can vary in cell size.

	/// Returns a reference to the plain array of the underlying data
	string getSymbol() { return symbol_name; }
	double sum() const;
	double mean() const;
	double variance() const;
	double min_val() const;
	double max_val() const;
//         valarray<value_type>& getBuffer() { return write_buffer; }
	uint get_data_index(const VINT& a) const { return Lattice_Data_Layer<double>::get_data_index(a); };

// 	/// Get the gradient in direction @p theta at position @p pos
// 	double getGrad(const VINT& pos, double theta);
//
// 	/// Get the gradient at position @p pos
// 	VDOUBLE getGrad(const VINT& pos);

	shared_ptr<PDE_Layer> clone() {return shared_ptr<PDE_Layer> ( new PDE_Layer(*this) ); };
	/**
		*  Write the layer data to stream @param out in a space/row/row separated ascii format.
		*/
	void write_ascii(ostream& out) const;
	/**
		*  Write the layer data to stream @param out in a gnuplot ascii format.
		*  @param max_value, min_value -- all values are cropped into the given range of values
		*  @param max_resolution specifies the maximal spatial resolution in terms of data points. You may reduce the amount of data to transfer by reducing the spatial resolution.
		*
		*/
	void write_ascii( ostream& out, float min_value, float max_value, int max_resolution = -1 ) const;

	/**
		*  Write the layer data to stream @param out in a gnuplot binary format.
		*  @param max_resolution specifies the maximal spatial resolution in terms of data points.
		*  Use this to reduce the amount of data to transfer by reducing the spatial resolution.
		*/
	void write_binary(ostream& out, int max_resolution = -1);

private:
	string symbol_name;
	string initial_expression;
	bool store_data;
	bool wellmixed;
// 	PDE_Layer(const PDE_Layer& a);
	vector<shared_ptr<Plugin> > plugins;
	
	// some information for Surface diffusion
	valarray<double> theta_y;          // theta of y
	valarray<uint> phi_coarsening;     // Lattice coarsening in phi

	uint pde_solve_freq;         /// Number of MCS between two elapse before repeating pde solving.
	double max_time_step;      /// Maximal time step allowed to assure accuracy of the solver.
/*
        Any of the following rates are in time units.
*/
	double node_length;
	double diffusion_rate;
	string diffusion_units;
	bool is_surface;
// 	VDOUBLE xy_thetaphi_mapping( VDOUBLE xy );

// 	void reset_boundaries(bool diffusion=false);
	bool solve_adi_diffusion(double time_interval);

/**  @brief Forward Euler Solver for time step @param time_interval
*/
	void set_fwd_euler_diffusion_boundaries();
	bool solve_fwd_euler_diffusion(double time_interval);
	bool solve_fwd_euler_diffusion_spheric(double time_interval);
	
	bool solve_fwd_euler_diffusion_generalized(double time_interval);

/**  @brief Tridiagonal Solver for a,b,c beeing a tridiagonal system
 *   This function solves a tridiagonal system using the Thomas algorithm.
 *   a,b,c are the vectors at the subdiagonal, diagonal and superdiagonal
 *   of the tridiagonal matrix.
 *   d is the vector containing the independent term of the system
 *   n is the dimension of the system (squared)
 *
 *   a,b,c and d must be all n-dimensional vectors
 *
 *   The tridiagonal matrix should be diagonal dominance:
 *              |a_i| + |c_i| < |b_i|  for all i
 */
	void tridiag_solver(const valarray<value_type>& a, const valarray<value_type>& b,  valarray<value_type> c, valarray<value_type> d,  valarray<value_type>& x);
};


class VectorField_Layer : public Lattice_Data_Layer<VDOUBLE> {
public: 
	VectorField_Layer(shared_ptr< const Lattice > lattice, double node_length);
	void loadFromXML(XMLNode node);
	void init(const Scope* scope);
	string getSymbol() const { return symbol_name; }
	string getName() const { return name; }

private:
	string symbol_name;
	double node_length;
	string initial_expression;
};

#endif
