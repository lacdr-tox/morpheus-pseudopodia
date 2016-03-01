#include "parse_arg.h"


StringMap ParseArgv(int argc, char *argv[])
{
	StringMap Param;
	std::string key;
	std::string space = " \t";
	std::string key_val_sep= "=";
	bool is_value;
	std::string arg;
	//   printf("Grabbing command line parameters ... \n");  
	for (int i=1; i< argc; i++) {
		arg=argv[i];
// 		arg=arg.substr(arg.find_first_not_of(space));
		if (arg[0]=='-' and isalpha(arg[arg.find_first_not_of("-")] )) {
			if ( ! key.empty()) {
				Param[key]="";
//	 			std::cout << "key: " << key << " value: " << Param[key] << std::endl;
			}
			if ( arg.find_first_of(key_val_sep) != std::string::npos) {
				int pos = arg.find_first_of(key_val_sep);
				key = arg.substr(arg.find_first_not_of("-"),pos-1) ;
				arg = arg.substr(pos+1);
				is_value=true;
			}
			else {
				key=arg.substr(arg.find_first_not_of("-"));
				is_value=false;
			}
		}
		else {
			is_value = true;
		}
		if (is_value) {
			if (key.empty()) key="file";
			Param[key] = arg; 
// 			std::cout << "key: " << key << " value: " << Param[key] << std::endl;
			key="";
		}
	}
	if ( ! key.empty()) {
		Param[key]="";
	}
	return Param;
}
