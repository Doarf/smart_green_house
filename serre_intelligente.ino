

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>

#include <WiFi.h>
#include <WebServer.h>
#include <time.h>
#include <string.h>

//WiFi
const char* WIFI_SSID = "Arthur";
const char* WIFI_PASS = "mister95570";
WebServer server(80);

bool wifiConnected = false;

//Page
enum Page {
  PAGE_WIFI,    
  PAGE_ULTRA,
  PAGE_DS18B20,
  PAGE_SOIL,
  PAGE_DHT,
  PAGE_PUMP,
  PAGE_COUNT
};

static const int TFT_CS   = 5;
static const int TFT_DC   = 21;
static const int TFT_RST  = 22;
static const int TFT_SCK  = 18;
static const int TFT_MOSI = 23;

Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_RST);
//ultrasons
static const int TRIG_PIN = 27;
static const int ECHO_PIN = 26;

// Calibration niveau d'eau 
static const float WATER_FULL_CM  = 1.0f; // 100%
static const float WATER_EMPTY_CM = 9.0f; // 0%

//ds18b20
static const int ONE_WIRE_BUS = 4;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18(&oneWire);

//soil adc
static const int SOIL_PIN = 35; 

// Calibration soil
static const int RAW_DRY = 2550; // 0% (sec)
static const int RAW_WET = 900;  // 100% (humide)

// arrosage %
static const int SOIL_PCT_ON  = 30;
static const int SOIL_PCT_OFF = 45; 

int soilPercentFromRaw(int raw) {
  if (raw < RAW_WET) raw = RAW_WET;
  if (raw > RAW_DRY) raw = RAW_DRY;

  float pct = (float)(RAW_DRY - raw) * 100.0f / (float)(RAW_DRY - RAW_WET);
  int ipct = (int)(pct + 0.5f);
  if (ipct < 0) ipct = 0;
  if (ipct > 100) ipct = 100;
  return ipct;
}
//dht11
static const int DHT_PIN = 19;
#define DHTTYPE DHT11
DHT dht(DHT_PIN, DHTTYPE);

// relais pompe
static const int RELAY_PIN = 32;
static const bool RELAY_ACTIVE_LOW = true;
bool pump_state = false;

void pumpOn()  { digitalWrite(RELAY_PIN, RELAY_ACTIVE_LOW ? LOW  : HIGH); pump_state = true; }
void pumpOff() { digitalWrite(RELAY_PIN, RELAY_ACTIVE_LOW ? HIGH : LOW ); pump_state = false; }

bool pump_manual = false;

// secu reservoir
static const int MIN_WATER_PCT_TO_RUN = 10; // n'arrose pas si < 10%

static const float MIN_SOIL_TEMP_C_TO_WATER = 10.0f; 

//ntp /heure
static const char* TZ_PARIS = "CET-1CEST,M3.5.0/2,M10.5.0/3";
static const char* NTP1 = "pool.ntp.org";
static const char* NTP2 = "time.nist.gov";
static const char* NTP3 = "time.google.com";

bool time_ok = false;
int lastSchedYday = -1;

// Durée d’arrosage programmé
static const unsigned long SCHED_WATER_MS = 15000; // 15 s

bool schedActive = false;
unsigned long schedStopMs = 0;

// boutons
static const int BTN_PIN      = 25; 
static const int BTN_PUMP_PIN = 33; 
static const unsigned long DEBOUNCE_MS = 40;

// Debounce bouton pages
bool btnStable = HIGH;
bool btnLastReading = HIGH;
unsigned long btnLastChange = 0;

bool pumpBtnStable = HIGH;
bool pumpBtnLastReading = HIGH;
unsigned long pumpBtnLastChange = 0;

Page currentPage = PAGE_WIFI;
Page lastDrawnPage = PAGE_COUNT;

float dist_cm = -1;     
int   water_pct = -1;   
float t1_c = 999;
int   soil_raw = 0;
int   soil_pct = 0;
bool  dht_ok = false;
float t2_c = NAN, hum_pct = NAN;

