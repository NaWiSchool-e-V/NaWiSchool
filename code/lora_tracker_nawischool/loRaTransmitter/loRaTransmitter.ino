/* ============================================

    Sketch for NaWiSchool
    
    LoRa Transmitter with BME280, SCD41 and NEO-6M
    [This code transmits data from BME280 sensor, SCD41 sensor, and Neo-6M GPS module via LoRa.]
    Author: T. Schumann
    Date: 2024-02-13

    Dependencies:
    LoRa - https://github.com/sandeepmistry/arduino-LoRa 
    TinyGPS++ - https://github.com/mikalhart/TinyGPSPlus.git
    Adafruit_Sensor - https://github.com/adafruit/Adafruit_Sensor
    Adafruit_BME280_Library - https://github.com/adafruit/Adafruit_BME280_Library
    Sensirion_I2C_SCD4x - https://github.com/Sensirion/arduino-i2c-scd4x

    All rights reserved. Copyright Tim Schumann 2024

  ===============================================
*/


// Include necessary libraries
#include <SPI.h>
#include <Wire.h>
#include <LoRa.h>
#include <TinyGPS++.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <SensirionI2CScd4x.h>

//#define ENLOG  // Enable serial communication for debugging (optional)

// Define pins and constants
const int led = LED_BUILTIN;
const int definedDelay = 20000;            // Delay between sensor readings and transmissions (ms)
const int LORA_FREQUENCY = 868E6;          // LoRa frequency (MHz)
const uint32_t GPS_BAUD_RATE = 9600;       // GPS serial communication baud rate
const unsigned long GPS_WAIT_TIME = 5000;  // Maximum time to wait for GPS data (ms)

// Flags and variables
bool firstlock = false;            // Indicates if GPS lock is achieved
unsigned long previousMillis = 0;  // Time of last sensor reading
unsigned long count = 0;           // Packet counter
double START_LAT = 0;              // Starting latitude (set after first GPS lock)
double START_LON = 0;              // Starting longitude (set after first GPS lock)
double distanceToStart = 0;        // Distance from starting location
uint16_t co2 = 0;                  // CO2 reading from SCD41 sensor
float temperature_SCD = 0.0f;      // Temperature reading from SCD41 sensor
float humidity_SCD = 0.0f;         // Humidity reading from SCD41 sensor
bool isDataReady = false;          // Flag indicating if SCD41 data is ready

// Create objects for different sensors
TinyGPSPlus gps;        // GPS object
Adafruit_BME280 bme;    // BME280 sensor object
SensirionI2CScd4x scd;  // SCD41 sensor object

// UBX protocol byte arrays for communication with GPS
uint8_t ClearConfig[] = { 0xB5, 0x62, 0x06, 0x09, 0x0D, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x01, 0x19, 0x98 };
uint8_t SetPSM[] = { 0xB5, 0x62, 0x06, 0x11, 0x02, 0x00, 0x08, 0x01, 0x22, 0x92 };
uint8_t SetPlatform[] = { 0xB5, 0x62, 0x06, 0x24, 0x24, 0x00, 0xFF, 0xFF, 0x07, 0x03, 0x00, 0x00, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x05, 0x00, 0xFA, 0x00, 0xFA, 0x00, 0x64, 0x00, 0x2C, 0x01, 0x00, 0x00, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4E, 0xFD };
uint8_t SaveConfig[] = { 0xB5, 0x62, 0x06, 0x09, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x1D, 0xAB };


void setup() {
  // Initialize hardware and peripherals
  Wire.begin();
#ifdef ENLOG
  Serial.begin(115200);
  while (!Serial)
    delay(100);
  Serial.println("\r\nLoRa Transmitter V1");
  Serial.println("By Tim Schumann");
  Serial.println();
#endif

  pinMode(led, OUTPUT);

  // Start communication with GPS and OLED
  Serial1.begin(GPS_BAUD_RATE);

  if (!bme.begin(0x76, &Wire))
    error(5);  // BME280 sensor initialization error

  scd.begin(Wire);
  scd.stopPeriodicMeasurement();
  scd.startPeriodicMeasurement();

#ifdef ENLOG
  Serial.println("#, CPU time in ms, temp1, press1, hum1, temp2, hum2, co2, lat, lng, alt, speed, course, distanceToStart, satellites");
#endif

  if (!LoRa.begin(LORA_FREQUENCY)) {
    error(8);  // LoRa initialization error
  }

  // Configure UBX settings for GPS
  delay(200);
  sendUBX(ClearConfig, sizeof(ClearConfig) / sizeof(uint8_t));
  delay(1000);
  sendUBX(SetPlatform, sizeof(SetPlatform) / sizeof(uint8_t));
  sendUBX(SaveConfig, sizeof(SaveConfig) / sizeof(uint8_t));
  delay(200);
}

