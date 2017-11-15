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

#ifdef WIN32
typedef unsigned int uint;
#endif


#include <assert.h>
#include <cctype>
#include <iostream>
#include <cmath>
#include <ctime>

#ifdef HAVE_OPENMP
    #include <omp.h>
#else
    inline int omp_get_thread_num()  { return 0;} 
    inline int omp_get_num_threads() { return 1;}
    inline int omp_get_max_threads() { return 1;}
    typedef int omp_lock_t;
#endif

// due to non-linearity of the dependencies, we need forward declarations for all the classes using the plugin system
class PDE_Layer;
class VectorField_Layer;
class PDE_Sim;
class CellType;
class Cell;
class EdgeTrackerBase;

#include "lattice.h"
#include "cpm_layer.h"
#include "symbolfocus.h"
#include "cell_update.h"
#include "scales.h"
#include "symbol.h"
#include "scope.h"


bool getRandomBool();
double getRandom01();
double getRandomGauss(double s);
double getRandomGamma(double shape, double scale);
uint getRandomUint(uint max_val);


class MorpheusException {
public:
	MorpheusException(string what);
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
	enum class ErrorType {INF, DIVZERO, UNDEF };
    ExpressionException(string expression, ErrorType error, XMLNode where) : MorpheusException(string("Error in Expression \"") + expression + "\"",where) {} ;
	string getExpression() { return _expression; };
	ErrorType getError() { return _error; };
private:
	string _expression;
	ErrorType _error;
};


namespace CPM {
	
	
    /// Is a CPM model
    bool isEnabled();
	/// Duration of a Monte Carlo Step
	double getMCSDuration();
	/// Temperature for Metropolis Kinetics
// 	double getTemperature();
    
	shared_ptr<const CPM::LAYER> getLayer();
	
	const CPM::STATE& getNode(const VINT& pos);
	const CPM::STATE& getEmptyState();
		/**
	 *   Get the position of a random empty node in the sub-lattice provided by the 
	 *   range @param min , @param mx. If no range is provided, the whole lattice
	 *   is parsed.
	 *   This method throws an exception when it is unable to find an empty node
	 *   in the given range
	 */
	VINT findEmptyNode(VINT min = VINT(0,0,0) , VINT max = VINT(0,0,0));
	bool setNode(VINT position, CELL_ID cell_id);
	// Interface for the MonteCarloSampler

	const CPM::Update& createUpdate(VINT source, VINT direction, Update::Operation opx);
	void setUpdate(CPM::Update& update);
	bool executeCPMUpdate(const CPM::Update& update);
	
	const CPM::INDEX& getCellIndex(const CELL_ID cell_id);
	const Cell& getCell(CELL_ID cell_id);
	bool cellExists(CELL_ID cell_id);
	
	vector< weak_ptr<const CellType> > getCellTypes();
	uint getEmptyCelltypeID();
	weak_ptr<const CellType> getEmptyCelltype();
	weak_ptr<const CellType> findCellType(string name);  /// Seek for celltype named @p name. Returns a pointer to the Celltype or an empty pointer if the celltype is unknown.
	CELL_ID setCellType(CELL_ID cell_id, uint celltype);
	
	void enableEgdeTracking();
	shared_ptr<const EdgeTrackerBase> cellEdgeTracker();
	
	/// 
	bool isSurface(const VINT& pos);
	uint nSurfaces(const VINT& pos);
	void setInteractionSurface(bool enabled = true);
	const Neighborhood& getBoundaryNeighborhood(); /// Returns the Neighborhood of a node boundary, sorted counterclockwise
	const Neighborhood& getSurfaceNeighborhood(); /// Returns the Neighborhood, that designates a node to be surface node, sorted counterclockwise
	
}


namespace SIM {

	const string getTitle();
	
	/// Lenght of a lattice node in microns
	
	double getNodeLength();
	string getLengthScaleUnit();
	double getLengthScaleValue();
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
	
	/// Simulation time in terms of seconds as defined by Metropolis kinetics time scale

    /// Return user-defined total number of MCS steps to execute

	
	const Scope* getScope();
	const Scope* getGlobalScope();
	Scope* createSubScope(string name, CellType* ct = 0);
	void enterScope(const Scope *scope);
	void leaveScope();

	
	void defineSymbol(SymbolData symbol);
	
	/// Find a symbol in the current scope
	template <class S>
	SymbolAccessor<S> findSymbol(string name) { return getScope()->findSymbol<S>(name); }
	
	///  \briefFind a writable symbol in the current scope
	template <class S>
	SymbolRWAccessor< S > findRWSymbol(string name) { return getScope()->findRWSymbol<S>(name); }
	
	/// \brief Find a globally accessible Symbol (global scope)
	template <class S>
	SymbolAccessor<S> findGlobalSymbol(string name) { return getGlobalScope()->findSymbol<S>(name); };
	
	/** \brief Find a globally accessible Symbol (global scope), that assumes a default if it's not defined in some sub-scopes
	 * 
	 *  \note Fails if the symbol is not defined in any sub-scopes
	 */
	template <class S>
	SymbolAccessor<S> findGlobalSymbol(string name, const S& default_val) { return getGlobalScope()->findSymbol<S>(name,default_val); };
	
	/** Get the type name of the referred symbol
	 * 
	 * The naming convention is accessible via TypeInfo<type>::name()
	 */
	inline string getSymbolType(string name) { return getGlobalScope()->getSymbolType(name); };

	shared_ptr<PDE_Layer> findPDELayer(string symbol);
	shared_ptr<VectorField_Layer> findVectorFieldLayer(string symbol);

}




#endif // SIMULATION_INTERFACE_H