//ui 
static const uint16_t BG        = ST77XX_BLACK;
static const uint16_t CARD_BG   = 0x1082;
static const uint16_t CARD_EDGE = 0x39E7;
static const uint16_t ACCENT    = ST77XX_CYAN;
static const uint16_t TEXT      = ST77XX_WHITE;
static const uint16_t WARN      = ST77XX_RED;
static const uint16_t COL_OK    = ST77XX_GREEN;
static const uint16_t TITLE_BG  = 0x001F;
static const uint16_t TITLE_TXT = ST77XX_WHITE;

const int W = 160;
const int H = 128;

const int TOP_H = 22;
const int BOT_H = 18;

const int CARD_X = 10;
const int CARD_Y = TOP_H + 10;
const int CARD_W = W - 20;
const int CARD_H = H - TOP_H - BOT_H - 20;

const int VAL_X = CARD_X + 8;
const int VAL_Y = CARD_Y + 22;
const int VAL_W = CARD_W - 16;
const int VAL_H = CARD_H - 30;

void centerText(int y, const char* s, uint16_t color, uint8_t size) {
  int16_t x1, y1;
  uint16_t w, h;
  tft.setTextSize(size);
  tft.getTextBounds((char*)s, 0, y, &x1, &y1, &w, &h);
  int x = (W - (int)w) / 2;
  tft.setCursor(x, y);
  tft.setTextColor(color);
  tft.print(s);
}

void drawTopBar(const char* title) {
  tft.fillRect(0, 0, W, TOP_H, TITLE_BG);
  centerText(4, title, TITLE_TXT, 1);
  tft.drawLine(0, TOP_H - 1, W - 1, TOP_H - 1, CARD_EDGE);
}

void drawBottomNav(Page p) {
  tft.fillRect(0, H - BOT_H, W, BOT_H, BG);
  tft.drawLine(0, H - BOT_H, W - 1, H - BOT_H, CARD_EDGE);

  int dots = (int)PAGE_COUNT;
  int dotR = 3;
  int gap  = 10;
  int totalW = (dots - 1) * gap;
  int startX = (W - totalW) / 2;

  int y = H - 10;
  for (int i = 0; i < dots; i++) {
    int x = startX + i * gap;
    uint16_t c = (i == (int)p) ? ACCENT : CARD_EDGE;
    tft.fillCircle(x, y, dotR, c);
  }
}

void drawCardFrame(const char* title, const char* unit, uint16_t labelColor) {
  tft.fillScreen(BG);
  drawTopBar(title);
  drawBottomNav(currentPage);

  tft.fillRoundRect(CARD_X, CARD_Y, CARD_W, CARD_H, 10, CARD_BG);
  tft.drawRoundRect(CARD_X, CARD_Y, CARD_W, CARD_H, 10, CARD_EDGE);

  tft.setTextSize(1);
  tft.setTextColor(labelColor);
  tft.setCursor(CARD_X + 10, CARD_Y + 8);
  tft.print("Mesure");

  tft.setTextColor(CARD_EDGE);
  int unit_px = (int)strlen(unit) * 6;
  tft.setCursor(CARD_X + CARD_W - 10 - unit_px, CARD_Y + 8);
  tft.print(unit);

  tft.fillRect(CARD_X + 10, CARD_Y + 18, CARD_W - 20, 2, ACCENT);
}

void clearValueArea() {
  tft.fillRect(VAL_X, VAL_Y, VAL_W, VAL_H, CARD_BG);
}

void printBigCentered(const String& s, uint16_t color, uint8_t size) {
  int16_t x1, y1;
  uint16_t w, h;
  tft.setTextSize(size);
  tft.getTextBounds((char*)s.c_str(), 0, 0, &x1, &y1, &w, &h);

  int x = VAL_X + (VAL_W - (int)w) / 2;
  int y = VAL_Y + (VAL_H - (int)h) / 2;

  tft.setCursor(x, y);
  tft.setTextColor(color);
  tft.print(s);
}

