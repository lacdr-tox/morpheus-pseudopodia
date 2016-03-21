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


/** 
\defgroup ModelStructure
 */

// ============================================================
/** 
\defgroup Description
\ingroup ModelStructure

\b Description provides human readable title and annotation.

\b Title (required): name of the simulation model. 
This is used as folder name to store simulation results, appended by the job ID.

\b Details (optional): Human readable model annotations in plain text. 
This may include verbal model description, change history or references.
This information only serves for human readability and does not affect the simulation itself. 

\section Example
\verbatim
<Description>
	<Title>Your Model Name</Title>
	<Details>Your annotations, change log and references.</Details>
</Description>
\endverbatim
**/

// ============================================================

/**
\defgroup Global
\ingroup ModelStructure

Section to include mathematical variabes and equations in the global \ref Scope.

Globals provides section for:
- Global \ref Constant and \ref Variable that can be overwritten in local scopes such as \ref CellType and \ref System. In this case, Globals serve as default values.
- \b ODE systems: \ref System of \ref DiffEqn operating on \ref Variable
- \b Reaction-diffusion PDE systems: \ref System of \ref DiffEqn operating on diffusive \ref Field

\section Examples
- Globally defined constant 'e' and a variable 'v'. These symbols can be overwritten in subscopes, in which case the global values act as defaults.
\verbatim
<Global>
	<Constant symbol="e" value="2.7182818284"/>
	<Variable symbol="v" value="0.0"/>
</Global>
\endverbatim

- ODE System without cellular context, see ODE/PredatorPrey example
\verbatim
<Global>
	<Variable symbol="R" value="0.5"/>
	<Variable symbol="C" value="1.0"/>
	<System solver="runge-kutta" time-step="0.1">
		<Constant symbol="r" value="0.1"/>
		<Constant symbol="b" value="0.1"/>
		<Constant symbol="c" value="0.5"/>
		<Constant symbol="d" value="0.01"/>
		<Constant symbol="K" value="1"/>
		<DiffEqn symbol-ref="R">
			<Expression>r*R - b*R*C</Expression>
		</DiffEqn>
		<DiffEqn symbol-ref="C">
			<Expression>c*b*R*C - d*C</Expression>
		</DiffEqn>
	</System>
</Global>
\endverbatim

- PDE reaction-diffusion System with two diffusive scalar fields 'a' and 'i'. See ActivatorInhibitor_2D example.
\verbatim
<Global>
	<Field symbol="a" value="rand_norm(0.5,0.1)" name="activator">
		<Diffusion rate="0.02" unit="µm²/s"/>
	</Field>
	<Field symbol="i" value="0.1" name="inhibitor">
		<Diffusion rate="1" unit="µm²/s"/>
	</Field>
	<System solver="runge-kutta" time-step="5" name="Meinhardt">
		<Constant symbol="rho" value="0.001"/>
		<Constant symbol="rho_a" value="0.001"/>
		<Constant symbol="mu_i" value="0.03"/>
		<Constant symbol="mu_a" value="0.02"/>
		<Constant symbol="kappa" value="0.10"/>
		<DiffEqn symbol-ref="a">
			<Expression>(rho/i)*((a^2)/(1 + kappa*a^2)) - mu_a * a + rho_a</Expression>
		</DiffEqn>
		<DiffEqn symbol-ref="i">
			<Expression>rho*((a^2)/(1+kappa*a^2)) - mu_i *i</Expression>
		</DiffEqn>
	</System>
</Global>
\endverbatim

**/

// ============================================================
/**
\defgroup Symbols
\defgroup MathExpressions
**/

/**
\defgroup Constant
\ingroup Symbols

Symbol with a fixed scalar value.
**/
/**
\defgroup ConstantVector
\ingroup Symbols

Symbol with a fixed 3D vector.

Syntax is comma-separated: x,y,z
**/
/**
\defgroup Variable
\ingroup Symbols

Symbol with a variable scalar value.
**/
/**
\defgroup VariableVector
\ingroup Symbols

Symbol with a variable 3D vector value.

Syntax is comma-separated: x,y,z
**/
/**
\defgroup Property
\ingroup Symbols

Symbol with a cell-bound variable scalar value.
**/
/**
\defgroup PropertyVector
\ingroup Symbols

Symbol with cell-bound variable 3D vector value.

Syntax is comma-separated: x,y,z
**/
/**
\defgroup Field
\ingroup Symbols

Symbol with a variable scalar field, associating a scalar value to every lattice site in the domain.

- \b value: initial condition for the scalar field. May be given as symbolic expression.

Optionally, a \b Diffusion rate may be specified.

- \b rate: diffusion coefficient
- \b unit (optional): physical unit of diffusion coefficient
- \b well-mixed (optional): if true, homogenizes scalar field. Requires rate=0.

**/


