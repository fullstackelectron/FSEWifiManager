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

bool FSEWifiManager::addParameter(const char *key,const char *placeHolder, const char *defaultValue, int size) {
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

bool FSEWifiManager::addParameter(const char *key,const char *placeHolder, const char *defaultValue) {
	return addParameter(key, placeHolder, defaultValue, 40);
}

int FSEWifiManager::connectWifi(String ssid, String pass) {
  DEBUG_FSEWM(F("Connecting as wifi client..."));

  // check if we've got static_ip settings, if we do, use those.
  if (_sta_static_ip) {
    DEBUG_FSEWM(F("Custom STA IP/GW/Subnet"));
    WiFi.config(_sta_static_ip, _sta_static_gw, _sta_static_sn);
    DEBUG_FSEWM(WiFi.localIP());
  }
  //fix for auto connect racing issue
  if (WiFi.status() == WL_CONNECTED && (WiFi.SSID() == ssid)) {
    DEBUG_FSEWM(F("Already connected. Bailing out."));
    return WL_CONNECTED;
  }

  DEBUG_FSEWM(F("Status:"));
  DEBUG_FSEWM(WiFi.status());

  wl_status_t res;
  //check if we have ssid and pass and force those, if not, try with last saved values
  if (ssid != "") {
    //trying to fix connection in progress hanging
    ETS_UART_INTR_DISABLE();
    wifi_station_disconnect();
    ETS_UART_INTR_ENABLE();
    res = WiFi.begin(ssid.c_str(), pass.c_str(),0,NULL,true);
    if(res != WL_CONNECTED){
      DEBUG_FSEWM(F("[ERROR] WiFi.begin res:"));
      DEBUG_FSEWM(res);
    }
  } else {
    if (WiFi.SSID() != "") {
      //trying to fix connection in progress hanging
      ETS_UART_INTR_DISABLE();
      wifi_station_disconnect();
      ETS_UART_INTR_ENABLE();
      res = WiFi.begin();
    } else {
    	DEBUG_FSEWM(F("No saved credentials"));
    }
  }

  int connRes = waitForConnectResult();
  return connRes;
}

boolean FSEWifiManager::autoConnect(char const *apName, char const *apPassword) {
  WiFi.mode(WIFI_STA);

  if (connectWifi("", "") == WL_CONNECTED)   {
    //connected
    return true;
  }
  if (startConfigPortal(apName, apPassword)) { // that means config was successfull
	  saveConfigToSPIFFS();
	  return true;
  }
  return false;
}

template <typename Generic>
void FSEWifiManager::DEBUG_FSEWM(Generic text) {
  if (_debug) {
    Serial.print("*FSEWM: ");
    Serial.println(text);
  }
}

const char *FSEWifiManager::getByKey(const char* key) {
	if (!_configLoaded) {
		_mySpiffs.readData();
		_configLoaded = true;
	}
	return _mySpiffs.getByKey(key);
}

uint8_t FSEWifiManager::waitForConnectResult() {
	unsigned long start = millis();
	boolean keepConnecting = true;
	uint8_t status;
	if (_connectTimeout == 0) {
		return WiFi.waitForConnectResult();
	} else {
		while (keepConnecting) {
		  status = WiFi.status();
		  if (millis() > start + 3000) { // wait for 3 secs
			keepConnecting = false;
		  }
		  if (status == WL_CONNECTED) {
			keepConnecting = false;
		  }
		  DEBUG_FSEWM("Waiting connection...");
		  delay(100);
		}
	}
	return status;
}
