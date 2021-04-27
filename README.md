**Introduction**

This library is a complement of WifiManager by tzapu. The goal is to leverage the functionality of this amazing library and add saving to SPIFFS capabilities. 


**Dependencies**

	"tzapu/WifiManager": "^0.15.0",
	"fullstackelectron/FSESPIFFS": "^1.0.0"


**Usage**

```
#include "FSEWifiManager.h"

FSEWifiManager myWifiManager;

#define IOT_URL_PARAM_NAME "iot_url"
#define IOT_API_KEY_PARAM_NAME "iot_api_Key"

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(2000);
  // Wait for serial to initialize.
  while(!Serial) { }
  
  //myWifiManager.resetSettings();
  myWifiManager.addParameter(IOT_URL_PARAM_NAME, "Iot Server URL", "https://api.thingspeak.com");
  myWifiManager.addParameter(IOT_API_KEY_PARAM_NAME, "API Key", "");
  
  //myWifiManager.setSaveConfigCallback(saveConfig);
  
  if (!myWifiManager.begin("MYTESTSSID")) { // looks like something went wrong
    //myWifiManager.resetSettings();
    ESP.reset();
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.print("Connected to:");
  Serial.println(WiFi.SSID());
  Serial.print(IOT_URL_PARAM_NAME);
  Serial.print(":");
  Serial.println(myWifiManager.getByKey(IOT_URL_PARAM_NAME));

  Serial.print(IOT_API_KEY_PARAM_NAME);
  Serial.print(":");
  Serial.println(myWifiManager.getByKey(IOT_API_KEY_PARAM_NAME));
  delay(1000);
}

void saveConfig() {
  Serial.println("If you wish to do something after saving");
}
```

