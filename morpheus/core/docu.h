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
\defgroup MorpheusML MorpheusML
 */

// ============================================================
/** 
\defgroup Description
\ingroup MorpheusML

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
\defgroup ML_Global Global
\ingroup MorpheusML

Section to include mathematical variabes and equations in the global \ref Scope.

\b Global provides section for:
- Global \ref ML_Constant and \ref ML_Variable that can be overridden in local scopes such as \ref ML_CellType and \ref ML_System. In this case, \b Global serve as default values.
- \b ODE systems: \ref ML_System of \ref ML_DiffEqn operating on \ref ML_Variable
- \b Reaction-diffusion PDE systems: \ref ML_System of \ref ML_DiffEqn operating on diffusive \ref ML_Field
- Global \b Events: \ref ML_Event changing global variables.

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
		<Diffusion rate="0.02" />
	</Field>
	<Field symbol="i" value="0.1" name="inhibitor">
		<Diffusion rate="1" />
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

\b Symbols represent data sources that may vary in time, space. 

Valid symbol identifiers may contain the following characters

- Latin chars: abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ
- Greek chars: αβγδεζηθικλμνξοπρσςτυφχψωΑΒΓΔΕΖΗΘΙΚΛΜΝΞΟΠΡΣΤΥΦΧΨΩ
- Numbers: 0123456789
- Special chars: ._

but may not start with a number.

The \b name attribute is used for descriptive purposes only. In particular, graph labels will carry this information. You may use latex style super- and subscripts
  * (c_{1})^{2} will become \f$ (c_1)^{2} \f$

**/

/**
\defgroup MathExpressions
  
Mathematical expressions to be evaluated during run-time. The vector version uses the component-wise 'x,y,z' notation, or -- if available -- the spherical notation 'phi,theta,radius'. 

\section Available Operators:
+, -, *, /, ^, =, >=, <=, !=, ==, <, >
and, or, xor, !

\section Functions
  - Logical:  if([condition], [then], [else]), and, or, xor
  - Trigonometric: sin, cos, tan, asin, acos, atan, sinh, cosh, tanh, asinh, acosh, atanh
  - Exponential: log2, log10, ln, exp, pow, sqrt,
  - others: sign, rint, abs, min, max, sum, avg, mod
  

**MathML compatibility Functions**:
  - piecewise, lt, leq, eq, neq, geq, gt, arcsin, arccos, arctan, arcsinh, arccosh, arctanh

Additional functions can be defined using \ref ML_Function.

\section Random Random number generators
  - rand_uni([min], [max])
  - rand_norm([mean], [stdev])
  - rand_gamma([shape], [scale])
  - rand_int([min], [max])
  - rand_bool()

**/

/**
\defgroup ML_Constant Constant
\ingroup ML_Global
\ingroup ML_CellType
\ingroup ML_Contact
\ingroup ML_System
\ingroup ML_Event
\ingroup ML_Analysis
\ingroup Symbols

Symbol with a fixed scalar value given by a \ref MathExpressions.
**/
/**
\defgroup ML_ConstantVector ConstantVector
\ingroup ML_Global
\ingroup ML_CellType
\ingroup ML_System
\ingroup ML_Event
\ingroup ML_Analysis
\ingroup Symbols

Symbol with a fixed 3D vector.

Syntax is comma-separated: x,y,z
**/
/**
\defgroup ML_Variable Variable
\ingroup ML_Global
\ingroup ML_CellType
\ingroup Symbols

Symbol with a variable scalar value. The initial value is given by a  given by a \ref MathExpressions.
**/
/**
\defgroup ML_VariableVector VariableVector
\ingroup ML_Global
\ingroup ML_CellType
\ingroup Symbols

Symbol with a variable 3D vector value. The initial value is given by a  given by a \ref MathExpressions.

Syntax is comma-separated: x,y,z
**/

/**
\defgroup ML_DelayVariable DelayVariable
\ingroup ML_Global
\ingroup Symbols

Symbol with a scalar value and a \b delay time before an assigned values become current. The initial value and history is given by a  given by a \ref MathExpressions.
**/

/**
\defgroup ML_Property Property
\ingroup ML_CellType
\ingroup Symbols


Symbol with a cell-bound variable scalar value. The initial value is given by a  given by a \ref MathExpressions and may contain stochasticity to create diversity.
**/

/**
\defgroup ML_DelayProperty DelayProperty
\ingroup ML_CellType
\ingroup Symbols


Symbol with a cell-bound scalar value and a \b delay time before values become current. The initial value and history is given by a  given by a \ref MathExpressions
**/

