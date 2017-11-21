#include <memory>
#include <string>
#include <iostream>



#include "scope.h"

template <class T>
class Constant : public Plugin {
public:
	typedef T value_type;
	void loadFromXML();
	void init(const Scope* scope);
	class Accessor : public SymbolAccessorBase<T> {
	public:
		Accessor(std::string name, Constant* parent) : SymbolAccessorBase<T>(name), value(val) {};
		T get(const SymbolFocus<T>& /*f*/ ) const override { return value; };
		T safe_get(const SymbolFocus<T>& f) const override { parent->assert_init(); return value; };
		std::string link_type() const override { return "constant"; }
	private:
		T value;
		friend class Constant<T>;
	};
private: 
	std::shared_ptr<Accessor> accessor;
};

template <class T>
class VariableAccessor: public SymbolRWAccessorBase<T> {
public:
	VariableAccessor(std::string name, T val) : SymbolRWAccessorBase<T>(name), value(val) {};
	T get(const Focus<T>& /*f*/ ) const override { return value; };
	void set(const Focus<T>& /*f*/ , const T& val) const override { value = val; }
	std::string link_type() const override { return "variable"; }
	void assert_init() const override { }
private:
	mutable T value;
};


template <class T>
class TrivialAccessor : public SymbolReadAccessorBase<T> {
public:
	TrivialAccessor(std::string name) : SymbolReadAccessorBase<T>(name) {};
	T get(const Focus<T>& f) { return f.value; };
	std::string link_type() const override { return "trivial"; }
	void assert_init() const override { }
};
