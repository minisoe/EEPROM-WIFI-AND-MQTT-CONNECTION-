#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <MFRC522.h>

//Setup for wifi
const char* WIFI_SSID = "Xiaomi 13";
const char* WIFI_PASSWORD = "11228899";

//setup for mqtt server for auth
const char* MQTT_HOST = "broker.freemqtt.com";
const int MQTT_PORT = 1883; // Port ကို int

//setup for auth
const char* MQTT_USERNAME = "freemqtt";
const char* MQTT_PASSWORD = "public";

//==== RFID PIN configration ====
#define RFID_SS_PIN 5
#define RFID_RST_PIN 22
#define RFID_SCK_PIN 18
#define RFID_MOSI_PIN 23
#define RFID_MISO_PIN 19

//=== RECONNECT INTERVALS ====
const unsigned long WIFI_RETRY_MS = 100000; //wifi_retry_ms stored in unsigned long is consitant
const unsigned long MQTT_RETRY_MS = 5000;

//=== Object intializing ====
WiFiClient espClient;
PubSubClient mqttClient(espClient); //esp32Client is passed into MQTT clinent construstor
MFRC522 rfid(RFID_SS_PIN, RFID_RST_PIN);

//=== Timing ===
unsigned long lastWiFiAttempt = 0;
unsigned long lastMQTTAttempt = 0; 

const unsigned long WIFI_RETRY_INTERVAL = 10000;
const unsigned long MQTT_RETRY_INTERVAL = 5000;

//=== MQTT Topic 
String mqttTopic;

//connect/reconnect to wifi
void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) { //check te wifi connect or not
    return;
  }
  unsigned long now = millis();
  if (now - lastWiFiAttempt >= WIFI_RETRY_INTERVAL) { //check interval
    lastWiFiAttempt = now; //new the lastattempt
    Serial.println("Connecting to Wi-Fi.......");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  }
}

//=== Connect/Reconnect to MQTT ===
void connectMQTT() {
  if (WiFi.status() != WL_CONNECTED) { //check wifi
    return; //if doesn't stop here and return
  }
  //check mqtt is connected or not
  if (mqttClient.connected()) {
    return;
  }
  unsigned long now = millis(); // passed value of millisecond into now
  if (now - lastMQTTAttempt < MQTT_RETRY_INTERVAL) {
    return; // if time interval is more, donot connect
  }
  lastMQTTAttempt = now; 

  //create cient id 
  String clientID = "ESP32-RFID-";
  clientID += WiFi.macAddress(); //esp32's mapaddress is added to the clientid
  clientID.replace(":", ""); //remove ':' between address 
  Serial.print("Connecting to MQTT...."); 

  //request connection to blocker(auth)
  if (mqttClient.connect(
      clientID.c_str(),
      MQTT_USERNAME,
      MQTT_PASSWORD)) {

    Serial.println("Connected!");
    Serial.print("Publish topic: ");
    Serial.println(mqttTopic); 
  }
  else {
    //ချိတ်ဆက်မှု မအောင်မြင်ရင် အကြောင်းရင်း (Error Code) ကို ပြမယ်
    Serial.print("Failed, state = ");
    Serial.println(mqttClient.state());
  }
}

//read refi uid and publish  it to mqtt
void readRFID() {
  // Check whether a new card is present
  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }
  // Read the card serial number
  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }

  //convert UID bytes
  String uid = ""; //crate entry String to store uid
  for (byte i = 0; i < rfid.uid.size; i++) { //check card's uid size
    if (rfid.uid.uidByte[i] < 0x10) { 
      uid += "0";
    }
    uid += String(rfid.uid.uidByte[i], HEX); 
  }
  uid.toUpperCase();

  Serial.print("RFID UID:");
  Serial.println(uid);

  // Create JSON payload
  String payload = "{\"uid\":\"" + uid + "\"}";

  //publish only when mqtt is connected 
  if (mqttClient.connected()) { //check whther the mqtt is connected or not
    //public the topic and payload to mqtt broker
    bool published = mqttClient.publish( //publish() is function
      mqttTopic.c_str(),
      payload.c_str()
    );
    if (published) {
      Serial.println("MQTT publish successful!");
      Serial.print("Topic: ");
      Serial.println(mqttTopic);
      Serial.print("Payload: ");
      Serial.println(payload);
    } else {
      Serial.println("MQTT publish failed!");
    }
  } else {
    Serial.println("MQTT disconnected. UID not published.");
  }

  // Stop communication with this card
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

// ----------------------------------
// Setup
// ----------------------------------
void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("ESP32 RFID MQTT Project");

  // Initialize SPI using ESP32 pins
  SPI.begin(
    RFID_SCK_PIN,
    RFID_MISO_PIN,
    RFID_MOSI_PIN,
    RFID_SS_PIN
  );

  // Initialize RFID reader
  rfid.PCD_Init();
  Serial.println("MFRC522 initialized.");

  // Set MQTT broker
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setKeepAlive(30);
  mqttClient.setSocketTimeout(5);

  // Create a device-specific topic
  String mac = WiFi.macAddress();
  mac.replace(":", "");

  mqttTopic = "demo/rfid/" + mac + "/uid";

  // Start Wi-Fi connection
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  lastWiFiAttempt = millis();

  Serial.println("Wi-Fi connection started.");
  Serial.print("MQTT topic: ");
  Serial.println(mqttTopic);
}

// ----------------------------------
// Main loop
// ----------------------------------
void loop() {
  // Maintain Wi-Fi connection
  connectWiFi();

  // Maintain MQTT connection
  connectMQTT();

  // Process MQTT network traffic
  if (mqttClient.connected()) {
    mqttClient.loop();
  }

  // Check for RFID cards
  readRFID();

  delay(10);
}