#ifndef EXAMPLEPLUGIN_H
#define EXAMPLEPLUGIN_H

// include the plugin interfaces (required) and plugin parameters (very useful) 
#include "core/interfaces.h"
#include "core/plugin_parameter.h"
using namespace SIM;

// Doxygen documentation as it will appear in documentation panel of GUI
/** \defgroup ExamplePlugin
\ingroup ML_CellType
\ingroup CellTypePlugins
\brief Brief description of my example plugin

A detailed description of my example plugin that can contain mathematical formulae in LaTex format: \f$ \Delta A = \sum_{\sigma} a-b*c^2 \f$ and references to publications with <a href="http://dx.doi.org/">links</a>.

\section Example usage
\verbatim
<ExamplePlugin threshold="123" expression="a+b*c" />
\endverbatim
*/

// class declaration, and inheritance of plugin interface
class ExamplePlugin : public InstantaneousProcessPlugin
{
private:
	// parameters that are specified in XML (as values, strings or symbolic expressions)   
	PluginParameter2<double, XMLValueReader, DefaultValPolicy> threshold;
	PluginParameter2<double, XMLEvaluator, RequiredPolicy> expression;

	// auxiliary plugin-internal variables and functions can be declared here.
	CellType* celltype;
public:
	// constructor
	ExamplePlugin();
	// macro required for plugin integration 
	DECLARE_PLUGIN("ExamplePlugin");

	// the following functions are inherited from plugin interface and are overwritten
	// - load parameters from XML, called before initialization
	void loadFromXML( const XMLNode ) override;
	// - initialize plugin, called during initialization 
	void init( const Scope* ) override;
	// - execute plugin, called periodically during simulation (automatically scheduled)
	void executeTimeStep() override;
};
#endif
