/* ============================================

    Sketch for NaWiSchool
    Output on SD-Card (data logger)
    [This code is used to read and save the data from BME280 sensor, Ozon sensor Sen0321 and GPS module]
    Author: T. Schumann
    Date: 2023-06-09

    Dependencies:
    TinyGPS++ - https://github.com/mikalhart/TinyGPSPlus.git 
    Adafruit_Sensor - https://github.com/adafruit/Adafruit_Sensor
    Adafruit_BME280_Library - https://github.com/adafruit/Adafruit_BME280_Library
    SSD1306AsciiWire - https://github.com/greiman/SSD1306Ascii
    DFRobot_OzoneSensor - https://github.com/DFRobot/DFRobot_OzoneSensor

    All rights reserved. Copyright Tim Schumann 2023
  ===============================================
*/

#include <SPI.h>
#include <SD.h>
#include <TinyGPS++.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <SSD1306AsciiWire.h>
#include <DFRobot_OzoneSensor.h>

#define ENLOG

long definedDelay = 1000;  //set Delay between measurements
long previousMillis = 0;   // Time since last write

// collect number, range: 1-100
#define COLLECT_NUMBER 20
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
byte failflag = 0;

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

// Create a BME280 object
Adafruit_BME280 bme;

// Create a SEN0321 object
DFRobot_OzoneSensor ozone;

// Create a OLED object
SSD1306AsciiWire oled;



uint8_t col0 = 0;  // First value column
uint8_t col1 = 0;  // Last value column.
uint8_t rows;      // Rows per line.

const char* label[] = { " Temp.:", " O3.:", " Time:", " B.|S.:" };
const char* units[] = { "C", "ppb", " ", " " };

uint8_t ClearConfig[] = { 0xB5, 0x62, 0x06, 0x09, 0x0D, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x01, 0x19, 0x98 };
uint8_t SetPSM[] = { 0xB5, 0x62, 0x06, 0x11, 0x02, 0x00, 0x08, 0x01, 0x22, 0x92 };
uint8_t SetPlatform[] = { 0xB5, 0x62, 0x06, 0x24, 0x24, 0x00, 0xFF, 0xFF, 0x07, 0x03, 0x00, 0x00, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x05, 0x00, 0xFA, 0x00, 0xFA, 0x00, 0x64, 0x00, 0x2C, 0x01, 0x00, 0x00, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4E, 0xFD };
uint8_t SaveConfig[] = { 0xB5, 0x62, 0x06, 0x09, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x1D, 0xAB };


void setup() {
  Wire.begin();
#ifdef ENLOG
  Serial.begin(115200);

  Serial.println("BME280 Adalogger with Ozone By Tim Schumann");
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
  delay(300);
  digitalWrite(InternalPowerLED, HIGH);
  delay(100);
  digitalWrite(InternalPowerLED, LOW);
  delay(300);
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
  logfile.println("Name: AdaloggerV2, HW Version: 1.2, SW Version: 1.4, reduced Resolution with Ozone by Tim Schumann");
  logfile.println("#, CPU time in ms, hh:mm:ss(UTC), date, temp, press, hum, ozone, lat, lng, alt, speed, course, distanceToStart, satellites, batteryVoltage");
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
  Serial.println("Name: AdaloggerV2, HW Version: 1.2, SW Version: 1.4, reduced Resolution with Ozone by Tim Schumann");
  Serial.println("#, CPU time in ms, hh:mm:ss(UTC), date, temp, press, hum, ozone, lat, lng, alt, speed, course, distanceToStart, satellites, batteryVoltage");
#endif
  logfile.close();

  if (!bme.begin(0x76, &Wire)) {
    error(5);
  }

  if (!ozone.begin(0x73)) {
    error(8);
  }
  /**
   * set measuer mode
   * MEASURE_MODE_AUTOMATIC         active  mode
   * MEASURE_MODE_PASSIVE           passive mode
   */
  ozone.setModes(MEASURE_MODE_PASSIVE);

  delay(200);

  sendUBX(ClearConfig, sizeof(ClearConfig) / sizeof(uint8_t));

  delay(600);

  sendUBX(SetPlatform, sizeof(SetPlatform) / sizeof(uint8_t));
  sendUBX(SaveConfig, sizeof(SaveConfig) / sizeof(uint8_t));
  delay(200);
}

void loop() {
  if (millis() > 4000 && gps.charsProcessed() < 50) {
    error(10);
  }
  long currentMillis = millis();
  long elapsedMillis = currentMillis - previousMillis;
  // Dispatch incoming characters
  while (Serial1.available() > 0)
    gps.encode(Serial1.read());

  if (elapsedMillis >= definedDelay) {
    previousMillis = currentMillis;  // Reset the previous time
    sdwrite();
  }


  if (firstlock == 0 && gps.satellites.value() > 4) {
    sendUBX(SetPSM, sizeof(SetPSM) / sizeof(uint8_t));
    firstlock = 1;
#ifdef ENLOG
    Serial.println("PowerMode Changed");
#endif
    delay(1000);
    START_LAT = gps.location.lat();
    START_LON = gps.location.lng();
  }
  delay(100);
}

//---------write data to sd-----------
void sdwrite() {
  measuredvbat = analogRead(VBATPIN);
  measuredvbat *= 2;    // we divided by 2, so multiply back
  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  int voltageMapped = map(measuredvbat, 3277, 4301, 0, 100);
  measuredvbat /= 1024;  // convert to voltage
  int16_t ozoneConcentration = ozone.readOzoneData(COLLECT_NUMBER);

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
    logfile.print(bme.readTemperature());
    logfile.print(", ");
    logfile.print(bme.readPressure() / 100.0F);
    logfile.print(", ");
    logfile.print(bme.readHumidity());
    logfile.print(", ");
    logfile.print(ozoneConcentration);
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

  if (count % 5 == 0) displayState = !displayState;

  clearValue(0);
  oled.print(bme.readTemperature(), 1);
  clearValue(rows);
  oled.print(ozoneConcentration);
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
  Serial.print("/");
  Serial.print(gps.date.month());
  Serial.print("/");
  Serial.print(gps.date.year());
  Serial.print(", ");
  Serial.print(bme.readTemperature());
  Serial.print(", ");
  Serial.print(bme.readPressure() / 100.0F);
  Serial.print(", ");
  Serial.print(bme.readHumidity());
  Serial.print(", ");
  Serial.print(ozoneConcentration);
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

// Send a byte array of UBX protocol to the GPS
void sendUBX(uint8_t* MSG, uint8_t len) {
  for (int i = 0; i < len; i++) {
    Serial1.write(MSG[i]);
  }
  //Serial.println();
}

// blink out an error code
void error(uint8_t errno) {
  oled.clear();
  oled.setFont(Adafruit5x7);
  oled.set2X();
  oled.print("\n ");
  switch (errno) {
    case 5:
      oled.println("BME-ERROR");
      break;
    case 8:
      oled.println("O3-ERROR");
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
//------------------------------------------------------------------------------
void clearValue(uint8_t row) {
  oled.clear(col0, col1, row, row + rows - 1);
}


//end
