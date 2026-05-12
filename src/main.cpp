// ---------- BLYNK settings ----------
//#define BLYNK_TEMPLATE_ID      "TMPL4KNN1-VFn"
//#define BLYNK_TEMPLATE_NAME    "WeatherStation2.0"
#define BLYNK_FIRMWARE_VERSION "1.0"
#define BLYNK_PRINT Serial

// ---------- DEBUG OPTIONS ----------
// #define BLYNK_DEBUG
// #define APP_DEBUG

// ---------- USED BOARD ----------
#define USE_ESP32_S3_WROOM_1

#define WAKE_BUTTON_PIN 15
#define LONG_PRESS_TIME 2000   // 2 Sekunden

// ---------- Includes ----------
#include <esp_sleep.h>
#include <EEPROM.h>
#include <Adafruit_SHTC3.h>
#include <Adafruit_MAX1704X.h>
#include <Adafruit_MPL3115A2.h>
#include "BlynkEdgent.h"
#include "DEV_Config.h"
#include "EPD.h"
#include "GUI_Paint.h"
#include "imagedata.h"
#include "time.h"
#include "Pressure.h"

// ---------- MISC ----------
Adafruit_SHTC3     shtc3 = Adafruit_SHTC3();
Adafruit_MAX17048  maxlipo;
Adafruit_MPL3115A2 mpl;

// ==========================================================================
// SENSOR FUNCTIONS
// ==========================================================================

BLYNK_WRITE(V3) {
  uint16_t alt = param.asInt();
  EEPROM.put(1, alt);     // ← put() statt write(), Byte 1 statt 0
  EEPROM.commit();
  delay(100);
  PRESSURE_ALTITUDE_M = (float)alt;
}


void readData(float *temp, float *hum, float *batteryPercent) {
  sensors_event_t humidity, temperature;
  if (!shtc3.getEvent(&humidity, &temperature)) {
    Serial.println("Failed to read from SHTC3 sensor!");
    return;
  }
  *temp = temperature.temperature;
  *hum  = humidity.relative_humidity;
  if (isnan(*hum) || isnan(*temp)) {
    Serial.println("Invalid SHTC3 data!");
  }
  *batteryPercent = maxlipo.cellPercent();

  if (*batteryPercent < 0) *batteryPercent = 0;
  if (*batteryPercent > 100) *batteryPercent = 100;
}

void readPressure(float *pressure) {
  *pressure = mpl.getPressure();
  if (isnan(*pressure)) {
    Serial.println("Failed to read pressure from MPL3115A2!");
    return;
  }
  Serial.print("Pressure: ");
  Serial.print(*pressure, 1);
  Serial.println(" hPa");
}

// ==========================================================================
// BLYNK
// ==========================================================================

void sendData(float *temp, float *hum, float p_qnh, float batteryPercent) {
  Blynk.virtualWrite(V0, *temp);
  Blynk.virtualWrite(V1, *hum);
  Blynk.virtualWrite(V2, p_qnh);
  Blynk.virtualWrite(V4, batteryPercent);
}

// ==========================================================================
// DISPLAY — einzige Funktion die EPD_2IN66_Display aufruft
// Zeichnet in dieser Reihenfolge:
//   1. InnCubator-Logo (Hintergrund)
//   2. Messwerte (Temp / Hum / Druck)
//   3. Akkubalken
//   4. Wettericon + Label  (Platz des früheren Humidity-Indicators)
//   5. Uhrzeit
//   6. EPD_2IN66_Display + Sleep
// ==========================================================================

