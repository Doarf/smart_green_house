
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
#define BUTTON_PIN  18
#define LED_PIN     5
#define DHT22_PIN 23 
int button_state = 0;
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1); 
DHT dht22(DHT22_PIN, DHT22);

String temperature;
String humidity;

void setup() {
  Serial.begin(9600);
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (true);
  }

  delay(2000);        
  oled.clearDisplay(); 
  oled.setTextSize(3);     
  oled.setTextColor(WHITE); 
  oled.setCursor(0, 10);   
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  dht22.begin();             

  temperature.reserve(10); 
  humidity.reserve(10); 
}

void loop() {
  float humi  = dht22.readHumidity();    
  float tempC = dht22.readTemperature();
  button_state = digitalRead(BUTTON_PIN);


  if (button_state == LOW)       
    digitalWrite(LED_PIN, HIGH); 
  else                           
    digitalWrite(LED_PIN, LOW);  

  if (isnan(humi) || isnan(tempC)) {
    temperature = "Failed";
    humidity = "Failed";
  } else {
    temperature  = String(tempC, 1); 
    temperature += char (247); 
    temperature += "C";
    humidity = String(humi, 1); 
    humidity += "%";

    Serial.print(tempC); 
    Serial.print("°C | " ); 
    Serial.print(humi);  
    Serial.println("%");  
  }

  oledDisplayCenter(temperature, humidity); 
}

void oledDisplayCenter(String temperature, String humidity) {
  int16_t x1;
  int16_t y1;
  uint16_t width_T;
  uint16_t height_T;
  uint16_t width_H;
  uint16_t height_H;

  oled.getTextBounds(temperature, 0, 0, &x1, &y1, &width_T, &height_T);
  oled.getTextBounds(temperature, 0, 0, &x1, &y1, &width_H, &height_H);
  oled.clearDisplay(); 
  oled.setCursor((SCREEN_WIDTH - width_T) / 2, SCREEN_HEIGHT/2 - height_T - 5);
  oled.println(temperature); 
  oled.setCursor((SCREEN_WIDTH - width_H) / 2, SCREEN_HEIGHT/2 + 5);
  oled.println(humidity); 
  oled.display();
}