/**
\defgroup  ML_PropertyVector PropertyVector
\ingroup ML_CellType
\ingroup Symbols

Symbol with cell-bound, variable 3D vector value. The initial value and history is given by a  given by a \ref MathExpressions.

Syntax is comma-separated: x,y,z
**/


/**
\defgroup ML_DiffEqn DiffEqn
\ingroup MathExpressions
\ingroup ML_System

Assignment of a rate equation to a symbol.

Ordinary differential equation \f$ \frac{dX}{dt}=a \f$ if \f$ X \f$ is a \ref ML_Variable or a \ref ML_Property

Partial differential equation \f$ \frac{\partial X}{\partial x}=D_{X}\nabla^2X+a \f$ where \f$ X \f$ is a \ref ML_Field and \f$ D_{X} \f$ is its diffusion coefficient.

DiffEqn are only allowed within \ref ML_System
**/


/**
\defgroup ML_System System
\ingroup ML_Global
\ingroup ML_CellType
\ingroup MathExpressions ContinuousProcessPlugins

Environment for tightly coupled \ref ML_Rule and \ref ML_DiffEqn. Expressions with a System are synchronously updated and may contain recurrence relations.

- \b solver: numerical solver for DiffEqn:
  - Adaptive time step solvers: 
    - \b adaptive45 - Dormand-Prince 4/5th order, \b default
    - \b adaptice45-ck - Cash-Karp 4/5th order
    - \b adaptive23 - Bogacki-Shampine 2/3rd order
  - Fixed time step solvers
     - \b fixed1 aka \b euler - Euler 1st order
     - \b fixed2 aka \b heun - Heun 2nd order (aka explicit trapezoid rule)
     - \b fixes4 aka \b runge-kutta - Runge Kutta 4th order
  - \b Stochastic fixed time step: 
    - use \b stochastic - Euler Maruyama method (\b euler also autodetects stochasticity)
  - \b Stiff/non-stiff adaptive 
    - Cash-Karp + Rosenbrock (planned)
  
- \b time-step:
  - \b Fixed schemes: integration step size, given in system time.
  - \b Adaptive schemes: Coupling interval given in system time, i.e. maximum step size without coupling to other processes.
- \b time-scaling (optional): scales the dynamics of \b ML_System to the simulation time. Equivalent to multiplying all \b ML_DiffEqn in the \b ML_System with a scalar.

Note: Systems define their own \ref Scope. This implies that values of symbols defined within a System are not accessible outside of the System.

**/
/**
\defgroup ML_Intermediate Intermediate
\ingroup ML_System
\ingroup ML_Event

An Intermediate Symbol is available to all expressions within a System. Intermediates are evaluated prior to any other construct and may even depend on each other. Defining circular dependencies is discouraged, though.
**/

/**
\defgroup ML_IntermediateVector IntermediateVector
\ingroup ML_System
\ingroup ML_Event

An IntermediateVector Symbol is available to all expressions within a System. Intermediates are evaluated prior to any other construct and may even depend on each other. Defining circular dependencies is discouraged, though.
**/

/**
\defgroup ML_Event Event
\ingroup MathExpressions InstantaneousProcessPlugins
\ingroup ML_Global
\ingroup ML_CellType

Environment for conditionally executed set of assignments.

- \b time-step: if specified, Condition is evulated in regular intervals (\e time-step). If not specified the minimal time-step of the input symbols is used.
- \b Condition: expression that must evaluate true to trigger assignments.
- \b Condition/history: initial value of the condition. Used to determine whether an initially true condition may trigger if \e trigger="on-change".
- \b trigger: whether assigments are executed when the Condition turns from false to true (\e trigger="on-change", as in SBML) or whenever the condition is found true (\e trigger="when-true").
- \b delay: time by which the execution of the assignments of the event are delayed.
- \b persistent: a delayed event who's condition \e fell false meanwhile a delay does only execute if \e persistent="true". (default \e true)
- \b compute-time: time at which the values of the assignments are computes \e on-trigger / \e on-execution.

\section Example
Set symbol "c" (e.g. assume it's a CellProperty) to 1 after 1000 simulation time units
\verbatim
<Event trigger="on change" time-step="1">
    <Condition> time > 1000 </Condition>
    <Rule symbol-ref="c">
        <Expression>1</Expression>
    </Rule>
</Event>
\endverbatim
**/


