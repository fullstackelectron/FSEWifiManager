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
	_mySpiffs.saveParams();
}

FSEWifiManager::~FSEWifiManager() {

    if (_params != NULL)
    {
        free(_params);
    }
}

void FSEWifiManager::begin(String network, bool reset) {
	if (reset) {
		wifiManager.resetSettings();
	}

	begin(network);
}
void FSEWifiManager::begin(String network) {
	_network = network;
	wifiManager.setConfigPortalTimeout(120);
	initWifi();
}

void FSEWifiManager::initWifi() {
  if(!wifiManager.autoConnect(_network.c_str())) {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
  }
}

void FSEWifiManager::addParam(char *key,char *placeHolder, char *defaultValue, int size) {
	WiFiManagerParameter *p = new WiFiManagerParameter(key, placeHolder, defaultValue, size);
	addParam(p);
}

void FSEWifiManager::addParam(char *key,char *placeHolder, char *defaultValue) {
	WiFiManagerParameter *p = new WiFiManagerParameter(key, placeHolder, defaultValue, 40);
	addParam(p);
}

void FSEWifiManager::addParam(WiFiManagerParameter *param) {
	if(_paramsCount + 1 > _max_params) {
		_max_params += PARAMS_BUCKET_SIZE; // allocating another bucket
		WiFiManagerParameter** new_params = (WiFiManagerParameter**)realloc(_params, _max_params * sizeof(WiFiManagerParameter*));
		if (new_params != NULL) {
		  _params = new_params;
		} else {
		  return;
		}
	}
	wifiManager.addParameter(param);
	_params[_paramsCount] = param;
	_paramsCount++;
}