void refreshWeather(UBYTE *image, WeatherState weather,
                    float temp, float hum, float p_qnh, float batteryPercent) {

  // 1. InnCubator-Hintergrund (überschreibt ganzen Buffer)
  Paint_DrawBitMap(gImage_inc);

  // 2. Messwerte
  String tempStr     = String(temp,  1) + " ^C";
  String humStr      = String(hum,   1) + " %";
  String pressureStr = String(p_qnh, 1) + " hPa";
  Paint_DrawString_EN(70,  59, tempStr.c_str(),     &Font16, BLACK, WHITE);
  Paint_DrawString_EN(70, 105, humStr.c_str(),      &Font16, BLACK, WHITE);
  Paint_DrawString_EN(20, 150, pressureStr.c_str(), &Font16, BLACK, WHITE);

  // 3. Akkubalken
 
  int bars = 0;

  if      (batteryPercent >= 90) bars = 5;
  else if (batteryPercent >= 70) bars = 4;
  else if (batteryPercent >= 50) bars = 3;
  else if (batteryPercent >= 30) bars = 2;
  else if (batteryPercent >= 10) bars = 1;
  Serial.print("Battery: ");
  Serial.print(batteryPercent, 1);
  Serial.print("%  -> Bars: ");
  Serial.println(bars);
  for (int i = 0; i < bars; i++) {
    int x1 = 10 + i * (22 + 6);   // barWidth=22, gap=6
    Paint_DrawRectangle(x1, 9, x1 + 22, 19, WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
  }

  // 4. Wettericon — gleicher Bereich wie früher der Humidity-Indicator
  Paint_ClearWindows(0, 180, 155, 255, BLACK);
  const char *label = "";
  switch (weather) {
    case WEATHER_SUNNY:
      Paint_DrawImage(Weather_Sunny,    16, 189, 120, 60);
      label = "Sonnig";
      break;
    case WEATHER_PARTLY:
      Paint_DrawImage(Weather_Partly,   16, 189, 120, 60);
      label = "Wechselnd";
      break;
    case WEATHER_CHANGING:
      Paint_DrawImage(Weather_Changing, 16, 189, 120, 60);
      label = "Wechselhaft";
      break;
    case WEATHER_BAD:
      Paint_DrawImage(Weather_Bad,      16, 189, 120, 60);
      label = "Schlecht";
      break;
    case WEATHER_UNKNOWN:
    default:
      Paint_DrawImage(Weather_Unknown,  16, 189, 120, 60);
      label = "Kein Trend";
      break;
  }
  Paint_DrawString_EN(5, 243, label, &Font12, BLACK, WHITE);

  // 5. Uhrzeit
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    char timeString[10];
    strftime(timeString, sizeof(timeString), "%H:%M", &timeinfo);
    Serial.print("Zeit: ");
    Serial.println(timeString);
    Paint_DrawString_EN(35, 258, timeString, &Font16, BLACK, WHITE);
  }

  // 6. Finaler Display-Call
  EPD_2IN66_Display(image);
  EPD_2IN66_Sleep();
}

// ==========================================================================
// HILFSFUNKTIONEN
// ==========================================================================

bool isLongPress() {
  if (digitalRead(WAKE_BUTTON_PIN) == HIGH) return false;
  unsigned long pressStart = millis();
  while (digitalRead(WAKE_BUTTON_PIN) == LOW) {
    if (millis() - pressStart >= LONG_PRESS_TIME) return true;
    delay(10);
  }
  return false;
}

void showStatusPage(UBYTE *image) {
  Paint_Clear(WHITE);
  Paint_DrawString_EN(5, 5, "STATUS", &Font20, WHITE, BLACK);

  if (Blynk.connected()) {
    Paint_DrawString_EN(5, 60, "WLAN: connected",  &Font12, WHITE, BLACK);
    String ssidText = "SSID: " + WiFi.SSID();
    Paint_DrawString_EN(5, 70, ssidText.c_str(),   &Font12, WHITE, BLACK);
    Paint_DrawString_EN(5, 90, "Blynk: connected", &Font12, WHITE, BLACK);
  } else {
    Paint_DrawString_EN(5, 60, "WLAN: not connected",  &Font12, WHITE, BLACK);
    Paint_DrawString_EN(5, 85, "Blynk: not connected", &Font12, WHITE, BLACK);
  }

  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 5000)) {
    char timeString[30];
    strftime(timeString, sizeof(timeString), "%d.%m.%Y %H:%M", &timeinfo);
    Paint_DrawString_EN(5, 110, timeString, &Font12, WHITE, BLACK);
  } else {
    Paint_DrawString_EN(5, 110, "Time: no sync", &Font12, WHITE, BLACK);
  }

  Paint_DrawString_EN(5, 170, BLYNK_TEMPLATE_NAME, &Font12, WHITE, BLACK);
  Paint_DrawString_EN(5, 195, BLYNK_TEMPLATE_ID,   &Font12, WHITE, BLACK);

  EPD_2IN66_Display(image);
  EPD_2IN66_Sleep();
}

// ==========================================================================
// SETUP
// ==========================================================================

