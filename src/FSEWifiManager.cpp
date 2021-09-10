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
	_network = "FSEESP" + String(ESP.getChipId());
}

String FSEWifiManager::getNetwork() {
	return _network;
}

bool  FSEWifiManager::setHostname(const char * hostname){
  //@todo max length 32
  _hostname = hostname;
}
/** Redirect to captive portal if we got a request for another domain. Return true in that case so the page handler do not try to handle the request again. */
boolean FSEWifiManager::captivePortal() {
  if (!isIp(server->hostHeader()) ) {
	  DEBUG_FSEWM(F("Request redirected to captive portal"));
    server->sendHeader("Location", String("http://") + toStringIp(server->client().localIP()), true);
    server->send ( 302, "text/plain", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
    server->client().stop(); // Stop is needed because we sent no content length
    return true;
  }
  return false;
}

int FSEWifiManager::getRSSIasQuality(int RSSI) {
  int quality = 0;

  if (RSSI <= -100) {
    quality = 0;
  } else if (RSSI >= -50) {
    quality = 100;
  } else {
    quality = 2 * (RSSI + 100);
  }
  return quality;
}

/** Wifi config page handler */
void FSEWifiManager::handleWifi(boolean scan) {

  String page = FPSTR(HTTP_HEADER);
  page.replace("{v}", "Config ESP");
  page += FPSTR(HTTP_SCRIPT);
  page += FPSTR(HTTP_STYLE);
  page += _customHeadElement;
  page += FPSTR(HTTP_HEADER_END);

  if (scan) {
    int n = WiFi.scanNetworks();
    DEBUG_FSEWM(F("Scan done"));
    if (n == 0) {
    	DEBUG_FSEWM(F("No networks found"));
      page += F("No networks found. Refresh to scan again.");
    } else {

      //sort networks
      int indices[n];
      for (int i = 0; i < n; i++) {
        indices[i] = i;
      }

      // RSSI SORT

      // old sort
      for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
          if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i])) {
            std::swap(indices[i], indices[j]);
          }
        }
      }

      /*std::sort(indices, indices + n, [](const int & a, const int & b) -> bool
        {
        return WiFi.RSSI(a) > WiFi.RSSI(b);
        });*/

      // remove duplicates ( must be RSSI sorted )
      if (_removeDuplicateAPs) {
        String cssid;
        for (int i = 0; i < n; i++) {
          if (indices[i] == -1) continue;
          cssid = WiFi.SSID(indices[i]);
          for (int j = i + 1; j < n; j++) {
            if (cssid == WiFi.SSID(indices[j])) {
            	DEBUG_FSEWM("DUP AP: " + WiFi.SSID(indices[j]));
              indices[j] = -1; // set dup aps to index -1
            }
          }
        }
      }

      //display networks in page
      for (int i = 0; i < n; i++) {
        if (indices[i] == -1) continue; // skip dups
        DEBUG_FSEWM(WiFi.SSID(indices[i]));
        DEBUG_FSEWM(WiFi.RSSI(indices[i]));
        int quality = getRSSIasQuality(WiFi.RSSI(indices[i]));

        if (_minimumQuality == -1 || _minimumQuality < quality) {
          String item = FPSTR(HTTP_ITEM);
          String rssiQ;
          rssiQ += quality;
          item.replace("{v}", WiFi.SSID(indices[i]));
          item.replace("{r}", rssiQ);
          if (WiFi.encryptionType(indices[i]) != ENC_TYPE_NONE) {
            item.replace("{i}", "l");
          } else {
            item.replace("{i}", "");
          }
          //DEBUG_WM(item);
          page += item;
          delay(0);
        } else {
        	DEBUG_FSEWM(F("Skipping due to quality"));
        }

      }
      page += "<br/>";
    }
  }

  page += FPSTR(HTTP_FORM_START);
  char parLength[5];
  // add the extra parameters to the form
  for (int i = 0; i < _paramsCount; i++) {
    if (_params[i] == NULL) {
      break;
    }

    String pitem = FPSTR(HTTP_FORM_PARAM);
    if (_params[i]->getID() != NULL) {
      pitem.replace("{i}", _params[i]->getID());
      pitem.replace("{n}", _params[i]->getID());
      pitem.replace("{p}", _params[i]->getPlaceholder());
      snprintf(parLength, 5, "%d", _params[i]->getValueLength());
      pitem.replace("{l}", parLength);
      pitem.replace("{v}", _params[i]->getValue());
      pitem.replace("{c}", _params[i]->getCustomHTML());
    } else {
      pitem = _params[i]->getCustomHTML();
    }

    page += pitem;
  }
  if (_params[0] != NULL) {
    page += "<br/>";
  }

  if (_sta_static_ip) {

    String item = FPSTR(HTTP_FORM_PARAM);
    item.replace("{i}", "ip");
    item.replace("{n}", "ip");
    item.replace("{p}", "Static IP");
    item.replace("{l}", "15");
    item.replace("{v}", _sta_static_ip.toString());

    page += item;

    item = FPSTR(HTTP_FORM_PARAM);
    item.replace("{i}", "gw");
    item.replace("{n}", "gw");
    item.replace("{p}", "Static Gateway");
    item.replace("{l}", "15");
    item.replace("{v}", _sta_static_gw.toString());

    page += item;

    item = FPSTR(HTTP_FORM_PARAM);
    item.replace("{i}", "sn");
    item.replace("{n}", "sn");
    item.replace("{p}", "Subnet");
    item.replace("{l}", "15");
    item.replace("{v}", _sta_static_sn.toString());

    page += item;

    page += "<br/>";
  }

  page += FPSTR(HTTP_FORM_END);
  page += FPSTR(HTTP_SCAN_LINK);

  page += FPSTR(HTTP_END);

  server->sendHeader("Content-Length", String(page.length()));
  server->send(200, "text/html", page);


  DEBUG_FSEWM(F("Sent config page"));
}

