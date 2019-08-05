#include <SPI.h>
#include <zeichen.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>

const char* ssid = "";
const char* password = "";
const char* mqttServer = "192.168.0.0";
const int mqttPort = 1883;

WiFiServer wifiServer(23);
WiFiClient espClient;
PubSubClient client(espClient);
TaskHandle_t Task1;

// CLK PIN 18
// DATA PIN 23
//MQTT max 108 Zeichen

int r1[7]; // Int für die Reihen

const int ver = 1; // Aufblickzeit
const int scrollspeed = 30; // Scroll Geschwindigkeit
const int maxanz = 265;  // max 265

const char Starttext[] = "WILLKOMMEN IM EIGENBAUKOMBINAT";  //Text beim Starten
char akttext[maxanz]; // Aktueller Text
char teltext[maxanz]; // Text von Telnet
char anztext[13]; // Text auf den Display

int intbuf;
int f = 0;

int anzlaeng = 0;

int pos = 0;
int posbit = 0;
int posmax;
int posbyte;
int posinbyte = 0;

byte a1[12];

boolean telneu = 0;
boolean telakt2 = 0;
boolean telakt = 0;
boolean text2 = 0;
boolean laufen = 0;
boolean stati = 0;

boolean ba[8];

byte zael = 0;

const int IMAGES_LEN = sizeof(IMAGES) / 8;

String Topic = "";
String Payload = "";

unsigned long previousMillis1 = 0;
unsigned long interval1 = 60000;

void setup() {
  r1[0] = 22; //Pin für Reihe 1
  r1[1] = 5;  //Pin für Reihe 2
  r1[2] = 13; //Pin für Reihe 3
  r1[3] = 15; //Pin für Reihe 4
  r1[4] = 2;  //Pin für Reihe 5
  r1[5] = 21; //Pin für Reihe 6
  r1[6] = 4;  //Pin für Reihe 7
  xTaskCreatePinnedToCore(
    Task1code, /* Function to implement the task */
    "Task1", /* Name of the task */
    100000,  /* Stack size in words */
    NULL,  /* Task input parameter */
    0,  /* Priority of the task */
    &Task1,  /* Task handle. */
    0); /* Core where the task should run */
  pinMode(r1[0], OUTPUT);
  pinMode(r1[1], OUTPUT);
  pinMode(r1[2], OUTPUT);
  pinMode(r1[3], OUTPUT);
  pinMode(r1[4], OUTPUT);
  pinMode(r1[5], OUTPUT);
  pinMode(r1[6], OUTPUT);
  SPI.begin();
  Serial.begin(115200);
  strcpy(akttext, Starttext);
  anzlaeng = strlen(akttext);
  strcat(akttext, "            ");
  telneu = 0;
  anzlaeng = strlen(akttext);
  delay(1000);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
  Serial.println(WiFi.localIP());
  wifiServer.begin();
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
}

void Task1code( void * parameter) {
  for (;;) {
    WiFiClient client = wifiServer.available();
    if (client) {
      while (client.connected()) {
        while (client.available() > 0) {
          int c = client.read();
          if ( telakt == 1) {
            if (c != 13 & c != 10) {
              if (maxanz > (f + 15)) {
                teltext[f] = c;
                Serial.println(f);
              }
              telakt2 = 1;
              f++;
            }
            if (c == 13 & telakt2 == 1) {
              telakt = 0;
              telakt2 = 0;
              f = 0;
              memset(akttext, '\0', sizeof(akttext));
              strcpy(akttext, teltext);
              strcat(akttext, "            ");
              memset(teltext, '\0', sizeof(teltext));
              telneu = 1;
            }
          }
          if (c == 65) {
            telakt = 1;
            telakt2 = 0;
          }
        }
      }
    }
  }
  //yield();
}

void callback(char* topic, byte* payload, unsigned int length) {
  uint32_t number = 0;
  int nummer = 0;
  Serial.println("Message arrived [");
  Topic = String(topic);
  Payload = "";
  for (int i = 0; i < length; i++) {
    Payload = Payload + ((char)payload[i]);
  }
  Serial.print(Topic);
  if (Topic == "display/ledlaufschrift/text") {
    memset(akttext, '\0', sizeof(akttext));
    Payload.toCharArray(akttext, 250);
    strcat(akttext, "            ");
    telneu = 1;
    Serial.println(Payload);
    text2 = 0;
    laufen = 0;
  }
  if (Topic == "display/ledlaufschrift/text2") {
    memset(akttext, '\0', sizeof(akttext));
    Payload.toCharArray(akttext, 250);
    strcat(akttext, "            ");
    telneu = 1;
    Serial.println(Payload);
    text2 = 1;
    laufen = 0;
  }
  if (Topic == "display/ledlaufschrift/text3") {
    memset(akttext, '\0', sizeof(akttext));
    Payload.toCharArray(akttext, 250);
    strcat(akttext, "            ");
    telneu = 1;
    Serial.println(Payload);
    text2 = 0;
    laufen = 0;
    stati = 1;
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    //if (client.connect("LED_Matrix", mqttUser, mqttPassword,"/LED_Matrix/on", 0, true, "offline")) {
    if (client.connect("LED_Matrix", "display/ledlaufschrift/on", 0, true, "offline")) {
      client.subscribe("display/ledlaufschrift/#");
      Serial.println("MQTT");
      client.publish("display/ledlaufschrift/on", "online");
    } else {
      Serial.println("nicht MQTT");
      delay(5000);
    }
  }
}

void loop() {
  while (!client.connected()) {
    Serial.println("kein MQTT");
    reconnect();
  }
  anzlaeng = strlen(akttext);
  if (telneu == 1) {
    pos = 0;
    telneu = 0;
  }
  posmax = anzlaeng * 8;
  // Anzeige
  if (laufen == 0) {
    for (int x = 0; x <= 7; x++) {
      if ( stati == 0) {
        posbit = pos - 95;
      } else {
        posbit = pos;
      }
      for (int j = 0; j <= 11; j++) {
        for (int i = 0; i <= 7; i++) {
          posbyte = posbit / 8;
          posinbyte = posbit % 8;
          if (posbyte < 0) {
            posbyte = 255;
          }
          bitWrite(a1[j], 7 - i, bitRead(IMAGES[akttext[posbyte]][x], 7 - posinbyte));
          posbit++;
        }
      }
      //digitalWrite(r1[x], LOW);
      SPI.transfer(a1[0]);
      SPI.transfer(a1[1]);
      SPI.transfer(a1[2]);
      SPI.transfer(a1[3]);
      SPI.transfer(a1[4]);
      SPI.transfer(a1[5]);
      SPI.transfer(a1[6]);
      SPI.transfer(a1[7]);
      SPI.transfer(a1[8]);
      SPI.transfer(a1[9]);
      SPI.transfer(a1[10]);
      SPI.transfer(a1[11]);
      digitalWrite(r1[x], HIGH);
      delay(ver);
      digitalWrite(r1[x], LOW);
      if (pos == posmax) {
        pos = 0;
        if (text2 == 1) {
          Serial.println("Fertig");
          laufen = 1;
          client.publish("display/ledlaufschrift/text2fertig", "1");
        }
      } else {
        if (zael == scrollspeed) {
          if ( stati == 0) {
            pos++;
          }
          zael = 0;
        } else {
          zael++;
        }
      }
    }
  } else {
    delay(20);
  }
  //if (millis() - previousMillis1 >= interval1) {
  // previousMillis1 = millis();
  // Serial.println("SUbpub online");
  //client.publish("display/ledlaufschrift/on", "online");
  // }
  client.loop();
}