void setup() {
  Serial.begin(115200);

  pinMode(WAKE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(1, INPUT);   // WiFi-Reset

  Wire.begin(47, 48);  // SDA=IO47, SCL=IO48

  if (!mpl.begin())    Serial.println("MPL3115A2 not found!");
  else                 Serial.println("MPL3115A2 found.");

  if (!shtc3.begin()) {
    Serial.println("Couldn't find SHTC3 sensor!");
    while (1) delay(10);
  }

  if (!maxlipo.begin()) Serial.println("MAX17048 not found!");
  else                  Serial.println("MAX17048 found.");

  EEPROM.begin(4);   // mind. 4 Bytes: Byte 0 = sleepMin, Bytes 1-2 = altitude

  uint16_t savedAlt;
  EEPROM.get(1, savedAlt);
  if (savedAlt > 0 && savedAlt <= 10000) {
    PRESSURE_ALTITUDE_M = (float)savedAlt;
  }
  Serial.printf("Höhe aus EEPROM: %.0f m\n", PRESSURE_ALTITUDE_M);

  //EEPROM.begin(1);
  BlynkEdgent.begin();
  DEV_Module_Init();
  EPD_2IN66_Init();
}

// ==========================================================================
// MAIN LOOP
// ==========================================================================

void loop() {
  float temp     = 0;
  float hum      = 0;
  float pressure = 0;
  float p_qnh    = 0;
  float trend    = 0;
  float batteryPercent = 0;

  // ── WiFi-Reset (GPIO1 beim Start gedrückt) ────────────────────────────
  if (!digitalRead(1)) BlynkEdgent.ResetConfig();

  // ── Image-Buffer anlegen ─────────────────────────────────────────────
  UWORD Imagesize = ((EPD_2IN66_WIDTH % 8 == 0)
                       ? (EPD_2IN66_WIDTH / 8)
                       : (EPD_2IN66_WIDTH / 8 + 1)) * EPD_2IN66_HEIGHT;
  UBYTE *BlackImage = (UBYTE *)malloc(Imagesize);
  if (BlackImage == NULL) {
    Serial.println("Failed to apply for black memory...");
    while (1);
  }
  Paint_NewImage(BlackImage, EPD_2IN66_WIDTH, EPD_2IN66_HEIGHT, 0, WHITE);

  // ── Langer Button-Druck → Statusseite ────────────────────────────────
  if (isLongPress()) {
    Paint_Clear(WHITE);
    for (int i = 0; i < 120; i++) {
      if (Blynk.connected()) {
        configTime(0, 0, "pool.ntp.org");
        setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);
        tzset();
        break;
      }
      BlynkEdgent.run();
      delay(1000);
    }
    showStatusPage(BlackImage);
    free(BlackImage);
    delay(3000);
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_15, 0);
    esp_deep_sleep_start();
  }

  // ── Sensoren lesen ────────────────────────────────────────────────────
  readData(&temp, &hum,&batteryPercent);
  readPressure(&pressure);
  processPressure(pressure, &p_qnh, &trend);

  WeatherState weather = pressureHistoryReady()
                           ? classifyWeather(p_qnh, trend)
                           : WEATHER_UNKNOWN;
  Serial.printf("[Weather] %s | QNH: %.1f hPa | Trend: %.2f hPa/3h\n",
                weatherStateToString(weather), p_qnh, trend);

  // ── Blynk verbinden, NTP holen, Daten senden ─────────────────────────
for (int i = 0; i < 120; i++) {
  if (Blynk.connected()) {
    configTime(0, 0, "pool.ntp.org");
    setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);
    tzset();

    Blynk.syncVirtual(V3);
    // ← run() aufrufen damit BLYNK_WRITE verarbeitet wird
    for (int j = 0; j < 10; j++) {
      BlynkEdgent.run();
      delay(100);
    }

    sendData(&temp, &hum, p_qnh, batteryPercent);
    break;
  }
  BlynkEdgent.run();
  delay(1000);
}

  // ── Alles zeichnen & Display aktualisieren ────────────────────────────
  refreshWeather(BlackImage, weather, temp, hum, p_qnh, batteryPercent);

  // ── Aufräumen & Deep Sleep ────────────────────────────────────────────
  free(BlackImage);
  delay(500);

  esp_sleep_enable_timer_wakeup(30*1000000);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_15, 0);
  Blynk.disconnect();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  esp_deep_sleep_start();
}