/**
\defgroup DiffEqn
\ingroup MathExpressions

Assignment of an equation containing derivatives to a symbol.

Ordinary differential equation \f$ \frac{dX}{dt}=a \f$ if \f$ X \f$ is a \ref Variable or a \ref Property

Partial differential equation \f$ \frac{\partial X}{\partial x}=D_{X}\nabla^2X+a \f$ where \f$ X \f$ is a \ref Field and \f$ D_{X} \f$ is its diffusion coefficient.

DiffEqn are only allowed within \ref System
**/
/**
\defgroup VectorFunction
\ingroup MathExpressions

Symbol that defines a relation between vector \ref Symbols
**/

/**
\defgroup System
\ingroup MathExpressions

Environment for tightly coupled \ref Rule and \ref DiffEqn. Expressions with a System are synchronously updated and may contain recurrence relations.

- \b solver: numerical solver for DiffEqn: Euler (1st order), Heun (aka explicit trapezoid rule, 2nd order) or Runge-Kutta (4th order)
- \b time-step: integration step size.
- \b time-scaling (optional): scales the dynamics of \b System to the simulation time. Equivalent to multiplying all \b DiffEqn in the \b System with a scalar.

Note: Systems define their own \ref Scope. This implies that values of symbols defined within a System are not accessible outside of the System.

**/
/**
\defgroup Event
\ingroup MathExpressions

Environment for conditionally executed set of assignments.

- \b time-step: if specified, Condition is evulated in regular intervals (\b time-step). If not specified, If no time-step is provided, the minimal time-step of the input symbols is used.
- \b trigger: whether assigments are executed when the Condition turns from false to true (trigger = "on change", as in SBML) or whenever the condition is found true (trigger="when true").

\b Condition: expression to evaluate to trigger assignments.

\section Example
Set symbol "c" (e.g. assume its a CellProperty) to 1 after 1000 simulation time units
\verbatim
<Event trigger="on change" time-step="1">
    <Condition>a > 10</Condition>
    <Rule symbol-ref="c">
        <Expression>1</Expression>
    </Rule>
</Event>
\endverbatim
**/


/**
\defgroup Space
\ingroup ModelStructure

The \ref Space element specifies the size, structure and boundary conditions of the spatial lattice. 

A \ref SpaceSymbol can be used to create a symbol to the current (x,y,z) location. 

\ref MembraneLattice specifies the resolution of membrane-bound Fields. 

\section Examples
Linear lattice with periodic boundary conditions. See ShellCA example.
\verbatim
<Space>
	<Lattice class="linear">
		<Size value="100, 0, 0"/>
		<BoundaryConditions>
			<Condition boundary="x" type="periodic"/>
		</BoundaryConditions>
	</Lattice>
</Space>
\endverbatim

Hexagonal lattice. See LateralSignalling example.
\verbatim
<Space>
	<Lattice class="hexagonal">
		<Size value="20 20 0"/>
		<BoundaryConditions>
			<Condition boundary="x" type="periodic"/>
			<Condition boundary="y" type="periodic"/>
		</BoundaryConditions>
	</Lattice>
</Space>
\endverbatim

Space specification with image-based domain. See Crypt example.
\verbatim
<Space>
	<Lattice class="square">
		<Size symbol="size" value="600 600 0"/>
		<Neighborhood>
			<Order>3</Order>
		</Neighborhood>
		<Domain boundary-type="noflux">
			<Image path="crypt.tif"/>
		</Domain>
	</Lattice>
	<SpaceSymbol symbol="l"/>
</Space>
\endverbatim
**/

// ============================================================

