#include "Pressure.h"

// ---------- RTC RINGPUFFER ----------
// RTC_DATA_ATTR: Variablen bleiben im Deep Sleep erhalten
RTC_DATA_ATTR float pressure_history[PRESSURE_HISTORY_SIZE];
RTC_DATA_ATTR int   history_index = 0;   // Zeigt auf den nächsten Schreibplatz
RTC_DATA_ATTR int   history_count = 0;   // Anzahl bisher gespeicherter Werte

// ---------- IMPLEMENTIERUNG ----------
float PRESSURE_ALTITUDE_M = 570.0f;  // Default
float correctToQNH(float p_raw_hpa) {
  return p_raw_hpa * expf(PRESSURE_ALTITUDE_M / 8434.5f);
}

void processPressure(float p_raw_hpa, float *p_qnh_out, float *trend_out) {
  float p_qnh = correctToQNH(p_raw_hpa);
  *p_qnh_out  = p_qnh;
  *trend_out  = 0.0f;

  // Gradient: aktueller QNH minus ältester Wert im Ringpuffer
  // Erst berechnen, bevor der älteste Wert überschrieben wird
  if (history_count >= PRESSURE_HISTORY_SIZE) {
    *trend_out = p_qnh - pressure_history[history_index];
  }

  // Aktuellen Wert in Ringpuffer schreiben
  pressure_history[history_index] = p_qnh;
  history_index = (history_index + 1) % PRESSURE_HISTORY_SIZE;
  if (history_count < PRESSURE_HISTORY_SIZE) history_count++;

  // Serielle Ausgabe
  Serial.printf("[Pressure] Raw: %.1f hPa  |  QNH: %.1f hPa  |  Trend: %+.2f hPa  |  History: %d/%d\n",
    p_raw_hpa, p_qnh, *trend_out, history_count, PRESSURE_HISTORY_SIZE);

  if (!pressureHistoryReady()) {
    Serial.println("[Pressure] Ringpuffer noch nicht voll – Trend noch nicht aussagekräftig.");
  }
}

WeatherState classifyWeather(float p_qnh, float trend) {
  // Schlechtwetter: Absolutdruck tief ODER starker Abfall
  if (p_qnh < 1000.0f || trend < -3.0f)   return WEATHER_BAD;

  // Schön: Hochdruck und stabil oder steigend
  if (p_qnh > 1015.0f && trend >= -0.5f)  return WEATHER_SUNNY;

  // Hochdruck aber fallend: Achtung
  if (p_qnh > 1015.0f && trend < -0.5f)   return WEATHER_PARTLY;

  // Übergangslage
  return WEATHER_CHANGING;
}

const char* weatherStateToString(WeatherState state) {
  switch (state) {
    case WEATHER_SUNNY:    return "Sunny";
    case WEATHER_PARTLY:   return "Partly";
    case WEATHER_CHANGING: return "Changing";
    case WEATHER_BAD:      return "Bad";
    default:               return "Unknown";
  }
}

bool pressureHistoryReady() {
  return history_count >= PRESSURE_HISTORY_SIZE;
}
