#include <string>
#include <map>
#include "core/traits.h"

using std::string;
using std::map;


/** Internal data format for files stored in the  Ressource Manager */

struct RessourceData {
	std::string name;
	unsigned char* data;
	unsigned int length;
	string getDataAsString() const { return std::string(data, data + length); };
};

/// Ressource Manager implementation
class RessourceManager {
public:
	static bool insert(std::initializer_list<RessourceData> raw_data);
	
	/// Access to the internal storage format
	static RessourceData getData(std::string name);
	
	/// store file data on disc
	static void storeFile(std::string name, std::string path);
	
	/// convert the file data to string
	static std::string getDataAsString(std::string name);
	
private:
	RessourceManager() {};
	static RessourceManager& getInstance();
	std::map<std::string, RessourceData> res_map;
	
};

/** Import a model via a CMake/Ressource Compiler --> inject it via include into the code
	returns a pointer to a character array
*/

void registerData(std::initializer_list<RessourceData> raw_data);
RessourceData ImportFile(std::string relativePath);   
void StoreFile(std::string name, std::string path);
RessourceData GetFile(std::string name);
std::string GetFileString(std::string name);


class TestModel {
public:
	TestModel(string model); // Costructor accepting a string 

	/// Set a model parameter to a particular value
	template <class T>
	void setParam(string p, T value) { setParam(p, TypeInfo<T>::toString(value)); }
	void setParam(string p, string value);
	
	void run(double time = -1);
private:
	string model;
	map<string,string> param_overrides;
	bool is_initialized;
};
