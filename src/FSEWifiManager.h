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
class FSEWifiManagerParameterCheckBox: public WiFiManagerParameter{
	using WiFiManagerParameter::WiFiManagerParameter;
	void init(const char *id, const char *placeholder, const char *defaultValue, int length, const char *custom);
};

class FSEWifiManager: public WiFiManager{
	using WiFiManager::WiFiManager;
public:
	FSEWifiManager() : FSEWifiManager("/config.json") {};
	FSEWifiManager(const char *fileName);
	bool addParameter(const char *key,const char *placeHolder, const char *defaultValue);
	bool addParameterCheckBox(const char *key,const char *placeHolder);
	bool addParameter(const char *key,const char *placeHolder, const char *defaultValue, int size);
	bool addParameter(const char *key,const char *placeHolder, const char *defaultValue, int size, const char *custom);

	bool begin();
	bool begin(const char *apName, const char *apPassword = NULL);

	void _saveConfigCallback();
	virtual ~FSEWifiManager();
	void saveConfigToSPIFFS();
	const char *getByKey(const char* key);
	void resetSettings();
	bool has_wifi_settings();
	String getNetwork();
	bool  setHostname(const char * hostname);
protected:
	unsigned long _connectTimeout = 0;
	template <typename Generic>
	void DEBUG_FSEWM(Generic text);
	IPAddress _sta_static_sn;
	IPAddress     _sta_static_ip;
	IPAddress     _sta_static_gw;
	int _paramsCount = 0;
	WiFiManagerParameter** _params;
	int _max_params;
	bool initWifi();
	String _network, _password;
	FSESPIFFS _mySpiffs;
	boolean autoConnect(char const *apName, char const *apPassword);
	bool _configLoaded = false;
	uint8_t waitForConnectResult();
	int connectWifi(String ssid, String pass);
	bool _debug = true;
private:
	const char * _hostname = "";
};

#endif /* LIB_FSEWIFIMANAGER_FSEWIFIMANAGER_H_ */
