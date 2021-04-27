/*
 * FSECanadaWifiManager.cpp
 *
 *  Created on: Apr. 18, 2021
 *      Author: jareas
 */

#include "FSEWifiManager.h"

#include <WiFiManager.h>
#include <ArduinoJson.h>
#include "FSESPIFFSParams.h"


FSEWifiManager::FSEWifiManager(const char *fileName) {
	_mySpiffs.setConfigFile(fileName);
	_max_params = PARAMS_BUCKET_SIZE;
	_params = (WiFiManagerParameter**)malloc(_max_params * sizeof(WiFiManagerParameter*));
}

void FSEWifiManager::saveConfigToSPIFFS(){
	FSESPIFFSParams *p;
	for (int i = 0 ; i < _paramsCount ; i++) {
		p = new FSESPIFFSParams(_params[i]->getID(), _params[i]->getValue());
		_mySpiffs.addParam(p);
	}
	Serial.println("Saving data to SPIFFS");
	_mySpiffs.saveParams();
}

FSEWifiManager::~FSEWifiManager() {
    if (_params != NULL)
    {
        free(_params);
    }
}

bool FSEWifiManager::begin() {
	_network = "FSEESP" + String(ESP.getChipId());
	return begin(_network.c_str());
}
bool FSEWifiManager::begin(char const *apName, char const *apPassword) {
	_network = apName;
	_password = apPassword;
	setConfigPortalTimeout(120);
	return initWifi();
}

bool FSEWifiManager::initWifi() {
  if(!autoConnect(_network.c_str(), _password.c_str())) {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      return false;
  }

  return true;
}

bool FSEWifiManager::addParameter(char *key,char *placeHolder, char *defaultValue, int size) {
	WiFiManagerParameter *p = new WiFiManagerParameter(key, placeHolder, defaultValue, size);
	// adding parameter to parent
	WiFiManager::addParameter(p);

	//need to keep the pointers accessible for later saving
	if(_paramsCount + 1 > _max_params)
	{
		// rezise the params array
		_max_params += PARAMS_BUCKET_SIZE;
		WiFiManagerParameter** new_params = (WiFiManagerParameter**)realloc(_params, _max_params * sizeof(WiFiManagerParameter*));
		if (new_params != NULL) {
		  _params = new_params;
		} else {
		  return false;
		}
	}

	_params[_paramsCount] = p;
	_paramsCount++;
	return true;
}

bool FSEWifiManager::addParameter(char *key,char *placeHolder, char *defaultValue) {
	return addParameter(key, placeHolder, defaultValue, 40);
}

boolean FSEWifiManager::autoConnect(char const *apName, char const *apPassword) {
	if (WiFiManager::autoConnect(apName, apPassword)) {
		saveConfigToSPIFFS();
		return true;
	}
	return false;
}

const char *FSEWifiManager::getByKey(const char* key) {
	if (!_configLoaded) {
		_mySpiffs.readData();
		_configLoaded = true;
	}
	return _mySpiffs.getByKey(key);
}