void printSmallBelow(const String& s, uint16_t color) {
  tft.setTextSize(1);
  tft.setTextColor(color);

  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds((char*)s.c_str(), 0, 0, &x1, &y1, &w, &h);

  int x = VAL_X + (VAL_W - (int)w) / 2;
  int y = VAL_Y + VAL_H - 10;

  tft.setCursor(x, y);
  tft.print(s);
}

// Lectures capteurs
float readDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long us = pulseIn(ECHO_PIN, HIGH, 30000);
  if (us == 0) return -1.0f;

  return (us * 0.0343f) / 2.0f;
}

int waterPercentFromDistance(float dcm) {
  if (dcm < 0) return -1;

  if (dcm <= WATER_FULL_CM)  return 100;
  if (dcm >= WATER_EMPTY_CM) return 0;

  float pct = (WATER_EMPTY_CM - dcm) * 100.0f / (WATER_EMPTY_CM - WATER_FULL_CM);
  int ipct = (int)(pct + 0.5f);
  if (ipct < 0) ipct = 0;
  if (ipct > 100) ipct = 100;
  return ipct;
}

float readTempC_DS18B20() {
  ds18.requestTemperatures();
  float t = ds18.getTempCByIndex(0);
  if (t == DEVICE_DISCONNECTED_C) return 999.0f;
  return t;
}

bool readDHT(float &t, float &h) {
  h = dht.readHumidity();
  t = dht.readTemperature();
  return !(isnan(h) || isnan(t));
}

// Auto pompe 
void autoSoilPumpControl() {
  if (pump_manual) return;

  // sécurité réservoir
  if (water_pct < 0 || water_pct < MIN_WATER_PCT_TO_RUN) {
    pumpOff();
    return;
  }

  // sécurité température sol 
  if (t1_c <= 900.0f && t1_c < MIN_SOIL_TEMP_C_TO_WATER) {
    pumpOff();
    return;
  }

  if (!pump_state && soil_pct <= SOIL_PCT_ON) {
    pumpOn();
  } else if (pump_state && soil_pct >= SOIL_PCT_OFF) {
    pumpOff();
  }
}

// NTP 
void initNtpTime() {
  configTzTime(TZ_PARIS, NTP1, NTP2, NTP3);

  struct tm t;
  for (int i = 0; i < 20; i++) { // ~10s max
    if (getLocalTime(&t, 500)) {
      if ((t.tm_year + 1900) >= 2024) {
        time_ok = true;
        return;
      }
    }
    delay(500);
  }
  time_ok = false;
}

String getTimeStr() {
  if (!time_ok) return "NTP:OFF";
  struct tm t;
  if (!getLocalTime(&t, 20)) return "NTP:OFF";
  char buf[32];
  strftime(buf, sizeof(buf), "%a %H:%M", &t);
  return String(buf);
}

void startScheduledWatering() {
  if (pump_manual) return;
  if (water_pct < 0 || water_pct < MIN_WATER_PCT_TO_RUN) return;

  // Bloquer aussi l'arrosage programmé si sol trop froid
  if (t1_c <= 900.0f && t1_c < MIN_SOIL_TEMP_C_TO_WATER) return;

  schedActive = true;
  schedStopMs = millis() + SCHED_WATER_MS;
  pumpOn();
}

void serviceScheduledWatering() {
  if (!schedActive) return;
  if ((long)(millis() - schedStopMs) >= 0) {
    pumpOff();
    schedActive = false;
  }
}

// LUNDI (1) et VENDREDI (5) à 08:00
void checkScheduleAndTrigger() {
  if (!time_ok) return;
  if (schedActive) return;
  if (pump_manual) return;

  struct tm t;
  if (!getLocalTime(&t, 50)) return;

  bool isMonOrFri = (t.tm_wday == 1) || (t.tm_wday == 5);
  bool is0800 = (t.tm_hour == 8) && (t.tm_min == 0);

  if (isMonOrFri && is0800 && (t.tm_yday != lastSchedYday)) {
    lastSchedYday = t.tm_yday;
    startScheduledWatering();
  }
}

