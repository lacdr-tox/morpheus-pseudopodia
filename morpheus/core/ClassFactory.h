//////
//
// This file is part of the modelling and simulation framework 'Morpheus',
// and is made available under the terms of the BSD 3-clause license (see LICENSE
// file that comes with the distribution or https://opensource.org/licenses/BSD-3-Clause).
//
// Authors:  Joern Starruss and Walter de Back
// Copyright 2009-2016, Technische Universität Dresden, Germany
//
//////

// dynamic class factory template from
// http://www.codeproject.com/KB/architecture/SimpleDynCreate.aspx
// Can create clones (via CreatorFunction) of objects (derived from _Base) that have previously registered dynamically, depending on the key (provided with the registration _Key)
// some minor changes: 
//		use shared_ptr to ensure proper memory management for the pointer
//		use const ref for keys
//		use one parameter for the creator function

#ifndef CLASSFACTORY_H
#define CLASSFACTORY_H

#include "config.h"
#include <map>
#include <set>
#include <iostream>
#include <typeinfo>
#include <string>

/** @class ClassFactory
    @brief Class factory template that produces objects of the interface class Base depending on a given key of class Key. Creator functions are stored in a Factory instance.
*/
template <typename Key, typename Base, typename Predicator = std::less<Key> >
class ClassFactory
{
public:
	typedef shared_ptr<Base> InstancePtr;
	typedef Base* (*CreatorFunction) ();
	
protected:
	// map storing the construction info
	std::map<Key, CreatorFunction, Predicator > Registry;

public:
	ClassFactory() {};
	~ClassFactory() {};

	// called at the beginning of execution to register creation functions
	/**
		Register a creator method @p classCreator with a certain key @p idKey.
		Note that this function must return an object casted to the base type , explicitely shared_ptr<Base>
		@return true, if the creator method was successfully registered, false otherwise
	*/
	bool Register(const Key& idKey, CreatorFunction classCreator) 
	{
// 		std::cout << "ClassFactory " << typeid(Base).name() << ": ";
		if (Registry.find(idKey) != Registry.end()) 
			std::cout << "overriding previously registerd class " <<  idKey << std::endl;
// 		std::cout << "registering " << idKey << std::endl;
		return Registry.insert(std::pair<Key, CreatorFunction>(idKey, classCreator)).second;
		// insert returns a pair <iterator, bool>, where the second indicates whether the key has not existed before 
	}

	/**
		Creates an instance associated with the given the key @p idKey using a previously registered creator function.
		@return Auto pointer to the created object (shared_ptr<Base>). Note that the object gets destroyed as soon as the pointer leaves scope. Use release() method or store the pointer in a static variable.
	*/

	shared_ptr<Base> CreateInstance(const Key& idKey) const {
		auto it = Registry.find(idKey);
		if (it != Registry.end()) {
				if (it->second) {
					return  shared_ptr<Base>(it->second());
				}
		}
		std::cout << "ClassFactory: Requested class " << idKey << " is not registered" << std::endl;
		return shared_ptr<Base>();
	}
	
	bool contains(const Key& k) const  { return Registry.find(k) != Registry.end(); }
	
	void printKeys() const {
		for ( auto it = Registry.begin(); it != Registry.end(); it++) {
			std::cout << it->first << std::endl;
		}
	}
};


/** @class StaticClassFactory
    @brief Class factory template that produces objects of the interface class Base depending on a given key of class Key
    thanks to http://www.codeproject.com/KB/architecture/SimpleDynCreate.aspx for inspiration
*/
template <typename Key, typename Base, typename Predicator = std::less<Key> >
class StaticClassFactory
{
public:
	typedef Base* InstancePtr;
	typedef InstancePtr (*CreatorFunction) ();
protected:
	// map storing the construction info
	// to prevent inserting into map before initialisation takes place
	// place it into static function as static member, 
	// so it will be initialised only once - at first call

	typedef std::map<Key, CreatorFunction, Predicator > Registry;

	static Registry&  getRegistry()
	{
		static Registry registry;
		return registry;
	}


public:
	StaticClassFactory() {};
	~StaticClassFactory() {};

