#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <stdexcept>
#include <mutex>
#include "xmlParser/xmlParser.h"
#include "xml_functions.h"


/** Standard exception of the morpheus framework adding a reference to the error source in the model xml
 */
class MorpheusException {
public:
	MorpheusException(const string& what) : _what(what) {};
	MorpheusException(const string& what, XMLNode where) : _what(what),_where(getXMLPath(where)) {}
	MorpheusException(const string& what, const string& where) : _what(what),_where(where) {}
	const string& where() const { return _where; }
	const string& what() const { return _what; }
protected:
	string _what;
	string _where;
};


/** Expression evaluation exception of the morpheus framework adding respective Expression error codes
 */
class ExpressionException : public MorpheusException {
public:
	enum class ErrorType {INF, NaN, DIVZERO, UNDEF };
	ExpressionException(string expression, ErrorType error) : MorpheusException(string("Error \"") + getErrorName(error) + ("\" in Expression \"") + expression + "\""), _error(error) {} ;
    ExpressionException(string expression, ErrorType error, XMLNode where) : MorpheusException(string("Error \"") + getErrorName(error) + ("\" in Expression \"") + expression + "\"",where), _error(error) {} ;
	string getExpression() { return _expression; };
	ErrorType getError() { return _error; };
	string getErrorName() {
		
		switch (_error) {
			case ErrorType::INF: return "Infinity";
			case ErrorType::NaN: return "Not a number";
			case ErrorType::DIVZERO: return "Division by zero";
			default: return "Undefined ";
		}
	}
private:
	string getErrorName(ErrorType error) {
		
		switch (error) {
			case ErrorType::INF: return "Infinity";
			case ErrorType::NaN: return "Not a number";
			case ErrorType::DIVZERO: return "Division by zero";
			default: return "Undefined ";
		}
	}
	ErrorType _error;
	string _expression;
};

/** Exception catcher that catches all exception thrown in the Run method and saves the last one.
 *  
 * Rethrows the saved exception in Rethrow.
 * Used to catch exception in other threads and rethrow them in the main thread.
 */
class ExceptionCatcher {
private:
	std::exception_ptr Ptr;
	std::mutex         Lock;
public:
	ExceptionCatcher(): Ptr(nullptr) {}
	
	template <typename Function, typename... Parameters>
	void Run(Function f, Parameters... params)
	{
		try 
		{
			f(params...);
		}
		catch (...)
		{
			CaptureException();
		}
	}
	
	void Rethrow(){
		if(this->Ptr) std::rethrow_exception(this->Ptr);
	}
	
	void CaptureException() { 
		std::unique_lock<std::mutex> guard(this->Lock);
		this->Ptr = std::current_exception(); 
	}
};

#endif // EXCEPTIONS_H