// Boutons
bool buttonPressedEvent() {
  bool reading = digitalRead(BTN_PIN);

  if (reading != btnLastReading) {
    btnLastChange = millis();
    btnLastReading = reading;
  }

  if ((millis() - btnLastChange) > DEBOUNCE_MS) {
    if (reading != btnStable) {
      btnStable = reading;
      if (btnStable == LOW) return true;
    }
  }
  return false;
}

bool pumpButtonPressedEvent() {
  bool reading = digitalRead(BTN_PUMP_PIN);

  if (reading != pumpBtnLastReading) {
    pumpBtnLastChange = millis();
    pumpBtnLastReading = reading;
  }

  if ((millis() - pumpBtnLastChange) > DEBOUNCE_MS) {
    if (reading != pumpBtnStable) {
      pumpBtnStable = reading;
      if (pumpBtnStable == LOW) return true;
    }
  }
  return false;
}

// Pages 
void drawPageStatic(Page p) {
  switch (p) {
    case PAGE_WIFI:    drawCardFrame("WIFI",       "",    ACCENT); break;
    case PAGE_ULTRA:   drawCardFrame("NIVEAU EAU", "%",   ACCENT); break;
    case PAGE_DS18B20: drawCardFrame("DS18B20",    "C",   ACCENT); break;
    case PAGE_SOIL:    drawCardFrame("SOIL",       "%",   ACCENT); break;
    case PAGE_DHT:     drawCardFrame("DHT11",      "C/%", ACCENT); break;
    case PAGE_PUMP:    drawCardFrame("POMPE",      "",    ACCENT); break;
    default: break;
  }
}

void updatePageValue(Page p) {
  switch (p) {
    case PAGE_WIFI: {
      clearValueArea();
      bool ok = (WiFi.status() == WL_CONNECTED);
      if (!ok) {
        printBigCentered("OFF", WARN, 3);
        printSmallBelow("WiFi non connecte", CARD_EDGE);
      } else {
        printBigCentered("OK", COL_OK, 3);
        printSmallBelow(WiFi.localIP().toString(), CARD_EDGE);
      }
      break;
    }

    case PAGE_ULTRA:
      clearValueArea();
      if (dist_cm < 0 || water_pct < 0) {
        printBigCentered("---", WARN, 3);
        printSmallBelow("No echo", CARD_EDGE);
      } else {
        uint16_t c = (water_pct >= MIN_WATER_PCT_TO_RUN) ? TEXT : WARN;
        printBigCentered(String(water_pct) + "%", c, 3);
        printSmallBelow(String(dist_cm, 1) + " cm", CARD_EDGE);
      }
      break;

    case PAGE_DS18B20:
      clearValueArea();
      if (t1_c > 900) {
        printBigCentered("---", WARN, 3);
        printSmallBelow("Sensor error", CARD_EDGE);
      } else {
        uint16_t c = (t1_c < MIN_SOIL_TEMP_C_TO_WATER) ? WARN : TEXT;
        printBigCentered(String(t1_c, 1), c, 3);
        printSmallBelow("Temperature sol", CARD_EDGE);
      }
      break;

    case PAGE_SOIL:
      clearValueArea();
      {
        uint16_t c = (soil_pct <= SOIL_PCT_ON) ? WARN : TEXT;
        printBigCentered(String(soil_pct) + "%", c, 3);
        printSmallBelow("raw=" + String(soil_raw), CARD_EDGE);
      }
      break;

    case PAGE_DHT:
      clearValueArea();
      if (!dht_ok) {
        printBigCentered("---", WARN, 3);
        printSmallBelow("Read error", CARD_EDGE);
      } else {
        tft.setTextSize(2);
        tft.setTextColor(TEXT);

        tft.setCursor(VAL_X + 6, VAL_Y + 10);
        tft.print("T ");
        tft.print(t2_c, 1);
        tft.print("C");

        tft.setCursor(VAL_X + 6, VAL_Y + 40);
        tft.print("H ");
        tft.print(hum_pct, 0);
        tft.print("%");

        printSmallBelow("Air", CARD_EDGE);
      }
      break;

    case PAGE_PUMP: {
      clearValueArea();
      uint16_t c = pump_state ? COL_OK : WARN;
      printBigCentered(pump_state ? "ON" : "OFF", c, 3);

      String m = schedActive ? "PROG" : (pump_manual ? "MANUEL" : "AUTO");

      if (!pump_manual && !schedActive && (t1_c <= 900.0f) && (t1_c < MIN_SOIL_TEMP_C_TO_WATER)) {
        m += " | COLD";
      }

      String extra = m + " | " + getTimeStr();
      printSmallBelow(extra, CARD_EDGE);
      break;
    }

    default: break;
  }
}

