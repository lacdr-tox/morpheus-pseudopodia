///////////////////////////////////////////////////////////
//
// A C++ interface to gnuplot.
//
// This is a minimal interfac to gnuplot, based on the process
// implementation of TinyProcessLib
//////////////////////////////////////////////////////////


#include <fstream>              // for std::ifstream
#include <sstream>              // for std::ostringstream
#include <list>                 // for std::list
#include <set>                 // for std::set
#include <cstdio>               // for FILE, fputs(), fflush(), popen()
#include <cstdlib>              // for getenv()
// #include <boost/dll/runtime_symbol_info.hpp>  // for program path
#include "gnuplot_i.h"


#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__TOS_WIN__) //defined for 32 and 64-bit environments
 #include <io.h>                // for _access(), _mktemp()
 #include <windows.h>
#elif defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__) //all UNIX-like OSs (Linux, *BSD, MacOSX, Solaris, ...)
 #include <unistd.h>            // for access(), mkstemp()
#include <string.h>
#else
 #error unsupported or unknown operating system
#endif


//----------------------------------------------------------------------------------
//
// initialize static data
//

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__TOS_WIN__)
std::string Gnuplot::m_sGNUPlotFileName = "gnuplot.exe";
std::string Gnuplot::m_sGNUPlotPath = "";
#elif defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
std::string Gnuplot::m_sGNUPlotFileName = "gnuplot";
std::string Gnuplot::m_sGNUPlotPath = "";
#endif

//----------------------------------------------------------------------------------
//
// define static member function: set Gnuplot path manual
//   for windows: path with slash '/' not backslash '\'
//
bool Gnuplot::set_GNUPlotPath(const std::string &path)
{

    std::string tmp = path;


#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__TOS_WIN__)
    if ( Gnuplot::file_exists(tmp,0) ) // check existence
#elif defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
    if ( Gnuplot::file_exists(tmp,1) ) // check existence and execution permission
#endif
    {
        Gnuplot::m_sGNUPlotPath = path;
        return true;
    }
    else
    {
        Gnuplot::m_sGNUPlotPath.clear();
        return false;
    }
}


std::string Gnuplot::get_GNUPlotPath() {
	get_program_path();
	return Gnuplot::m_sGNUPlotPath;
}

string Gnuplot::get_gnuplot_out(const string& cmd, vector<string> args ) 
{
	// if gnuplot not available
	if (!Gnuplot::get_program_path())
	{
		throw GnuplotException("Can't find gnuplot");
	}

	//
	// open subprocess
	//
	
	stringstream gnuplot_exec;
	gnuplot_exec << "\"" << Gnuplot::m_sGNUPlotPath << "\"";
	if (!cmd.empty())
		gnuplot_exec << " -e \"set print '-'; "<< cmd << "\" ";  // set print to stdout, not stderr
	for (const auto& arg : args) {
		gnuplot_exec << " " << arg;
	}

	string result;
	
	TinyProcessLib::Process p(
		gnuplot_exec.str(),
		"",
		[&result](const char *bytes, size_t n) {
			result.append(bytes,n);
		},
		[](const char *bytes, size_t n) {},
		true
	);
	if (p.get_exit_status() != 0) {
		cerr << "Error runnning \"" << gnuplot_exec.str() << "\"" << endl;
	}
	return result;
}

std::set<std::string> Gnuplot::get_terminals() 
{
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__TOS_WIN__)
	std::set<std::string> fixed_terminals = {"wxt","windows","jpeg","svg","pngcairo","postscript","pdfcairo","gif"};
	return fixed_terminals;
	
#endif
	std::string result = get_gnuplot_out("print GPVAL_TERMINALS");
	
	//
	// tokenize response
	//
	
	std::string delimiters = " \t\n\r";
	std::set<std::string> terminals;
	std::string::size_type fPos=0,lPos=0;
	do {
		// Skip delimiters at beginning.
		fPos = result.find_first_not_of(delimiters, lPos);
		// Don't add empty strings
		if (fPos == std::string::npos) break;
		// Iterate until first delimiter
		lPos = result.find_first_of(delimiters, fPos);
		// eat all the string left, if no further delimiter was found
		if (lPos == std::string::npos) lPos = result.length();
		// Add the token to the vector.
		auto token = result.substr(fPos, lPos - fPos);
// 		// Manually remove all cairo terminals, for testing only
// 		auto s = token.size();
// 		if (s>5 and (token.substr(s-5,5) == "cairo" || token.substr(0,5) == "cairo")) {
// 			continue;
// 		}
		terminals.insert(token);
	} while (lPos != result.length());

	return terminals;
}

std::string Gnuplot::get_screen_terminal()
{
	std::vector<string> screen_terminals = {"qt","wxt","windows","aqua","x11"};
	auto terminals = get_terminals();
	for (const auto& term : screen_terminals) {
		if (terminals.find(term) != terminals.end()) {
// 			cout << "Found gnuplot screen terminal " << term << endl;
			return term;
		}
	}
	cout << "Gnuplot Warning: Did not find a screen terminal" << endl;
	return "";
}

