#include "property.h"
// #include "expression_evaluator.h"

template <> string Container<double>::ConstantXMLName()  { return "Constant";};
template <> string Container<VDOUBLE>::ConstantXMLName() { return "ConstantVector";};
template <> string Container<double>::VariableXMLName() { return "Variable";};
template <> string Container<VDOUBLE>::VariableXMLName() { return "VariableVector";};
template <> string Container<double>::CellPropertyXMLName() { return "Property";};
template <> string Container<VDOUBLE>::CellPropertyXMLName() { return "PropertyVector";};

template <> string Container<vector<double> >::ConstantXMLName() { return "ConstantArray";};
template <> string Container<vector<double> >::VariableXMLName() { return "VariableArray";};
template <> string Container<vector<double> >::CellPropertyXMLName() { return "PropertyArray";};

template <> 
bool Container<double>::type_registration = PluginFactory::RegisterCreatorFunction( Container<double>::ConstantXMLName(),Container<double>::createConstantInstance) 
								   && PluginFactory::RegisterCreatorFunction( Container<double>::VariableXMLName(), Container<double>::createVariableInstance)
								   && PluginFactory::RegisterCreatorFunction( Container<double>::CellPropertyXMLName(), Container<double>::createCellPropertyInstance);

template <> 
bool Container<VDOUBLE>::type_registration = PluginFactory::RegisterCreatorFunction( Container<VDOUBLE>::ConstantXMLName(),Container<VDOUBLE>::createConstantInstance) 
								   && PluginFactory::RegisterCreatorFunction( Container<VDOUBLE>::VariableXMLName(), Container<VDOUBLE>::createVariableInstance)
								   && PluginFactory::RegisterCreatorFunction( Container<VDOUBLE>::CellPropertyXMLName(), Container<VDOUBLE>::createCellPropertyInstance);