//  Moniteur Série
void printAllSensorsToSerial() {
  static unsigned long lastPrint = 0;
  const unsigned long PRINT_MS = 1000;

  unsigned long now = millis();
  if (now - lastPrint < PRINT_MS) return;
  lastPrint = now;

  Serial.println();
  Serial.println("===== CAPTEURS =====");
  Serial.print("Heure    : ");
  Serial.println(getTimeStr());

  Serial.print("WiFi     : ");
  Serial.print((WiFi.status() == WL_CONNECTED) ? "OK" : "OFF");
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print(" | IP=");
    Serial.print(WiFi.localIP());
  }
  Serial.println();

  if (dist_cm < 0) Serial.println("Niveau eau : No echo");
  else {
    Serial.print("Niveau eau : ");
    Serial.print(water_pct);
    Serial.print("% (dist=");
    Serial.print(dist_cm, 1);
    Serial.println(" cm)");
  }

  if (t1_c > 900) Serial.println("DS18B20   : Sensor error");
  else {
    Serial.print("DS18B20   : ");
    Serial.print(t1_c, 1);
    Serial.print(" C");
    if (!pump_manual && t1_c < MIN_SOIL_TEMP_C_TO_WATER) {
      Serial.print(" (COLD LOCK < ");
      Serial.print(MIN_SOIL_TEMP_C_TO_WATER, 1);
      Serial.print("C)");
    }
    Serial.println();
  }

  Serial.print("Soil      : ");
  Serial.print(soil_pct);
  Serial.print("% (raw=");
  Serial.print(soil_raw);
  Serial.print(") | ON<=");
  Serial.print(SOIL_PCT_ON);
  Serial.print(" OFF>=");
  Serial.println(SOIL_PCT_OFF);

  if (!dht_ok) Serial.println("DHT11     : Read error");
  else {
    Serial.print("DHT11 T   : ");
    Serial.print(t2_c, 1);
    Serial.println(" C");
    Serial.print("DHT11 H   : ");
    Serial.print(hum_pct, 0);
    Serial.println(" %");
  }

  Serial.print("Pompe     : ");
  Serial.print(pump_state ? "ON" : "OFF");
  Serial.print(" | Mode: ");
  Serial.print(schedActive ? "PROG" : (pump_manual ? "MANUEL" : "AUTO"));
  Serial.print(" | EauOK: ");
  Serial.print((water_pct >= MIN_WATER_PCT_TO_RUN) ? "Y" : "N");
  Serial.print(" | SolOK: ");
  if (t1_c > 900) Serial.println("UNK");
  else Serial.println((t1_c >= MIN_SOIL_TEMP_C_TO_WATER) ? "Y" : "N");
}

