/* ============================================

    Sketch for NaWiSchool
    
    ThermoLogger_OLED
    [Arduino Nano with BME280, HTU21, DHT22, DS18B20 and SH1106 OLED]
    Author: T. Schumann
    Date: 2024-11-13

    Dependencies:
    SSD1306Ascii - https://github.com/greiman/SSD1306Ascii
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
#include <SSD1306AsciiWire.h>
#include <DallasTemperature.h>

// Enable logging to serial monitor for debugging
#define ENLOG

// Define pins and constants
const uint16_t definedDelay = 2000;  // Delay between sensor readings in milliseconds
#define cardSelect 5                 // Chip select pin for SD card module
#define DHTPIN 3                     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22                // DHT 22 (AM2302) sensor type
#define DSPIN 4                      // Digital pin connected to DS18B20 sensor
#define LED LED_BUILTIN              // LED pin for error indication

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
SSD1306AsciiWire oled;                      // OLED display object

uint8_t col0 = 50;   // First value column
uint8_t col1 = 100;  // Last value column.
uint8_t rows = 2;   // Rows per line.

// Define labels for display
const char* label[] = { " BME:", " HTU:", " DTH:", " DS:" };
const char* units[] = { "C", "C", "C", "C" };

void setup() {
  // Initialize I2C communication
  Wire.begin();
  Wire.setClock(400000L);

  // Initialize serial communication if logging enabled
#ifdef ENLOG
  Serial.begin(9600);
  Serial.println("\r\nThermoLogger");
  Serial.println("By Tim Schumann");
  Serial.println();
#endif

  // Set LED pin as output
  pinMode(LED, OUTPUT);


  oled.begin(&SH1106_128x64, 0x3C);
  oled.setFont(Callibri15);
  oled.setLetterSpacing(3);
  oled.clear();


  // Initialize sensors and handle errors if they fail to initialize
  if (!bme.begin(0x76, &Wire)) error(5);
  if (!htu.begin()) error(6);
  dht.begin();
  ds.begin();

  // Initialize SD card
  if (!SD.begin(cardSelect)) error(2);

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
  logfile.close();
#ifdef ENLOG
  Serial.println(filename);
  Serial.println("#, CPU time in ms, bme_temp, bme_press, bme_hum, htu_temp, htu_hum, dht_temp, dht_hum, ds18_temp");
#endif


  // Setup form and find longest label.
  for (uint8_t i = 0; i < 4; i++) {
    oled.println(label[i]);
  }
  for (uint8_t i = 0; i < 4; i++) {
    oled.setCursor(col1 + 1, i * rows);
    oled.print(units[i]);
  }
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
  unsigned long timestamp = millis();
  digitalWrite(LED, HIGH);

  float reading;

  // Request temperature from DS18B20 sensor once
  ds.requestTemperatures();

  // Open the log file for appending data
  logfile = SD.open(filename, FILE_WRITE);
  if (logfile) {
    logfile.print(++count);
    logfile.print(", ");
    logfile.print(timestamp);
    logfile.print(", ");

    // Read BME280 temperature once and write to all outputs
    reading = bme.readTemperature();
    logfile.print(reading);
    logfile.print(", ");
    oled.clear(col0, col1, 0, rows - 1);  // Clear and update OLED display line
    oled.print(reading, 2);
#ifdef ENLOG
    Serial.print(count);
    Serial.print(", ");
    Serial.print(timestamp);
    Serial.print(", ");
    Serial.print(reading);
    Serial.print(", ");
#endif

    // Read BME280 pressure once and write to all outputs
    reading = bme.readPressure() / 100.0F;
    logfile.print(reading);
    logfile.print(", ");
#ifdef ENLOG
    Serial.print(reading);
    Serial.print(", ");
#endif

    // Read BME280 humidity once and write to all outputs
    reading = bme.readHumidity();
    logfile.print(reading);
    logfile.print(", ");
#ifdef ENLOG
    Serial.print(reading);
    Serial.print(", ");
#endif

    // Read HTU21 temperature once and write to all outputs
    reading = htu.readTemperature();
    logfile.print(reading);
    logfile.print(", ");
    oled.clear(col0, col1, rows, 2 * rows - 1);
    oled.print(reading, 2);
#ifdef ENLOG
    Serial.print(reading);
    Serial.print(", ");
#endif

    // Read HTU21 humidity once and write to all outputs
    reading = htu.readHumidity();
    logfile.print(reading);
    logfile.print(", ");
#ifdef ENLOG
    Serial.print(reading);
    Serial.print(", ");
#endif

    // Read DHT22 temperature once and write to all outputs
    reading = dht.readTemperature();
    logfile.print(reading);
    logfile.print(", ");
    oled.clear(col0, col1, 2 * rows, 3 * rows - 1);
    oled.print(reading, 2);
#ifdef ENLOG
    Serial.print(reading);
    Serial.print(", ");
#endif

    // Read DHT22 humidity once and write to all outputs
    reading = dht.readHumidity();
    logfile.print(reading);
    logfile.print(", ");
#ifdef ENLOG
    Serial.print(reading);
    Serial.print(", ");
#endif

    // Read DS18B20 temperature once and write to all outputs
    reading = ds.getTempCByIndex(0);
    logfile.println(reading);
    oled.clear(col0, col1, 3 * rows, 3 * rows - 1);
    oled.print(reading, 2);
#ifdef ENLOG
    Serial.println(reading);
#endif

    logfile.close();
  }

  digitalWrite(LED, LOW);
}


// Function to clear a value on the OLED display
void clearValue(uint8_t row) {
  oled.clear(col0, col1, row, row + rows - 1);
}

// Function to handle errors and provide error indication through LED blinks
void error(uint8_t errno) {
  const char* errorMsg;
  switch (errno) {
    case 5: errorMsg = "BME-Error"; break;
    case 6: errorMsg = "HTU-Error"; break;
    case 7: errorMsg = "DHT-Error"; break;
    case 2:
    case 3: errorMsg = "SD-Error"; break;
    case 10: errorMsg = "GPS-Error"; break;
  }
#ifdef ENLOG
  Serial.println(errorMsg);
  Serial.println("   !!!!");
#endif

  while (1) {
    for (uint8_t i = 0; i < errno; i++) {
      digitalWrite(LED, HIGH);
      delay(100);
      digitalWrite(LED, LOW);
      delay(100);
    }
    delay(1000 - (errno * 200));
  }
}




//end of file
