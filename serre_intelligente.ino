#include "DHT.h"
#include <OneWire.h>
#include <DallasTemperature.h>
const int oneWireBus = 16;     
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);
#define DHTPIN 4     
#define DHTTYPE DHT22  
#define AOUT_PIN 35 
DHT dht(DHTPIN, DHTTYPE);
const int trig_pin = 5;
const int echo_pin = 18;
#define SOUND_SPEED 340
#define TRIG_PULSE_DURATION_US 10
long ultrason_duration;
float distance_cm;

void setup() {
  Serial.begin(9600);
  dht.begin();
  sensors.begin();
  analogSetAttenuation(ADC_11db);
  pinMode(trig_pin, OUTPUT); 
  pinMode(echo_pin, INPUT); 
}

void ds20b18(){
  sensors.requestTemperatures(); 
  float temperatureC = sensors.getTempCByIndex(0);
  float temperatureF = sensors.getTempFByIndex(0);
  Serial.print(temperatureC);
  Serial.println("ºC");
  delay(1000);
}

void moisture(){
    int value = analogRead(AOUT_PIN); 

  Serial.print("Moisture value: ");
  Serial.println(value);

  delay(1000);
}

void ultrason(){
  digitalWrite(trig_pin, LOW);
  delayMicroseconds(2);
  digitalWrite(trig_pin, HIGH);
  delayMicroseconds(TRIG_PULSE_DURATION_US);
  digitalWrite(trig_pin, LOW);
  ultrason_duration = pulseIn(echo_pin, HIGH);
  distance_cm = ultrason_duration * SOUND_SPEED/2 * 0.0001;
  Serial.print("Distance (cm): ");
  Serial.println(distance_cm);

  delay(1000);
}
void dht22(){

  delay(1000);
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float f = dht.readTemperature(true);
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Problème capteur DHT22"));
    return;
  }
  float hif = dht.computeHeatIndex(f, h);
  float hic = dht.computeHeatIndex(t, h, false);

  Serial.println(F("Humidité : "));
  Serial.print(h);
  Serial.println(F("%  Température: "));
  Serial.print(t);
  Serial.print(F("°C "));
}
void loop() {
  dht22();
  ds20b18();
  moisture();
  ultrason();
}

