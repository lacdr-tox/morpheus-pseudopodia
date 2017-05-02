#include "external.h"

REGISTER_PLUGIN(External);
using namespace subprocess;

External::External() {}

void External::loadFromXML(const XMLNode xNode)
{
	AnalysisPlugin::loadFromXML(xNode);

	if (xNode.nChildNode("Command")) {
		XMLNode xCommand = xNode.getChildNode("Command");
		getXMLAttribute(xCommand, "text", command_orig);
	}

	if( command_orig.empty() )
		throw MorpheusException("External analysis plugin requires a (non-empty) command.", stored_node);
	
	for(uint i=0; i < xNode.nChildNode("Environment"); i++){
		XMLNode xEnvVar =  xNode.getChildNode("Environment",i);
		// get variable name and value from XML
		string variable, value;
		getXMLAttribute(xEnvVar, "variable", variable);
		getXMLAttribute(xEnvVar, "value", value);
		// store them in a map
		environvars[variable]=value; 
	}	
	fork.setXMLPath("separate-thread");
	fork.setDefault("true");
	registerPluginParameter( fork );

}

void External::init(const Scope* scope)
{
	AnalysisPlugin::init(scope);
	replace_symbols = false;
	// find Global symbols in command
	vector<string> tokens = tokenize(command_orig);
	for(string& token : tokens){
		string symbol_name;
		if( *(token.begin()) == '%' ){
			symbol_name = token.substr(1,token.size());
			//cout << "SYMBOL "<< symbol_name << " FOUND" << endl;
			SymbolAccessor<double> symbol = SIM::findGlobalSymbol<double>( symbol_name );
			replace_symbols = true;
		}
	}

#if defined(_WIN32)
	throw MorpheusException("External plugin is not available for Windows-based systems. See https://github.com/arun11299/cpp-subprocess", stored_node);
#endif
}

void External::analyse(double time){
	execute();
}

void External::finish() {
	execute();
}

void External::execute(){
	string command = update_command(command_orig);
	
	if(environvars.size() == 0){
		auto p = Popen(command, shell{true}, output{"external_output.txt"}, error{"external_error.txt"});
		p.wait(); // Wait for the child to exit.
		auto obuf = p.communicate().first;
		//cout << "External output: " << obuf.buf.data() << endl;
		int returncode = p.retcode(); // The return code of the exited child.
		cout << "Posthoc analysis finished with return code: " << returncode << endl;
	}
	else{
		//cout << "With environment" << endl;
		for (auto& kv : environvars) {
			cout << kv.first << " : " << kv.second << endl;
		}
		auto p = Popen(command, environment{environvars}, shell{true}, output{"posthoc_output.txt"}, error{"posthoc_error.txt"});
		p.wait(); // Wait for the child to exit.
		auto obuf = p.communicate().first;
		//cout << "External output: " << obuf.buf.data() << endl;
		int returncode = p.retcode(); // The return code of the exited child.
		cout << "Posthoc analysis finished with return code: " << returncode << endl;
	}
}

/*
string External::update_command(string command){
	if(!replace_symbols){
		string command_new = command;
		// add ampersand to fork process into separate thread
		if( fork() )
			command_new += " &";
		return command_new;
	}
	
	// find Global symbols in command
	vector<string> tokens = tokenize(command);
	for(string& token : tokens){
		string symbol_name;
		if( *(token.begin()) == '%' ){
			symbol_name = token.substr(1,token.size());
			SymbolAccessor<double> symbol = SIM::findGlobalSymbol<double>( symbol_name );
			string old_token = token;
			token = to_str( symbol(SymbolFocus()) );
			//cout << "Replacing symbol "<< old_token << " with " << token << endl;
		}
	}
	// reassemble command from tokens 
	string command_new;
	for (auto const& t : tokens) { 
		command_new += t + " "; 
	}
	// add ampersand to fork process into separate thread
	if( fork() )
		command_new += " &";
	
	//cout << "Command = " << command_new << endl;
	return command_new;
}
*/

string External::update_command(string command){
	if(!replace_symbols){
		string command_new = command;
		// add ampersand to fork process into separate thread
		if( fork() )
			command_new += " &";
		return command_new;
	}
	
	// find Global symbols in command
	vector<string> tokens = tokenize(command);
	for(string& token : tokens){
		string symbol_name;
		if( *(token.begin()) == '%' ){
			symbol_name = token.substr(1,token.size());
			SymbolAccessor<double> symbol = SIM::findGlobalSymbol<double>( symbol_name );
			string old_token = token;
			token = to_str( symbol(SymbolFocus()) );
			//cout << "Replacing symbol "<< old_token << " with " << token << endl;
		}
	}
	// reassemble command from tokens 
	string command_new;
	for (auto const& t : tokens) { 
		command_new += t + " "; 
	}
	// add ampersand to fork process into separate thread
	if( fork() )
		command_new += " &";
	
	//cout << "Command = " << command_new << endl;
	return command_new;
}
