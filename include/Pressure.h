#pragma once

#include <Arduino.h>
#include <esp_sleep.h>

// ---------- KONFIGURATION ----------
#define PRESSURE_HISTORY_SIZE  6        // Anzahl Messpunkte für Trendberechnung
extern float PRESSURE_ALTITUDE_M;    //570.0f   // Standorthöhe in Meter (Innsbruck ~570m, Neustift ~1000m)

// ---------- WETTERKLASSIFIKATION ----------
typedef enum {
  WEATHER_SUNNY,      // ☀️  Hochdruck, stabil oder steigend
  WEATHER_PARTLY,     // ⛅  Hochdruck, aber fallend
  WEATHER_CHANGING,   // 🌤  Übergangslage
  WEATHER_BAD,         // 🌧  Tiefdruck oder starker Abfall
  WEATHER_UNKNOWN     // ❓  Keine oder unzureichende Daten
} WeatherState;

// ---------- FUNKTIONSDEKLARATIONEN ----------

/**
 * Normiert einen Rohdruckwert (hPa) auf Meereshöhe (QNH).
 * @param p_raw_hpa  Gemessener Luftdruck in hPa
 * @return           QNH-normierter Luftdruck in hPa
 */
float correctToQNH(float p_raw_hpa);

/**
 * Verarbeitet einen neuen Druckwert:
 * - QNH-Korrektur
 * - Eintrag in RTC-Ringpuffer (überlebt Deep Sleep)
 * - Gradientenberechnung über alle PRESSURE_HISTORY_SIZE Messpunkte
 *
 * @param p_raw_hpa   Rohwert vom Sensor (hPa)
 * @param p_qnh_out   Ausgabe: QNH-normierter Druck (hPa)
 * @param trend_out   Ausgabe: Gradient (hPa) über das gesamte Messfenster
 *                    0.0 solange Ringpuffer noch nicht voll
 */
void processPressure(float p_raw_hpa, float *p_qnh_out, float *trend_out);

/**
 * Klassifiziert das Wetter anhand von QNH-Druck und Trend.
 * @param p_qnh   QNH-normierter Luftdruck (hPa)
 * @param trend   Gradient über Messfenster (hPa)
 * @return        WeatherState (SUNNY / PARTLY / CHANGING / BAD)
 */
WeatherState classifyWeather(float p_qnh, float trend);

/**
 * Gibt den WeatherState als lesbaren String zurück (für Serial/Display).
 * @param state  WeatherState
 * @return       const char* z.B. "Sunny", "Bad", ...
 */
const char* weatherStateToString(WeatherState state);

/**
 * Gibt an ob der Ringpuffer bereits vollständig gefüllt ist.
 * Solange false: Trendwert ist 0 und nicht aussagekräftig.
 * @return true wenn alle PRESSURE_HISTORY_SIZE Werte vorhanden
 */
bool pressureHistoryReady();
