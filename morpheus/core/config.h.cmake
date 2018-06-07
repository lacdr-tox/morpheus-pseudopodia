#ifndef CONFIG_H
#define CONFIG_H


#define MORPHEUS_OS @MORPHEUS_OS@

#cmakedefine HAVE_GNU_SYSLIB_H
#cmakedefine HAVE_OPENMP

#ifdef HAVE_OPENMP
    #include <omp.h>
#else
    inline int omp_get_thread_num()  { return 0;} 
    inline int omp_get_num_threads() { return 1;}
    inline int omp_get_max_threads() { return 1;}
    typedef int omp_lock_t;
#endif


#ifdef WIN32
#include <windows.h>
typedef unsigned int uint;
#endif


#include <assert.h>
#include <cstdint>
#include <cctype>
#include <iostream>

#include <string>
#include <vector>
#include <deque>
#include <set>
#include <unordered_set>
#include <map>

#include <cmath>
#include <ctime>
#include <memory>
#include <random>


using namespace std;

#if __cplusplus == 201103L
	
	// fix missing make_unique in C++11
	template<typename T, typename ...Args>
	std::unique_ptr<T> make_unique( Args&& ...args )
	{
		return std::unique_ptr<T>( new T( std::forward<Args>(args)... ) );
	}

	#if defined(__GNUC__) || defined(__clang__)
		#define DEPRECATED __attribute__ ((deprecated))
	#elif defined(_MSC_VER)
		#define DEPRECATED __declspec(deprecated)
	#else
		#pragma message("WARNING: You need to implement DEPRECATED for this compiler")
		#define DEPRECATED
	#endif

#elif __cplusplus == 201402L
	#define DEPRECATED [[deprecated]]
#elif __cplusplus == 201704L
	#define DEPRECATED [[deprecated]]
#endif


#endif // CONFIG_H
