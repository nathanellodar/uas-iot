#include <ESP8266WiFi.h>
#include <espnow.h>

// MAC slave
uint8_t mac_slave[] = {0xDC, 0x4F, 0x22, 0x05, 0xDF, 0xB6}; 

// Pin Button
int btnMaju = 5;
int btnKiri = 12;
int btnKanan = 14;
int btnMonitor = 4;
int potPin = A0;

// Data remote
typedef struct {
    bool maju;
    bool kiri;
    bool kanan;
    bool monitor;
    int potValue;
  } RemoteData;

RemoteData myData;

int value = 0;
int state = 0;

void OnDataSent(uint8_t *mac_addr, uint8_t status) {
  Serial.print("Last Packet Send Status: ");
  Serial.println(status == 0 ? "Delivery Success" : "Delivery Fail");
}

void setup() {

  WiFi.mode(WIFI_STA);

  Serial.begin(9600);

  pinMode(btnMaju, INPUT_PULLUP);
  pinMode(btnKiri, INPUT_PULLUP);
  pinMode(btnKanan, INPUT_PULLUP);
  pinMode(btnMonitor, INPUT_PULLUP);

  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_register_send_cb(OnDataSent);
  esp_now_add_peer(mac_slave,ESP_NOW_ROLE_SLAVE,1,NULL,0);

}

void loop() {

  // Tombol aktif LOW
  myData.maju    = digitalRead(btnMaju)    == LOW;
  myData.kiri    = digitalRead(btnKiri)    == LOW;
  myData.kanan   = digitalRead(btnKanan)   == LOW;

  int buttonMonitor = digitalRead(btnMonitor);
  if(buttonMonitor == 0){
    value += 1;
  }

  if(value % 2 == 0){
      state = 1;
  } else {
      state = 0;
  }

  myData.monitor = state;
  myData.potValue= analogRead(potPin);

  Serial.print("Maju: "); 
  Serial.print(myData.maju);
  Serial.print(" | Kiri: "); 
  Serial.print(myData.kiri);
  Serial.print(" | Kanan: "); 
  Serial.print(myData.kanan);
  Serial.print(" | Monitor: "); 
  Serial.print(myData.monitor);
  Serial.print(" | Pot: "); 
  Serial.println(myData.potValue);

  esp_now_send(mac_slave, (uint8_t *) &myData, sizeof(myData));

  delay(100);

}
