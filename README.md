# Fitnesstracker auf Basis des ESP32-C3

Dieses Projekt realisiert einen Fitness-Tracker, der verschiedene Sensordaten erfasst und lokal auf einer SD-Karte speichert. Die gesammelten Werte werden zudem auf einem OLED-Display dargestellt.

## Projektziel
Entwicklung eines Fitness-Trackers auf Basis des ESP32-C3, der Bewegungsdaten, Puls und SpO2, auf einer SD-Karte im „.csv“ Format speichert und auf dem OLEDDisplay visualisiert.

## Hardware
| Komponente                | Funktion                                      |
|---------------------------|-----------------------------------------------|
| **ESP32-C3 Devboard**     | Zentrale Steuerung                            |
| **MAX30102** (extern)     | Puls- und SpO2-Messung                        |
| **ICM-42688-P**           | Beschleunigungssensor (auf dem Devboard)      |
| **SD-Karte**              | Lokale Speicherung aller Daten (CSV)          |
| **OLED-Display**          | Anzeige von Puls, SpO2 und weiteren Werten    |

## Funktionen
- Schrittzähler
- Pulsmessung
- SpO2-Messung
- Minütliche Protokollierung der gelaufenen Schritte auf SD-Karte im `.csv`-Format
- Darstellung der wichtigsten Messwerte auf dem OLED-Display

## Mögliche Erweiterungen (optional)
- Kommunikation mit einem Smartphone per Bluetooth oder WLAN zur Anzeige der Daten in einer App
- Bereitstellung eines Webservers auf dem ESP32, um die Daten direkt im Browser zu visualisieren
- GPS-basierte Geschwindigkeit und Streckenaufzeichnung

## Entwicklung und Build
Zum Kompilieren und Flashen wird die ESP-IDF verwendet. Die üblichen Schritte sind:

```bash
idf.py build
idf.py -p PORT flash monitor
```
