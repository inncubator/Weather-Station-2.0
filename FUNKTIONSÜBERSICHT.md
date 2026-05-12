# Weather-Station 2.0 - Funktionsübersicht

## 1. Projektübersicht

Die **Weather-Station 2.0** ist ein IoT-Wetterstation-Gerät, das auf einem ESP32-S3 basiert und in Echtzeit Wetterdaten erfasst, lokal auf einem E-Paper-Display anzeigt und diese Daten über das Blynk-IoT-Framework in die Cloud sendet.

### Hauptmerkmale:
- 📊 Erfassung von Temperatur, Luftfeuchtigkeit und Luftdruck
- 📡 Drahtlose Verbindung über WiFi und Blynk IoT-Cloud
- 🖥️ E-Paper-Display (2.66 Zoll) mit niedriger Stromaufnahme
- 🔋 Batteriebetrieb mit Deep-Sleep-Modus
- ⏰ NTP-Zeit-Synchronisation
- 🎯 Visueller Feuchtigkeitsindikator (Smiley-Status)
- ⚙️ Konfigurierbare Messintervalle

---

## 2. Hardwarekomponenten

### 2.1 Mikrocontroller
- **Board**: ESP32-S3 WROOM-1
- **Framework**: Arduino (PlatformIO)
- **Betriebsspannung**: 3.3V (USB oder Batterie)
- **Speicher**: 
  - RAM: 8 MB
  - Flash: für Firmware und Konfiguration

### 2.2 Sensoren

| Sensor | Modell | Funktion | Schnittstelle |
|--------|--------|----------|---------------|
| Temperatur & Luftfeuchtigkeit | Adafruit SHTC3 | Misst Temperatur und Luftfeuchtigkeit | I2C |
| Luftdruck | Adafruit MPL3115A2 | Misst barometrischen Luftdruck | I2C |
| Batterie-Überwachung | Adafruit MAX17048 | Überwacht Batterieladung | I2C |

### 2.3 Display
- **Typ**: E-Paper (ePaper) Display
- **Größe**: 2.66 Zoll (67,4 mm)
- **Auflösung**: 296 x 152 Pixel (1-Bit Schwarzweiß)
- **Verbindung**: SPI
- **Stromaufnahme**: Minimal im Ruhezustand
- **Technologie**: Waveshare EPD-2IN66

### 2.4 Eingabeschnittstellen
- **Wake-Button**: GPIO-15 (mit Pull-up)
  - Normaler Druck: Gerät aufwecken
  - Längerdruck (2 Sekunden): Statusseite anzeigen
- **WiFi-Reset**: GPIO-1
  - Zum Zurücksetzen der WiFi-Konfiguration

### 2.5 Kommunikation
- **WiFi**: Integriertes ESP32-WLAN (2.4 GHz)
- **I2C**: GPIO-47 (SDA) und GPIO-48 (SCL) für Sensoren

---

## 3. Software-Funktionen

### 3.1 Sensorauswertung

#### Temperature & Humidity (SHTC3)
```cpp
readData(&temp, &hum)
```
- Liest Temperatur- und Luftfeuchtigkeitswerte aus
- Validiert die Daten auf NaN-Werte
- Gibt Werte in °C und % Luftfeuchtigkeit zurück

#### Luftdruck (MPL3115A2)
```cpp
readPressure(&pressure)
```
- Liest barometrischen Druck in hPa aus
- Validiert die Druckwerte

#### Batterie-Status
```cpp
refreshBattery(&image)
```
- Liest aktuelle Batterieladung aus (0-100%)
- Zeigt 5-Stufen-Balkendiagramm auf Display
  - 0-10%: 0 Balken
  - 10-30%: 1 Balken
  - 30-50%: 2 Balken
  - 50-70%: 3 Balken
  - 70-90%: 4 Balken
  - 90-100%: 5 Balken

### 3.2 Display-Management

#### E-Paper Display Rendering
- **Hintergrund**: InnCubator-Logo als Basis-Hintergrundbild
- **Partielle Aktualisierung**: Nur bestimmte Display-Bereiche neu zeichnen (spart Energie)
- **Auflösung**: 296 x 152 Pixel
- **Farben**: Schwarz-Weiß

#### Display-Elemente