// Web: pages + API 
String buildIndexHtml() {
  String page = R"HTML(
<!doctype html><html lang="fr"><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32 Serre</title>
<style>
  body{font-family:system-ui,Segoe UI,Arial;margin:16px;max-width:820px}
  .card{border:1px solid #ddd;border-radius:12px;padding:12px;margin:10px 0}
  .row{display:flex;justify-content:space-between;gap:10px;align-items:baseline}
  .big{font-size:28px;font-weight:700}
  button{padding:10px 14px;border-radius:10px;border:1px solid #ccc;background:#f7f7f7}
  button:active{transform:scale(0.99)}
  .ok{color:green}.bad{color:#c00}
  code{background:#f3f3f3;padding:2px 6px;border-radius:6px}
  .muted{color:#666;font-size:13px;margin-top:6px}
  .btnrow{display:flex;gap:10px;flex-wrap:wrap}
</style>
</head><body>
<h2>ESP32 - Serre</h2>

<div class="card">
  <div class="row"><div>IP</div><div><code id="ip">-</code></div></div>
  <div class="row" style="margin-top:6px"><div>Heure</div><div><code id="time">-</code></div></div>
</div>

<div class="card">
  <div class="row"><div>Niveau d'eau</div><div class="big"><span id="water">-</span> %</div></div>
  <div class="muted">distance: <span id="watercm">-</span> cm</div>
</div>

<div class="card">
  <div class="row"><div>DS18B20</div><div class="big"><span id="ds18">-</span> °C</div></div>
  <div class="muted">Verrou froid (AUTO): <span id="coldlock">-</span></div>
</div>

<div class="card">
  <div class="row"><div>Soil</div><div class="big"><span id="soil">-</span> %</div></div>
  <div class="muted">raw: <span id="soilraw">-</span></div>
</div>

<div class="card">
  <div class="row"><div>DHT11</div><div class="big"><span id="dhtt">-</span> °C / <span id="dhth">-</span> %</div></div>
</div>

<div class="card">
  <div class="row">
    <div>Pompe</div>
    <div class="big" id="pump">-</div>
  </div>
  <div class="row" style="margin-top:6px">
    <div>Mode</div>
    <div><code id="mode">-</code></div>
  </div>

  <div style="margin-top:10px" class="btnrow">
    <button onclick="togglePump()">Pompe</button>
    <button onclick="nextPage()">Page suivante (TFT)</button>
  </div>
</div>

<script>
async function refresh(){
  const r = await fetch('/api');
  const j = await r.json();

  document.getElementById('ip').textContent = j.ip;
  document.getElementById('time').textContent = j.time;

  document.getElementById('water').textContent   = (j.water_pct === null) ? '---' : j.water_pct;
  document.getElementById('watercm').textContent = (j.ultra_cm  === null) ? '---' : j.ultra_cm.toFixed(1);

  document.getElementById('ds18').textContent  = (j.ds18_c === null) ? '---' : j.ds18_c.toFixed(1);
  document.getElementById('coldlock').textContent = j.cold_lock ? 'ON' : 'OFF';

  document.getElementById('soil').textContent    = j.soil_pct;
  document.getElementById('soilraw').textContent = j.soil_raw;

  document.getElementById('dhtt').textContent  = (j.dht_ok ? j.dht_c.toFixed(1) : '---');
  document.getElementById('dhth').textContent  = (j.dht_ok ? Math.round(j.dht_h) : '---');

  const p = document.getElementById('pump');
  p.textContent = j.pump_on ? 'ON' : 'OFF';
  p.className = 'big ' + (j.pump_on ? 'ok' : 'bad');
  document.getElementById('mode').textContent = j.mode;
}

async function togglePump(){
  await fetch('/pump/toggle', {method:'POST'});
  refresh();
}

async function nextPage(){
  await fetch('/page/next', {method:'POST'});
  refresh();
}

refresh();
setInterval(refresh, 1000);
</script>
</body></html>
)HTML";
  return page;
}

void handleIndex() {
  server.send(200, "text/html; charset=utf-8", buildIndexHtml());
}

void handleApi() {
  String json = "{";
  json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
  json += "\"time\":\"" + getTimeStr() + "\",";

  if (dist_cm < 0) json += "\"ultra_cm\":null,";
  else json += "\"ultra_cm\":" + String(dist_cm, 2) + ",";

  if (water_pct < 0) json += "\"water_pct\":null,";
  else json += "\"water_pct\":" + String(water_pct) + ",";

  if (t1_c > 900) json += "\"ds18_c\":null,";
  else json += "\"ds18_c\":" + String(t1_c, 2) + ",";

  json += "\"soil_raw\":" + String(soil_raw) + ",";
  json += "\"soil_pct\":" + String(soil_pct) + ",";

  json += "\"dht_ok\":" + String(dht_ok ? "true" : "false") + ",";
  json += "\"dht_c\":" + String(isnan(t2_c) ? 0.0 : t2_c, 2) + ",";
  json += "\"dht_h\":" + String(isnan(hum_pct) ? 0.0 : hum_pct, 2) + ",";

  json += "\"pump_on\":" + String(pump_state ? "true" : "false") + ",";

  bool cold_lock = (t1_c <= 900.0f) && (t1_c < MIN_SOIL_TEMP_C_TO_WATER);
  json += "\"cold_lock\":" + String(cold_lock ? "true" : "false") + ",";

  String mode = schedActive ? "PROG" : (pump_manual ? "MANUEL" : "AUTO");
  json += "\"mode\":\"" + mode + "\"";

  json += "}";
  server.send(200, "application/json", json);
}

void handlePumpToggle() {
  pump_manual = true; // action web => manuel
  if (pump_state) pumpOff();
  else pumpOn();
  server.send(200, "text/plain", pump_state ? "ON" : "OFF");
}

void handlePageNext() {
  currentPage = (Page)((currentPage + 1) % PAGE_COUNT);
  lastDrawnPage = PAGE_COUNT; // force redraw
  server.send(200, "text/plain", "OK");
}

void setupWebServer() {
  server.on("/", HTTP_GET, handleIndex);
  server.on("/api", HTTP_GET, handleApi);
  server.on("/pump/toggle", HTTP_POST, handlePumpToggle);
  server.on("/page/next", HTTP_POST, handlePageNext);
  server.begin();
}

//  WiFi 
void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("WiFi: connexion a ");
  Serial.println(WIFI_SSID);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.print("WiFi OK, IP = ");
    Serial.println(WiFi.localIP());
  } else {
    wifiConnected = false;
    Serial.println("WiFi ECHEC (timeout).");
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(RELAY_PIN, OUTPUT);
  pumpOff();

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);

  pinMode(BTN_PIN, INPUT_PULLUP);
  pinMode(BTN_PUMP_PIN, INPUT_PULLUP);

  ds18.begin();
  dht.begin();

  analogReadResolution(12);
  analogSetPinAttenuation(SOIL_PIN, ADC_11db);

  SPI.begin(TFT_SCK, -1, TFT_MOSI, TFT_CS);
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);

  drawPageStatic(currentPage);
  updatePageValue(currentPage);
  connectWiFi();
  if (WiFi.status() == WL_CONNECTED) {
    setupWebServer();

    initNtpTime();
    if (time_ok) Serial.println("NTP OK (heure synchronisee).");
    else         Serial.println("NTP ECHEC (heure non valide).");
  }

  Serial.println("Init OK");
}

void loop() {

  server.handleClient();
  dist_cm   = readDistanceCm();
  water_pct = waterPercentFromDistance(dist_cm);

  t1_c      = readTempC_DS18B20();

  soil_raw  = analogRead(SOIL_PIN);
  soil_pct  = soilPercentFromRaw(soil_raw);

  dht_ok    = readDHT(t2_c, hum_pct);

  checkScheduleAndTrigger();
  serviceScheduledWatering();
  if (!schedActive) {
    autoSoilPumpControl();
  }
  if (pumpButtonPressedEvent()) {
    pump_manual = true;
    if (pump_state) pumpOff();
    else            pumpOn();
  }
  printAllSensorsToSerial();

  if (buttonPressedEvent()) {
    currentPage = (Page)((currentPage + 1) % PAGE_COUNT);
  }
  if (currentPage != lastDrawnPage) {
    lastDrawnPage = currentPage;
    drawPageStatic(currentPage);
  }
  updatePageValue(currentPage);

  delay(250);
}
