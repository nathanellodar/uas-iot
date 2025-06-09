#include <ESP8266WiFi.h>
#include <espnow.h>
#include <Servo.h>

// Pin Relay
int relay1 = 14;
int relay2 = 12;

// Servo Pin
Servo myServo;  

// state untuk belok
bool stateBelok = false;
String stateNow;

// Data dari remote
  typedef struct {
    bool maju;
    bool kiri;
    bool kanan;
    bool monitor;
    int potValue;
  } RemoteData;

RemoteData payload;
void setup() {

  // relay
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);

  // servo
  myServo.attach(4);  

  // ESP-NOW
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != 0) {
    Serial.println("ESP-NOW init failed!");
    while (1);
  }
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(onDataRecv);

  Serial.begin(9600);
} 

void loop() {

  receiveBelok();    
  if (stateBelok) {
    belokKanan();
  } else {
    receiveRemoteMessageRemote();
  }
  testServo();

}

// Callback ESP-NOW, data dari remote
void onDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&payload, incomingData, sizeof(payload));
}

// fungsi terima pesan dari remote
void receiveRemoteMessageRemote(){
  if(payload.maju){
    digitalWrite(relay1, LOW);
    digitalWrite(relay2, LOW);
  } else if(payload.kiri){
    digitalWrite(relay1, HIGH);
    digitalWrite(relay2, LOW);
  } else if(payload.kanan){
    digitalWrite(relay1, LOW);
    digitalWrite(relay2, HIGH);
  } else {
    digitalWrite(relay1, HIGH);
    digitalWrite(relay2, HIGH);
  }

  String dataToSend = String(payload.monitor) + "," + String(payload.potValue);
  Serial.println(dataToSend);

}

void testServo(){
  int valueServo = map(payload.potValue, 0, 1023, 0, 180);
  myServo.write(valueServo);

  // aktifkan jika ingin memantau di serial
  // Serial.print("value servo : ");
  // Serial.print(valueServo);
  // Serial.print("\n");
  delay(15);
}

// fungsi terima perintah belok dari esp2
void receiveBelok(){
  if(Serial.available()){
    String belok = Serial.readStringUntil('\n');
    belok.trim();
    Serial.print("[Debug] Diterima: [");
    Serial.print(belok);
    Serial.println("]");
    stateNow = belok;
    if(stateNow == "auto"){
      Serial.println("woii");
      stateBelok = true;

    } else if(stateNow == "autono") {
      stateBelok = false;
      // aksi autono di sini
      digitalWrite(relay1, HIGH);
      digitalWrite(relay2, HIGH); 
      // STOP
    }
  }
}

// fungsi belok kanan
void belokKanan(){
  digitalWrite(relay1, LOW);
  digitalWrite(relay2, HIGH);  
}
