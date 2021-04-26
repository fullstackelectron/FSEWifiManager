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

typedef std::function<void(void)> saveConfig_CB;

class WiFiManagerParameter;

class FSEWifiManager {
public:
	FSEWifiManager() : FSEWifiManager("/config.json") {};
	FSEWifiManager(const char *fileName);
	WiFiManager wifiManager;
	void addParam(char *key,char *placeHolder, char *defaultValue);
	void addParam(char *key,char *placeHolder, char *defaultValue, int size);
	void begin(String network);
	void begin(String network, bool reset);

	virtual ~FSEWifiManager();
	void saveConfigToSPIFFS();
private:
	void addParam(WiFiManagerParameter *param);
	int _paramsCount = 0;
	WiFiManagerParameter** _params;
	int _max_params;
	String _network;
	FSESPIFFS _mySpiffs;
	saveConfig_CB _saveConfig;
	void initWifi();
};

#endif /* LIB_FSEWIFIMANAGER_FSEWIFIMANAGER_H_ */