/**
\defgroup Lattice
\ingroup Space

Specifies the size and structure of the lattice. 

The \b class attribute determines the structure of the regular lattice:
- linear: 1D
- square: 2D
- hexagonal: 2D
- cubic: 3D

\b Size determines the size of the lattice in (x,y,z). A symbol can be specified to refer to the lattice size.

\b NodeLength specifies the physical length of a lattice node. 

\b BoundaryConditions specify the type of boundary condition for each boundary:
- periodic (a.k.a. wrapped). Default.
- noflux (a.k.a. Neumann)
- constant (a.k.a. Dirichlet)

\b Neighborhood determines the size of the neighborhood to be used in calculations. This can be provided in terms of:
- Distance: Maximal distance to take into account, in units of lattice nodes.
- Order: Order of the neighborhood. E.g. in a 2D lattice, 1st order=4-members (von Neumann), 2nd order=8-members (Moore), etc.

\b Domain specifies a non-regular geometry to restrict the simulation to a domain within the lattice
This can imported from a 8-bit TIFF image, loaded from file. By convention, non-zero pixels are foreground, zero pixels are background.
**/

// ============================================================

/**
\defgroup MembraneLattice
\ingroup Space

Defines the discretization of the membrane property system (\ref MembraneProperty), which is represented by a field on a unit sphere / circle, that is mapped to the actual cell boundary. 

\b Resolution specifies the lattice discretization of the membrane-bound fields.
This resolution is equal for all MembraneProperties and for all cells. By convention, the x and y-resolution in 2D MembraneProperties are identical.

Optionally, a \b symbol can be specified to refer to the lattice discretization.

\b SpaceSymbol can be specified to refer to the current location with respect ot a membrane property. Positions are given as a vector (x,y,z) within the unit sphere / circle representing the memrane field.
This can be used to initialize membrane properties (see example below).

\section Note
The resolution can have serious impact on computational performance, in particular for reaction-diffusion systems on membranes of large cell populations.

\section Example
To specify a membrane property with a lattice discretization of 100 and definition of symbols for the membrane size and location (from PCP example):
\verbatim
<MembraneLattice>
	<Resolution symbol="memsize" value="100"/>
	<SpaceSymbol symbol="m"/>
</MembraneLattice>
\endverbatim

Note that the symbols defined above can be used initialize the membrane property, independent of the lattice discretization. 
Here, using a sine wave, scaled between 0 and 1 by just referring to the x part of the current membrane position.
\verbatim
<InitProperty symbol-ref="membrane">
	<Expression> 0.5*(m.x+1.0) </Expression>
</InitProperty>
\endverbatim


**/

// ============================================================

/**
\defgroup SpaceSymbol
\ingroup Space
\brief Specifies a symbol referring to the current location.

Specifies a symbol referring to the current location as a 3D vector (x,y,z). 

This symbol can then be used to make aspects dependent on space, such as gradient field

\section Example
To create a gradient along the x direction from 0 to 1, first specify a SpaceSymbol:
\verbatim
<Space>
	<Lattice class="square">
		<Size symbol="size" value="20 20 0"/>
	</Lattice>
	<SpaceSymbol symbol="l" name="location"/>
</Space>
\endverbatim

And then create a Field with location-dependent initial condition, using this symbol (see FrenchFlag example):
\verbatim
<Global>
	<Field init-expression="l.x / size.x" symbol="f" >
	</Field>
<Global>
\endverbatim

Or to make variable spatially heterogeneous (see Turing pattern example):
\verbatim
<Function symbol="A">
	<Expression>0.07 + ((0.07 * l.y)/ size.y)</Expression>
</Function>
\endverbatim

**/

/**
\defgroup Time
\ingroup ModelStructure

Sets duration of simulation and random seed. 

\b StartTime specifies the initial time.

\b StopTime specifies the final time. Should be larger than StartTime.

\b TimeSymbol specifies a symbol to refer to the current time.

\b RandomSeed specifies a seed for the pseudo-random number generator. 
For multithreaded simulations, not that each thread receives its own seed, based on the one specified by the user. 
This implies that for exact reproduction of simulation results, the RandomSeed as well as the number of parallel threads must be equal to the original simulation.

\b StopCondition provides a condition to terminate the simulation.

\b SaveInterval specifies the interval for checkpointing: writing complete simulation state to file (xml.gz). 
Use the special value '-1' to never save simulation state (default) or '0' to save state at end of simulation (either \b StopTime or after fulfilling \b StopCondition).


\section Example
\verbatim
<Time>
	<StartTime value="0.0"/>
	<StopTime value="1.0"/>
	<TimeSymbol symbol="t" name="time">
	<RandomSeed value="1234"/>
	<SaveInterval value="-1"/>
	<StopCondition>
		<Condition> celltype.ct1.size == 0 </Condition>
	</StopCondition>
</Time>
\endverbatim
**/

