#include <Arduino.h>

// ---- Capteurs ----
#include "DHT.h"
#include <OneWire.h>
#include <DallasTemperature.h>

// ---- Ecran ST7735 ----
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

// ===================== PINS =====================

// DHT
#define DHTPIN   4
#define DHTTYPE  DHT22      
DHT dht(DHTPIN, DHTTYPE);

// DS18B20 (OneWire)
const int oneWireBus = 16;
OneWire oneWire(oneWireBus);
DallasTemperature ds18b20(&oneWire);

// Humidité sol (analog)
#define AOUT_PIN 35

// Ultrason
const int trig_pin = 5;
const int echo_pin = 19;  

// ST7735 (SPI)
#define TFT_CS   15
#define TFT_DC   2
#define TFT_RST  17
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// ===================== CONSTANTES =====================
#define TRIG_PULSE_DURATION_US 10

// Couleurs custom (RGB565)
#define GREY      0x7BEF  // gris moyen
#define DARKGREY  0x4208  // gris foncé

// ===================== VARIABLES =====================
float dht_h = NAN, dht_t = NAN;
float ds_t  = NAN;
int   moisture_raw = 0;
float distance_cm = NAN;


unsigned long lastDhtMs = 0;
const unsigned long DHT_PERIOD_MS = 2000; 

// ===================== FONCTIONS CAPTEURS =====================
void readDHT() {
  // DHT22 : évite de lire trop vite
  if (millis() - lastDhtMs < DHT_PERIOD_MS) return;
  lastDhtMs = millis();

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    dht_h = NAN;
    dht_t = NAN;
    Serial.println("Probleme capteur DHT");
    return;
  }

  dht_h = h;
  dht_t = t;

  Serial.print("DHT H: "); Serial.print(dht_h, 1); Serial.print("%  ");
  Serial.print("T: "); Serial.print(dht_t, 1); Serial.println("C");
}

void readDS18B20() {
  ds18b20.requestTemperatures();
  ds_t = ds18b20.getTempCByIndex(0);

  Serial.print("DS18B20: ");
  Serial.print(ds_t, 2);
  Serial.println("C");
}

void readMoisture() {
  moisture_raw = analogRead(AOUT_PIN);
  Serial.print("Soil raw: ");
  Serial.println(moisture_raw);
}

float readUltrasonCM() {
  digitalWrite(trig_pin, LOW);
  delayMicroseconds(4);
  unsigned long t0 = millis();
  while (digitalRead(echo_pin) == HIGH) {
    if (millis() - t0 > 20) return NAN;
  }
  digitalWrite(trig_pin, HIGH);
  delayMicroseconds(TRIG_PULSE_DURATION_US);
  digitalWrite(trig_pin, LOW);

  // mesure avec timeout (30ms)
  unsigned long duration = pulseIn(echo_pin, HIGH, 30000);
  if (duration == 0) return NAN;

  // conversion (µs -> cm) : 0.0343 cm/us
  return (duration * 0.0343f) / 2.0f;
}

// ===================== AFFICHAGE ECRAN =====================
void printRightValue(int x, int y, const String &txt, uint16_t color) {
  tft.setTextColor(color);
  tft.setCursor(x, y);
  tft.print(txt);
}

void drawScreen() {
  tft.fillScreen(ST77XX_BLACK);

  // Cadre + titre
  tft.drawRect(0, 0, 128, 160, GREY);

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(6, 6);
  tft.print("CAPTEURS");

  tft.drawFastHLine(1, 18, 126, DARKGREY);

  // Colonnes
  const int xLabel = 6;
  const int xValue = 74;
  int y = 28;

  // --- DHT Humidite
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(xLabel, y);
  tft.print("DHT H:");
  if (isnan(dht_h)) printRightValue(xValue, y, "--", ST77XX_CYAN);
  else             printRightValue(xValue, y, String(dht_h, 1) + "%", ST77XX_CYAN);

  // --- DHT Temperature
  y += 14;
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(xLabel, y);
  tft.print("DHT T:");
  if (isnan(dht_t)) printRightValue(xValue, y, "--", ST77XX_CYAN);
  else              printRightValue(xValue, y, String(dht_t, 1) + "C", ST77XX_CYAN);

  // --- DS18B20
  y += 14;
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(xLabel, y);
  tft.print("DS18B20:");
  if (isnan(ds_t)) printRightValue(xValue, y, "--", ST77XX_CYAN);
  else            printRightValue(xValue, y, String(ds_t, 1) + "C", ST77XX_CYAN);

  // --- Soil raw
  y += 14;
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(xLabel, y);
  tft.print("SOIL RAW:");
  printRightValue(xValue, y, String(moisture_raw), ST77XX_CYAN);

  // --- Ultrason distance
  y += 14;
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(xLabel, y);
  tft.print("DIST:");
  if (isnan(distance_cm)) printRightValue(xValue, y, "--", ST77XX_CYAN);
  else                   printRightValue(xValue, y, String(distance_cm, 0) + "cm", ST77XX_CYAN);

  // Pied de page
  tft.drawFastHLine(1, 150, 126, DARKGREY);
  tft.setTextColor(GREY);
  tft.setCursor(6, 154);
  tft.print("ESP32 + ST7735");
}

// ===================== SETUP / LOOP =====================
void setup() {
  Serial.begin(9600);

  // Capteurs
  dht.begin();
  ds18b20.begin();

  analogSetAttenuation(ADC_11db);

  pinMode(trig_pin, OUTPUT);
  pinMode(echo_pin, INPUT);

  // Ecran
  tft.initR(INITR_BLACKTAB);    
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);

  drawScreen();
}

void loop() {
  readDHT();
  readDS18B20();
  readMoisture();
  distance_cm = readUltrasonCM();

  // Logs distance
  Serial.print("Distance: ");
  if (isnan(distance_cm)) Serial.println("nan cm");
  else { Serial.print(distance_cm, 1); Serial.println(" cm"); }

  drawScreen();
  delay(500);   
}
