# Weather-Station 2.0 - Funktionsübersicht

## 1. Projektübersicht

Die **Weather-Station 2.0** ist ein IoT-Wetterstation-Gerät für ein ESP32-S3-basiertes System. Die Firmware erfasst Temperatur, Luftfeuchtigkeit, Luftdruck und Batteriestand, zeigt die Daten auf einem E-Paper-Display an und sendet sie optional an die Blynk IoT-Cloud.

### Hauptmerkmale:
- 📊 Messung von Temperatur, Luftfeuchtigkeit, Luftdruck und Batterieladung
- 📡 WLAN-Verbindung und Blynk-Cloud-Anbindung
- 🖥️ E-Paper-Display (Waveshare EPD-2IN66, 296x152)
- 🔋 Deep-Sleep-basiertes Energiemanagement
- ⏰ NTP-Zeit-Synchronisation mit CET/CEST
- 🌦 Drucktrendbasierte Wetterklassifikation
- ⚙️ Höhenkorrektur über EEPROM-konfigurierbare Altitude-Einstellung

---

## 2. Hardwarekomponenten

### 2.1 Mikrocontroller
- **Board**: ESP32-S3 WROOM-1
- **Framework**: Arduino (PlatformIO)
- **Spannung**: 3.3V

### 2.2 Sensoren

| Sensor | Modell | Funktion | Schnittstelle |
|--------|--------|----------|---------------|
| Temperatur & Luftfeuchtigkeit | Adafruit SHTC3 | Temperatur und relative Luftfeuchtigkeit | I2C |
| Luftdruck | Adafruit MPL3115A2 | Barometrischer Druck | I2C |
| Batterie-Überwachung | Adafruit MAX17048 | Batteriestand in % | I2C |

### 2.3 Display
- **Typ**: E-Paper (EPD)
- **Modell**: Waveshare EPD-2IN66
- **Auflösung**: 296 x 152 Pixel
- **Farbe**: 1-Bit Schwarzweiß
- **Schnittstelle**: SPI

### 2.4 Eingabe & Reset
- **Wake-Button**: GPIO-15 mit `INPUT_PULLUP`
  - Normaler Druck: Prozess ausführen und anschließend schlafen
  - Langer Druck (> 2 Sekunden): Statusseite anzeigen
- **WiFi-Reset**: GPIO-1 als Eingabe zum Zurücksetzen der Blynk/WiFi-Konfiguration

### 2.5 Kommunikation
- **WiFi**: ESP32 integriert, 2.4 GHz
- **I2C**: SDA = GPIO-47, SCL = GPIO-48

---

## 3. Software-Funktionen

### 3.1 Sensorauswertung

#### `readData(&temp, &hum, &batteryPercent)`
- Liest Temperatur und Luftfeuchtigkeit vom SHTC3
- Liest Batterieladung vom MAX17048
- Validiert Messwerte auf NaN
- Beschneidet Batteriewerte auf den Bereich 0-100%

#### `readPressure(&pressure)`
- Liest den Rohdruck in hPa vom MPL3115A2
- Prüft auf ungültige Werte

#### `processPressure(pressure, &p_qnh, &trend)`
- Normiert Rohdruck auf Meereshöhe (QNH)
- Verwendet die Höhenkorrektur `PRESSURE_ALTITUDE_M`
- Speichert den QNH-Wert im RTC-Ringpuffer über Deep Sleep
- Berechnet den Trend gegenüber dem ältesten gespeicherten Wert
- Gibt Trend und QNH aus

#### Wetterklassifikation
- `WEATHER_BAD` bei niedrigem Druck (< 1000 hPa) oder starkem Abfall (< -3.0 hPa)
- `WEATHER_SUNNY` bei hohem Druck (> 1015 hPa) und stabilen/steigenden Werten
- `WEATHER_PARTLY` bei hohem Druck, aber fallendem Trend
- `WEATHER_CHANGING` bei Übergangslage

