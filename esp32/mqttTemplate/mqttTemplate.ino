/*
  Template MQTT Program
  ESP32 only!!
 */

//#define DEBUGGING

// Includes
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>

// Defines
#define WIFI_SSID "[SSID]"
#define WIFI_PASS "[PASS]"

#define MQTT_PORT 1883
#define MQTT_ID "[ID]_ESP8266"
#define MQTT_DATA_BUFFER_LEN 10

#define MQTT_SERVER "[SERVER]"

#define MQTT_USER "[USER]"
#define MQTT_PASS "[PASS]"

#define MQTT_TOPIC  "/user/[USER]/grafana/[DEVICE]/[MEASUREMENT]"

#define MQTT_TIMEOUT_MAX_RETRIES 5

#define MQTT_FLOAT_PRECISION 3

// MILLIS time difference
#define TIME_BETWEEN_SAMPLES 60000

// Globals
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient); 
bool isConnected = false;
char mqttDataBuffer[MQTT_DATA_BUFFER_LEN];

// Times for the millis()-wait
unsigned long oldTime = 0;

// Reconnect to WiFi network
void reconnectWiFi(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("Disconnected from WIFI access point");
  Serial.print("Reason: ");
  Serial.println(info.wifi_sta_disconnected.reason);
  Serial.println("Reconnecting...");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
}

// Reconnect to MQTT server
void reconnect(){
  // Loop until we're reconnected
  if(!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqttClient.connect(MQTT_ID, MQTT_USER, MQTT_PASS)) {
      Serial.println("connected");
      delay(1000);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// Publish int value to MQTT
void publishInt(const char *const topic, uint32_t value){
  // Turn int into string
  sprintf(mqttDataBuffer, "%d", value);
  // Publish if connected
  if (mqttClient.connected()){
    mqttClient.publish(topic, mqttDataBuffer);
  }
}
// Publish str value to MQTT (value _must_ be null terminated)
void publishStr(const char *const topic, char *value){
  // Publish if connected
  if (mqttClient.connected()){
    mqttClient.publish(topic, value);
  }
}
// Publish float value to MQTT
void publishFloat(const char *const topic, double value){
  // Turn float into string
  dtostrf( value, MQTT_DATA_BUFFER_LEN, MQTT_FLOAT_PRECISION, mqttDataBuffer);
  // Publish if connected
  if (mqttClient.connected()){
    mqttClient.publish(topic, mqttDataBuffer);
  }
}


void setup()
{
  Serial.begin(115200);
  Serial.println("Booting...");
  
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  WiFi.onEvent(reconnectWiFi, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  Serial.println();
  Serial.print("Connecting to MQTT server at ");
  Serial.println(MQTT_SERVER);
  Serial.print("As user \"");
  Serial.print(MQTT_USER);
  Serial.print("\" with PW: ");
  Serial.println(MQTT_PASS);
  if (mqttClient.connect(MQTT_ID, MQTT_USER, MQTT_PASS)) {
    Serial.println("Connected to MQTT");
  } else {    
    Serial.println("Could not connect to MQTT server!!");
    Serial.print("Current state: ");
    Serial.println(mqttClient.state());
    
    delay(1000);
  }

  // Port defaults to 3232
  ArduinoOTA.setPort(3232);
  // Hostname defaults to esp32-[MAC]
  ArduinoOTA.setHostname("esp32-[NAME]");
  // No authentication by default -> Set password
  // Plaintext
  ArduinoOTA.setPassword("[PASSWORD]");
  // Or by MD5 Hash
  // ArduinoOTA.setPasswordHash("[HASH VALUE]");
  
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {   
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {    
    Serial.printf("Progress: %u%%\r\n", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();

}

void loop(){
  if (!mqttClient.connected()) {
    reconnect();
  }
  mqttClient.loop();
  
  // Handle OTA requests
  ArduinoOTA.handle();
  
  gatherData();
}

void gatherData(){
  // Only gather data if timer has run out
  if(millis() - oldTime > TIME_BETWEEN_SAMPLES){
    oldTime = millis();
    // Do Stuff
  }
}
