# LoRa Transmitter with BME280, SCD41, and NEO-6M

## Overview

This Arduino sketch is designed to transmit data from a BME280 sensor, SCD41 sensor, and NEO-6M GPS module via LoRa communication. The transmitted data includes temperature, pressure, humidity, CO2 levels, GPS coordinates, altitude, speed, course, distance from the starting location, and satellite information.

## Author

- **Author:** T. Schumann
- **Date:** 2024-02-13

## Dependencies

- **LoRa Library:** [arduino-LoRa](https://github.com/sandeepmistry/arduino-LoRa)
- **TinyGPS++:** [TinyGPSPlus](https://github.com/mikalhart/TinyGPSPlus.git)
- **Adafruit Sensor Library:** [Adafruit_Sensor](https://github.com/adafruit/Adafruit_Sensor)
- **Adafruit BME280 Library:** [Adafruit_BME280_Library](https://github.com/adafruit/Adafruit_BME280_Library)
- **Sensirion I2C SCD4x Library:** [Sensirion_I2C_SCD4x](https://github.com/Sensirion/arduino-i2c-scd4x)

## Usage

1. Install the required libraries by following the instructions provided in the respective repositories.
2. Upload this sketch to your Arduino board connected with the necessary sensors and LoRa transmitter module.
3. Adjust the pin connections for the sensors if needed.
4. Ensure that the LoRa transmitter is configured to operate on the same frequency as the receiver.
5. Monitor the LoRa receiver to receive and process the transmitted data.

## Setup

1. Connect the BME280, SCD41, and NEO-6M modules to your Arduino board.
2. Adjust the LoRa frequency in the `LoRa.begin()` function call if required.
3. Ensure proper wiring and connections for SPI and I2C communication.
4. Configure the GPS module to output the desired NMEA sentences.

## Troubleshooting

- If the LoRa transmitter fails to initialize, check the wiring and ensure that the LoRa module is properly connected.
- Verify that all sensor modules are functioning correctly and are properly connected to the Arduino board.
- Ensure that the GPS module has a clear view of the sky to receive satellite signals.

## License

All rights reserved. Copyright Tim Schumann 2024