| Element | Funktion | Position |
|---------|----------|----------|
| Temperatur | Anzeige in °C mit 1 Dezimalstelle | Oben rechts |
| Luftfeuchtigkeit | Anzeige in % mit 1 Dezimalstelle | Mitte rechts |
| Luftdruck | Anzeige in hPa mit 1 Dezimalstelle | Unten rechts |
| Feuchtigkeitsindikator (Smiley) | Visuelles Feedback je nach Feuchtigkeitswert | Unten links |
| Aktualisierungszeit | Zeitstempel der letzten Messung (HH:MM Format) | Unten |
| Batteriestandanzeige | 5-Balken-Diagramm der Batterieladung | Oben links |

#### Feuchtigkeitsindikatoren
```cpp
refreshIndicator(&image, humidity)
```
- **30-40% (Niedrig)**: Trockenes Smiley-Symbol
- **40-60% (Ideal)**: Glückliches Smiley-Symbol
- **60-70% (Hoch)**: Warnendes Smiley-Symbol
- **>70% (Sehr Hoch)**: Besorgtes Smiley-Symbol
- **<30% (Sehr Niedrig)**: Trauriges Smiley-Symbol

### 3.3 WiFi & Blynk-Integration

#### WiFi-Verbindung
- Verwendet **BlynkEdgent** für automatische WiFi-Verwaltung
- Versucht sich für max. 120 Sekunden zu verbinden
- Fallback: Konfigurationsmodus bei fehlender Konfiguration

#### Blynk IoT-Cloud
- **Template ID**: TMPL4KNN1-VFn (konfigurierbar pro Gerät)
- **Template Name**: WS_1_MW (pro Umgebung anpassbar)
- **Virtuelle Pins**:
  - **V5**: Temperaturwert (°C)
  - **V6**: Luftfeuchtigkeitswert (%)

#### Datenübertragung
```cpp
sendData(&temp, &hum)
```
- Sendet Temperatur auf V5
- Sendet Luftfeuchtigkeit auf V6
- Erfolgt nur bei erfolgreicher Blynk-Verbindung

### 3.4 Zeit-Synchronisation

#### NTP-Zeit
- Synchronisiert mit **pool.ntp.org**
- Zeitzone: **CET-1CEST,M3.5.0/2,M10.5.0/3** (Mitteleuropäische Zeit)
- Wird alle 5 Sekunden versucht (mit Timeout)

#### Zeit-Anzeige
```cpp
refreshTime(&image)
```
- Zeigt Messzeitstempel im Format "HH:MM" auf Display
- Wird nach erfolgreicher Zeit-Synchronisation angezeigt

### 3.5 Statusseite

#### Aktivierung
- Längerdruck auf Wake-Button (>2 Sekunden)
- Zeigt Geräteinformation und Verbindungsstatus

#### Angezeigter Status
- **WLAN**: Connected/Not Connected + SSID
- **Blynk**: Connected/Not Connected
- **Zeit**: Aktuelles Datum und Uhrzeit (Format: DD.MM.YYYY HH:MM)
- **Device-Info**: Template Name und Template ID

---

## 4. Stromverwaltung

### 4.1 Deep-Sleep-Modus
- Nach jeder Messung: ESP32 wird in den Tiefschlaf versetzt
- Stromaufnahme: Minimal (<100 µA im Sleep)
- Wake-Quellen:
  - **Timer-basiert**: Konfigurierbar über EEPROM (Standard: Wert in EEPROM Adresse 0 * 60 Sekunden)
  - **Button-basiert**: GPIO-15 Low-Signal (Wake-Button)

### 4.2 EEPROM-Speicherverwaltung
- **Adresse 0**: Mess-Intervall in Minuten (beim Booten gelesen)
- Wird zur Konfiguration von Sleep-Dauer verwendet

### 4.3 Power-Down Sequenz
1. WiFi trennen
2. Blynk trennen
3. WiFi-Modus ausschalten
4. Deep-Sleep mit Timer- und Button-Wake-Quelle aktivieren
5. Gerät schläft bis zum nächsten Weck-Event

---

## 5. Starten und Betriebsablauf

### 5.1 Initialisierung (Setup)
1. Serial Debug-Konsole initialisieren (115200 Baud)
2. GPIO-Pin konfigurieren (Button, WiFi-Reset)
3. I2C-Bus initialisieren (GPIO-47 SDA, GPIO-48 SCL)
4. Sensoren initialisieren:
   - SHTC3 Temperatur/Luftfeuchtigkeit
   - MPL3115A2 Druck
   - MAX17048 Batterie-Überwachung
5. EEPROM initialisieren
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
