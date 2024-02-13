# LoRa Data Receiver Script

This script reads data from an Arduino connected via serial port, parses the data, and pushes it to a Firebase Realtime Database.

## Dependencies

- Python 3.x
- `firebase_admin` library (install via `pip install firebase-admin`)
- `serial` library (install via `pip install pyserial`)

## Usage

1. Install the required Python libraries using pip:
   ```
   pip install firebase-admin pyserial
   ```

2. Ensure you have a Firebase project set up and obtain the Firebase SDK JSON file.

3. Update the `SERIAL_PORT` variable in the script to match your Arduino's serial port.

4. Run the script:
   ```
   python lora_data_receiver.py
   ```

## Functionality

The script performs the following tasks:

- Establishes a connection with an Arduino device over a serial port.
- Reads sensor data transmitted by the Arduino.
- Parses the data into a readable format.
- Uploads the parsed data to a Firebase Realtime Database for real-time monitoring and analysis.

## Features

- Gracefully handles exceptions, ensuring a stable connection with the Arduino.
- Manages reconnection attempts in case of connection failures.
- Catches keyboard interrupts for a clean exit from the script.

