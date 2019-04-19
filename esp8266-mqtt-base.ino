#include <FS.h>

// WiFi requirements
#include <ESP8266WiFi.h> // https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h> // https://github.com/bblanchon/ArduinoJson

#define configMaxSize 1024

// MQTT
#include <PubSubClient.h>

WiFiClient espClient;
PubSubClient client(espClient);
bool doConfigSave;

// MQTT config vars
#define mqttHost "1.2.3.4"
#define mqttPort 1883
#define mqttUser ""
#define mqttPass ""

// Callback function setting flag telling us to save the wifi config.
void saveConfigCallback() {
  doConfigSave = true;
}

void setup() {
  Serial.begin(115200);

  // Uncomment to wipe FS to reset saved data
  //SPIFFS.format();


  // Load Settings
  Serial.println("Mounting file system...");
  if (!SPIFFS.begin()) {
    Serial.println("Couldn't mount file system");
  } else {
    Serial.println("File system mounted");

    if (SPIFFS.exists("/config.json")) {
      Serial.println("Reading config.json");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        size_t size = configFile.size();
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        StaticJsonDocument<configMaxSize> json;
        deserializeJson(json, buf.get());

        Serial.println("Settings loaded:");
        serializeJsonPretty(json, Serial);
        Serial.println();

        // Copy settings from JSON
        strcpy(mqttHost, json["mqtt_host"]);
      }
    } else {
      Serial.println("config.json not found");
    }
  }


  // WiFiManager configuration
  WiFiManager wifiManager;

  // Custom portal parameters
  WiFiManagerParameter custom_mqtt_host("host", "mqtt host", mqttHost, 40);
  wifiManager.addParameter(&custom_mqtt_host);

  // Config portal timeout
  wifiManager.setTimeout(180);

  // Save settings after wifi setup
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  // Immanentize the Eschaton
  wifiManager.autoConnect();

  // Read modified settings
  strcpy(mqttHost, custom_mqtt_host.getValue());

  if (doConfigSave) {
    Serial.println("Saving settings to config.json");
    StaticJsonDocument<configMaxSize> json;
    JsonObject root = json.to<JsonObject>();
    root["mqtt_host"] = mqttHost;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("Failed to open config.json for writing");
    }

    Serial.println("Saving settings:");
    serializeJsonPretty(json, Serial);
    Serial.println("");
    serializeJson(json, configFile);
    configFile.close();
    Serial.println("Saved config.json");
  }

  client.setServer(mqttHost, mqttPort);
}

void connectMQTT() {
  while (!client.connected()) {
    Serial.println("connecting to MQTT broker");

    if (client.connect("ESP8266Client")) {
      Serial.println("connected to MQTT broker");
    } else {
      Serial.print("connection failed, rc=");
      Serial.print(client.state());
      Serial.println();
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
      connectMQTT();
    }

    client.publish("hello/world", String(millis()).c_str(), false);
    delay(1000);
}
