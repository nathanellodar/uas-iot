#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"

// DHT Pin
#define DHTPIN 2    
#define DHTTYPE DHT11

// LDR Pin
int ldrPin = A0;
int ldrValue;

// Pin Sensor Ping
const int trigPin = 5;
const int echoPin = 4;
long duration;
long distance;

// Buzzer Pin
int buzzer = 12;

// LED 1 ,2 Pin
int lamp = 14;

// nilai diteruskan dari remote lewat esp1
int remoteMonitor = 0;
int remotePotValue = 0;

int stateMonitor = 0; // 0 = hanya LDR, 1 = semua sensor

bool obstacleDetected = false;

String data;

// WiFi
const char *ssid = "hahaha"; 
const char *password = "12345678";

// MQTT Broker
const char *mqtt_broker = "broker.emqx.io";
const char *topic_suhu_temp = "kel4/DHT";
const char *topic_ping = "kel4/PING";
const char *topic_ldr = "kel4/LDR";
const char *topic_monitor = "kel4/MONITOR";
const char *mqtt_username = "qwerty";
const char *mqtt_password = "public";
const int mqtt_port = 1883;

// batas LDR
const int LDR_THRESHOLD = 500;

// DHT SET
DHT dht(DHTPIN, DHTTYPE);

WiFiClient espClient;
PubSubClient client(espClient);
void setup() {

  // DHT
  Serial.println(F("DHTxx test!"));
  dht.begin();

  // LDR
  pinMode(ldrPin, INPUT);

  // Ping
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // LED
  pinMode(lamp, OUTPUT);

  // buzzer
  pinMode(buzzer, OUTPUT);
  digitalWrite(buzzer, LOW);

  Serial.begin(9600);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.println("Connecting to WiFi..\n");
  }
  Serial.println("Connected to the WiFi network");
  //connecting to a mqtt broker
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);
  while (!client.connected()) {
      String client_id = "esp8266-client-";
      client_id += String(WiFi.macAddress());
      Serial.printf("The client %s connects to the public mqtt broker\n", client_id.c_str());
      if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
          Serial.println("Public emqx mqtt broker connected");
      } else {
          Serial.print("failed with state ");
          Serial.print(client.state());
          delay(2000);
      }
  }
  // publish dan subscribe
  client.subscribe(topic_suhu_temp);
  client.subscribe(topic_ping);
  client.subscribe(topic_ldr);
  client.subscribe(topic_monitor);

}

void callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
      Serial.print((char) payload[i]);
  }
  Serial.println();
  Serial.println("-----------------------");

}


void loop() {

  if (WiFi.status() != WL_CONNECTED) {
    reconnectWiFi();
  }
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  readSerial();
  sensorLDR();  
  sensorSuhu();
  sensorPing();

}

// fungsi kirim ke mqtt
void sendToMqtt(){

  String dht_str = String(data);
  client.publish(topic_suhu_temp, dht_str.c_str());

  String ping_str = String(distance);
  client.publish(topic_ping, ping_str.c_str());

  String ldr_str = String(ldrValue);
  client.publish(topic_ldr, ldr_str.c_str());

}

// fungsi baca dari esp1
void readSerial(){
  if(Serial.available()){
    String data = Serial.readStringUntil('\n');
    data.trim();
    int komaIdx = data.indexOf(',');
    if (komaIdx > 0) {
      remoteMonitor = data.substring(0, komaIdx).toInt();
      remotePotValue = data.substring(komaIdx + 1).toInt();

    if(remoteMonitor == 0){
        String ldr_str = String(ldrValue);
        client.publish(topic_ldr, ldr_str.c_str());
    } else if(remoteMonitor == 1){
        sendToMqtt();
    }

      // aktifkan jika ingin memantau di serial
      // Serial.print("Monitor: "); 
      // Serial.print(remoteMonitor);
      // Serial.print(" | PotValue: "); 
      // Serial.println(remotePotValue);

      String monitor_str = String(remoteMonitor);
      client.publish(topic_monitor, monitor_str.c_str());
    }
  }
}

void reconnect() {
  // Loop sampai reconnect kembali
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("connected");
      // Subscribe ulang semua 
      client.subscribe(topic_suhu_temp);
      client.subscribe(topic_ping);
      client.subscribe(topic_ldr);
      client.subscribe(topic_monitor);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 2 seconds");
      delay(2000);
    }
  }
}

void reconnectWiFi() {
  Serial.println("Reconnecting WiFi...");
  WiFi.disconnect();
  WiFi.begin(ssid, password);
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 10) {
    delay(500);
    Serial.print(".");
    retry++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi Connected!");
  } else {
    Serial.println("WiFi failed, rebooting ESP...");
    ESP.restart();
  }
}

void sensorSuhu() {

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  String suhu =  String(t);
  String lembab = String(h);
  data = suhu + "," + lembab;

  float hic = dht.computeHeatIndex(t, h, false);

  // aktifkan jika ingin memantau di serial
  // Serial.print(F("Temperature: "));
  // Serial.print(t);
  // Serial.print(F("°C "));
  // Serial.print("\n");
  // Serial.print(F("Humidity: "));
  // Serial.print(h);
  // Serial.print("\n");

  delay(100);

}

void sensorPing() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.034 / 2;

  if (distance > 0 && distance < 10) {
    digitalWrite(buzzer, HIGH);
    if (!obstacleDetected) {
      Serial.println("auto");
    }
  } else {
    digitalWrite(buzzer, LOW);
    Serial.println("autono");
    obstacleDetected = false;
  }
  // aktifkan jika ingin memantau di serial
  // Serial.print("Distance :");
  // Serial.println(distance);
  delay(100);
}

void sensorLDR(){

  ldrValue = analogRead(ldrPin);

  // aktifkan jika ingin memantau di serial
  // Serial.print("ldr : ");
  // Serial.print(ldrValue);
  // Serial.print("\n");


  if(ldrValue < LDR_THRESHOLD){
    digitalWrite(lamp, HIGH);
  } else {
    digitalWrite(lamp, LOW);
  }

    delay(50);
}

// fungsi kirim perintah belok ke esp1
void sendBelok(String cmd){
  Serial.println(cmd);
}
