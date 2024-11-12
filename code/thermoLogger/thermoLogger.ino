/* ============================================

    Sketch for NaWiSchool
    
    ThermoLogger
    [Arduino Nano with BME280, HTU21, DHT22 and DS18B20]
    Author: T. Schumann
    Date: 2024-11-12

    Dependencies:
    OneWire library - https://github.com/PaulStoffregen/OneWire
    Adafruit_Sensor - https://github.com/adafruit/Adafruit_Sensor
    DHT sensor library - https://github.com/adafruit/DHT-sensor-library
    Adafruit_BME280_Library - https://github.com/adafruit/Adafruit_BME280_Library
    Adafruit HTU21D-F Library - https://github.com/adafruit/Adafruit_HTU21DF_Library
    DallasTemperature library - https://github.com/milesburton/Arduino-Temperature-Control-Library

    All rights reserved. Copyright Tim Schumann 2024

  ===============================================
*/

// Include necessary libraries
#include <SD.h>
#include <Wire.h>
#include <DHT.h>
#include <OneWire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_HTU21DF.h>
#include <DallasTemperature.h>

// Enable logging to serial monitor for debugging
#define ENLOG

// Define pins and constants
const int definedDelay = 1000;  // Delay between sensor readings in milliseconds
#define cardSelect 5            // Chip select pin for SD card module
#define DHTPIN 3                // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22           // DHT 22 (AM2302) sensor type
#define DSPIN 4                 // Digital pin connected to DS18B20 sensor
#define LED LED_BUILTIN         // LED pin for error indication

// Variables for timing and logging
unsigned long previousMillis = 0;  // Tracks time since last sensor reading
unsigned long count = 0;           // Counter for readings

File logfile;       // File object for logging data
char filename[15];  // Filename for the log file

// Create sensor objects
Adafruit_BME280 bme;                        // BME280 sensor object for temperature, pressure, and humidity
Adafruit_HTU21DF htu = Adafruit_HTU21DF();  // HTU21 sensor object for temperature and humidity
DHT dht(DHTPIN, DHTTYPE);                   // DHT22 sensor object for temperature and humidity
OneWire oneWire(DSPIN);                     // OneWire object for communication with DS18B20
DallasTemperature ds(&oneWire);             // DS18B20 sensor object

void setup() {
  // Initialize I2C communication
  Wire.begin();

  // Initialize serial communication if logging enabled
#ifdef ENLOG
  Serial.begin(9600);
  Serial.println("\r\nThermoLogger");
  Serial.println("By Tim Schumann");
  Serial.println();
#endif

  // Set LED pin as output
  pinMode(LED, OUTPUT);

  // Initialize sensors and handle errors if they fail to initialize
  if (!bme.begin(0x76, &Wire)) {
    error(5);  // BME280 initialization error
  }
  if (!htu.begin()) {
    error(6);  // HTU21 initialization error
  }
  dht.begin();
  ds.begin();

  // Initialize SD card
  if (!SD.begin(cardSelect)) {
#ifdef ENLOG
    Serial.println("Card init. failed!");
#endif
    error(2);  // SD card initialization error
  }

  // Create a unique log file name
  strcpy(filename, "/THERMO00.CSV");
  for (uint8_t i = 0; i < 100; i++) {
    filename[7] = '0' + i / 10;
    filename[8] = '0' + i % 10;
    if (!SD.exists(filename)) {
      break;  // Found an available filename
    }
  }

  // Open the log file and write the header
  logfile = SD.open(filename, FILE_WRITE);
  logfile.println("Name: ThermoLogger by Tim Schumann");
  logfile.println("#, CPU time in ms, bme_temp, bme_press, bme_hum, htu_temp, htu_hum, dht_temp, dht_hum, ds18_temp");
  if (!logfile) {
#ifdef ENLOG
    Serial.print("Couldn't create ");
    Serial.println(filename);
#endif
    error(3);  // Log file creation error
  }
#ifdef ENLOG
  Serial.print("Writing to: ");
  Serial.println(filename);
  Serial.println("Name: ThermoLogger by Tim Schumann");
  Serial.println("#, CPU time in ms, bme_temp, bme_press, bme_hum, htu_temp, htu_hum, dht_temp, dht_hum, ds18_temp");
#endif
  logfile.close();
}


void loop() {
  // Check if the defined delay has passed, then write sensor data to the SD card
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= definedDelay) {
    previousMillis = currentMillis;
    sdWrite();
  }
}

// Function to write sensor data to the SD card
void sdWrite() {
  // Request temperature from DS18B20 sensor
  ds.requestTemperatures();

  // Turn on LED while writing data
  digitalWrite(LED, HIGH);

  // Open the log file for appending data
  logfile = SD.open(filename, FILE_WRITE);
  if (logfile) {
    logfile.print(++count);
    logfile.print(", ");
    logfile.print(millis());
    logfile.print(", ");
    logfile.print(bme.readTemperature());
    logfile.print(", ");
    logfile.print(bme.readPressure() / 100.0F);  // Convert pressure to hPa
    logfile.print(", ");
    logfile.print(bme.readHumidity());
    logfile.print(", ");
    logfile.print(htu.readTemperature());
    logfile.print(", ");
    logfile.print(htu.readHumidity());
    logfile.print(", ");
    logfile.print(dht.readTemperature());
    logfile.print(", ");
    logfile.print(dht.readHumidity());
    logfile.print(", ");
    logfile.println(ds.getTempCByIndex(0));
    logfile.close();
  }

  // Log data to serial monitor if logging enabled
#ifdef ENLOG
  Serial.print(count);
  Serial.print(", ");
  Serial.print(millis());
  Serial.print(", ");
  Serial.print(bme.readTemperature());
  Serial.print(", ");
  Serial.print(bme.readPressure() / 100.0F);
  Serial.print(", ");
  Serial.print(bme.readHumidity());
  Serial.print(", ");
  Serial.print(htu.readTemperature());
  Serial.print(", ");
  Serial.print(htu.readHumidity());
  Serial.print(", ");
  Serial.print(dht.readTemperature());
  Serial.print(", ");
  Serial.print(dht.readHumidity());
  Serial.print(", ");
  Serial.println(ds.getTempCByIndex(0));
#endif

  // Turn off LED after writing data
  digitalWrite(LED, LOW);
}


// Function to handle errors and provide error indication through LED blinks
void error(uint8_t errno) {
  switch (errno) {
    case 5:
#ifdef ENLOG
      Serial.print("BME-Error");
#endif
      break;
    case 6:
#ifdef ENLOG
      Serial.print("HTU-Error");
#endif
      break;
    case 7:
#ifdef ENLOG
      Serial.print("DHT-Error");
#endif
      break;
    case 2:
    case 3:
#ifdef ENLOG
      Serial.println("SD-Error");
#endif
      break;
    case 10:
#ifdef ENLOG
      Serial.println("GPS-Error");
#endif
      break;
  }
#ifdef ENLOG
  Serial.println("   !!!!");
#endif

  // Blink LED to indicate error code
  while (1) {
    uint8_t i;
    for (i = 0; i < errno; i++) {
      digitalWrite(LED, HIGH);
      delay(100);
      digitalWrite(LED, LOW);
      delay(100);
    }
    for (i = errno; i < 10; i++) {
      delay(200);
    }
  }
}




//end of file
