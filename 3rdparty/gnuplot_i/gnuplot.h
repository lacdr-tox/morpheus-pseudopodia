////////////////////////////////////////////////////////////////////////////////////////
///
///  \brief Minimal platform independent C++ gnuplot wrapper.
///
///  Inspired by the 
///  The interface uses pipes and so won't run on a system that doesn't have POSIX pipe support
///  Tested on Windows (MinGW and Visual C++) and Linux (GCC)
///
/// Version history:
/// 0. C interface
///    by N. Devillard (27/01/03)
/// 1. C++ interface: direct translation from the C interface
///    by Rajarshi Guha (07/03/03)
/// 2. corrections for Win32 compatibility
///    by V. Chyzhdzenka (20/05/03)
/// 3. some member functions added, corrections for Win32 and Linux compatibility
///    by M. Burgis (10/03/08)
///
/// Requirements:
/// * gnuplot has to be installed (http://www.gnuplot.info/download.html)
/// * for Windows: set Path-Variable for Gnuplot path (e.g. C:/program files/gnuplot/bin)
///             or set Gnuplot path with: Gnuplot::set_GNUPlotPath(const std::string &path);
///
////////////////////////////////////////////////////////////////////////////////////////


#ifndef _GNUPLOT_PIPES_H_
#define _GNUPLOT_PIPES_H_


#include <iostream>
#include <string>
// #include <cstring>
// #include <vector>
// #include <fstream>    // for std::ifstream
#include <stdexcept>  // for std::runtime_error class in GnuplotException
#include <cstdio>     // for FILE (identifies a stream and contains a pointer to its buffer, its position indicator and all its state indicators)
#include <memory>
#include "tiny-process/process.hpp"

using namespace std;   


//declare classes in global namespace


class GnuplotException : public std::runtime_error
{
    public:
        GnuplotException(const std::string &msg) : std::runtime_error(msg){}
};


class Gnuplot
{
	private:
		///\brief validation of gnuplot session      
// 		bool                     valid;
		///\brief process running the Gnuplot instance;
		shared_ptr<TinyProcessLib::Process> process;
		stringstream sstream;
		shared_ptr<FILE> gnuplot_command_log;
		
		///\brief name of executed GNUPlot file     
		static std::string       m_sGNUPlotFileName; 
		///\brief gnuplot path
		static std::string       m_sGNUPlotPath;
		
		// ---------------------------------------------------------------------------------
		///\brief checks if file exists
		///
		/// \param filename --> the filename
		/// \param mode 	--> the mode [optional,default value = 0]
		///
		/// \return file exists (yes == true, no == false)
		// ---------------------------------------------------------------------------------                                  
		static bool    file_exists(const std::string &filename, int mode=0);
		
		//----------------------------------------------------------------------------------
		///\brief gnuplot path found?
		///
		/// \param ---
		///
		/// \return <-- found the gnuplot path (yes == true, no == false)
		static bool get_program_path();

		//----------------------------------------------------------------------------------
		///\brief execute single gnuplot command?
		///
		/// \param cmd gnuplot command to execute
		/// \param args additional command line argument
		///
		/// \return <-- gnuplot stdout
		// ---------------------------------------------------------------------------------
		static string get_gnuplot_out(const string& cmd, vector<string> args = {} ); 
	
    public:

		// ----------------------------------------------------------------------------
		/// \brief optional function: set Gnuplot path manual
			/// attention:  for windows: path with slash '/' not backslash '\'
		/// 
		/// \param path --> the gnuplot path
		///
		/// \return true on success, false otherwise
		// ----------------------------------------------------------------------------
        static bool set_GNUPlotPath(const std::string &path);
		static std::string get_GNUPlotPath();
		
		// ----------------------------------------------------------------------------
		/// \brief read all available terminals from GnuPlot
		///
		/// \return set of available terminals
		// ----------------------------------------------------------------------------
		static std::set<std::string> get_terminals();
		
		// ----------------------------------------------------------------------------
		/// \brief get a suitable available screen terminal
		///
		/// \return terminal name, empty if none is found
		// ----------------------------------------------------------------------------
		static std::string get_screen_terminal();

		// ----------------------------------------------------------------------------
		/// \brief sanitize a string for plotting
		///
		/// \return sanitized string
		// ----------------------------------------------------------------------------
		static std::string sanitize(const string& s);

		// ----------------------------------------------------------------------------
		/// \brief read GnuPlot version
		///
		/// \return version number
		// ----------------------------------------------------------------------------
		static std::string version();
		
		/// constructor
		Gnuplot();
		/// destructor: needed to ensure the Process is finished properly
		~Gnuplot();

		//----------------------------------------------------------------------------------
		void setLogfile(string filename = "");

		/// send a command to gnuplot
			Gnuplot& cmd(const std::string &cmdstr);
		// ---------------------------------------------------------------------------------
		///\brief Sends a command to an active gnuplot session, identical to cmd()
		/// send a command to gnuplot using the <<  operator
		///
		/// \param cmdstr --> the command string
		/// 
		/// \return <-- a reference to the gnuplot object	
		// ---------------------------------------------------------------------------------
		inline Gnuplot& operator<<(const std::string &cmdstr) { cmd(cmdstr); return(*this); };
		
// 		template <class T>
// 		Gnuplot& operator<<(const T& data) { 
// 			sstream << data;
// 			if (gnuplot_command_log)
// 				fputs(sstream.str().c_str(), gnuplot_command_log.get());
// 			if (process)
// 				process->write(sstream.str().c_str(), sstream.str().size());
// 			sstream.str("");
// 			return *this;
// 		}
};


#endif