### 3.2 Blynk-Integration

#### Virtuelle Pins
- `V0`: Temperatur (°C)
- `V1`: Luftfeuchtigkeit (%)
- `V2`: Normierter Druck QNH (hPa)
- `V3`: Altitude-Einstellung (EEPROM)
- `V4`: Batterieladung (%)

#### Datenversand
- `sendData(&temp, &hum, p_qnh, batteryPercent)` überträgt Messwerte an Blynk
- Nur bei bestehender Blynk-Verbindung

#### Konfiguration über Blynk
- `BLYNK_WRITE(V3)` speichert die eingestellte Höhe in EEPROM
- `PRESSURE_ALTITUDE_M` wird dadurch angepasst

### 3.3 Display-Management

#### `refreshWeather(BlackImage, weather, temp, hum, p_qnh, batteryPercent)`
- Zeichnet das InnCubator-Hintergrundbild
- Zeigt Temperatur, Luftfeuchtigkeit, Druck, Batteriestand, Wettericon und Uhrzeit
- Ruft anschließend `EPD_2IN66_Display()` und `EPD_2IN66_Sleep()` auf

#### Anzeigeelemente
| Element | Inhalt | Position |
|---------|--------|----------|
| Temperatur | `temp` in °C | links oben |
| Luftfeuchtigkeit | `hum` in % | Mitte links |
| Druck | QNH in hPa | unten links |
| Batteriestand | 5 Balken | oben links |
| Wettericon | Klassifizierter Wetterzustand | unten links |
| Uhrzeit | lokale Uhrzeit `HH:MM` | unten |

#### Batteriestand
- >= 90%: 5 Balken
- >= 70%: 4 Balken
- >= 50%: 3 Balken
- >= 30%: 2 Balken
- >= 10%: 1 Balken
- < 10%: 0 Balken

#### Wettericons
- `Sunny` → Sonnig
- `Partly` → Wechselnd
- `Changing` → Wechselhaft
- `Bad` → Schlecht
- `Unknown` → Kein Trend

### 3.4 Statusseite

#### Aktivierung
- Langer Druck auf Wake-Button (> 2 Sekunden)
- Versucht bis zu 120 Sekunden, Blynk und Zeit zu verbinden

#### Inhalt
- WLAN-Status + SSID
- Blynk-Verbindungsstatus
- Datum und Uhrzeit (`DD.MM.YYYY HH:MM`)
- Blynk Template Name und Template ID

### 3.5 Zeit-Synchronisation

- `configTime()` mit `pool.ntp.org`
- Zeitzone `CET-1CEST,M3.5.0/2,M10.5.0/3`
- Anzeige auf Display, falls lokal verfügbar
- Statusseite zeigt `Time: no sync`, wenn NTP nicht erreichbar

---

## 4. Stromverwaltung

### 4.1 Deep Sleep
- Nach jeder Messung und Anzeige geht das Gerät in Deep Sleep
- Weckt auf durch:
  - Timer nach 30 Sekunden
  - Wake-Button an GPIO-15

### 4.2 Schlafsequenz
- Trennt Blynk-Verbindung
- Trennt WiFi
- Schaltet WiFi-Modus ab
- Aktiviert Deep Sleep

### 4.3 EEPROM
- `EEPROM.begin(4)` reserviert 4 Bytes
- Speichert die Höhe in EEPROM-Adressbereich 1-2
- Erlaubt QNH-Korrekturen nach Höhenanpassung

---

## 5. Starten und Betriebsablauf

### 5.1 Setup
1. Serielle Ausgabe konfigurieren (115200 Baud)
2. GPIOs einrichten: Wake-Button und WiFi-Reset
3. I2C initialisieren auf GPIO-47 (SDA) und GPIO-48 (SCL)
4. Sensoren initialisieren: MPL3115A2, SHTC3, MAX17048
5. EEPROM initialisieren und Höhe auslesen
6. BlynkEdgent starten
7. EPD-Display initialisieren