	// called at the beginning of execution to register creation functions
	/**
		Register a creator method @p classCreator with a certain key @p idKey.
		Note that this function must return an object casted to the base type , explicitely shared_ptr<Base>
		@return true, if the creator method was successfully registered, false otherwise
	*/

	static bool RegisterCreatorFunction(const Key& idKey, CreatorFunction classCreator) 
	{
// 		std::cout << "ClassFactory " << typeid(Base).name() << ": ";
		if (getRegistry().find(idKey) != getRegistry().end()) 
			std::cout << "overriding previously registerd class " <<  idKey << std::endl;
// 		std::cout << "registering " << idKey << std::endl;
		return getRegistry().insert(std::pair<Key, CreatorFunction>(idKey, classCreator)).second;
		// insert returns a pair <iterator, bool>, where the second indicates whether the key has not existed before 
	}

	/**
		Creates an instance associated with the given the key @p idKey using a previously registered creator function.
		@return Auto pointer to the created object (shared_ptr<Base>). Note that the object gets destroyed as soon as the pointer leaves scope. Use release() method or store the pointer in a static variable.
	*/

	static shared_ptr<Base> CreateInstance(const Key& idKey) {
		typename Registry::iterator it = getRegistry().find(idKey);
		if (it != getRegistry().end()) 
			{
				if (it->second) 
				{
					return  shared_ptr<Base>(it->second());
				}
		}
		std::cout << "ClassFactory: Requested class " << idKey << " is not registered" << std::endl;
		return shared_ptr<Base>();
	}
	
	static bool contains(const Key& k) { return getRegistry().find(k) != getRegistry().end(); }
	
	static void printKeys() {
		typename Registry::iterator it ;
		for (it = getRegistry().begin(); it != getRegistry().end(); it++) {
			std::cout << it->first << std::endl;
		}
	}
};


/* Another variant of the Factory that uses a parameter in the creator function */
/** @class StaticClassFactoryP1
    @brief Class factory template that produces objects of the interface class Base depending on a given key of class Key, this version allows for a parameter in the creator function
    thanks to http://www.codeproject.com/KB/architecture/SimpleDynCreate.aspx for inspiration
*/
template <typename Key, typename Base, typename ParameterType, typename Predicator = std::less<Key> >
class StaticClassFactoryP1
{

public:
	typedef Base* InstancePtr;
	typedef InstancePtr (*CreatorFunction) (ParameterType);
protected:
	// map where the construction info is stored
	// to prevent inserting into map before initialisation takes place
	// place it into static function as static member, 
	// so it will be initialised  : Plugin() {};only once - at first call

	typedef std::map<Key, CreatorFunction, Predicator > Registry;
	static Registry&  getRegistry()
	{
		static Registry registry;
		return registry;
	}

public:
	StaticClassFactoryP1() {};
	~StaticClassFactoryP1() {};

	// called at the beginning of execution to register creation functions

	static bool RegisterCreatorFunction(const Key& idKey, CreatorFunction classCreator) 
	{
// 		std::cout << "ClassFactory " << typeid(Base).name() << ": ";
		if (getRegistry().find(idKey) != getRegistry().end())
			std::cout << "overriding previously registerd class " <<  idKey << std::endl;
// 		std::cout << "registering " << idKey << std::endl;
		return getRegistry().insert(std::pair<Key, CreatorFunction>(idKey, classCreator)).second;
		// insert returns a pair <iterator, bool>, where the second indicates whether the key has not existed before 
	}

	// tries to create an instance based on the key using a creator function
	static shared_ptr<Base> CreateInstance(const Key& idKey,  ParameterType p1 ) {
		typename Registry::iterator it = getRegistry().find(idKey);
		if (it != getRegistry().end()) 
			{
				if (it->second) 
				{
					return shared_ptr<Base>( it->second(p1) );
				}
		}
		std::cout << "ClassFactory: Requested class " << idKey << " is not registered" << std::endl;
		return shared_ptr<Base>();
	}

	static bool contains(const Key& k) { return getRegistry().find(k) != getRegistry().end(); }

	static void printKeys() {
		typename Registry::iterator it ;
		for (it = getRegistry().begin(); it != getRegistry().end(); it++) {
			std::cout << it->first << std::endl;
		}
	}
};

#endif

