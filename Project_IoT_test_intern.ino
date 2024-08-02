#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"

#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "LAPTOP IJAL";
const char* password = "12345678";
const char* mqtt_server = "broker.hivemq.com";

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastMsg = 0; // Variabel untuk melacak waktu terakhir pesan dikirim
unsigned long lastDisplayChange = 0; // Variabel untuk melacak waktu terakhir tampilan berubah
const long interval = 2000; // Interval waktu untuk mengambil data (2000 ms = 2 detik)
const long displayInterval = 10000; // Interval waktu tampilan (10000 ms = 10 detik)

bool displayWifiInfo = true; // Menyimpan status tampilan saat ini
bool wifiConnected = false; // Menyimpan status koneksi WiFi

LiquidCrystal_I2C lcd(0x27, 16, 2); // Alamat I2C LCD 0x27, 16 kolom dan 2 baris

void setup() {
  Serial.begin(9600);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  dht.begin();

  lcd.init(); // Inisialisasi LCD
  lcd.backlight(); // Aktifkan backlight LCD
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi...");
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    lcd.setCursor(0, 1);
    lcd.print("Connecting WiFi...");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  wifiConnected = true; // Set WiFi connection status to true
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi connected");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  delay(10000); // Tampilkan status WiFi selama 10 detik
  lcd.clear();
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    lcd.setCursor(0, 0);
    lcd.print("Connecting MQTT...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("MQTT connected");
      delay(10000); // Tampilkan status MQTT selama 10 detik
      lcd.clear();
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      lcd.setCursor(0, 1);
      lcd.print("MQTT reconnect...");
      delay(5000);
    }
  }
}

void displaySensorData() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    lcd.setCursor(0, 1);
    lcd.print("Sensor error");
    return;
  }

  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.println(" *C");

  // Publish data to MQTT broker
  char tempString[8];
  dtostrf(t, 6, 2, tempString);
  char humString[8];
  dtostrf(h, 6, 2, humString);

  client.publish("daq1/suhu", tempString);
  client.publish("daq1/kelembapan", humString);

  // Update LCD with sensor data
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(tempString);
  lcd.print(" C");
  lcd.setCursor(0, 1);
  lcd.print("Humid: ");
  lcd.print(humString);
  lcd.print(" %");
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    if (wifiConnected) {
      wifiConnected = false;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Jaringan terputus");
    }
    setup_wifi(); // Attempt to reconnect to WiFi
  } else {
    wifiConnected = true;
  }

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis(); // Ambil waktu saat ini
  if (now - lastMsg > interval) { // Jika interval waktu telah berlalu
    lastMsg = now; // Perbarui waktu terakhir pesan dikirim
    displaySensorData();
  }

  if (now - lastDisplayChange > displayInterval) { // Jika waktu tampilan telah berlalu
    lastDisplayChange = now; // Perbarui waktu terakhir tampilan berubah
    if (displayWifiInfo) {
      // Tampilkan informasi WiFi
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("WiFi: Connected");
      lcd.setCursor(0, 1);
      lcd.print(WiFi.localIP());
      displayWifiInfo = false;
    } else {
      // Tampilkan informasi sensor
      displaySensorData();
      displayWifiInfo = true;
    }
  }
}
