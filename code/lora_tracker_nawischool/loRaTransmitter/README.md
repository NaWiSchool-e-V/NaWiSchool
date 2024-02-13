# LoRa Transmitter with BME280, SCD41, and NEO-6M

## Overview

This code transmits data from BME280 sensor, SCD41 sensor, and Neo-6M GPS module via LoRa.

- **Author:** T. Schumann
- **Date:** 2024-02-13

## Dependencies

- **LoRa Library:** [arduino-LoRa](https://github.com/sandeepmistry/arduino-LoRa)
- **TinyGPS++:** [TinyGPSPlus](https://github.com/mikalhart/TinyGPSPlus.git)
- **Adafruit Sensor:** [Adafruit_Sensor](https://github.com/adafruit/Adafruit_Sensor)
- **Adafruit BME280 Library:** [Adafruit_BME280_Library](https://github.com/adafruit/Adafruit_BME280_Library)
- **Sensirion I2C SCD4x:** [arduino-i2c-scd4x](https://github.com/Sensirion/arduino-i2c-scd4x)

## Usage

1. Install the necessary libraries listed above.
2. Upload this sketch to your Arduino board with the LoRa transmitter module connected.
3. Ensure that the LoRa transmitter is configured to operate on the same frequency as the LoRa receiver.

## Setup

1. Connect the BME280 sensor, SCD41 sensor, and Neo-6M GPS module to your Arduino board.
2. Adjust the LoRa frequency in the `LoRa.begin()` function call if needed.
3. Ensure proper pin connections for SPI communication and sensor connections.

## Troubleshooting

- If there are errors related to sensor initialization or LoRa module initialization, refer to the troubleshooting section in the code comments for guidance.

## License

All rights reserved. Copyright Tim Schumann 2024
