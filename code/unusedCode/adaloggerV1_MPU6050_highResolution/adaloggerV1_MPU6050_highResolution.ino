/* ============================================

    Sketch for NaWiSchool
    Output on SD-Card (data logger)
    [This code is used to read and save the data from MPU6050 sensor and Clock module]
    Author: T. Schumann
    Date: 2023-05-07

    Dependencies:
    Adafruit_Sensor - https://github.com/adafruit/Adafruit_Sensor
    Adafruit_MPU6050 - https://github.com/adafruit/Adafruit_MPU6050 
    SSD1306AsciiWire - https://github.com/greiman/SSD1306Ascii
    TinyGPS++ - https://github.com/mikalhart/TinyGPSPlus.git 

    All rights reserved. Copyright Tim Schumann 2023
  ===============================================
*/

#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <TinyGPS++.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_MPU6050.h>
#include <SSD1306AsciiWire.h>

//#define ENLOG

long definedDelay = 100;  //set Delay between measurements
long previousMillis = 0;  // Time since last write

// Set the pins used
#define cardSelect 4
#define PowerLED 6
#define InternalPowerLED 13
#define SDLED 8
#define GPSLED 5
#define VBATPIN A7

File logfile;

bool blinkState = false;
bool displayState = true;

static const uint32_t GPSBaud = 9600;
int firstlock = 0;
double START_LAT = 0;
double START_LON = 0;
double distanceToStart = 0;
char filename[15];
float measuredvbat;
long count = 0;

// Create a TinyGPS++ object
TinyGPSPlus gps;

// Create a MPU6050 object
Adafruit_MPU6050 mpu;

// Create a OLED object
SSD1306AsciiWire oled;

uint8_t col0 = 0;  // First value column
uint8_t col1 = 0;  // Last value column.
uint8_t rows;      // Rows per line.

const char* label[] = { " x:", " y:", " z:", " B.|S.:" };
const char* units[] = { "m/s2", "m/s2", "m/s2", "" };


void setup() {
  Wire.begin();
#ifdef ENLOG
  Serial.begin(115200);

  Serial.println("\r\nAdalog logger test");
  Serial.println("MPU6050 Adalogger By Tim Schumann");
  Serial.println();
#endif
  pinMode(InternalPowerLED, OUTPUT);
  pinMode(SDLED, OUTPUT);
  pinMode(GPSLED, OUTPUT);
  pinMode(PowerLED, OUTPUT);
  digitalWrite(InternalPowerLED, HIGH);
  digitalWrite(PowerLED, HIGH);
  delay(100);
  digitalWrite(InternalPowerLED, LOW);
  delay(500);
  digitalWrite(InternalPowerLED, HIGH);
  delay(100);
  digitalWrite(InternalPowerLED, LOW);
  delay(500);
  digitalWrite(InternalPowerLED, HIGH);

  //start GPS
  Serial1.begin(GPSBaud);

  //start OLED
  oled.begin(&Adafruit128x64, 0x3C);
  oled.setFont(Callibri15);
  oled.setLetterSpacing(3);
  oled.clear();

  // Setup form and find longest label.
  for (uint8_t i = 0; i < 4; i++) {
    oled.println(label[i]);
    uint8_t w = oled.strWidth(label[i]);
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
    oled.print(units[i]);
  }

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

  logfile = SD.open(filename, FILE_WRITE);
  logfile.println("Name: AdaloggerV2, HW Version: 1.2, SW Version: 2.0, with MPU6050 high Resolution by Tim Schumann");
  logfile.println("#, CPU time in ms, hh:mm:ss(UTC), date, x in m/s^2, y in m/s^2, z in m/s^2, lat, lng, alt, speed, course, distanceToStart, satellites, batteryVoltage");
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
  Serial.println("#, CPU time in ms, hh:mm:ss(UTC), date, x in m/s^2, y in m/s^2, z in m/s^2, lat, lng, alt, speed, course, distanceToStart, satellites, batteryVoltage");
  Serial.println("#, CPU time in ms, hh:mm:ss(UTC), date, x in m/s^2, y in m/s^2, z in m/s^2, batteryVoltage");
#endif
  logfile.close();


  if (!mpu.begin()) {
    error(5);
  }

}

void loop() {
  if (millis() > 4000 && gps.charsProcessed() < 50) {
    error(10);
  }

  long currentMillis = millis();
  long elapsedMillis = currentMillis - previousMillis;
  
  while (Serial1.available() > 0)
    gps.encode(Serial1.read());

  if (elapsedMillis >= definedDelay) {
    previousMillis = currentMillis;  // Reset the previous time
    sdwrite();
  }

  delay(10);
}

//---------write data to sd-----------
void sdwrite() {
  measuredvbat = analogRead(VBATPIN);
  measuredvbat *= 2;    // we divided by 2, so multiply back
  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  int voltageMapped = map(measuredvbat, 3277, 4301, 0, 100);
  measuredvbat /= 1024;  // convert to voltage

  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  /*fx = (ax * 9.81 / 16384) + offsetX;
  fy = (ay * 9.81 / 16384) + offsetY;
  fz = -((az * 9.81 / 16384) + offsetZ);*/


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
    logfile.print("/");
    logfile.print(gps.date.month());
    logfile.print("/");
    logfile.print(gps.date.year());
    logfile.print(", ");
    logfile.print(a.acceleration.x);
    logfile.print(", ");
    logfile.print(a.acceleration.y);
    logfile.print(", ");
    logfile.print(a.acceleration.z);
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

  if (count % 50 == 0) displayState = !displayState;

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
  Serial.print("/");
  Serial.print(gps.date.month());
  Serial.print("/");
  Serial.print(gps.date.year());
  Serial.print(", ");
  Serial.print(a.acceleration.x);
  Serial.print(", ");
  Serial.print(a.acceleration.y);
  Serial.print(", ");
  Serial.print(a.acceleration.z);
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

  if (gps.altitude.meters()) {  // && bme.readTemperature() != 0) {
    blinkState = !blinkState;
    digitalWrite(GPSLED, blinkState);
  } else {
    digitalWrite(GPSLED, LOW);
  }

}



// blink out an error code
void error(uint8_t errno) {
  oled.clear();
  oled.setFont(Adafruit5x7);
  oled.set2X();
  oled.print("\n ");
  switch (errno) {
    case 5:
      oled.println("MPU-ERROR");
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
      digitalWrite(13, HIGH);
      delay(100);
      digitalWrite(13, LOW);
      delay(100);
    }
    for (i = errno; i < 10; i++) {
      delay(200);
    }
  }
}

//------------------------------------------------------------------------------
void clearValue(uint8_t row) {
  oled.clear(col0, col1, row, row + rows - 1);
}


//end
