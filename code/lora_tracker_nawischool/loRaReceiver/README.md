# LoRa Receiver

## Overview

This Arduino sketch is designed to work with a LoRa receiver module. It receives data packets sent via LoRa and prints them to the console. The code is intended to be used in conjunction with a LoRa transmitter to establish communication between two Arduino devices using LoRa technology.

## Author

- **Author:** T. Schumann
- **Date:** 2024-02-13

## Dependencies

- **LoRa Library:** [arduino-LoRa](https://github.com/sandeepmistry/arduino-LoRa)

## Usage

1. Install the LoRa library by following the instructions provided in the [arduino-LoRa](https://github.com/sandeepmistry/arduino-LoRa) repository.
2. Upload this sketch to your Arduino board with the LoRa receiver module connected.
3. Ensure that the serial monitor is open with a baud rate of 9600 to view the received data.
4. Use in conjunction with a LoRa transmitter that sends data packets to this receiver.

## Setup

1. Connect the LoRa receiver module to your Arduino board.
2. Ensure proper pin connections for SPI communication.
3. Adjust the LoRa frequency in the `LoRa.begin()` function call if needed.

## Troubleshooting

- If the serial monitor displays "Starting LoRa failed!" it indicates a problem with initializing the LoRa module. Check the wiring and ensure that the LoRa module is properly connected.
- Ensure that the LoRa transmitter is configured to operate on the same frequency and with compatible settings.

## License

All rights reserved. Copyright Tim Schumann 2024
