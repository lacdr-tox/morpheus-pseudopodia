#ifndef RSS_STAT_H
#define RSS_STAT_H

#include <ctime>
#include <fstream>
// #ifdef _WIN32
// #include <Windows.h>
// #else
// #include <sys/time.h>
// #endif


#ifdef __cplusplus
extern "C"
{
#endif

#if defined(_WIN32)
#include <windows.h>
#include <psapi.h>

#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
#include <unistd.h>
#include <sys/resource.h>

#if defined(__APPLE__) && defined(__MACH__)
#include <mach/mach.h>

#elif (defined(_AIX) || defined(__TOS__AIX__)) || (defined(__sun__) || defined(__sun) || defined(sun) && (defined(__SVR4) || defined(__svr4__)))
#include <fcntl.h>
#include <procfs.h>

#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
#include <stdio.h>

#endif

#else
#error "Cannot define getPeakRSS( ) or getCurrentRSS( ) for an unknown OS."
#endif
	
extern "C" size_t getPeakRSS();
extern "C" size_t getCurrentRSS();

#ifdef __cplusplus
}
#endif

double get_wall_time();
double get_cpu_time();

#endif
