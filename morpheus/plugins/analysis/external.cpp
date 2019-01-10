#include "external.h"

REGISTER_PLUGIN(External);

int External::instance_counter=0;

External::External() { 
	instance_id=instance_counter; instance_counter++;
	detach.setXMLPath("detach");
	detach.setDefault("true");
	registerPluginParameter( detach );
	
	timeout.setXMLPath("timeout");
	timeout.setDefault("30");
	registerPluginParameter( timeout );
	
	command_orig.setXMLPath("Command/text");
	registerPluginParameter( command_orig );
}
External::~External() { }


void External::loadFromXML(const XMLNode xNode, Scope* scope)
{
	AnalysisPlugin::loadFromXML(xNode, scope);

	if( command_orig().empty() )
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
}

void External::init(const Scope* scope)
{
	AnalysisPlugin::init(scope);
	replace_symbols = false;
	// find Global symbols in command
	vector<string> tokens = tokenize(command_orig());
	for(string& token : tokens){
		string symbol_name;
		if( *(token.begin()) == '%' ){
			symbol_name = token.substr(1,token.size());
			//cout << "SYMBOL "<< symbol_name << " FOUND" << endl;
			SymbolAccessor<double> symbol = SIM::findGlobalSymbol<double>( symbol_name );
			replace_symbols = true;
		}
	}
}

void External::analyse(double time){
	execute();
}

void External::finish() {
	int unfinished = 0;
	for (auto d : detached_processes) {
		if (d->state == DetachedProcess::RUNNING) {
			unfinished++;
		}
	}
	
	int wait=0;
	cout << "\n";
	while (wait<timeout() && unfinished !=0) {
		cout << "External: Waiting " << setw(3) << timeout() - wait << " more seconds for " << unfinished << " unfinished processes \r";
		cout.flush();
		wait++;
		this_thread::sleep_for(chrono::seconds(1));

		unfinished=0;
		for (auto d : detached_processes) {
			if (d->state == DetachedProcess::RUNNING) {
				unfinished++;
			}
		}
	}
	cout << "\nExternal: There were " << unfinished << " running jobs at destruction." << endl;
	if (unfinished) cout << "External: Those are killed now." <<endl;
	
	for (auto d : detached_processes) {
		if (d->state == DetachedProcess::RUNNING) {
			d->process->kill();
			d->state = DetachedProcess::KILLED;
		}
	}
}

void External::execute(){
	string command = update_command(command_orig());
	
	string output_log = string("external_") + to_str(instance_id) + "_output.log";
	string error_log = string("external_") + to_str(instance_id) + "_error.log";
	for (auto& kv : environvars) {
#ifdef WIN32
		string def=kv.first +"="+kv.second;
		putenv(def.c_str());
#else
		setenv(kv.first.c_str(),kv.second.c_str(),1);
#endif
	}
	
	auto process = shared_ptr<TinyProcessLib::Process>(new 
		TinyProcessLib::Process( 
			command,
			"",
			[output_log](const char *bytes, size_t n) {
				ofstream fout;
				fout.open(output_log);
				fout.write(bytes, n);
				fout.close();
			},
			[error_log](const char *bytes, size_t n) {
				ofstream fout;
				fout.open(error_log);
				fout.write(bytes, n);
				fout.close();
			},
			true
		)
	);
	if (detach()) {
		cout << "External: running in detached mode"<<endl;
		auto det = shared_ptr<DetachedProcess>(new DetachedProcess());
		det->process = process;
		process.reset();
		det->state = DetachedProcess::RUNNING;
		detached_processes.push_back(det);
		thread thread_1([det]() {
			det->return_code = det->process->get_exit_status();
			det->state = DetachedProcess::FINISHED;
			det->process = nullptr;
			cout << "Posthoc analysis finished with return code: " << det->return_code << endl;
		});
		thread_1.detach();
	}
	else {
		// Wait for the child to exit.
		cout << "External: waiting for job to finish"<<endl;
		int returncode = process->get_exit_status();
		cout << "Posthoc analysis finished with return code: " << returncode << endl;
	}
}

string External::update_command(string command){
	if(!replace_symbols){
		return command;
	}
	
	// find Global symbols in command
	vector<string> tokens = tokenize(command);
	for(string& token : tokens){
		string symbol_name;
		if( *(token.begin()) == '%' ){
			symbol_name = token.substr(1,token.size());
			SymbolAccessor<double> symbol = SIM::findGlobalSymbol<double>( symbol_name );
			string old_token = token;
			token = to_str( symbol->get(SymbolFocus()) );
			//cout << "Replacing symbol "<< old_token << " with " << token << endl;
		}
	}
	
	// reassemble command from tokens 
	string command_new;
	for (auto const& t : tokens) { 
		command_new += t + " "; 
	}

	//cout << "Command = " << command_new << endl;
	return command_new;
}
