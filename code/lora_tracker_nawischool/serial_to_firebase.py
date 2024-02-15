"""
LoRa Data Receiver Script

This script reads data from an Arduino connected via serial port, parses the data,
and pushes it to a Firebase Realtime Database.
"""

import time
import serial
import firebase_admin
from firebase_admin import credentials
from firebase_admin import db

# Initialize Firebase credentials
cred = credentials.Certificate("firebase-sdk.json")
firebase_admin.initialize_app(cred,
                              {'databaseURL':'https://lora-tracker-nawischool-default-rtdb.europe-west1.firebasedatabase.app/'})

# Serial port configuration
SERIAL_PORT = "COM16"  # Change this to match your Arduino's serial port
BAUD_RATE = 9600

# Reference to Firebase database
firebase_ref = db.reference('/UsersData/AnW7hkCS7nONGCJSJZ3enD4QBgG3')


def read_and_push_data(ser):
    """
    Read data from the Arduino and push it to Firebase.

    Parameters:
    - ser: Serial connection object.
    """
    try:
        while True:
            # Read data from Arduino
            arduino_data = ser.readline()

            # Skip empty or incomplete lines
            if not arduino_data:
                continue

            try:
                # Decode byte data using UTF-8 encoding
                arduino_data_decoded = arduino_data.decode('utf-8').strip()
            except UnicodeDecodeError:
                print("Error decoding data, ignoring...")
                continue

            print("Received data:", arduino_data_decoded)

            # Parse the data, handling potential formatting errors
            data_parts = arduino_data_decoded.split(', ')

            # Ensure correct number of data parts received
            if len(data_parts) != 16:
                print("Invalid data format, ignoring...")
                continue

            try:
                # Extract data fields
                count, millis, temp_bme, press_bme, hum_bme, temp_scd, hum_scd, co2, lat, lng, alt, speed, course, \
                distance_to_start, satellites, rssi = map(float, data_parts[:16])
            except ValueError as e:
                print(f"Error parsing data: {e}, ignoring...")
                continue

            # Generate a timestamp for the reading
            timestamp = str(int(time.time()))

            # Create a dictionary to represent the reading
            reading = {
                'timestamp': timestamp,
                'count': count,
                'millis': millis,
                'temperature_BME': temp_bme,
                'pressure_BME': press_bme,
                'humidity_BME': hum_bme,
                'temperature_SCD': temp_scd,
                'humidity_SCD': hum_scd,
                'co2': co2,
                'latitude': lat,
                'longitude': lng,
                'altitude': alt,
                'speed': speed,
                'course': course,
                'distance_to_start': distance_to_start,
                'satellites': satellites,
                'rssi': rssi
            }

            # Update Firebase database with the reading
            firebase_ref.child("readings").child(timestamp).set(reading)
            print("Data pushed to Firebase")

    except (serial.SerialException, serial.SerialTimeoutException) as e:
        print("Serial connection lost:", e)

        # Close the existing connection
        ser.close()

        # Try to reconnect
        attempt_reconnection(ser)


def attempt_reconnection(ser):
    """
    Attempt to reconnect to the Arduino.

    Parameters:
    - ser: Serial connection object.
    """
    reconnection_attempts = 5
    delay_between_attempts = 5  # Seconds

    for attempt in range(reconnection_attempts):
        print(f"Attempting to reconnect... (attempt {attempt+1}/{reconnection_attempts})")
        try:
            # Reopen the serial port with the same settings
            ser.open()
            print("Reconnected to Arduino on port:", SERIAL_PORT)

            # If reconnected, resume data reading
            read_and_push_data(ser)
            break

        except serial.SerialException as e:
            print("Reconnection failed:", e)
            time.sleep(delay_between_attempts)

    # If unable to reconnect after all attempts, exit
    else:
        print("Connection failed after all attempts, exiting...")


if __name__ == "__main__":
    try:
        # Connect to the Arduino
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE)
        print("Connected to Arduino on port:", SERIAL_PORT)

        # Continuously read data from Arduino and push to Firebase
        read_and_push_data(ser)

    except KeyboardInterrupt:
        print("Exiting...")
        if 'ser' in locals():
            ser.close()

    except Exception as e:
        print("An error occurred:", str(e))
