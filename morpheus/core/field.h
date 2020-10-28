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
#include "config.h"
#include "interfaces.h"
#include <iostream>
#include <fstream>
#include <iterator>

class Diffusion;

/**
\defgroup ML_Field Field
\ingroup Symbols
\ingroup ML_Global

A \b Field defines a variable scalar field, associating a scalar value to every lattice site, and defines a symbol with it.


Spatio-temporal dynamics can be implemented explicitly by using an \ref ML_Equation, an \ref ML_Event or by using \ref ML_DiffEqn.
In addition, homogeneous \b Diffusion can optionally be specified.


- \b value: initial condition for the scalar field. May be given as expression, also depending on the  spatial position (see \ref ML_Space)

\b BoundaryValue defines the value of the \ref ML_Field at the respective boundary.
- \b boundary: specifies a boundery with either constant or no-flux boundary condition (\ref ML_Lattice)
- \b value: mathematical expression providing the value at the boundary that may vary in space

\b TiffReader:  specify the initial condition of the field in terms of a float tiff image

Optionally, a \b Diffusion rate may be specified.

- \b rate: diffusion coefficient [(node length)² per (global time)]
- \b well-mixed (optional): if true, homogenizes scalar field. Requires rate=0.


**/




/**
\defgroup ML_VectorField VectorField
\ingroup Symbols
\ingroup ML_Global

A \b VectorField  defines a variable vector field, associating an (x,y,z) value to every lattice site, and defines a \b symbol for it.

Spatio-temporal dynamics can be implemented explicitly by using a \ref ML_VectorEquation or \ref ML_VectorRule.
The \ref ML_Space symbol allows the initial expression to directly depend on the spatial position. 


- \b value: initial condition for the vector field. May be given as expression.

**/




/** \brief Class representing the State of a scalar Field atop the \ref Lattice_Data_Layer
 * 
 * Also takes care of the implementation of Diffusion on all Lattice types (incl. spherical lattices) and the respective
 * boundary handling.
 */

class PDE_Layer : public Lattice_Data_Layer<double>
{
public:
	static const float NO_VALUE;

	PDE_Layer(shared_ptr<const Lattice> l, double p_node_length, bool surface=false);
	~PDE_Layer();

	class ExpressionReader : public ValueReader {
		public:
			ExpressionReader(const Scope* scope) : scope(scope) {};
			void set(string string_val) override { value = make_unique<ExpressionEvaluator<double> >(string_val, scope); value->init(); };
			bool isSpaceConst() const override { return value->flags().space_const; }
			bool isTimeConst() const override { return value->flags().time_const; };
			double get(const VINT& pos) const override { return value->get(SymbolFocus(pos)); }
			shared_ptr<ValueReader> clone() const override { return make_shared<ExpressionReader>(scope); }
		private:
			unique_ptr<ExpressionEvaluator<double> > value;
			const Scope* scope;
	};

	void loadFromXML(const XMLNode xnode, const Scope* scope);
	const string getXMLPath();
	XMLNode saveToXML() const;
	
	bool restoreData(const XMLNode xnode);
	XMLNode storeData(string filename="") const;

	void init(const SymbolFocus& focus = SymbolFocus::global);
	void setInitializer(shared_ptr<ExpressionEvaluator<double>> init) { initializer = init; }

// 	void execute_once_each_mcs(uint mcs);
	/// Calculate the values of time + delta_t  without changing the current values of the layer.
	void doDiffusion(double delta_t );
	/// The maximal time step to proceed without loosing too much precision.
	double getMaxTimeStep();
	double getDiffusionRate();
	void setDiffusionRate(double diff_rate);
	void updateNodeLength(double nl); /// Update the physical length the lattice discretization. Used in MembraneProperties of CPM cells that can vary in cell size.

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
	string initial_expression;
	shared_ptr<ExpressionEvaluator<double>> initializer;
	bool initialized;
	const Scope* local_scope;
	bool init_by_restore;
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

class Field : public Plugin {
public:
	DECLARE_PLUGIN("Field");
	Field(): Plugin() { symbol_name.setXMLPath("symbol"); registerPluginParameter(symbol_name); }
	void loadFromXML(const XMLNode node, Scope * scope) override;
	XMLNode saveToXML() const override;
	void init(const Scope * scope) override;
	
