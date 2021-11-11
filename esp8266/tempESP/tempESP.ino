/*
  temp and humidity MQTT Program
  ESP8266 only!!
 */

//#define DEBUGGING

// Includes
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <SoftwareSerial.h>
#include <DHTesp.h>

// Defines
#define WIFI_SSID "[SSID]"
#define WIFI_PASS "[PASS]"

#define MQTT_PORT 1883
#define MQTT_ID "WETTER_ESP8266"
#define MQTT_DATA_BUFFER_LEN 10

#define MQTT_SERVER "status.schlarmann.me"

#define MQTT_USER "makeathon"
#define MQTT_PASS "makeathon"

#define MQTT_TOPIC_TEMP  "/user/makeathon_demo/grafana/makeathon_demo_temp/temp"
#define MQTT_TOPIC_HUMID "/user/makeathon_demo/grafana/makeathon_demo_temp/humid"

#define MQTT_TIMEOUT_MAX_RETRIES 5

#define MQTT_FLOAT_PRECISION 3

// MILLIS time difference
#define TIME_BETWEEN_SAMPLES 60000

// Pin D6 on NodeMCU
#define PIN_DHT22_SENSOR 12

// Globals
DHTesp dht;
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient); 
bool isConnected = false;
char mqttDataBuffer[MQTT_DATA_BUFFER_LEN];

// Times for the millis()-wait
unsigned long oldTime = 0;

// Reconnect to WiFi network
void reconnectWiFi(WiFiEvent_t event){
  Serial.println("Disconnected from WIFI access point");
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
  WiFi.onEvent(reconnectWiFi, WIFI_EVENT_STAMODE_DISCONNECTED);
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
  
  // Port defaults to 8266
  ArduinoOTA.setPort(8266);
  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("esp8266-[ChipID]");
  
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

  dht.setup(PIN_DHT22_SENSOR, DHTesp::DHT22);
}

void loop(){
  if (!mqttClient.connected()) {
    reconnect();
  } else {
    mqttClient.loop();
  }
  
  // Handle OTA requests
  ArduinoOTA.handle();
  
  gatherData();
}

void gatherData(){
  // Only gather data if timer has run out
  if(millis() - oldTime > TIME_BETWEEN_SAMPLES){
    oldTime = millis();
    // Do Stuff
    double temperature = 0.0;
    double humidity = 0.0;
    // Gather data
    humidity = dht.getHumidity();
    temperature = dht.getTemperature();
    // Publish
    publishFloat(MQTT_TOPIC_TEMP, temperature);
    publishFloat(MQTT_TOPIC_HUMID, humidity);
  }
}
