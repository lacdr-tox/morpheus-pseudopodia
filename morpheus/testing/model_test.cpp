#include "model_test.h"

#define MODEL_TEST_CPP
#include "core/simulation_p.h"

extern RessourceManager ressources;
// RessourceManager ressources;

/// Ressource Manager implementation

RessourceManager::RessourceManager(std::initializer_list<RessourceData> raw_data) {
	for (const auto& d : raw_data) {
		res_map[d.name] = d;
	}
}

RessourceData RessourceManager::getData(std::string name) const{
	auto res = res_map.find(name);
	if (res==res_map.end()) {
		string msg = string("File \'") + name +"\' not found in ressources" + res_map.begin()->first;
		throw std::out_of_range(msg.c_str()); 
		return RessourceData {"",NULL,0};
	}
	return res->second;
}

void RessourceManager::storeFile(std::string name, std::string path) const{
	auto data = getData(name);
	if (data.data == nullptr)
		return;
	FILE* f = fopen(path.c_str(),"wb");
	if (f==nullptr)
		return;
	fwrite(data.data,sizeof(char),data.length,f);
}


std::string RessourceManager::getDataAsString(std::string name) const {
	auto res = res_map.find(name);
	if (res!=res_map.end()) {
		return std::string(res->second.data, res->second.data+res->second.length);
	}
	return "";
}



RessourceData ImportFile(std::string relativePath) {
	return ressources.getData(relativePath);
};

void StoreFile(std::string name, std::string path) {
	ressources.storeFile(name,path);
};

RessourceData GetFile(std::string name) {
	return ressources.getData(name);
};

std::string GetFileString(std::string name) {
	auto data = ressources.getData(name);
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
		SIM::wipe();
	}
	SIM::init(model, param_overrides);
	is_initialized = true;
	if (time_step > 0) {
		auto current_time = SIM::getTime();
		TimeScheduler::setStopTime(current_time+time_step);
	}
	TimeScheduler::compute();
}