/**
\defgroup CellTypes
\ingroup ModelStructure

Container for specification of different \ref CellType elements that specify cell properties and behaviors.

**/


/**
\defgroup CellType
\ingroup CellTypes

A CellType specifies the cell properties, intracellular dynamics and cell behaviors of the cells within the population.

CellType can contain any of the following plugin types:
- \ref Symbols
- \ref MathExpressions
- \ref CellMotilityPlugins
- \ref CellShapePlugins
- \ref ReporterPlugins
- \ref MiscellaneousPlugins

A CellType defines its own \ref Scope. This implies that symbols defined within a CellType are not accessible outside of the CellType.
However, if the identical symbol is defined in all CellTypes, it is also accessible at the global scope.

Initial configuration of cell population must be specified in \ref CellPopulations section.

**/

/**
\defgroup CPM
\ingroup ModelStructure

Specifies parameters for a cellular Potts model (CPM) which provides a MonteCarlo sampler that evolves a spatial cell configuration on the basis of a Hamiltonian definition by statistical sampling.

\f$ H = \f$

\f$ P = \f$

\b Interaction specifies adhesion energies \f$ J_{\sigma, \sigma} \f$for different intercellular \ref Contact. The interaction energy is normalized by the size of the interaction neighborhood.
- \b Neighborhood specifies the neighborhood for the interaction energy.

\b MCSDuration scales the Monte Carlo Step (MCS) to the simulation time. One MCS is defined as a number of update attempts equal to the number of lattice sites.

\b MetropolisKinetics:
- \b stepper: algorithm to sample lattice sites: \b edgelist chooses form list all lattice sites that can potentially change cofiguration; \b random sampling chooses lattice site with uniform random distribution over all lattice sites.
- \b temperature: specifies Boltzmann probability to accept updates that increase energy.
- \b yield: offset for Boltzmann probability distribution representing resistance to membrane deformations (see Kafer, Hogeweg and Maree, PLoS Comp Biol, 2006).

\section References

Graner, Glazier, 1992

Kafer, Hogeweg and Maree, PLoS Comp Biol, 2006


**/


/**
\defgroup CellPopulations
\ingroup ModelStructure

Container for multiple \ref Population elements that specify the initial conditions or spatial configuration.

**/

/**
\defgroup Population
\ingroup CellPopulations

Specify the spatial configuration and cell states of a cellular population.

Spatial configuration can be generated by \ref InitializerPlugins.

If SaveInterval is specified (see \ref Time), the simulation state for each cells in a population is written to Population/Cell elements. 

**/

/**
\defgroup InitProperty
\ingroup Population
\brief Sets property value cell created during initialization.

InitProperty sets the value of a cell-bound \ref Property during the initialization of a cell. May contain expressions. 

Expressions are evaluated separately for each cell, such that properties can be made stochastically or dependent on cell-position.

Note the difference to initialization with CellType/Property: InitProperty is called ONLY during initialization (at StartTime, see \ref Time). 
Therefore, InitProperty is NOT called for cells created during simulation, e.g. using the \ref AddCell plugin.

**/

/**
\defgroup Cell
\ingroup Population
\brief Stores cell state

This element is used to store the cell-based simulation state. 

If SaveInterval (see \ref Time) is specified, cell states are automatically written to this element in the checkpointing files (xml.gz).

This includes:
- \b Center: cell center, center of mass
- \b Nodes: full list of lattice nodes occupied by the cell
- \b PropertyData: symbol with current value
- \b PropertyVectorData: symbol with current x,y,z values
- \b DelayPropertyData: symbol with history of values
- \b MembranePropertyData: symbol with scalar field with current values

Note, this element is not meant for human specification. 
**/

/**
\defgroup Analysis
\ingroup ModelStructure

Container for \ref AnalysisPlugins for data analysis, logging and visualization. Output is written to text files and/or images.

**/

/**
\defgroup ParamSweep
\ingroup ModelStructure

Specification of batch process for parameter exploration or sensitivity analysis.

- Parameter(s) are selected via the context menu.
- Parameter values are specified as a list or range under \b Values.
- To change parameters consecutively instead of combinatorily, drag-and-drop a parameter on another.
- Execute the batch process through the \b Start button in the \b ParamSweep panel.

**/

/**
 \defgroup Plugins
 \brief Plugins 
 **/ 

/** \defgroup CellMotilityPlugins Cell Motility Plugin
 \ingroup Plugins
 \brief Plugins that implement cell motility mechanisms
*/