void loop() {

  if (gps.charsProcessed() < 50 && millis() > GPS_WAIT_TIME) {
    error(10);
  }

  // Update GPS data from serial input
  updateGPSData();

  // Update sensor data and write to SD
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= definedDelay) {
    previousMillis = currentMillis;
    sendDataToLoRa();
  }
}

// Function to update GPS data from serial input
void updateGPSData() {
  while (Serial1.available() > 0)
    gps.encode(Serial1.read());

  if (!firstlock && gps.satellites.value() > 4) {
    sendUBX(SetPSM, sizeof(SetPSM) / sizeof(uint8_t));
    firstlock = true;
    delay(1000);
    START_LAT = gps.location.lat();
    START_LON = gps.location.lng();
  }
}


// Function to write data to SD card
void sendDataToLoRa() {

  digitalWrite(led, HIGH);

  scd.getDataReadyFlag(isDataReady);
  if (isDataReady)
    scd.readMeasurement(co2, temperature_SCD, humidity_SCD);

  LoRa.beginPacket();
  LoRa.print(++count);
  LoRa.print(", ");
  LoRa.print(millis());
  LoRa.print(", ");
  LoRa.print(bme.readTemperature());
  LoRa.print(", ");
  LoRa.print(bme.readPressure() / 100.0F);
  LoRa.print(", ");
  LoRa.print(bme.readHumidity());
  LoRa.print(", ");
  LoRa.print(temperature_SCD);
  LoRa.print(", ");
  LoRa.print(humidity_SCD);
  LoRa.print(", ");
  LoRa.print(co2);
  LoRa.print(", ");
  LoRa.print(String(gps.location.lat(), 6));
  LoRa.print(", ");
  LoRa.print(String(gps.location.lng(), 6));
  LoRa.print(", ");
  LoRa.print(gps.altitude.meters());
  LoRa.print(", ");
  LoRa.print(gps.speed.kmph());
  LoRa.print(", ");
  LoRa.print(gps.course.deg());
  LoRa.print(", ");
  if (gps.location.isValid() && START_LAT != 0) {
    distanceToStart =
      TinyGPSPlus::distanceBetween(
        gps.location.lat(),
        gps.location.lng(),
        START_LAT,
        START_LON);
    LoRa.print(distanceToStart, 6);
    LoRa.print(", ");
  } else {
    LoRa.print("0");
    LoRa.print(", ");
  }
  LoRa.print(gps.satellites.value());
  LoRa.endPacket(true);


#ifdef ENLOG

  Serial.print(++count);
  Serial.print(", ");
  Serial.print(millis());
  Serial.print(", ");
  Serial.print(bme.readTemperature());
  Serial.print(", ");
  Serial.print(bme.readPressure() / 100.0F);
  Serial.print(", ");
  Serial.print(bme.readHumidity());
  Serial.print(", ");
  Serial.print(temperature_SCD);
  Serial.print(", ");
  Serial.print(humidity_SCD);
  Serial.print(", ");
  Serial.print(co2);
  Serial.print(", ");
  Serial.print(String(gps.location.lat(), 6));
  Serial.print(", ");
  Serial.print(String(gps.location.lng(), 6));
  Serial.print(", ");
  Serial.print(gps.altitude.meters());
  Serial.print(", ");
  Serial.print(gps.speed.kmph());
  Serial.print(", ");
  Serial.print(gps.course.deg());
  Serial.print(", ");
  Serial.print(distanceToStart, 6);
  Serial.print(", ");
  Serial.println(gps.satellites.value());

#endif

  digitalWrite(led, LOW);
}

// Function to send UBX protocol data to GPS
void sendUBX(uint8_t* MSG, uint8_t len) {
  for (int i = 0; i < len; i++) {
    Serial1.write(MSG[i]);
  }
}

// Function to handle errors
void error(uint8_t errno) {
  switch (errno) {
    case 5:
      Serial.println("BME280 Sensor Initialization Failed");
      break;
    case 8:
      Serial.println("LoRa Module Initialization Failed");
      break;
    case 10:
      Serial.println("GPS Data Reception Timeout");
      break;
    default:
      Serial.println("Unknown Error");
      break;
  }
  while (1) {
    uint8_t i;
    for (i = 0; i < errno; i++) {
      digitalWrite(led, HIGH);
      delay(100);
      digitalWrite(led, LOW);
      delay(100);
    }
    for (i = errno; i < 10; i++) {
      delay(200);
    }
  }
}




//end