	class Symbol : public SymbolRWAccessorBase<double> {
	public:
		Symbol(string name, string descr): SymbolRWAccessorBase<double>(name), descr(descr) {
			this->flags().granularity = Granularity::Node;
		};
		const string&  description() const override { return descr; }
		const string XMLPath() const override { return field->getXMLPath(); } 
		std::string linkType() const override { return "FieldLink"; }
		
		TypeInfo<double>::SReturn get(const SymbolFocus & f) const override { return field->get(f.pos()); }
		TypeInfo<double>::SReturn safe_get(const SymbolFocus & f) const override { 
			field->init();
			return field->get(f.pos());
			
		};
		shared_ptr<PDE_Layer> getField() const { return field; };
		void set(const SymbolFocus & f, typename TypeInfo<double>::Parameter value) const override { field->set(f.pos(), value); };
		void setBuffer(const SymbolFocus & f, TypeInfo<double>::Parameter value) const override { field->setBuffer(f.pos(), value); }
		void applyBuffer() const override { field->swapBuffer(); };
		void applyBuffer(const SymbolFocus & f) const override { field->applyBuffer(f.pos()); }
		
	private: 
		string descr;
		shared_ptr<PDE_Layer> field;
		friend Field;
	};
private:
	shared_ptr<Symbol> accessor;
	PluginParameter2<string, XMLValueReader, RequiredPolicy> symbol_name;
	shared_ptr<Diffusion> diffusion_plugin;
};


class VectorField_Layer : public Lattice_Data_Layer<VDOUBLE> {
public: 
	VectorField_Layer(shared_ptr< const Lattice > lattice, double node_length);
	void loadFromXML(XMLNode node, const Scope* scope);
	XMLNode saveToXML() const;
	
	bool restoreData(const XMLNode xnode);
	XMLNode storeData(string filename="") const;
	
	void init(const Scope* scope);

private:
	
	class ExpressionReader : public ValueReader {
		public:
			ExpressionReader(const Scope* scope) : scope(scope) {};
			void set(string string_val) override { value = make_unique<ExpressionEvaluator<VDOUBLE> >(string_val,scope); value->init(); };
			bool isSpaceConst() const override { return value->flags().space_const; }
			bool isTimeConst() const override { return value->flags().time_const; }
			VDOUBLE get(const VINT& pos) const override { return value->get(SymbolFocus(pos)); }
			shared_ptr<ValueReader> clone() const override { return make_shared<ExpressionReader>(scope); }
		private:
			unique_ptr<ExpressionEvaluator<VDOUBLE> > value;
			const Scope* scope;
	};
	
	double node_length;
	string initial_expression;
	bool init_by_restore;
};

class VectorField : public Plugin {
public:
	DECLARE_PLUGIN("VectorField");
	VectorField();
	void loadFromXML(const XMLNode node, Scope * scope) override;
	XMLNode saveToXML() const override;
	void init(const Scope * scope) override;
	
	class Symbol : public SymbolRWAccessorBase<VDOUBLE> {
	public:
		Symbol(string name, string descr): SymbolRWAccessorBase<VDOUBLE>(name), descr(descr) {
			this->flags().granularity = Granularity::Node;
		};
		const string&  description() const override { return descr; }
		std::string linkType() const override { return "VectorFieldLink"; }
		
		TypeInfo<VDOUBLE>::SReturn get(const SymbolFocus & f) const override { return field->get(f.pos()); }
		TypeInfo<VDOUBLE>::SReturn safe_get(const SymbolFocus & f) const override{  return field->get(f.pos()); };
		shared_ptr<VectorField_Layer> getField() const { return field; };
		void set(const SymbolFocus & f, typename TypeInfo<VDOUBLE>::Parameter value) const override { field->set(f.pos(), value); }
		void setBuffer(const SymbolFocus & f, TypeInfo<VDOUBLE>::Parameter value) const override { field->setBuffer(f.pos(), value); }
		void applyBuffer() const override { field->swapBuffer(); };
		void applyBuffer(const SymbolFocus & f) const override { field->applyBuffer(f.pos()); }
		
	private: 
		string descr;
		shared_ptr<VectorField_Layer> field;
		friend VectorField;
	};
	
private: 
	shared_ptr<Symbol> accessor;
	PluginParameter2<string, XMLValueReader, RequiredPolicy> symbol_name;
};

#endif
