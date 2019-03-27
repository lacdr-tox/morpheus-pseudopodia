# ChangeLog

## Release 2.1

### GUI
  * Reworked DelayProperty/Variable that allows varying delay times and history initialisation from expression
  * Largely improve SBML import
     * Provide more import target options
     * Add support for multiple compartments and variable size compartments
     * Support delays by via DelayProperties
     * Support Events with delays
     * Support HMC (comp)
  * Fix MacOS crash on double-clicking symbol list
  * Fix Windows SBML support (suitible library build)

### MorpheusML
  * Expose local symbols to the input of the Neighborhood(Vector)Reporter
  * Allow constant expressions in time-step specifications
  * MorpheusML version bump 4.0

### Simulator
  * Add an adaptive step size ODE solver (RK45-DormandPrince, RK45-CashKarp, RK23-BogackiShampine)
  * Renewed implementation of Poissonian Disc Population Initializer
  * Command line option for setting the output directory added
  * Fix rare misplacement in box object initializer

## Patch 2.0.1
  * Fixed VectorRules not working
  * Fixed broken image table for parameter sweeps

## Release 2.0

### MorpheusML
  * The generalized **Mapper** now takes care to map information between spatial contexts, replaces the CellReporter
  * The **Function** plugin now supports parametric functions and function overloading.
  * The new **External** plugin allows to run external code during analysis steps
  * The new **ContactLogger** tracks cell contacts over time
  * The value of a **Constant** can be provided via expression
  * The new **VectorField** can represent spatial vector data
  * Multiple **Population** per CellType are now supported
  * **GnuPlotter** layout adjustments to efficiently support large lattices
  * Binary **VTK** export (performance)
  * Full support for **snapshotting** simulation states
  * Removed any remains of time / space units

### GUI
  * Largely improved inApp documentation
  * Adaptive multiline editor panel for expressions in attributes
  * Reenabled SBML import
  * Allow Copy/Paste & Drag/Drop of external XML model snippets 
  * Optionally preset random seeds for parameter sweeps

### Simulator
  * Many fixes to cell population initializers
  * **Symbolic Linking** infrastructure rewritten to enable extensibility
  * Scheduling fixes for **DelayProperties**
  * **Gnuplotter** and **Logger** now deal better with sub and superscript in symbol names or descriptions
  * **Field boundaries** can be expressions
  * Added **GoogleTest** as testing framework
  * Performance improvements, i.e.
    * Added dynamic EdgeTracker defragmentation
    * Parallelized diffusion in domain Fields
    * Precalculating constant expressions at initialisation
    * ...

### others
  * Tons of compiler compatibility fixes
  * Doxygen issue workarounds
  
## 1.9.3 
  * Initial import

