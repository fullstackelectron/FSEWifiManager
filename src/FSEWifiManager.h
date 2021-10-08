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
	boolean startConfigPortal(char const *apName, char const *apPassword);
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
	int connectWifi(String ssid, String pass);
protected:
    std::unique_ptr<DNSServer>        dnsServer;
    std::unique_ptr<ESP8266WebServer> server;

    IPAddress     _ap_static_ip;
    IPAddress     _ap_static_gw;
    IPAddress     _ap_static_sn;
    const byte    DNS_PORT = 53;
    const char*   _apName                 = "no-net";
    const char*   _apPassword             = NULL;
    String        _ssid                   = "";
    String        _pass                   = "";
    boolean       _removeDuplicateAPs     = true;
    int           _minimumQuality         = -1;
    int           getRSSIasQuality(int RSSI);
    void (*_apcallback)(WiFiManager*) = NULL;
    void (*_savecallback)(void) = NULL;
    boolean       connect;
    unsigned long _configPortalTimeout    = 0;
    unsigned long _configPortalStart      = 0;
    boolean       configPortalHasTimeout();
    boolean       _shouldBreakAfterConfig = false;
    unsigned long _connectTimeout = 0;
    const char*   _customHeadElement      = "";
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
	bool _debug = true;
	void handleRoot();
	void setupConfigPortal();
	boolean captivePortal();
    boolean       isIp(String str);
    String        toStringIp(IPAddress ip);
    void handleNotFound();
    void handleWifi(boolean scan);
    String getLogo();
    void handleInfo();
    void handleReset();
    void handleWifiSave();
    template <class T>
	auto optionalIPFromString(T *obj, const char *s) -> decltype(  obj->fromString(s)  ) {
	  return  obj->fromString(s);
	}
private:
	const char * _hostname = "";
};

#endif /* LIB_FSEWIFIMANAGER_FSEWIFIMANAGER_H_ */
