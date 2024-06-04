// Bibliotheken
#include <Wire.h> // Für I2C-Kommunikation (Sensoren)
#include <Adafruit_Sensor.h> // Für Adafruit Sensoren
#include <Adafruit_BME280.h> // Speziel für BME280
#include <MPU6050.h> // Speziel für MPU6050
#include <SPI.h> // Ermöglicht Kommunikation des SPI-Bus
#include <SD.h> // Für Interaktion mit SD_Karten

// Deklarationen
// I2C-Adresse vom BME280
#define BME280_ADRESSE 0x76

#define SD_CS_PIN D4 // Digitaler Pin für SD-Karte
#define START_BUTTON_PIN D3 // Digitaler Pin für Startschalter

// Dateiname auf der microSD-Karte
#define FILE_NAME "sensor_data.txt"

// BME280-Sensor instanziieren
Adafruit_BME280 bme;

// MPU6050-Sensor instanziieren
MPU6050 mpu;

// Zeitvariablen
unsigned long Zeitstempel = 0;

// Anzahl der Messungen zur Mittelwertbildung
const int numReadings = 10;

// Arrays zur Speicherung von dem Messungen
float pressureReadings[numReadings]; 
int16_t axReadings[numReadings], ayReadings[numReadings], azReadings[numReadings];

// Index für die aktuelle Messung
int readIndex = 0;

// Summen der Messungen
float totalPressure = 0.0;
int32_t totalAx = 0, totalAy = 0, totalAz = 0;

// Durchschnittswerte
float averagePressure = 0.0;
int16_t averageAx = 0, averageAy = 0, averageAz = 0;

// SD-Karten-Datei
File datenDatei;

// Funktion zur Umrechnung der Zeit in hh:mm:ss:ms Format
String formatTime(unsigned long ms) {
  unsigned long seconds = ms / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;

  seconds = seconds % 60;
  minutes = minutes % 60;
  ms = ms % 1000;

  char timeString[13]; // hh:mm:ss:ms hat maximal 12 Zeichen plus Nullterminierung
  snprintf(timeString, sizeof(timeString), "%02lu:%02lu:%02lu:%03lu", hours, minutes, seconds, ms);

  return String(timeString);
}

void setup() {
  // Serielle Kommunikation starten
  Serial.begin(115200);

  pinMode(SD_CS_PIN, OUTPUT); // Setze den Chip-Selekt-Pin als Ausgang
  pinMode(START_BUTTON_PIN, INPUT_PULLUP); // Setze den Schalter-Pin als Eingang mit Pull-Up

  // Statusmeldung für die Initialisierung der Sensoren ausgeben
  Serial.println("Initialisiere Sensoren...");

  // I2C-Kommunikation initialisieren
  Wire.begin(D2, D1); // D2 = SDA, D1 = SCL

  // BME280-Sensor initialisieren
  if (!bme.begin(BME280_ADRESSE)) {
    Serial.println("Konnte keinen gültigen BME280-Sensor finden, überprüfe die Verkabelung!");
    while (1);
  }
  Serial.println("BME280-Sensor erfolgreich initialisiert");

  // MPU6050-Sensor initialisieren
  mpu.initialize();
  if (!mpu.testConnection()) {
    Serial.println("MPU6050-Verbindung fehlgeschlagen");
    while (1);
  }
  Serial.println("MPU6050-Sensor erfolgreich initialisiert");

  // microSD-Karte initialisieren
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("Fehler beim Initialisieren der microSD-Karte!");
    return;
  }
  Serial.println("microSD-Karte erfolgreich initialisiert");

  // Datei für die Ausgabe öffnen (oder erstellen)
  datenDatei = SD.open(FILE_NAME, FILE_WRITE);
  if (!datenDatei) {
    Serial.println("Fehler beim Öffnen der Datei!");
    return;
  }
  Serial.println("Datei erfolgreich geöffnet");

  Serial.println("Sensoren und microSD-Karte erfolgreich initialisiert");
}

void loop() {
  static bool isButtonPressed = false;
  static unsigned long lastDebounceTime = 0;
  const unsigned long debounceDelay = 50;

  int buttonState = digitalRead(START_BUTTON_PIN);

  // Status des Schalters überprüfen
  if (buttonState == LOW && !isButtonPressed) { // LOW bedeutet gedrückt
    if ((millis() - lastDebounceTime) > debounceDelay) {
      isButtonPressed = true;
      Serial.println("Schalter ist an, führe den Hauptcode aus.");
    }
  } else if (buttonState == HIGH) { // HIGH bedeutet nicht gedrückt
    isButtonPressed = false;
    lastDebounceTime = millis();
  }

  if (isButtonPressed) {
    ausfuehrenHauptcode();
  } else {
    Serial.println("Warte auf Schalter...");
    delay(1000); // Kurze Verzögerung, um ein Prellen des Schalters zu vermeiden
  }
}

void ausfuehrenHauptcode() {
  // Alte Werte von der Summe subtrahieren
  totalPressure -= pressureReadings[readIndex];
  totalAx -= axReadings[readIndex];
  totalAy -= ayReadings[readIndex];
  totalAz -= azReadings[readIndex];

  // Neue Werte lesen
  pressureReadings[readIndex] = bme.readPressure() / 100.0F; // Pa in hPa umwandeln
  mpu.getAcceleration(&axReadings[readIndex], &ayReadings[readIndex], &azReadings[readIndex]);

  // Neue Werte zur Summe hinzufügen
  totalPressure += pressureReadings[readIndex];
  totalAx += axReadings[readIndex];
  totalAy += ayReadings[readIndex];
  totalAz += azReadings[readIndex];

  // Index erhöhen
  readIndex = (readIndex + 1) % numReadings;

  // Durchschnittswerte berechnen
  averagePressure = totalPressure / numReadings;
  averageAx = totalAx / numReadings;
  averageAy = totalAy / numReadings;
  averageAz = totalAz / numReadings;

  // Rohwerte in g umrechnen
  float axG = averageAx / 16384.0;
  float ayG = averageAy / 16384.0;
  float azG = averageAz / 16384.0;

  // Zeitstempel speichern
  Zeitstempel = millis();

  // Durchschnittswerte und Zeitstempel im Format hh:mm:ss:ms ausgeben
  Serial.print(formatTime(Zeitstempel));
  Serial.print("; ");
  Serial.print(averagePressure);
  Serial.print("; ");
  Serial.print(axG, 4); // 4 Dezimalstellen
  Serial.print("; ");
  Serial.print(ayG, 4);
  Serial.print("; ");
  Serial.println(azG, 4);

  // Daten in die Datei auf der microSD-Karte schreiben
  datenDatei.print(formatTime(Zeitstempel));
  datenDatei.print("; ");
  datenDatei.print(averagePressure);
  datenDatei.print("; ");
  datenDatei.print(axG, 4);
  datenDatei.print("; ");
  datenDatei.print(ayG, 4);
  datenDatei.print("; ");
  datenDatei.println(azG, 4);

  // Datei flushen
  datenDatei.flush();

  // Eine Verzögerung hinzufügen, um die serielle Ausgabe nicht zu überfluten
  delay(80);
}