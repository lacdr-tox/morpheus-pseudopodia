#include "model_test.h"

#define MODEL_TEST_CPP
#include "core/simulation_p.h"

// extern RessourceManager ressources;
// RessourceManager ressources;

/// Ressource Manager implementation
RessourceManager& RessourceManager::getInstance() {
	static RessourceManager r;
	return r;
}

bool RessourceManager::insert(std::initializer_list<RessourceData> raw_data) {
	RessourceManager& rm = getInstance();
	for (const auto& d : raw_data) {
		rm.res_map[d.name] = d;
	}
	return true;
}

RessourceData RessourceManager::getData(std::string name) {
	RessourceManager& rm = getInstance();
	auto res = rm.res_map.find(name);
	if (res==rm.res_map.end()) {
		string msg = string("File \'") + name +"\' not found in ressources.\nUse \"ImportFile()\" macro to include the file.";
		throw std::out_of_range(msg.c_str()); 
		return RessourceData {"",NULL,0};
	}
	return res->second;
}

void RessourceManager::storeFile(std::string name, std::string path) {
	auto data = getData(name);
	if (data.data == nullptr)
		return;
	FILE* f = fopen(path.c_str(),"wb");
	if (f==nullptr)
		return;
	fwrite(data.data,sizeof(char),data.length,f);
	fclose(f);
}


std::string RessourceManager::getDataAsString(std::string name) {
	auto data = getData(name);
	return std::string(data.data, data.data+data.length);
}



RessourceData ImportFile(std::string relativePath) {
	return RessourceManager::getData(relativePath);
};

void StoreFile(std::string name, std::string path) {
	RessourceManager::storeFile(name,path);
};

RessourceData GetFile(std::string name) {
	return RessourceManager::getData(name);
};

std::string GetFileString(std::string name) {
	auto data = RessourceManager::getData(name);
	return string(data.data, data.data + data.length);
}



TestModel::TestModel(std::string model) : model(model), is_initialized(false) {
}

void TestModel::setParam(std::string p, std::string value)
{
	param_overrides[p] = value;
}

void TestModel::run(double time_step) {
	if (is_initialized) {
	}
	// unconditionally wipe the simulator 
	SIM::wipe();
	SIM::init(model, param_overrides);
	is_initialized = true;
	if (time_step > 0) {
		auto current_time = SIM::getTime();
		TimeScheduler::setStopTime(current_time+time_step);
	}
	TimeScheduler::compute();
}

