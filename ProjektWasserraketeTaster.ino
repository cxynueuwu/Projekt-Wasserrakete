#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <MPU6050.h>

#define START_BUTTON_PIN D3 // Pin, an dem der Starttaster angeschlossen ist
#define SD_CS_PIN D4 // Chip Select Pin für SD-Karte

MPU6050 mpu;
Adafruit_BME280 bme;

bool isStarted = false; // Variable, um den Startzustand zu speichern
bool buttonPressed = false; // Variable, um den Tastendruck zu speichern

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    // Warten, bis serielle Verbindung hergestellt ist
  }
  
  pinMode(START_BUTTON_PIN, INPUT_PULLUP); // Der Starttaster ist als Eingang mit Pull-Up-Widerstand konfiguriert

  // Initialisiere SD-Karte
  Serial.println("Initialisiere SD-Karte...");
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("Initialisierung der SD-Karte fehlgeschlagen!");
    return;
  }
  Serial.println("SD-Karte erfolgreich initialisiert.");

  // Initialisiere MPU6050
  Serial.println("Initialisiere MPU6050...");
  mpu.initialize();
  if (!mpu.testConnection()) {
    Serial.println("Fehler beim Verbindungstest des MPU6050, überprüfen Sie die Verkabelung!");
    while (1);
  }
  Serial.println("MPU6050 erfolgreich initialisiert.");

  // Initialisiere BME280
  Serial.println("Initialisiere BME280...");
  if (!bme.begin(0x76)) { // Standard I2C-Adresse für BME280
    Serial.println("Fehler beim Verbindungstest des BME280, überprüfen Sie die Verkabelung!");
    while (1);
  }
  Serial.println("BME280 erfolgreich initialisiert.");

  // Öffne eine Datei zum Schreiben
  Serial.println("Öffne Datei 'data.txt' zum Schreiben...");
  File dataFile = SD.open("data.txt", FILE_WRITE);
  if (dataFile) {
    dataFile.println("Beschleunigungs- und Druckdaten:");
    dataFile.close();
    Serial.println("Datei 'data.txt' erfolgreich geöffnet und initialisiert.");
  } else {
    Serial.println("Fehler beim Öffnen der Datei.");
  }
}

void loop() {
  // Überprüfe den Zustand des Starttasters
  if (digitalRead(START_BUTTON_PIN) == LOW) {
    if (!buttonPressed) { // Prüfe, ob der Taster vorher nicht gedrückt war
      buttonPressed = true;
      isStarted = !isStarted; // Wechsel den Zustand
      if (isStarted) {
        Serial.println("Messung gestartet...");
      } else {
        Serial.println("Messung gestoppt...");
      }
    }
  } else {
    buttonPressed = false; // Setze den Tastendruck zurück, wenn der Taster losgelassen wird
  }

  // Wenn der Taster gedrückt ist, führe den Code aus
  if (isStarted) {
    // Lese Beschleunigungsdaten
    int16_t ax, ay, az;
    mpu.getAcceleration(&ax, &ay, &az);

    // Lese Druckdaten
    float pressure = bme.readPressure() / 100.0; // Druck in hPa umwandeln

    // Öffne die Datei für das Schreiben
    File dataFile = SD.open("data.txt", FILE_WRITE);
    if (dataFile) {
      // Schreibe Beschleunigungsdaten in die Datei
      dataFile.print("Beschleunigung: ");
      dataFile.print(ax);
      dataFile.print(", ");
      dataFile.print(ay);
      dataFile.print(", ");
      dataFile.println(az);

      // Schreibe Druckdaten in die Datei
      dataFile.print("Druck: ");
      dataFile.print(pressure);
      dataFile.println(" hPa");

      dataFile.close();
    } else {
      Serial.println("Fehler beim Öffnen der Datei.");
    }

    // Warte für eine kurze Zeit, um den SD-Kartenschreibezyklus abzuschließen
    delay(100);

    // Warte für eine weitere Zeit, bevor die nächste Messung durchgeführt wird
    delay(1000);
  } else {
    // Wenn der Taster nicht gedrückt ist, tue nichts
    delay(100);
  }
}