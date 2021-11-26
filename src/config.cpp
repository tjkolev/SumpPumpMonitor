#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <main.h>

extern HTTPClient httpClient;
extern WiFiClient wifiClient;

ApplicationConfig AppConfig;

#define CONFIG_URL    IOT_API_BASE_URL "/config?deviceid=sump"

template <typename T>
bool updateValue(unsigned int newValue, T &currentValue, int multiplier = 1000) {
  if (newValue) {
    T cfgValue = newValue * multiplier;
    if(cfgValue != currentValue) {
      currentValue = cfgValue;
      return true;
    }
  }
  return false;
}

bool parseConfig(const char* json) {
  StaticJsonBuffer<1024> jsonBuffer;
  JsonObject& config = jsonBuffer.parseObject(json);
  if (!config.success()) {
    log("Failed to parse json:\n%s", json);
    return false;
  }

  bool updated = updateValue(config["MainLoopSec"], AppConfig.MainLoopMs);
  updated |= updateValue(config["UpdateConfigSec"], AppConfig.UpdateConfigMs);
  updated |= updateValue(config["DebounceMask"], AppConfig.DebounceMask, 1);
  updated |= updateValue(config["MinNotifyPeriodSec"], AppConfig.MinNotifyPeriodMs);
  updated |= updateValue(config["DryAgeNotifySec"], AppConfig.DryAgeNotifyMs);
  updated |= updateValue(config["MaxPumpRunTimeSec"], AppConfig.MaxPumpRunTimeMs);
  updated |= updateValue(config["PumpTestRunSec"], AppConfig.PumpTestRunMs);

  log("Configuration pulled from %s - %s", CONFIG_URL, (updated ? "updated:": "no changes."));
  if(updated) {
    log(json);
  }

  return updated;
}

unsigned long lastConfigUpdate = 0;
bool updateConfig() {
  unsigned long now = millis();
  if(now - lastConfigUpdate < AppConfig.UpdateConfigMs) {
    return false;
  }
  lastConfigUpdate = now;

  bool result = false;
  if(ensureWiFi()) {
    httpClient.setTimeout(10000);
    httpClient.begin(wifiClient, CONFIG_URL);
    int code = httpClient.GET();
    if(code == 200) {
      String body = httpClient.getString();
      result = parseConfig(body.c_str());
    }
    else {
      log("Cannot pull config. Http code %d", code);
    }
    httpClient.end();
  }
  else {
   log("Cannot pull config: no wifi.");
  }

  return result;
}