/**
\defgroup ML_Space Space
\ingroup MorpheusML

The \ref ML_Space element specifies the size, structure and boundary conditions of the spatial lattice. 
The types of boundary conditions are homogeneous among all model parts, while the exact values can be specified through \b BoundaryValue in a \ref ML_Field or in \ref ML_CellPopulations for the cell layer.

A \ref ML_SpaceSymbol can be used to create a symbol representing the current (x,y,z) location. The symbol always provides the location in orthogonal coordinates. It is not (yet) scaled by the defined node length.

\ref ML_MembraneLattice specifies the resolution of membrane-bound Fields. These fields are represented by a sphere-wrapped periodic lattice (0..resolution-1, 0..resolution/2-1, 0). The \b SpaceSymbol refers to the index position of within the lattice.

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
\defgroup ML_Lattice Lattice
\ingroup ML_Space 

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

\ref ML_Domain specifies a non-regular geometry to restrict the simulation to a domain within the lattice.
**/

// ============================================================

/**
\defgroup ML_MembraneLattice MembraneLattice
\ingroup ML_Space

Defines the discretization of the membrane property system (\ref ML_MembraneProperty), which is represented by a field on a unit sphere / circle, that is mapped to the actual cell boundary. 

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

Note that the symbols defined above can be used to initialize the membrane property, independent of the lattice discretization. 
Here, using a sine wave, scaled between 0 and 1 by just referring to the x part of the current membrane position.
\verbatim
<InitProperty symbol-ref="membrane">
	<Expression> 0.5*(m.x+1.0) </Expression>
</InitProperty>
\endverbatim


**/

/**

\defgroup ML_Domain Domain
\ingroup ML_Lattice

A \b Domain specifies a non-regular geometry that restricts the simulation to a domain within the lattice. Boundary condititions can be chosen to be either constant or no-flux, but are required to be homogeneous. 

The \b Image tag allows to import a domain shape from a 8-bit TIFF image. By convention, non-zero pixels are foreground, zero pixels are background.

The \b Circle tag allows to define circular domain shapes.

The \b Hexagon tag allows to define hexagonal domain shapes.

**/
// ============================================================

/**
\defgroup ML_SpaceSymbol SpaceSymbol
\ingroup ML_Space

Specifies a symbol referring to the current location as a 3D vector (x,y,z). 

This symbol can then be used to make aspects dependent on space, such as a gradient field

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
\defgroup ML_Time Time
\ingroup MorpheusML

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
\defgroup ML_CellTypes CellTypes
\ingroup MorpheusML

Container for specification of different \ref ML_CellType elements that specify cell properties and behaviors.

**/


/**
\defgroup ML_CellType CellType
\ingroup ML_CellTypes

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

In addition, celltype scopes provide symbols to access cell properties
- cell.id
- cell.center
- cell.volume
- cell.type
- cell.surface

Initial configuration of cell population must be specified in \ref ML_CellPopulations section.

**/


/**
\defgroup ML_CellPopulations CellPopulations
\ingroup MorpheusML

Container for multiple \ref ML_Population elements that specify the initial conditions or spatial configuration.

**/

/**
\defgroup ML_Population Population
\ingroup ML_CellPopulations

Specify the spatial configuration and cell states of a cellular population.

Spatial configuration can be generated by \ref InitializerPlugins.

If SaveInterval is specified (see \ref ML_Time), the simulation state for each cell in a population is written to Population/Cell elements. 

**/

/**
\defgroup ML_Cell Cell
\ingroup ML_Population
\brief Stores cell state

This element is used to store the cell-based simulation state. 

If SaveInterval (see \ref ML_Time) is specified, cell states are automatically written to this element in the checkpointing files (xml.gz).

This includes:
- \b Center: cell center, center of mass
- \b Nodes: list of lattice nodes occupied by the cell
- \b PropertyData: value of a \ref ML_Property symbol
- \b PropertyVectorData: x,y,z values of a \ref ML_PropertyVector symbol
- \b DelayPropertyData: history of values of a \ref ML_DelayProperty symbol
- \b MembranePropertyData: current values of the scalar field of a \ref ML_MembraneProperty

Note, this element is not meant for manual specification by humans. 
**/

/**
\defgroup ML_InitProperty InitProperty
\ingroup ML_Population

InitProperty sets the value of a cell-bound \ref ML_Property or \ref ML_MembraneProperty during the initialization of a cell. May contain expressions. 

Expressions are evaluated separately for each cell, such that properties can become stochastic or dependent on cell-position.

Note the difference to initialization with CellType/Property: InitProperty is called ONLY during initialization (at StartTime, see \ref ML_Time). 
Therefore, InitProperty is NOT called for cells created during simulation, e.g. using the \ref ML_AddCell plugin.

**/