std::string Gnuplot::sanitize(const string& s)
{
	string out;
	
	string token_bdry = "()[].;: _^";
	
	auto pos = s.find_first_of("_^");
	if (pos == string::npos) {
		out = s;
	}
	else {
		out = s.substr(0,pos);
		
		while (pos != string::npos) {
			bool insert_braces = ((s[pos] == '_' || s[pos] == '^') && s[pos+1] != '{');
			
			out+=s[pos];
			pos++;
			if (insert_braces) out+="{";
			
			
			auto new_pos = s.find_first_of(token_bdry,pos);
			
			if (new_pos == string::npos) {
				out += s.substr(pos,s.size()-pos);
			}
			else {
				out += s.substr(pos,new_pos-pos);
			}
			
			if (insert_braces) out+="}";
			pos = new_pos;
		}
	}
	return out;
}

std::string Gnuplot::version()
{
	return get_gnuplot_out("",{"--version"});
}



//----------------------------------------------------------------------------------
//
// A string tokenizer taken from http://www.sunsite.ualberta.ca/Documentation/
// /Gnu/libstdc++-2.90.8/html/21_strings/stringtok_std_h.txt
//
template <typename Container>
void stringtok (Container &container,
                std::string const &in,
                const char * const delimiters = " \t\n")
{
    const std::string::size_type len = in.length();
          std::string::size_type i = 0;

    while ( i < len )
    {
        // eat leading whitespace
        i = in.find_first_not_of (delimiters, i);

        if (i == std::string::npos)
            return;   // nothing left but white space

        // find the end of the token
        std::string::size_type j = in.find_first_of (delimiters, i);

        // push token
        if (j == std::string::npos)
        {
            container.push_back (in.substr(i));
            return;
        }
        else
            container.push_back (in.substr(i, j-i));

        // set up for next loop
        i = j + 1;
    }

    return;
}


//----------------------------------------------------------------------------------
//
// Destructor: needed to delete temporary files
//
Gnuplot::~Gnuplot()
{
	cmd("; exit;");
	process->close_stdin();
	
// 	process->kill();
// 	cerr << "Problem closing communication to gnuplot";
}



//----------------------------------------------------------------------------------
//
// Sends a command to an active gnuplot session
//
Gnuplot& Gnuplot::cmd(const std::string &cmdstr)
{
	if (gnuplot_command_log)
		fputs((cmdstr+"\n").c_str(), gnuplot_command_log.get());
	if (process)
		process->write((cmdstr+"\n").c_str(), cmdstr.size());
	return *this;
}



//----------------------------------------------------------------------------------
//
// Opens up a gnuplot session, ready to receive commands
//
Gnuplot::Gnuplot()
{
	// if gnuplot not available
	if (!Gnuplot::get_program_path())
	{
		cerr << "Can't find gnuplot. Please install gnuplot and adjust the PATH accordingly.";
		throw GnuplotException("Can't find gnuplot");
	}


	//
	// open pipe
	//
	static int instance_counter=0;
	stringstream gnuplot_exec;
	gnuplot_exec << "\"" << Gnuplot::m_sGNUPlotPath << "\" 2> gnuplot_error" << instance_counter << ".log";
    // std::string tmp = std::string("\"") + Gnuplot::m_sGNUPlotPath + "/" + Gnuplot::m_sGNUPlotFileName + "\" 2> gnuplot_error";
	stringstream filename;
	filename << "gnuplot_error_" << instance_counter << ".log";
	string gnuplot_error_log = filename.str();

// 	cout << "Starting gnuplot Path " << Gnuplot::m_sGNUPlotPath << " exec " << Gnuplot::m_sGNUPlotFileName << endl;
	process = shared_ptr<TinyProcessLib::Process>(new 
		TinyProcessLib::Process( 
			Gnuplot::m_sGNUPlotPath,
			"",
			nullptr,
			[gnuplot_error_log](const char *bytes, size_t n) {
				ofstream fout;
				fout.open(gnuplot_error_log);
				fout.write(bytes, n);
				fout.close();
			},
			true
		)
	);
    // popen() shall return a pointer to an open stream that can be used to read or write to the pipe.
    // Otherwise, it shall return a null pointer and may set errno to indicate the error.
    if (!process->get_id()) {
        throw GnuplotException("Couldn't open connection to gnuplot");
    }

	instance_counter++;
    return;
}

void Gnuplot::setLogfile(string filename)
{
	if (!filename.empty()) {
		gnuplot_command_log = shared_ptr<FILE>(fopen(filename.c_str(),"w"), &fclose);
	}
}

