/* ============================================

    Sketch for NaWiSchool
    
    LoRa Receiver
    [This code receives data via LoRa and prints them to the console.]
    Author: T. Schumann
    Date: 2024-02-13

    Dependencies:
    LoRa - https://github.com/sandeepmistry/arduino-LoRa 
    
    All rights reserved. Copyright Tim Schumann 2024

  ===============================================
*/

#include <SPI.h>
#include <LoRa.h>

// Constants
const uint32_t LORA_FREQUENCY = 868E6;
const int BAUD_RATE = 115200;

void setup() {
  // Initialize serial communication
  Serial.begin(BAUD_RATE);
  while (!Serial);
  
  // Initialize LoRa module
  if (!LoRa.begin(868E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
}

void loop() {  
  // Try to parse packet
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // Received a packet

    // Read packet and print to serial monitor
    while (LoRa.available()) {
      Serial.print((char)LoRa.read());
    }

    // Print Received Signal Strength Indicator (RSSI) of packet
    Serial.print(", ");
    Serial.println(LoRa.packetRssi());
  }
}
