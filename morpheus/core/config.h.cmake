#ifndef CONFIG_H
#define CONFIG_H

#cmakedefine HAVE_GNU_SYSLIB_H
#cmakedefine USING_STD_TR1
#cmakedefine USING_BOOST_TR1
#cmakedefine USING_CXX0X_TR1
#cmakedefine HAVE_OPENMP
#cmakedefine MORPHEUS_OS
// #cmakedefine MORPHEUS_REVISION @MORPHEUS_REVISION@

#include <cstdint>



#include <memory>
#include <random>
#include <unordered_set>

#define USING_CXX0X_TR1
#define TR1_NAMESPACE std

using TR1_NAMESPACE::unique_ptr;
using TR1_NAMESPACE::shared_ptr;
using TR1_NAMESPACE::weak_ptr;
using TR1_NAMESPACE::dynamic_pointer_cast;
using TR1_NAMESPACE::const_pointer_cast;
using TR1_NAMESPACE::static_pointer_cast;
using TR1_NAMESPACE::unordered_set;

template<typename T, typename ...Args>
std::unique_ptr<T> make_unique( Args&& ...args )
{
    return std::unique_ptr<T>( new T( std::forward<Args>(args)... ) );
}

#endif // CONFIG_H
