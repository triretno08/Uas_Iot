#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// Konfigurasi WiFi dan MQTT
const char* ssid = "Sam";
const char* password = "20020329";
const char* mqtt_server = "usm.revolusi-it.com";
const int mqtt_port = 1883;
const char* mqtt_user = "usm";
const char* mqtt_password = "usmjaya24";
const char* topic = "test/test";

// Inisialisasi objek WiFiClient dan PubSubClient
WiFiClient espClient;
PubSubClient client(espClient);

// Definisi pin dan tipe sensor DHT
#define DHTPIN D5
#define DHTTYPE DHT11

// Inisialisasi objek DHT dan LCD
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Fungsi callback untuk menangani pesan MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Pesan dari MQTT [");
  Serial.print(topic);
  Serial.print("] ");
  
  StaticJsonDocument<256> doc; 
  DeserializationError error = deserializeJson(doc, payload, length);
  
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  const char* nim = doc["nim"];
  const char* messages = doc["messages"];
  
  if (String(nim) == "G.231.21.0199") {
    Serial.println(messages);
    
    if (String(messages) == "D6=1") { digitalWrite(D6, HIGH); }
    if (String(messages) == "D7=1") { digitalWrite(D7, HIGH); }
    if (String(messages) == "D8=1") { digitalWrite(D8, HIGH); }
    if (String(messages) == "D6=0") { digitalWrite(D6, LOW); }
    if (String(messages) == "D7=0") { digitalWrite(D7, LOW); }
    if (String(messages) == "D8=0") { digitalWrite(D8, LOW); }
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP8266Client", mqtt_user, mqtt_password)) {
      Serial.println("connected");
      client.subscribe(topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void konek_wifi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
}

void setup() {
  Serial.begin(115200);
  
  // Inisialisasi I2C dan LCD
  Wire.begin(D1, D2);
  lcd.begin(16, 2);
  lcd.backlight();

  // Inisialisasi sensor DHT
  dht.begin();

  // Menghubungkan ke WiFi
  konek_wifi();

  // Menghubungkan ke server MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  reconnect();

  // Inisialisasi pin untuk LED
  
  pinMode(D6, OUTPUT);
  pinMode(D7, OUTPUT);
  pinMode(D8, OUTPUT);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) { konek_wifi(); }
  if (!client.connected()) { reconnect(); }
  client.loop();

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    lcd.setCursor(0, 0);
    lcd.print("Sensor Error   ");
    lcd.setCursor(0, 1);
    lcd.print("                ");
    delay(2000);
    return;
  }

  // Tampilkan di LCD
  lcd.setCursor(0, 0);
  lcd.print("Suhu: " + String(t) + "C   ");
  lcd.setCursor(0, 1);
  lcd.print("Kelembaban: " + String(h) + "%   ");

  // Buat pesan JSON
  String jsonMessage = "{ \"nim\" : \"G.231.21.0199\", \"suhu\" : " + String(t) + ", \"kelembapan\" : " + String(h) + " }";

  // Kirim pesan JSON melalui MQTT
  client.publish(topic, jsonMessage.c_str());
  Serial.println("MQTT message published: " + jsonMessage);

  delay(10000); // Interval pengiriman pesan (10 detik)
}