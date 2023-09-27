/* ============================================

    Sketch for NaWiSchool
    
    Adalogger V3 with BME280 or MPU6050, CAJOE Geiger Counter and NEO-6M
    [This code is used to read and save the data from BME280 sensor or MPU6050 sensor, CAJOE Geiger Counter and Neo-6M GPS module]
    Author: T. Schumann
    Date: 2023-09-27

    Dependencies:
    TinyGPS++ - https://github.com/mikalhart/TinyGPSPlus.git
    Adafruit_Sensor - https://github.com/adafruit/Adafruit_Sensor
    Adafruit_BME280_Library - https://github.com/adafruit/Adafruit_BME280_Library
    Adafruit_MPU6050 - https://github.com/adafruit/Adafruit_MPU6050 
    SSD1306AsciiWire - https://github.com/greiman/SSD1306Ascii

    All rights reserved. Copyright Tim Schumann 2023

  ===============================================
*/

// Include necessary libraries
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <TinyGPS++.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_MPU6050.h>
#include <SSD1306AsciiWire.h>

#define ENLOG

// Define pins and constants
const int definedDelay = 1000;
const int modulus = 50;
const int cardSelect = 4;
const int PowerLED = 6;
const int InternalPowerLED = 13;
const int SDLED = 8;
const int GPSLED = 5;
const int VBATPIN = A7;
const int geiger = A5;
const uint32_t GPSBaud = 9600;

// Other constants
const int DISPLAY_TOGGLE_INTERVAL = 50;

// Flags and variables
bool blinkState = false;
bool displayState = true;
bool firstlock = false;
unsigned long previousMillis = 0;
unsigned long count = 0;
double START_LAT = 0;
double START_LON = 0;
double distanceToStart = 0;
float measuredvbat;
unsigned int counts = 0;
unsigned long lastCountMillis;
unsigned int lastCPM = 0;


File logfile;
char filename[15];

// Create objects for different sensors
TinyGPSPlus gps;        // GPS object
Adafruit_BME280 bme;    // BME280 sensor object
Adafruit_MPU6050 mpu;   // MPU6050 sensor object
SSD1306AsciiWire oled;  // OLED display object

uint8_t col0 = 0;  // First value column
uint8_t col1 = 0;  // Last value column.
uint8_t rows;      // Rows per line.

// Define labels and units for display
const char* labelBME[] = { " Temp.:", " Hum.:", " Time:", " B.|S.:" };
const char* unitsBME[] = { "C", "%", " ", " " };

const char* labelMPU[] = { " x:", " y:", " z:", " B.|S.:" };
const char* unitsMPU[] = { "m/s2", "m/s2", "m/s2", "" };

// UBX protocol byte arrays for communication with GPS
uint8_t ClearConfig[] = { 0xB5, 0x62, 0x06, 0x09, 0x0D, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x01, 0x19, 0x98 };
uint8_t SetPSM[] = { 0xB5, 0x62, 0x06, 0x11, 0x02, 0x00, 0x08, 0x01, 0x22, 0x92 };
uint8_t SetPlatform[] = { 0xB5, 0x62, 0x06, 0x24, 0x24, 0x00, 0xFF, 0xFF, 0x07, 0x03, 0x00, 0x00, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x05, 0x00, 0xFA, 0x00, 0xFA, 0x00, 0x64, 0x00, 0x2C, 0x01, 0x00, 0x00, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4E, 0xFD };
uint8_t SaveConfig[] = { 0xB5, 0x62, 0x06, 0x09, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x1D, 0xAB };

// Flag to indicate which sensor is active
enum SensorType {
  BME280,
  MPU6050
};
SensorType activeSensor;


