# ChangeLog

## Release 2.2.4

### GUI
  * Reduce memory consumption of text preview in the job view
  
### Simulator
  * Rework circular reference detection in the initialisation cycle
  * Fix color bar limits in Logger Plots
  * Add command line option to skip any gnuplot tasks (thus does not choke on missing gnuplot)
  * Fix command line parameter override using "--set" option


## Release 2.2.3

### GUI
  * Fixed model graph generator library that may have caused outdated model graphs.

### Simulator
  * Added loop dependency detection in Field initialization
  * Fix loading 8-bit Tiff images
  * Consistently center Tiff image data when smaller than lattice size
  * Fixed regression: Prevent rescheduling of the CPM sampler
  * Fix build in debug mode

## Release 2.2.2

### MorpheusML
  * Merged the **MechanicalLink** component

### GUI
  * Fixed command line parsing of certain url/file references 
  * Fixed checkbox lists on MacOS (issue #223)
  * Preserve XML Comments in MorpheusML models

### Simulator
  * Reenable output of performance statistics, if no json stats are requested.
  * Fixed logging MembraneProperties
  * Fixed certain parallel **Mapper** modes

## Release 2.2.1

### Simulator
  * Fix boost linking for static builds
  * Fix writing performance statistics

## Release 2.2.0

### GUI
  * Introduced an Interactive Configurable Model Graph
  * Support Tagging, Filtering and Sorting of Model Components
  * Switched to Qt5
  * Support reading and writing (gz) compressed models
  * Support for morpheus://... url scheme, that allows loading resources through url links

### MorpheusML
  * The space symbol (Space/SpaceSymbol) now always provides the location in orthogonal coordinates, also on hexagonal lattices.
  * AddCell accepts Count instead of Condition as the number of cells to be placed.
  * ClusterTracker can cluster cells of multiple cell types.
  * Logger
    * gained support for conditional logging using Logger/Restrictions/@condition
    * learned to use discrete colors for integer data
  * Gnuplotter
    * allows to set a z-slice per Plot and also Arrows and Labels follow the z-slize filter.
    * cell opacity moved to Plot/Cells/@opacity
  * Added Populations InitVectorProperty with optional spherical notation
  * Added VectorMapper as a counterpart to the scalar Mapper
  * Lattice size now can also be specified through expressions
  * Contact energies now support expressions with access to symbols of involved cells
  * Rework of XSD specifcation
    * XSD parser supports extension of complexTypes
    * Plugins are extension of base types
    * Registration of plugins as members of <xs:all> groups
  * Added **@tags** and **Annotation** nodes to all model components

### Simulator
  * Performance improvements for Mappers and Reporters using OpenMP parallelization
  * Support reading (gz) compressed models
  * Provide performance statistics as json file using the --perf-stats option
  * Added Test system for full XML models
  * Restructured CMake build to make use of targets and boosted requrements to cmake>=3.3.0
  * Build system now supports building on Windows MSys2 and Mac Homebrew in addition to linux systems.

### Bug Fixes
  * Fix cell property initialization override priority (InitProperty takes highest priority)
  * Fixed diffusion scaling on hexagonal lattices (was raised by factor 0.5)
  
## Release 2.1.1

### Bug fixes
  * Fix a rare crash upon ChangeCellType 
  * Fix reading numeric html encoded utf8 characters

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
  * Fix Windows Job removal to also remove all related files

### MorpheusML
  * Expose local symbols to the input of the Neighborhood(Vector)Reporter
  * Allow constant expressions in time-step specifications
  * MorpheusML version bump 4.0

### Simulator
  * Add adaptive step size ODE solvers: adaptive45 (Dormand-Prince), adaptive45_ck (Cash-Karp), adaptive_23 (Bogacki-Shampine)
  * Renewed implementation of Poissonian Disc Population Initializer
  * Command line option for setting the output directory added
  * Fix rare misplacement in box object initializer
  * Reduce memory footprint

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

