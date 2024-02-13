"""
LoRa Data Receiver Script

This script reads data from an Arduino connected via serial port, parses the data, and pushes it to a Firebase Realtime Database.
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

def main():
    """
    Establishes a connection with an Arduino device over a serial port,
    reads sensor data transmitted by the Arduino, parses the data into
    readable format, and uploads the parsed data to a Firebase Realtime
    Database for real-time monitoring and analysis.

    This function continuously listens for data from the Arduino and
    handles exceptions gracefully. It ensures a stable connection with
    the Arduino, processes incoming data, and manages reconnection
    attempts in case of connection failures. Keyboard interrupts are
    caught to ensure a clean exit from the script.

    Parameters:
    None

    Returns:
    None
    """
    # Connect to the Arduino
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE)
    print("Connected to Arduino on port:", SERIAL_PORT)

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
                count = int(data_parts[0])
                millis = int(data_parts[1])
                temp_bme = float(data_parts[2])
                press_bme = float(data_parts[3])
                hum_bme = float(data_parts[4])
                temp_scd = float(data_parts[5])
                hum_scd = float(data_parts[6])
                co2 = int(data_parts[7])
                lat = float(data_parts[8])
                lng = float(data_parts[9])
                alt = float(data_parts[10])
                speed = float(data_parts[11])
                course = float(data_parts[12])
                distance_to_start = float(data_parts[13])
                satellites = int(data_parts[14])
                rssi = int(data_parts[15])
            except ValueError as e:
                print(f"Error parsing data: {e}, ignoring...")
                continue

            # Create a dictionary to represent the reading
            reading = {
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
            timestamp = str(int(time.time()))
            firebase_ref.child("readings").child(timestamp).set(reading)
            print("Data pushed to Firebase")

    except (serial.SerialException, serial.SerialTimeoutException) as e:
        print("Serial connection lost:", e)

        # Try to reconnect for x attempts with a delay in between
        reconnection_attempts = 5
        delay_between_attempts = 5  # Seconds

        for attempt in range(reconnection_attempts):
            print(f"Attempting to reconnect... (attempt {attempt+1}/{reconnection_attempts})")
            try:
                # Close the existing connection (important)
                ser.close()

                # Reopen the serial port with the same settings
                ser = serial.Serial(SERIAL_PORT, BAUD_RATE)
                print("Reconnected to Arduino on port:", SERIAL_PORT)
                break  # If successful, break out of the loop and resume data processing

            except serial.SerialException as e2:
                print("Reconnection failed:", e2)
                time.sleep(delay_between_attempts)

        # If unable to reconnect after all attempts, exit
        else:
            print("Connection failed after all attempts, exiting...")
        
    except KeyboardInterrupt:
        print("Exiting...")
        ser.close()

    finally:
        # Always close the serial port even if reconnection fails
        ser.close()
        print("Script terminated.")


if __name__ == "__main__":
    main()