// Setup function
void setup() {
  // Initialize hardware and peripherals
  Wire.begin();
#ifdef ENLOG
  Serial.begin(115200);

  Serial.println("\r\nAdalogger V3");
  Serial.println("By Tim Schumann");
  Serial.println();
#endif

  pinMode(InternalPowerLED, OUTPUT);
  pinMode(SDLED, OUTPUT);
  pinMode(GPSLED, OUTPUT);
  pinMode(PowerLED, OUTPUT);
  pinMode(geiger, INPUT);
  attachInterrupt(geiger, impulse, FALLING);
  digitalWrite(InternalPowerLED, HIGH);
  digitalWrite(PowerLED, HIGH);
  delay(100);
  digitalWrite(InternalPowerLED, LOW);
  delay(300);
  digitalWrite(InternalPowerLED, HIGH);
  delay(100);
  digitalWrite(InternalPowerLED, LOW);
  delay(300);
  digitalWrite(InternalPowerLED, HIGH);

  // Start communication with GPS and OLED
  Serial1.begin(GPSBaud);

  oled.begin(&Adafruit128x64, 0x3C);
  oled.setFont(Callibri15);
  oled.setLetterSpacing(3);
  oled.clear();

  // Initialize the selected sensor (BME280 or MPU6050)
  if (bme.begin(0x76, &Wire)) {
    activeSensor = BME280;
  } else if (mpu.begin()) {
    activeSensor = MPU6050;
  } else
    error(5);


  // see if the card is present and can be initialized:
  if (!SD.begin(cardSelect)) {
#ifdef ENLOG
    Serial.println("Card init. failed!");
#endif
    error(2);
  }
  strcpy(filename, "/ADALOG00.CSV");
  for (uint8_t i = 0; i < 100; i++) {
    filename[7] = '0' + i / 10;
    filename[8] = '0' + i % 10;
    // create if does not exist, do not open existing, write, sync after write
    if (!SD.exists(filename)) {
      break;
    }
  }


  if (activeSensor == BME280) {
    logfile = SD.open(filename, FILE_WRITE);
    logfile.println("AdaloggerV3 low Resolution by Tim Schumann");
    logfile.println("#, CPU time in ms, hh:mm:ss(UTC), date, temp, press, hum, geigerCounter in cpm, lat, lng, alt, speed, course, distanceToStart, satellites, batteryVoltage");
    if (!logfile) {
#ifdef ENLOG
      Serial.print("Couldnt create ");
      Serial.println(filename);
#endif
      error(3);
    }
#ifdef ENLOG
    Serial.print("Writing to: ");
    Serial.println(filename);
    Serial.println("AdaloggerV3 low Resolution by Tim Schumann");
    Serial.println("#, CPU time in ms, hh:mm:ss(UTC), date, temp, press, hum, geigerCounter in cpm, lat, lng, alt, speed, course, distanceToStart, satellites, batteryVoltage");
#endif
    logfile.close();

    // Setup form and find longest label.
    for (uint8_t i = 0; i < 4; i++) {
      oled.println(labelBME[i]);
      uint8_t w = oled.strWidth(labelBME[i]);
      col0 = col0 < w ? w : col0;
    }
    // Six pixels after label.
    col0 += 6;
    // Allow two or more pixels after value.
    col1 = col0 + oled.strWidth("99.99") + 2;
    // Line height in rows.
    rows = oled.fontRows();
    // Print units.
    for (uint8_t i = 0; i < 4; i++) {
      oled.setCursor(col1 + 1, i * rows);
      oled.print(unitsBME[i]);
    }


  } else if (activeSensor == MPU6050) {
    logfile = SD.open(filename, FILE_WRITE);
    logfile.println("AdaloggerV3 low Resolution by Tim Schumann");
    logfile.println("#, CPU time in ms, hh:mm:ss(UTC), date, x in m/s^2, y in m/s^2, z in m/s^2, geigerCounter in cpm, lat, lng, alt, speed, course, distanceToStart, satellites, batteryVoltage");
    if (!logfile) {
#ifdef ENLOG
      Serial.print("Couldnt create ");
      Serial.println(filename);
#endif
      error(3);
    }
#ifdef ENLOG
    Serial.print("Writing to: ");
    Serial.println(filename);
    Serial.println("AdaloggerV3 low Resolution by Tim Schumann");
    Serial.println("#, CPU time in ms, hh:mm:ss(UTC), date, x in m/s^2, y in m/s^2, z in m/s^2, geigerCounter in cpm, lat, lng, alt, speed, course, distanceToStart, satellites, batteryVoltage");
#endif
    logfile.close();
    // Setup form and find longest label.
    for (uint8_t i = 0; i < 4; i++) {
      oled.println(labelMPU[i]);
      uint8_t w = oled.strWidth(labelMPU[i]);
      col0 = col0 < w ? w : col0;
    }
    // Six pixels after label.
    col0 += 6;
    // Allow two or more pixels after value.
    col1 = col0 + oled.strWidth("99.99") + 2;
    // Line height in rows.
    rows = oled.fontRows();
    // Print units.
    for (uint8_t i = 0; i < 4; i++) {
      oled.setCursor(col1 + 1, i * rows);
      oled.print(unitsMPU[i]);
    }
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

  if (gps.charsProcessed() < 50 && millis() > 4000) {
    error(10);
  }

  // Update GPS data from serial input
  updateGPSData();

  unsigned long currentMillis = millis();
  //Update clicksPerMinute from geiger counter
  if (currentMillis - lastCountMillis >= 60000) {
    lastCountMillis = currentMillis;
    lastCPM = counts;
    counts = 0;
  }

  // Update sensor data and write to SD
  if (currentMillis - previousMillis >= definedDelay) {
    previousMillis = currentMillis;
    sdWrite();
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
void sdWrite() {

  measuredvbat = analogRead(VBATPIN);
  measuredvbat *= 2;    // we divided by 2, so multiply back
  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  int voltageMapped = map(measuredvbat, 3277, 4301, 0, 100);
  measuredvbat /= 1024;  // convert to voltage


  if (activeSensor == BME280) {
    logfile = SD.open(filename, FILE_WRITE);
    if (logfile) {
      digitalWrite(SDLED, HIGH);
      logfile.print(++count);
      logfile.print(", ");
      logfile.print(millis());
      logfile.print(", ");
      logfile.print(gps.time.hour());
      logfile.print(":");
      logfile.print(gps.time.minute());
      logfile.print(":");
      logfile.print(gps.time.second());
      logfile.print(", ");
      logfile.print(gps.date.day());
      logfile.print("-");
      logfile.print(gps.date.month());
      logfile.print("-");
      logfile.print(gps.date.year());
      logfile.print(", ");
      logfile.print(bme.readTemperature());
      logfile.print(", ");
      logfile.print(bme.readPressure() / 100.0F);
      logfile.print(", ");
      logfile.print(bme.readHumidity());
      logfile.print(", ");
      logfile.print(lastCPM);
      logfile.print(", ");
      logfile.print(String(gps.location.lat(), 6));
      logfile.print(", ");
      logfile.print(String(gps.location.lng(), 6));
      logfile.print(", ");
      logfile.print(gps.altitude.meters());
      logfile.print(", ");
      logfile.print(gps.speed.kmph());
      logfile.print(", ");
      logfile.print(gps.course.deg());
      logfile.print(", ");
      if (gps.location.isValid() && START_LAT != 0) {
        distanceToStart =
          TinyGPSPlus::distanceBetween(
            gps.location.lat(),
            gps.location.lng(),
            START_LAT,
            START_LON);
        logfile.print(distanceToStart, 6);
        logfile.print(", ");
      } else {
        logfile.print("0");
        logfile.print(", ");
      }
      logfile.print(gps.satellites.value());
      logfile.print(", ");
      logfile.println(measuredvbat);
      logfile.close();
    }

    if (count % modulus == 0) displayState = !displayState;

    clearValue(0);
    oled.print(bme.readTemperature(), 1);
    clearValue(rows);
    oled.print(bme.readHumidity(), 1);
    clearValue(2 * rows);
    oled.print(gps.time.hour());
    oled.print(":");
    oled.print(gps.time.minute());
    oled.print(":");
    oled.print(gps.time.second());
    clearValue(3 * rows);
    if (displayState)
      oled.print(voltageMapped);
    else
      oled.print(gps.satellites.value());

#ifdef ENLOG
    Serial.print(count);
    Serial.print(", ");
    Serial.print(millis());
    Serial.print(", ");
    Serial.print(gps.time.hour());
    Serial.print(":");
    Serial.print(gps.time.minute());
    Serial.print(":");
    Serial.print(gps.time.second());
    Serial.print(", ");
    Serial.print(gps.date.day());
    Serial.print("-");
    Serial.print(gps.date.month());
    Serial.print("-");
    Serial.print(gps.date.year());
    Serial.print(", ");
    Serial.print(bme.readTemperature());
    Serial.print(", ");
    Serial.print(bme.readPressure() / 100.0F);
    Serial.print(", ");
    Serial.print(bme.readHumidity());
    Serial.print(", ");
    Serial.print(lastCPM);
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
    Serial.print(gps.satellites.value());
    Serial.print(", ");
    Serial.print(measuredvbat);
    Serial.print(", ");
    Serial.println(voltageMapped);
#endif

    digitalWrite(SDLED, LOW);

  } else if (activeSensor == MPU6050) {

    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    logfile = SD.open(filename, FILE_WRITE);
    if (logfile) {
      digitalWrite(SDLED, HIGH);
      logfile.print(++count);
      logfile.print(", ");
      logfile.print(millis());
      logfile.print(", ");
      logfile.print(gps.time.hour());
      logfile.print(":");
      logfile.print(gps.time.minute());
      logfile.print(":");
      logfile.print(gps.time.second());
      logfile.print(", ");
      logfile.print(gps.date.day());
      logfile.print("-");
      logfile.print(gps.date.month());
      logfile.print("-");
      logfile.print(gps.date.year());
      logfile.print(", ");
      logfile.print(a.acceleration.x);
      logfile.print(", ");
      logfile.print(a.acceleration.y);
      logfile.print(", ");
      logfile.print(a.acceleration.z);
      logfile.print(", ");
      logfile.print(lastCPM);
      logfile.print(", ");
      logfile.print(String(gps.location.lat(), 6));
      logfile.print(", ");
      logfile.print(String(gps.location.lng(), 6));
      logfile.print(", ");
      logfile.print(gps.altitude.meters());
      logfile.print(", ");
      logfile.print(gps.speed.kmph());
      logfile.print(", ");
      logfile.print(gps.course.deg());
      logfile.print(", ");
      if (gps.location.isValid() && START_LAT != 0) {
        distanceToStart =
          TinyGPSPlus::distanceBetween(
            gps.location.lat(),
            gps.location.lng(),
            START_LAT,
            START_LON);
        logfile.print(distanceToStart, 6);
        logfile.print(", ");
      } else {
        logfile.print("0");
        logfile.print(", ");
      }
      logfile.print(gps.satellites.value());
      logfile.print(", ");
      logfile.println(measuredvbat);
      logfile.close();
    }

    if (count % modulus == 0) displayState = !displayState;

    clearValue(0);
    oled.print(a.acceleration.x);
    clearValue(rows);
    oled.print(a.acceleration.y);
    clearValue(2 * rows);
    oled.print(a.acceleration.z);
    clearValue(3 * rows);
    if (displayState)
      oled.print(voltageMapped);
    else
      oled.print(gps.satellites.value());



#ifdef ENLOG
    Serial.print(count);
    Serial.print(", ");
    Serial.print(millis());
    Serial.print(", ");
    Serial.print(gps.time.hour());
    Serial.print(":");
    Serial.print(gps.time.minute());
    Serial.print(":");
    Serial.print(gps.time.second());
    Serial.print(", ");
    Serial.print(gps.date.day());
    Serial.print("-");
    Serial.print(gps.date.month());
    Serial.print("-");
    Serial.print(gps.date.year());
    Serial.print(", ");
    Serial.print(a.acceleration.x);
    Serial.print(", ");
    Serial.print(a.acceleration.y);
    Serial.print(", ");
    Serial.print(a.acceleration.z);
    Serial.print(", ");
    Serial.print(lastCPM);
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
    Serial.print(gps.satellites.value());
    Serial.print(", ");
    Serial.print(measuredvbat);
    Serial.print(", ");
    Serial.println(voltageMapped);
#endif

    digitalWrite(SDLED, LOW);
  }

  if (gps.altitude.meters()) {  // && bme.readTemperature() != 0) {
    blinkState = !blinkState;
    digitalWrite(GPSLED, blinkState);
  } else {
    digitalWrite(GPSLED, LOW);
  }
}

// Function to send UBX protocol data to GPS
void sendUBX(uint8_t* MSG, uint8_t len) {
  for (int i = 0; i < len; i++) {
    Serial1.write(MSG[i]);
  }
}

// Function to handle errors
void error(uint8_t errno) {
  oled.clear();
  oled.setFont(Adafruit5x7);
  oled.set2X();
  oled.print("\n ");
  switch (errno) {
    case 5:
      oled.println("SEN-ERROR");
      break;
    case 2:
    case 3:
      oled.println("SD-Error");
      break;
    case 10:
      oled.println("GPS-Error");
      break;
  }
  oled.println("   !!!!");
  while (1) {
    uint8_t i;
    for (i = 0; i < errno; i++) {
      digitalWrite(InternalPowerLED, HIGH);
      digitalWrite(PowerLED, HIGH);
      delay(100);
      digitalWrite(InternalPowerLED, LOW);
      digitalWrite(PowerLED, LOW);
      delay(100);
    }
    for (i = errno; i < 10; i++) {
      delay(200);
    }
  }
}

// Function to clear a value on the OLED display
void clearValue(uint8_t row) {
  oled.clear(col0, col1, row, row + rows - 1);
}

// Function to count the Geiger Counter inputs
void impulse() {
  counts++;
}


//end