void FSEWifiManager::handleNotFound() {
  if (captivePortal()) { // If captive portal redirect instead of displaying the error page.
    return;
  }
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server->uri();
  message += "\nMethod: ";
  message += ( server->method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server->args();
  message += "\n";

  for ( uint8_t i = 0; i < server->args(); i++ ) {
    message += " " + server->argName ( i ) + ": " + server->arg ( i ) + "\n";
  }
  server->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server->sendHeader("Pragma", "no-cache");
  server->sendHeader("Expires", "-1");
  server->sendHeader("Content-Length", String(message.length()));
  server->send ( 404, "text/plain", message );
}

void FSEWifiManager::handleRoot() {
	DEBUG_FSEWM(F("Handle root"));
	if (captivePortal()) { // If caprive portal redirect instead of displaying the page.
		return;
	}

	String page = FPSTR(HTTP_HEADER);
	page.replace("{v}", "Options");
	page += FPSTR(HTTP_SCRIPT);
	page += FPSTR(HTTP_STYLE);
	page += _customHeadElement;
	page += FPSTR(HTTP_HEADER_END);
	page += String(F("<h1>"));
	page += _apName;
	page += String(F("</h1>"));
	page += getLogo();
	page += String(F("<center><h3>WiFiManager</h3></center>"));
	page += FPSTR(HTTP_PORTAL_OPTIONS);
	page += FPSTR(HTTP_END);
	server->sendHeader("Content-Length", String(page.length()));
	server->send(200, "text/html", page);
}

/** Handle the reset page */
void FSEWifiManager::handleReset() {
	DEBUG_FSEWM(F("Reset"));

	String page = FPSTR(HTTP_HEADER);
	page.replace("{v}", "Info");
	page += FPSTR(HTTP_SCRIPT);
	page += FPSTR(HTTP_STYLE);
	page += _customHeadElement;
	page += FPSTR(HTTP_HEADER_END);
	page += F("Module will reset in a few seconds.");
	page += FPSTR(HTTP_END);

	server->sendHeader("Content-Length", String(page.length()));
	server->send(200, "text/html", page);

	DEBUG_FSEWM(F("Sent reset page"));
	delay(5000);
	ESP.reset();
	delay(2000);
}

/** Handle the info page */
void FSEWifiManager::handleInfo() {
	DEBUG_FSEWM(F("Info"));

	String page = FPSTR(HTTP_HEADER);
	page.replace("{v}", "Info");
	page += FPSTR(HTTP_SCRIPT);
	page += FPSTR(HTTP_STYLE);
	page += _customHeadElement;
	page += FPSTR(HTTP_HEADER_END);
	page += getLogo();
	page += String(F("<center><h3>Info</h3></center>"));
	page += F("<dl>");
	page += F("<dt>Chip ID</dt><dd>");
	page += ESP.getChipId();
	page += F("</dd>");
	page += F("<dt>Flash Chip ID</dt><dd>");
	page += ESP.getFlashChipId();
	page += F("</dd>");
	page += F("<dt>IDE Flash Size</dt><dd>");
	page += ESP.getFlashChipSize();
	page += F(" bytes</dd>");
	page += F("<dt>Real Flash Size</dt><dd>");
	page += ESP.getFlashChipRealSize();
	page += F(" bytes</dd>");
	page += F("<dt>Soft AP IP</dt><dd>");
	page += WiFi.softAPIP().toString();
	page += F("</dd>");
	page += F("<dt>Soft AP MAC</dt><dd>");
	page += WiFi.softAPmacAddress();
	page += F("</dd>");
	page += F("<dt>Station MAC</dt><dd>");
	page += WiFi.macAddress();
	page += F("</dd>");
	page += F("</dl>");
	page += FPSTR(HTTP_END);

	server->sendHeader("Content-Length", String(page.length()));
	server->send(200, "text/html", page);

	DEBUG_FSEWM(F("Sent info page"));
}

/** Handle the WLAN save form and redirect to WLAN config page again */
void FSEWifiManager::handleWifiSave() {
	DEBUG_FSEWM(F("WiFi save"));

  //SAVE/connect here
  _ssid = server->arg("s").c_str();
  _pass = server->arg("p").c_str();

//  //parameters
//  for (int i = 0; i < _paramsCount; i++) {
//    if (_params[i] == NULL) {
//      break;
//    }
//    //read parameter
//    String value = server->arg(_params[i]->getID()).c_str();
//    //store it in array
//    value.toCharArray(_params[i]->_value, _params[i]->_length + 1);
//    DEBUG_FSEWM(F("Parameter"));
//    DEBUG_FSEWM(_params[i]->getID());
//    DEBUG_FSEWM(value);
//  }

  if (server->arg("ip") != "") {
	  DEBUG_FSEWM(F("static ip"));
	  DEBUG_FSEWM(server->arg("ip"));
    //_sta_static_ip.fromString(server->arg("ip"));
    String ip = server->arg("ip");
    optionalIPFromString(&_sta_static_ip, ip.c_str());
  }
  if (server->arg("gw") != "") {
	  DEBUG_FSEWM(F("static gateway"));
	  DEBUG_FSEWM(server->arg("gw"));
    String gw = server->arg("gw");
    optionalIPFromString(&_sta_static_gw, gw.c_str());
  }
  if (server->arg("sn") != "") {
	  DEBUG_FSEWM(F("static netmask"));
	  DEBUG_FSEWM(server->arg("sn"));
    String sn = server->arg("sn");
    optionalIPFromString(&_sta_static_sn, sn.c_str());
  }

  String page = FPSTR(HTTP_HEADER);
  page.replace("{v}", "Credentials Saved");
  page += FPSTR(HTTP_SCRIPT);
  page += FPSTR(HTTP_STYLE);
  page += _customHeadElement;
  page += FPSTR(HTTP_HEADER_END);
  page += FPSTR(HTTP_SAVED);
  page += FPSTR(HTTP_END);

  server->sendHeader("Content-Length", String(page.length()));
  server->send(200, "text/html", page);

  DEBUG_FSEWM(F("Sent wifi save page"));

  connect = true; //signal ready to connect/reset
}

/** Is this an IP? */
boolean FSEWifiManager::isIp(String str) {
  for (size_t i = 0; i < str.length(); i++) {
    int c = str.charAt(i);
    if (c != '.' && (c < '0' || c > '9')) {
      return false;
    }
  }
  return true;
}

/** IP to String? */
String FSEWifiManager::toStringIp(IPAddress ip) {
  String res = "";
  for (int i = 0; i < 3; i++) {
    res += String((ip >> (8 * i)) & 0xFF) + ".";
  }
  res += String(((ip >> 8 * 3)) & 0xFF);
  return res;
}

void FSEWifiManager::setupConfigPortal() {
	dnsServer.reset(new DNSServer());
	server.reset(new ESP8266WebServer(80));

	DEBUG_FSEWM(F(""));
	_configPortalStart = millis();

	DEBUG_FSEWM(F("Configuring access point... "));
	DEBUG_FSEWM(_apName);
	if (_apPassword != NULL) {
	if (strlen(_apPassword) < 8 || strlen(_apPassword) > 63) {
	  // fail passphrase to short or long!
		DEBUG_FSEWM(F("Invalid AccessPoint password. Ignoring"));
	  _apPassword = NULL;
	}
	DEBUG_FSEWM(_apPassword);
	}

	//optional soft ip config
	if (_ap_static_ip) {
		DEBUG_FSEWM(F("Custom AP IP/GW/Subnet"));
	WiFi.softAPConfig(_ap_static_ip, _ap_static_gw, _ap_static_sn);
	}

	if (_apPassword != NULL) {
	WiFi.softAP(_apName, _apPassword);//password option
	} else {
	WiFi.softAP(_apName);
	}

	delay(500); // Without delay I've seen the IP address blank
	DEBUG_FSEWM(F("AP IP address: "));
	DEBUG_FSEWM(WiFi.softAPIP());

	/* Setup the DNS server redirecting all the domains to the apIP */
	dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
	dnsServer->start(DNS_PORT, "*", WiFi.softAPIP());
	DEBUG_FSEWM(F("DNS start "));
	/* Setup web pages: root, wifi config pages, SO captive portal detectors and not found. */
	server->on(String(F("/")).c_str(), std::bind(&FSEWifiManager::handleRoot, this));
	server->on(String(F("/wifi")).c_str(), std::bind(&FSEWifiManager::handleWifi, this, true));
	server->on(String(F("/0wifi")).c_str(), std::bind(&FSEWifiManager::handleWifi, this, false));
	server->on(String(F("/wifisave")).c_str(), std::bind(&FSEWifiManager::handleWifiSave, this));
	server->on(String(F("/i")).c_str(), std::bind(&FSEWifiManager::handleInfo, this));
	server->on(String(F("/r")).c_str(), std::bind(&FSEWifiManager::handleReset, this));
//	server->on("/generate_204", std::bind(&WiFiManager::handle204, this));  //Android/Chrome OS captive portal check.
//	server->on(String(F("/fwlink")).c_str(), std::bind(&WiFiManager::handleRoot, this));  //Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
	server->onNotFound (std::bind(&FSEWifiManager::handleNotFound, this));
	server->begin(); // Web server start
	DEBUG_FSEWM(F("HTTP server started"));
}

boolean FSEWifiManager::configPortalHasTimeout(){
    if(_configPortalTimeout == 0 || wifi_softap_get_station_num() > 0){
      _configPortalStart = millis(); // kludge, bump configportal start time to skew timeouts
      return false;
    }
    return (millis() > _configPortalStart + _configPortalTimeout);
}


boolean  FSEWifiManager::startConfigPortal(char const *apName, char const *apPassword) {
	if(!WiFi.isConnected()){
	    WiFi.persistent(false);
	    // disconnect sta, start ap
	    WiFi.disconnect(); //  this alone is not enough to stop the autoconnecter
	    WiFi.mode(WIFI_AP);
	    WiFi.persistent(true);
	  }
	  else {
	    //setup AP
	    WiFi.mode(WIFI_AP_STA);
	    DEBUG_FSEWM(F("SET AP STA"));
	  }

	  _apName = apName;
	  _apPassword = apPassword;

	  //notify we entered AP mode
	  if ( _apcallback != NULL) {
	    _apcallback(this);
	  }

	  connect = false;
	  setupConfigPortal();

	  while(1){
		// check if timeout
	    if(configPortalHasTimeout()) break;
	    //DNS
	    dnsServer->processNextRequest();
	    //HTTP
	    server->handleClient();
	    if (connect) {
	      delay(1000);
	      connect = false;

	      // if saving with no ssid filled in, reconnect to ssid
	      // will not exit cp
	      if(_ssid == ""){
	    	  DEBUG_FSEWM(F("No ssid, skipping wifi"));
	      }
	      else{
	    	  DEBUG_FSEWM(F("Connecting to new AP"));
	        if (connectWifi(_ssid, _pass) != WL_CONNECTED) {
	          delay(2000);
	          // using user-provided  _ssid, _pass in place of system-stored ssid and pass
	          DEBUG_FSEWM(F("Failed to connect."));
	        }
	        else {
	          //connected
	          WiFi.mode(WIFI_STA);
	          //notify that configuration has changed and any optional parameters should be saved
	          if ( _savecallback != NULL) {
	            //todo: check if any custom parameters actually exist, and check if they really changed maybe
	            _savecallback();
	          }
	          break;
	        }
	      }
	      if (_shouldBreakAfterConfig) {
	        //flag set to exit after config after trying to connect
	        //notify that configuration has changed and any optional parameters should be saved
	        if ( _savecallback != NULL) {
	          //todo: check if any custom parameters actually exist, and check if they really changed maybe
	          _savecallback();
	        }
	        WiFi.mode(WIFI_STA); // turn off ap
	        // reconnect to ssid
	        // int res = WiFi.begin();
	        // attempt connect for 10 seconds
	        DEBUG_FSEWM(F("Waiting for sta (10 secs) ......."));
	        for(size_t i = 0 ; i<100;i++){
	          if(WiFi.status() == WL_CONNECTED) break;
	          DEBUG_FSEWM(".");
	          // Serial.println(WiFi.status());
	          delay(100);
	        }
	        delay(1000);
	        break;
	      }
	    }
	    yield();
	  }

	  server.reset();
	  dnsServer.reset();

	  return  WiFi.status() == WL_CONNECTED;
}

bool FSEWifiManager::has_wifi_settings() {
	if (connectWifi("", "") == WL_CONNECTED)   {
		//connected
		return true;
	}
	return false;
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

bool FSEWifiManager::addParameter(const char *key,const char *placeHolder, const char *defaultValue, int size, const char *custom){
	WiFiManagerParameter *p;
	if(key == NULL) {
		Serial.println("Found null key param");
		Serial.println(custom);
		p = new WiFiManagerParameter(custom);
	} else {
		p = new WiFiManagerParameter(key, placeHolder, defaultValue, size, custom);
	}

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

bool FSEWifiManager::addParameterCheckBox(const char *key,const char *placeHolder) {
	String custom = "<br/><input id='{i}' name='{n}' value=0 type=checkbox {c}><label>{l}</label>";
	custom.replace("{i}", key);
	custom.replace("{n}", key);

	custom.replace("{l}", placeHolder);
	DEBUG_FSEWM(custom.c_str());
	return addParameter(NULL, placeHolder, NULL, 10, custom.c_str());
}
bool FSEWifiManager::addParameter(const char *key,const char *placeHolder, const char *defaultValue, int size) {
	return addParameter(key, placeHolder, defaultValue, size, "");
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

  if(connRes == WL_CONNECTED && _hostname != ""){
    #ifdef ESP8266
      WiFi.hostname(_hostname);
      DEBUG_FSEWM(F("Hostname set"));
      DEBUG_FSEWM(_hostname);
    #elif defined(ESP32)
      WiFi.setHostname(_hostname);
    #endif
  }

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

void FSEWifiManager::resetSettings() {
	WiFi.mode(WIFI_STA);
	WiFi.disconnect(true);
	delay(200);
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


String FSEWifiManager::getLogo() {
	return F("<center><img src='data:image/jpeg;base64,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' width=90px></center>");
}