/**
\defgroup ML_Analysis Analysis
\ingroup MorpheusML

Container for \ref AnalysisPlugins for data analysis, logging and visualization. Output is written to text files and/or images.

**/

/**
\defgroup ParamSweep
\ingroup MorpheusML

Specification of a batch process for parameter exploration or sensitivity analysis.

- Parameter(s) are selected via the context menu.
- Parameter values are specified as a list or range under \b Values.
- To change parameters consecutively instead of combinatorily, drag-and-drop a parameter on another.
- Execute the batch process through the \b Start button in the \b ParamSweep panel.

**/

/**
 \defgroup Plugins Plugin Types
 \brief All Modules of morpheus belong to one or multiple basic plugin types that are listed below.
 **/ 

/** \defgroup CellMotilityPlugins Cell Motility Plugin
 \ingroup Plugins
 \brief Plugins that implement cell motility mechanisms
*/

/** \defgroup CellShapePlugins Cell Shape Plugins
 \ingroup Plugins
 \brief Plugins that alter cell shape
 */

/** \defgroup InteractionPlugins Interaction Plugins
 \ingroup Plugins
 \brief Plugins that determine cell interactions in terms of energies
*/

/** \defgroup MiscellaneousPlugins Miscellaneous Plugins
 \ingroup Plugins
 \brief Plugins for population management and auxiliary plugins
 */

/**
 * \defgroup Concepts
\defgroup Scope
\ingroup Concepts

\b Scopes manage the symbol of the (nested) the model sections, allow symbols registration by model components and symbol retrieval by others. Symbols defined in a scope are invalid outside of this scope, but available in all sub-scopes, i.e. nested sections. This is analogous to the local variable scoping in most programming languages.

The top-most scope is \ref ML_Global.
The following model elements define their own sub-scopes:
- \ref ML_CellType
- \ref ML_System (including \b Triggers of  \ref ML_Event and \ref ML_CellDivision)
- \ref ML_Function 

Ss stated above, symbols are inherited from the parental scope, but may be overridden, even to differ in constness and granularity (e.g. Global/\ref ML_Constant may be overwritten in a System by a \ref ML_Variable). 
The type of the symbol (scalar / vector), however, has to be conserved. In this way, global symbols can be used as default values.

Unlike the other scopes, the \ref ML_CellType scopes also represent spatial compartments. In order to adhere to intuitive modelling logics, we apply \b spatial \b scoping, such that symbols defined in a \ref ML_CellType scope can override parental, i.e. global, symbols in the (dynamic) spatial region the celltype occupies. Therefore, a global symbol can be effectively composed of a global value and celltype specific values defined within the celltypes themself. 
Forwarding the global access to a symbol at a specific position to through such a spatially structured symbols is what we call <b>spatial scoping</b>.

When a symbol is declared in \b all CellType scopes (e.g. in all CellTypes), it also becomes available in the global scope (known as a virtual composite symbol.)

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

/*
\defgroup Interpreter
\ingroup Concepts



*/


/**
\defgroup Scheduling
\ingroup Concepts

\section Schedule

Morpheus currently applies a static scheduling scheme, which means that the schedule is constructed before the simulation starts and remains unchanged until the end of the simulation. Numerical schemes, however, may subdivide the stepping provided by the static scheduling, as done by adaptive solvers and the stability criterion of fwd. euler diffusion.

The final scheme can be found in a simulation log.

In the initialization phase, all symbols are registered, the plugins and their interdependencies are analysed and a <b>dependency tree</b> is constructed. Using the dependency tree, a schedule is constructed along the following guidelines. 

- \b Correctness: Update time steps must be fine-grained enough.
- \b Order: Sequential order must obey the order in the directed acyclic dependency graph (DAG), which is constructed by opening up potential closed loops.
- \b Validity: Updates must be performed frequently enough to provide the latest input values for other plugins.
- \b Efficiency: Updates are not scheduled more often than the plugins' output is required.

The \b sequential update scheme will look as follows:

- \b Phase 1: \ref ContinuousProcessPlugins

  + \ref ML_Field diffusion (CFL condition)
  + \ref ML_System
  + \ref ML_CPM

- \b Phase 2: Sequential schemes ordered by dependencies 

  + \ref ReporterPlugins
  + \ref InstantaneousProcessPlugins

  
- \b Phase 3:

  + \ref AnalysisPlugins

**/

/**
\defgroup Parallelization
\ingroup Concepts

Morpheus employs \b OpenMP as the workload-sharing construct. CPM computation, however, does not yet make use of OpenMP.
Use the environmental variable \b OMP_NUM_THREADS to adjust the number of threads used.

**/

