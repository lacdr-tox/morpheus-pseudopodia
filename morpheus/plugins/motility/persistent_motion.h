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

#ifndef PERSISTENTMOTION_H
#define PERSISTENTMOTION_H

#include "core/interfaces.h"
#include "core/celltype.h"

/** \defgroup PersistentMotion
 \ingroup ML_CellType
 \ingroup CellMotilityPlugin
 \ingroup CPM_EnergyPlugins

PersistentMotion models the tendency of cells to maintain their previous direction of movement (memory or inertia), due to the non-instantaneous turnover of the cytoskeleton

Cells have a target direction \f$  \vec{t} \f$ based on previous movement, and CPM update in this direction are preferred (i.e. they decrease energy \f$ E \f$).

For each proposed copy attempt \f$ \mathbf{x} \rightarrow \mathbf{x_{neighbor}} \f$ in direction \f$ \vec{s} \f$ , the change in effective energy is computed as:

\f$ \Delta E = - \mu \cdot v_{\sigma,t} \cdot ( \vec{t} \cdot \vec{s} ) \f$

where
- \f$ \mu \f$ is the strength of persistence, in units of energy.
- \f$ v_{\sigma,t} \f$ is the cell volume
- \f$ \vec{t} \f$ is a vector giving the target direction of previous movement
- \f$ \vec{s} \f$ is a vector giving the direction of CPM update

The target direction is updated continuously according to the shift of cell centroid \f$\mathbf{x_t} - \mathbf{x_{t-1}}\f$, keeping a memory of of length 'decaytime' \f$ dt \f$ (units of MCS):

\f$ \vec{t_t} = (1-dt) \cdot \vec{t_{t_1}} + dt \cdot \mathbf{x_t} - \mathbf{x_{t-1}} \f$ 

\section Input 
Required
--------
- *decaytime*: Expression representing the memory of direction of cell movement. This may be a constant (e.g. "50.0"), a symbol (e.g. "St"), or an expression (e.g. "S0 * 2.0")
- *strength*: Expression describing the strength of persistence. This may be a constant (e.g. "2.0"), a symbol (e.g. "Ss"), or an expression (e.g. "S0 * 2.0")

Optional
--------
- *protrusion* (default=true): Boolean describing whether persistence should be considered during protrusions.
- *retraction* (default=false): Boolean describing whether persistence should be considered during retractions.

\section Notes

- This plugin stores the old cell center and target direction as VectorProperties in the cell.

\section Reference
- Szabó, A., R. Ünnep, E. Méhes, W. O. Twal, W. S. Argraves, Y. Cao, and A. Czirók. "Collective cell motion in endothelial monolayers." Physical biology 7, no. 4, 2010.
- Vroomans, Renske MA, Paulien Hogeweg, and Kirsten HWJ ten Tusscher. "Segment-Specific Adhesion as a Driver of Convergent Extension." PLOS Computational Biology 11, no. 2, 2015.

\section Example
\verbatim
<PersistentMotion decaytime="50" strength="1"/>

<PersistentMotion decaytime="dt" strength="s"/>

<PersistentMotion decaytime="dt" strength="s" protrusion="true" retraction="true"/>
\endverbatim
*/

class PersistentMotion : public ReporterPlugin, public CPM_Energy
{
protected:

    enum class PersistenceType {MORPHEUS, SZABO};

    class Persistence
    {
    protected:
        PersistentMotion& pmp; // link to the PersistentMotion plugin instance (outer class)
    public:
        explicit Persistence(PersistentMotion& pM): pmp(pM) {}
        virtual ~Persistence() {};
        virtual double delta(const SymbolFocus& cell_focus, const CPM::Update& update) = 0;
        static std::unique_ptr<Persistence> create(const PersistenceType persistenceType, PersistentMotion& pM);
        static VDOUBLE update_direction(const SymbolFocus& cell_focus);
    };

    class MorpheusPersistence: public Persistence {
        using Persistence::Persistence;
    public:
        double delta(const SymbolFocus &cell_focus, const CPM::Update &update) override ;
    };

    class SzaboPersistence: public Persistence {
        using Persistence::Persistence;
    public:
        double delta(const SymbolFocus &cell_focus, const CPM::Update &update) override ;
    };

private:
	
	PluginParameter2<double, XMLEvaluator, RequiredPolicy> decaytime;
	PluginParameter2<double, XMLEvaluator, RequiredPolicy> strength;

	PluginParameter2<bool, XMLValueReader, DefaultValPolicy> retraction;
	PluginParameter2<bool, XMLValueReader, DefaultValPolicy> protrusion;

    PluginParameter2<PersistenceType, XMLNamedValueReader, RequiredPolicy> persTypeEval;

	CellType* celltype;

	PersistenceType persistenceType;
	
	// We store the direction and old position in 
	// CellPropertyAccessor
	SymbolRWAccessor<VDOUBLE> cell_direction; // stores direction of cell from cell property
	SymbolRWAccessor<VDOUBLE> cell_position_memory; // stores cell.center from cell property

	unique_ptr<Persistence> persistence;

public:
	PersistentMotion();
	DECLARE_PLUGIN("PersistentMotion")

    void init(const Scope* scope) override;
	void report() override;
	double delta( const SymbolFocus& cell_focus, const CPM::Update& update) const override;
	double hamiltonian(CPM::CELL_ID cell_id) const override;  

};

#endif // PERSISTENTMOTION_H
