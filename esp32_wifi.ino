#include <WiFi.h>
#include <PubSubClient.h>
//for wifi setup and MQTT
const char* ssid ="Xiaomi 13";
const char* password ="11228899";
const char* mqtt_server= "broker.hivemq.com";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastMsg = 0;
const long interval = 5000;
int value = 0;
//--Wi-fi connection
void setup_wifi(){
  delay(10);
  Serial.println();
  Serial.print("Connection to");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() !=WL_CONNECTED){
    delay (500);
    Serial.print(".");

    

  }
  Serial.println("\nWifi Connected!");
  Serial.print("IP address:");
  Serial.println(WiFi.localIP());
  
  }
//when MQTT get the message
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  
  String message = "";
  for (unsigned int i = 0; i < length; i++) {

    message += (char)payload[i];
  }
  Serial.println(message);
}
void reconnect(){
    //try reapidly to connect with MQTT
  while(!client.connected()){
    Serial.print("Attempting MQTT connection...");
    String clientID="ESP32Client";
    clientID += String(random(0xffff),HEX);//prevent to mix same ID
    //to connect broker server 
    if (client.connect(clientID.c_str())){
      Serial.println("Connected");
      //subscribe the topic to get message
      client.subscribe("esp32/output");
    }
    //if doesn't connect
    else{
      Serial.print("Failed, rc=");
      Serial.print(client.state());//to give reason for fail as rc value!
      Serial.println(" Try again in 5 seconds");
      delay(5000);
    }

    
  }

}
void setup(){
  Serial.begin(115200);
  //Start to connect  Wifi
  setup_wifi();
  //define MQTT server
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}
void loop() {
  // put your main code here, to run repeatedly:
  //1.check if wifi is connect or not , if it dose't, reconnect
  if(WiFi.status()!=WL_CONNECTED){
    Serial.println("Network Disconnected! Reconnecting....");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED){
      delay(500);
      Serial.print(".");
    }
    Serial.println("\nWiFI Reconnected!");
  }
  //2.Check MQTT connect or not, and reconnect
  if (!client.connected()){
    reconnect();//
  }
  client.loop();//loop is not to lose connection with server and to catch the massage income 
  //3.Publish messages to MQTT in time
  unsigned long now =millis();
  if (now - lastMsg > interval) {
    lastMsg = now;
    value++;
    
    String msg = "Hello from ESP32 count: " + String(value);
    Serial.print("Publishing message: ");
    Serial.println(msg);
    
    // "esp32/input" သို့ ပို့မည်
    client.publish("esp32/input", msg.c_str());
  }
}