/** \defgroup CellShapePlugins Cell Shape Plugins
 \ingroup Plugins
 \brief Plugins that alter cell shape
 */

/** \defgroup InteractionEnergyPlugins Interaction Energy Plugins
 \ingroup Plugins
 \brief Plugins that determine interaction energies
*/

/** \defgroup MiscellaneousPlugins Miscellaneous Plugins
 \ingroup Plugins
 \brief Plugins for population management and auxiliary plugins
 */

/**
 * \defgroup Concepts
\defgroup Scope
\ingroup Concepts

A \b Scope is a portion of the model in which a symbol is defined and valid. Symbols defined in any of these scopes are invalid outside of this scope. This is analogous to the local and global variables in most programming languages.

The following model elements define their own scopes:
- \ref Global
- \ref CellType
- \ref System (including Trigger environments)

Symbols are inherited from the global to local scopes, but may be overwritten in local scopes, even to differ in constness and granularity (e.g. Global/Constant may be overwritten by a System/Variable). 
The type of the symbol (scalar / vector), however, has to be identical. In this way, global symbols can be used as default values.

Unlike the other scopes, the \ref CellType scope is provides a spatial compartment, such that symbol defined in the CellType scope
can only be resolved at the spatial positions occupied by cells of this CellType. Therefore, in some cases, it may be required to provide a global constant as a default value.

As a special case, when a symbol is declared in all local scopes (e.g. in all CellTypes), it also becomes available in the global scope. (Known as a virtual composite symbol.)

\section Examples
In the following example, 'a=1' is declared in the Global scope, and 'b=2' is declared in the System scope. The global variable 'result' will yield '3'.
\verbatim
<Global>
	<Constant symbol="a" value="1"/>
	<Variable symbol="result" value="0"/>

	<System solver="euler" time-step="1.0">
		<Constant symbol="b" value="2"/>
		<Rule symbol-ref="result">
			<Expression>a+b</Expression>
		</Rule>
	</System>
</Global>
\endverbatim
______
Here, the global constant 'a=1' is overwritten in by the local constant 'a=2', such that 'result' will yield '4'.
\verbatim
<Global>
	<Constant symbol="a" value="1"/>
	<Variable symbol="result" value="0"/>

	<System solver="euler" time-step="1.0">
		<Constant symbol="a" value="2"/>
		<Constant symbol="b" value="2"/>
		<Rule symbol-ref="result">
			<Expression>a+b</Expression>
		</Rule>
	</System>
</Global>
\endverbatim
______
Symbols can be re-used with different local scopes. Here, the symbol 'p' is used in different CellTypes. 
In 'ct1', 'p' is a constant with value '0'. In 'ct2', 'p' is a constant with value '1.0'.
In 'ct3', 'p' denote a cell-bound Property and in 'ct4' it represents a MembraneProperty.

Because 'p' is defined in all CellTypes, it is automatically also available in the Global scope. 
\verbatim
<CellTypes>
	<CellType class="biological" name="ct1">
		<Constant symbol="p" value="0"/>
	</CellType>
	<CellType class="biological" name="ct2">
		<Constant symbol="p" value="1.0"/>
	</CellType>
	<CellType class="biological" name="ct3">
		<Property symbol="p" value="1"/>
	</CellType>
	<CellType class="biological" name="ct4">
		<MembraneProperty symbol="p" value="l.x / size.x">
			<Diffusion rate="0.0"/>
		</MembraneProperty>
	</CellType>
</CellTypes>
\endverbatim

**/

/**
\defgroup Interpreter
\ingroup Concepts



**/


/**
\defgroup Scheduling
\ingroup Concepts

\section Schedule

Initialization

- \b Phase 1: \ref ContinuousProcessPlugins

  + Diffusion, ... CFL
  + System, including 
  + CPM, incl. \ref CPMHamiltonianPlugins

- \b Phase 2: \ref InstantaneousProcessPlugins

  + \ref ReporterPlugins
  + ...?

  
- \b Phase 3:

  + \ref AnalysisPlugins
  + ...?

\section Order

Tracking symbolic dependencies.

\section Intervals

1. as often as input can change.
2. not more often than output is used.
**/

/**
\defgroup Parallelization
\ingroup Concepts

Multithreading
openMP

**/


/**
\defgroup MuParser Evaluating math expressions
\ingroup Concepts

muParser


**/