//----------------------------------------------------------------------------------
//
// Get Path of the executable
//

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__TOS_WIN__)
size_t getExecutablePathName_c(char* pathName, size_t pathNameCapacity)
{
	// TODO Should use GetModuleFileNameW here
	return GetModuleFileNameA(NULL, pathName, pathNameCapacity);
}
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
size_t getExecutablePathName_c(char* pathName, size_t pathNameCapacity)
{
	uint32_t pathNameSize = 0;
	_NSGetExecutablePath(NULL, &pathNameSize);
	if (pathNameSize > pathNameCapacity)
// 		return 0;
		pathNameSize = pathNameCapacity;

	if (!_NSGetExecutablePath(pathName, &pathNameSize))
	{
		char real[PATH_MAX];

		if (realpath(pathName, real) != NULL)
		{
			pathNameSize = strlen(real);
			strncpy(pathName, real, pathNameSize);
		}
		return pathNameSize;
	}
	return 0;
}
#elif defined(unix) || defined(__unix) || defined(__unix__) || defined(__linux__) 
#include <unistd.h>
size_t getExecutablePathName_c(char* pathName, size_t pathNameCapacity)
{
	size_t pathNameSize = readlink("/proc/self/exe", pathName, pathNameCapacity - 1);
	pathName[pathNameSize]='\0';
	return pathNameSize;
}
#else
  #error provide your own implementation
#endif

std::string getExecutablePathName()
{
	static int pathNameCapacity = 256;
	const int pathNameCapacityMax = 2048;
	size_t pathNameSize=0;
	string pathName;
	while(pathNameCapacity<=pathNameCapacityMax && pathNameSize==0) {
		char pathBuffer[pathNameCapacity];
		pathNameSize = getExecutablePathName_c(pathBuffer,pathNameCapacity);
		if (pathNameSize) {
			pathBuffer[pathNameSize]='\0';
			pathName = pathBuffer;
			break;
		}
		pathNameCapacity*=2;
	}
	return pathName;
}



//----------------------------------------------------------------------------------
//
// Find out if a command lives in m_sGNUPlotPath or in PATH
//
bool Gnuplot::get_program_path()
{
	std::string tmp = Gnuplot::m_sGNUPlotPath;
	
	//
    // try to find gnuplot in the standart path in m_sGNUPlotPath for Gnuplot
    //
	if (!tmp.empty()) {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__TOS_WIN__)
		if ( Gnuplot::file_exists(tmp,0) ) // check existence
#elif defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
		if ( Gnuplot::file_exists(tmp,1) ) // check existence and execution permission
#endif
		{
// 			cout << "Gnuplot_i uses gnuplot executable: " << tmp << endl;
			return true;
		}
	}

    
    //
    // second: look in PATH for Gnuplot
    //

    std::list<std::string> ls;
    char *path;
    // Retrieves a C string containing the value of the environment variable PATH
    path = getenv("PATH");
	
        //split path (one long string) into list ls of strings
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__TOS_WIN__)
        stringtok(ls,path,";");
	string path_sep = "\\";
#elif defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
        stringtok(ls,path,":");
	string path_sep = "/";
#endif

	auto exec_path = getExecutablePathName();
	auto last_sep = exec_path.find_last_of(path_sep);
	if (last_sep != string::npos)  exec_path.resize(last_sep);

	ls.push_front(exec_path);

        // scan list for Gnuplot program files
        for (std::list<std::string>::const_iterator i = ls.begin(); i != ls.end(); ++i)
        {
            tmp = (*i) + path_sep + Gnuplot::m_sGNUPlotFileName;
//             cout << "Checking path for gnuplot " << tmp << endl;
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__TOS_WIN__)
            if ( Gnuplot::file_exists(tmp,0) ) // check existence
#elif defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
            if ( Gnuplot::file_exists(tmp,1) ) // check existence and execution permission
#endif
            {
                Gnuplot::m_sGNUPlotPath = tmp; // set m_sGNUPlotPath
                return true;
            }
        }

    Gnuplot::m_sGNUPlotPath = "";
    tmp = "Can't find gnuplot neither in PATH (" + string(path) + ") nor in \"" + Gnuplot::m_sGNUPlotPath + "\"";
    throw GnuplotException(tmp);
    return false;
}



//----------------------------------------------------------------------------------
//
// check if file exists
//
bool Gnuplot::file_exists(const std::string &filename, int mode)
{
    if ( mode < 0 || mode > 7)
    {
        throw std::runtime_error("In function \"Gnuplot::file_exists\": mode has to be an integer between 0 and 7");
        return false;
    }

    // int _access(const char *path, int mode);
    //  returns 0 if the file has the given mode,
    //  it returns -1 if the named file does not exist or is not accessible in the given mode
    // mode = 0 (F_OK) (default): checks file for existence only
    // mode = 1 (X_OK): execution permission
    // mode = 2 (W_OK): write permission
    // mode = 4 (R_OK): read permission
    // mode = 6       : read and write permission
    // mode = 7       : read, write and execution permission
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__TOS_WIN__)
    if (_access(filename.c_str(), mode) == 0)
#elif defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
    if (access(filename.c_str(), mode) == 0)
#endif
    {
        return true;
    }
    else
    {
        return false;
    }

}