### 5.2 Loop
1. Prüft WiFi-Reset beim Start über GPIO-1
2. Allokiert Display-Buffer und bereitet den Grafikpuffer vor
3. Langer Druck zeigt die Statusseite und löst Deep Sleep aus
4. Liest Sensoren und Batteriestatus
5. Berechnet QNH und Drucktrend
6. Bestimmt `WeatherState`
7. Versucht, Blynk zu verbinden, synchronisiert Zeit und sendet Daten
8. Aktualisiert das E-Paper-Display
9. Gibt Speicher frei und startet Deep Sleep

6. Blynk EdgeAgent initialisieren
7. E-Paper Display initialisieren

### 5.2 Hauptschleife (Loop)

```
┌─────────────────────────────────────────┐
│      Power-On (Deep-Sleep beendet)      │
├─────────────────────────────────────────┤
│    1. Button-Status prüfen              │
│       ├─ Normaler Druck: Messmodus     │
│       └─ Langer Druck: Statusmodus     │
├─────────────────────────────────────────┤
│    2. Statusmodus (>2s Druck)?          │
│       ├─ JA: Statusseite zeigen        │
│       │    → Zurück in Deep-Sleep       │
│       └─ NEIN: Weiter zum Schritt 3    │
├─────────────────────────────────────────┤
│    3. Hintergrundbild zeichnen          │
├─────────────────────────────────────────┤
│    4. Sensoren auslesen                 │
│       ├─ Temperatur & Luftfeuchtigkeit │
│       ├─ Luftdruck                     │
│       └─ Batteriestand                 │
├─────────────────────────────────────────┤
│    5. Display aktualisieren             │
│       ├─ Sensordaten anzeigen          │
│       └─ Batteriestand anzeigen        │
├─────────────────────────────────────────┤
│    6. WiFi verbinden (max. 120s)        │
├─────────────────────────────────────────┤
│    7. Zeit synchronisieren (NTP)        │
├─────────────────────────────────────────┤
│    8. Daten an Blynk senden             │
├─────────────────────────────────────────┤
│    9. Zeit auf Display anzeigen         │
├─────────────────────────────────────────┤
│    10. Feuchtigkeitsindikator anzeigen  │
├─────────────────────────────────────────┤
│    11. Deep-Sleep aktivieren            │
│        └─ Warte auf Timer oder Button   │
└─────────────────────────────────────────┘
```

---

## 6. Konfiguration und Anpassung

### 6.1 Firmware-Optionen
- **Build Flags**: 
  - `ARDUINO_USB_MODE=1`: USB-Modus aktiviert
  - `ARDUINO_USB_CDC_ON_BOOT=1`: CDC-Debugging beim Booten
  - `BLYNK_TEMPLATE_ID`: Eindeutige Template-ID pro Gerät
  - `BLYNK_TEMPLATE_NAME`: Template-Name pro Gerät

### 6.2 Abhängigkeiten
- `Adafruit SHTC3`: Temperatur/Luftfeuchtigkeit
- `Adafruit MAX1704X`: Batterie-Überwachung
- `Adafruit MPL3115A2`: Luftdruckmes sung
- `Blynk@^1.3.2`: IoT-Cloud-Plattform

### 6.3 Participant-Konfiguration
Jedes Gerät hat eigene Konfiguration in `platformio.ini`:
- Eindeutige Blynk Template ID
- Eindeutige Blynk Template Name
- Unterschiedliche Build-Umgebungen pro Gerät

Beispiel:
```ini
[env:1AB]
extends = common
build_flags =
    ${common.build_flags}
    '-DBLYNK_TEMPLATE_ID="TMPL4KNN1-VFn"'
    '-DBLYNK_TEMPLATE_NAME="WS_1_MW"'
```

---

## 7. Fehlerbehandlung

