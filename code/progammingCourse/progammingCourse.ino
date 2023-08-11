/* ============================================

    Sketch for BME280 with OLED and WS2812B
    Author: T. Schumann
    Date: 2023-05-05

    Dependencies:
    Adafruit_Sensor - https://github.com/adafruit/Adafruit_Sensor
    Adafruit_BME380 - https://github.com/adafruit/Adafruit_BME280_Library
    SSD1306AsciiWire - https://github.com/greiman/SSD1306Ascii
    FastLed - https://github.com/FastLED/FastLED

    All rights reserved. Copyright Tim Schumann 2023
  ===============================================
*/

#include <SSD1306AsciiWire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <FastLED.h>
#include <Wire.h>
#define NUM_LEDS 8
#define DATA_PIN 2
// Create a WS2812B object
CRGB leds[NUM_LEDS];
CRGBPalette16 colorPalette = CRGBPalette16(
  CRGB::Green, CRGB::Yellow, CRGB::Red
);

// Create a BME280 object
Adafruit_BME280 bme;

// Create a OLED object
SSD1306AsciiWire oled;

float val = 0;
float lastVal = 0;

int upperLimit = 2900;
int lowerLimit = 2300;

void setup() {
  Wire.begin();
  Wire.setClock(400000L);
  Serial.begin(9600);
  Serial.println("\r\nBME280 with OLED and WS2812B");
  Serial.println("By Tim Schumann");
  Serial.println();

  //start leds
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(50);

  //start OLED
  oled.begin(&Adafruit128x64, 0x3C);
  oled.setFont(Callibri15);
  oled.setLetterSpacing(3);
  oled.set2X();
  oled.clear();

  //start bme
  bme.begin(0x76, &Wire);
}

void loop() {

  val = bme.readTemperature();
  //Serial.println(val);
  if (val != lastVal) {
    oled.setCursor(10,2);
    oled.print(val);
    lastVal = val;
  }
  int limitVal = constrain(round(val*100), lowerLimit, upperLimit); // clamp input value to the range of 25 to 30
  int numLedsToLight = map(limitVal, lowerLimit, upperLimit, 1, NUM_LEDS);
  int colorIndex = map(limitVal, lowerLimit, upperLimit, 0, 240);
  Serial.println(colorIndex);
  CRGB color = ColorFromPalette(colorPalette, colorIndex);
  // First, clear the existing led values
  FastLED.clear();
  for (int led = 0; led < numLedsToLight; led++) {
    leds[led] = color;
  }
  FastLED.show();
}
