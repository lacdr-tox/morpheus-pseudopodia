# ChangeLog

## Unreleased

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
  * Full support for **snapshoting** simulation states
  * Removed any remains of time / space units

### GUI
  * Largely Improved inApp Documentation
  * Adaptive size editor for expressions in attributes
  * Reenabled SBML import
  * Option to preset random seeds for parameter sweeps

### Simulator
  * Many fixes to cell pattern initializers
  * Rewrite of the **Symbolic Linking** infrastructure, allows extensibility
  * Scheduling fixes for **Functions** and **DelayProperties**
  * **Gnuplotter** and **Logger** now deal better with sub and superscript in symbol names or descriptions
  * Parallelize diffusion for **Fields** in domains
  * **Field boundaries** can be expressions
  * Add **GoogleTest** as testing framework

### others
  * Tons of compiler compatibility fixes
  * Doxygen issue workarounds
  
## 1.9.2 
  * Initial import