### 7.1 Sensor-Fehler
- **SHTC3 nicht gefunden**: Error im Serial Monitor, Gerät freezed
- **MPL3115A2 nicht gefunden**: Warning im Serial Monitor, Funktion noch ausführbar
- **MAX17048 nicht gefunden**: Warning im Serial Monitor, Batterie-Display leer

### 7.2 WiFi/Blynk
- **Verbindung fehlgeschlagen**: Nach 120s Timeout wird trotzdem Deep-Sleep aktiviert
- **Daten-Sync**: Erfolgt nur bei erfolgreicher Blynk-Verbindung

### 7.3 Zeit-Synchronisation
- **NTP-Fehler**: Display zeigt "Time: no sync" auf Statusseite
- **Timeout**: 5 Sekunden Timeout bei Zeit-Abfrage

---

## 8. Hardware-Pinbelegung

| Funktion | GPIO | Typ | Bemerkung |
|----------|------|-----|----------|
| Wake-Button | GPIO-15 | Input (Pull-Up) | Zum Aufwecken und Statusseite |
| WiFi-Reset | GPIO-1 | Input | Zum Zurücksetzen der WiFi-Konfiguration |
| I2C SDA | GPIO-47 | I2C | Sensoren-Kommunikation |
| I2C SCL | GPIO-48 | I2C | Sensoren-Kommunikation |
| E-Paper SPI | GPIO-xx | SPI | Display-Kommunikation |
| USB CDC | USB | Serial | Debug/Upload |

---

## 9. Typische Messintervalle

Da das Gerät nach jeder Messung in Deep-Sleep geht, sind typische Messintervalle:

- **Schnell**: 15-30 Minuten (höhere Batterieentladung)
- **Standard**: 1-2 Stunden (empfohlen)
- **Lang**: 4-6 Stunden (für lange Batterielebensdauer)
- **Ultra-Lang**: 12-24 Stunden (extrem sparsamerer Betrieb)

Das Intervall wird über das EEPROM konfiguriert (standardmäßig 1 Minute im Code).

---

## 10. Externe Bibliotheken & Abhängigkeiten

### BlynkEdgent
- Automatische WiFi-Konfiguration
- Cloud-Synchronisation
- OTA-Updates (Firmware-Updates over-the-air)

### Adafruit Sensor Libraries
- Abstraktionsebene für Sensoren-Kommunikation
- Event-basierte Datenauswertung

### E-Paper Display Library
- Waveshare EPD 2.66" Unterstützung
- Partielle Display-Aktualisierung
- Sleep-Modi für Energieersparnis

---

## 11. Entwicklungs- & Debug-Möglichkeiten

### Serial Debug
- Baud-Rate: 115200
- Ausgabe aller kritischen Funktionen:
  - Sensorwerte
  - Verbindungsstatus
  - Fehler und Warnungen

### Build-Varianten
```bash
# Für spezifisches Gerät (z.B. 1AB)
platformio run -e 1AB --target upload

# Für mehrere Geräte
platformio run -e 1AB -e 2CD --target build
```

---

## 12. Sicherheitshinweise

- **WiFi-Passwort**: Wird verschlüsselt in ESP32-Flash gespeichert (BlynkEdgent)
- **Blynk Auth Token**: Wird während Setup übermittelt
- **OTA-Updates**: Werden signiert (via Blynk)
- **Deep-Sleep**: Speichert Flash-Konfiguration persistent

---

## Zusammenfassung

Die Weather-Station 2.0 ist eine vollständig autonome, batteriebetriebene IoT-Wetterstation mit:
- ✅ Lokaler E-Paper-Display-Anzeige
- ✅ Cloud-Konnektivität via Blynk
- ✅ Ernergieeffizienz durch Deep-Sleep
- ✅ Intuitive Benutzeroberfläche mit visuellen Indikatoren
- ✅ Multi-Sensor-Integration
- ✅ Flexible Konfiguration pro Gerät
