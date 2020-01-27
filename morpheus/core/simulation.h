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

#ifndef SIMULATION_INTERFACE_H
#define SIMULATION_INTERFACE_H


/*
    Copyright (c) 2011, Jörn Starruß <joern.starruss@tu-dresden.de>, 
    Walter de Back <walter.deback@zih.tu-dresden.de>, Technische Universität Dresden.
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
        * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
        * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
        * Neither the name of the Technische Universität Dresden nor the
        names of its contributors may be used to endorse or promote products
        derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY JÖRN STARRUSS ''AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL JÖRN STARRUSS BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


// draw in all macros from cmake configuration 
// .. this file is generated by cmake from config.h.cmake and written to the binary folder

// draws in the memory and random header
#include "config.h"

// due to non-linearity of the dependencies, we need forward declarations
class PDE_Layer;
class VectorField_Layer;

#include "xml_functions.h"
#include "scope.h"
#include "scales.h"
#include "cpm.h"
// #include "cpm_layer.h"
// #include "cell_update.h"



class MorpheusException {
public:
	MorpheusException(string what) : _what(what) {};
	MorpheusException(string what, XMLNode where) : _what(what),_where(getXMLPath(where)) {}
	MorpheusException(string what, string where) : _what(what),_where(where) {}
	string where() const { return _where; }
	string what() const { return _what; }
protected:
	string _what;
	string _where;
};

class ExpressionException : public MorpheusException {
public:
	enum class ErrorType {INF, NaN, DIVZERO, UNDEF };
	ExpressionException(string expression, ErrorType error) : _error(error), MorpheusException(string("Error \"") + getErrorName(error) + ("\" in Expression \"") + expression + "\"") {} ;
    ExpressionException(string expression, ErrorType error, XMLNode where) :  _error(error), MorpheusException(string("Error \"") + getErrorName(error) + ("\" in Expression \"") + expression + "\"",where) {} ;
	string getExpression() { return _expression; };
	ErrorType getError() { return _error; };
	string getErrorName() {
		
		switch (_error) {
			case ErrorType::INF: return "Infinity";
			case ErrorType::NaN: return "Not a number";
			case ErrorType::DIVZERO: return "Division by zero";
			default: return "Undefined ";
		}
	}
private:
	string getErrorName(ErrorType error) {
		
		switch (error) {
			case ErrorType::INF: return "Infinity";
			case ErrorType::NaN: return "Not a number";
			case ErrorType::DIVZERO: return "Division by zero";
			default: return "Undefined ";
		}
	}
	ErrorType _error;
	string _expression;
};


namespace SIM {
	/// Simulation title as defined by XML
	const string& getTitle();
	
	/// Input directory, i.e. directory of the model file
	const string& getInputDirectory();
	
	/// Output directory as defined by command line or cwd
	const string& getOutputDirectory();
	
	/// Just create the dependency graph
	bool dependencyGraphMode();
	
	/// Lenght of a lattice node in microns
	double getNodeLength();
	
	string getLengthScaleUnit();
	double getLengthScaleValue();
	Lattice::Structure getLatticeStructure();
	shared_ptr<const Lattice> getLattice();
	const Lattice& lattice();
	
	/// Duration of the Simulation
	double getStopTime();
	// Starting time of simulation (can be nonzero in checkpointed simulation models)
	double getStartTime();
	/// Current Time of Simulation
	double getTime();
	// TODO: This shall become getTimeString
	string getTimeName();
	string getTimeName(double time);
	string getTimeScaleUnit();
	void saveToXML();
	
   /// Get the global Scope. All other scopes are direct or indirect sub-scopes of the global scope.
	Scope* getGlobalScope();
	
	void wipe();
	
	/// \brief Find a globally accessible Symbol (global scope)
	template <class S>
	SymbolAccessor<S> findGlobalSymbol(string name) { return getGlobalScope()->findSymbol<S>(name); };
	
	/** \brief Find a globally accessible Symbol (global scope), that assumes a default if it's not defined in some sub-scopes
	 * 
	 *  \note Fails if the symbol is not defined in any sub-scopes
	 */
	template <class S>
	SymbolAccessor<S> findGlobalSymbol(string name, const S& default_val) { return getGlobalScope()->findSymbol<S>(name,default_val); };
		
	/// Global symbol providing the simulation time
	class TimeSymbol : public SymbolAccessorBase<double> {
	public:
		TimeSymbol(string symbol) : SymbolAccessorBase<double>(symbol) {
			flags().space_const = true;
		}
		double get(const SymbolFocus&) const override ;
		const string& description() const override { static const string descr = "Time" ; return descr; }
		string linkType() const override { return "TimeLink"; }
	};
	
	/// Global symbol providing the position
	class LocationSymbol : public SymbolAccessorBase<VDOUBLE> {
	public:
		LocationSymbol(string symbol) : SymbolAccessorBase<VDOUBLE>(symbol) {
			flags().granularity = Granularity::Node;
			flags().time_const = true;
		}
		VDOUBLE get(const SymbolFocus& f) const override {
			return f.global_pos();
		}
		const string&  description() const override { static const string descr = "Location" ; return descr; }
		string linkType() const override { return "LocationLink"; }
	};
	
	/// Global symbol providing the position
	class LatticeLocationSymbol : public SymbolAccessorBase<VDOUBLE> {
	public:
		LatticeLocationSymbol(string symbol) : SymbolAccessorBase<VDOUBLE>(symbol) {
			flags().granularity = Granularity::Node;
			flags().time_const = true;
		}
		VDOUBLE get(const SymbolFocus& f) const override {
			return f.lattice_pos();
		}
		const string&  description() const override { static const string descr = "LatticeLocation" ; return descr; }
		string linkType() const override { return "LatticeLocationLink"; }
	};
	
}




#endif // SIMULATION_INTERFACE_H
