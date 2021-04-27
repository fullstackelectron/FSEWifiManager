/*
 * FSECanadaWifiManager.h
 *
 *  Created on: Apr. 18, 2021
 *      Author: jareas
 */
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <memory>

#include "FSESPIFFS.h"
#ifndef LIB_FSEWIFIMANAGER_FSEWIFIMANAGER_H_
#define LIB_FSEWIFIMANAGER_FSEWIFIMANAGER_H_
#define PARAMS_BUCKET_SIZE 10



class WiFiManagerParameter;

class FSEWifiManager: public WiFiManager{
	using WiFiManager::WiFiManager;
public:
	FSEWifiManager() : FSEWifiManager("/config.json") {};
	FSEWifiManager(const char *fileName);
	bool addParameter(char *key,char *placeHolder, char *defaultValue);
	bool addParameter(char *key,char *placeHolder, char *defaultValue, int size);
	bool begin();
	bool begin(char const *apName, char const *apPassword = NULL);

	void _saveConfigCallback();
	virtual ~FSEWifiManager();
	void saveConfigToSPIFFS();
	const char *getByKey(const char* key);
protected:
	int _paramsCount = 0;
	WiFiManagerParameter** _params;
	int _max_params;
	bool initWifi();
	String _network, _password;
	FSESPIFFS _mySpiffs;
	boolean autoConnect(char const *apName, char const *apPassword);
	bool _configLoaded = false;
private:
};

#endif /* LIB_FSEWIFIMANAGER_FSEWIFIMANAGER_H_ */
