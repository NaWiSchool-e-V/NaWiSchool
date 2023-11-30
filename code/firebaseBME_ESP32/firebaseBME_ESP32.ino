/* ============================================

    Sketch for NaWiSchool
    
    ESP32 with BME280
    [This code is used to read and stream the data from BME280 sensor to a firebase database]
    Author: T. Schumann
    Date: 2023-11-30

    Dependencies:
    WiFiManager - https://github.com/tzapu/WiFiManager 
    Firebase_Arduino_Client_Library_for_ESP8266_and_ESP32 - https://github.com/mobizt/Firebase-ESP-Client
    Adafruit_Sensor - https://github.com/adafruit/Adafruit_Sensor
    Adafruit_BME280_Library - https://github.com/adafruit/Adafruit_BME280_Library
    ArduinoJson - https://arduinojson.org/?utm_source=meta&utm_medium=library.properties

    All rights reserved. Copyright Tim Schumann 2023

  ===============================================
*/

#include <FS.h>
#include <Arduino.h>
#include <time.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <Firebase_ESP_Client.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>


// Include additional helper functions
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// Firebase project API Key
#define API_KEY "AIzaSyDfqQx9TGn64siHzvk5MuJe44cqxIBODGM";

// Authorized Email and Corresponding Password
char USER_EMAIL[40];
char USER_PASSWORD[20];

// Flag for saving data configuration
bool shouldSaveConfig = false;

// Define RTDB URL
#define DATABASE_URL "https://esp-demo-adc0a-default-rtdb.europe-west1.firebasedatabase.app/"

// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variable to save USER UID
String uid;

// Database main path (to be updated in setup with the user UID)
String databasePath;
// Database child nodes
String tempPath = "/temperature";
String humPath = "/humidity";
String presPath = "/pressure";
String timePath = "/timestamp";

// Parent Node (to be updated in every loop)
String parentPath;

int timestamp;
FirebaseJson json;

// NTP server
const char* ntpServer = "pool.ntp.org";

// BME280 sensor
Adafruit_BME280 bme;
float temperature;
float humidity;
float pressure;

// Timer variables
unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 300000;


//callback to save config
void saveConfigCallback() {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

// Initialize BME280
void initBME() {
  if (!bme.begin(0x76)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1)
      ;
  }
}

// Initialize WiFi
void initWiFi() {

  //clean FS, for testing
  //SPIFFS.format();

  // SPIFFS initialization
  if (SPIFFS.begin(true)) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        // Read configuration file into the buffer
        configFile.readBytes(buf.get(), size);

#if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
        DynamicJsonDocument json(1024);
        auto deserializeError = deserializeJson(json, buf.get());
        serializeJson(json, Serial);
        if (!deserializeError) {
#else
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
#endif

          Serial.println("\nparsed json");
          // Get user email and password from configuration
          strcpy(USER_EMAIL, json["user_email"]);
          strcpy(USER_PASSWORD, json["user_password"]);
        } else {
          Serial.println("failed to load json config");
        }
        configFile.close();
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }

  // custom parameter
  // id/name, placeholder/prompt, default, length
  WiFiManagerParameter custom_user_name("user_name", "Insert provided email", USER_EMAIL, 40);
  WiFiManagerParameter custom_user_password("user_password", "Insert provided password", USER_PASSWORD, 20);

  WiFiManager wifiManager;

  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.setConnectRetries(100);

  wifiManager.addParameter(&custom_user_name);
  wifiManager.addParameter(&custom_user_password);

  //reset settings - for testing
  //wifiManager.resetSettings();

  wifiManager.setMinimumSignalQuality();

  //start AP
  //name access point, password
  if (!wifiManager.autoConnect("NaWiSchool Wetterstation", "nawischool")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    ESP.restart();
    delay(5000);
  }
  strcpy(USER_EMAIL, custom_user_name.getValue());
  strcpy(USER_PASSWORD, custom_user_password.getValue());
  Serial.print("USER_EMAIL: ");
  Serial.println(USER_EMAIL);
  Serial.print("USER_PASSWORD: ");
  Serial.println(USER_PASSWORD);
  Serial.println();


  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
#if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
    DynamicJsonDocument json(1024);
#else
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
#endif
    json["user_email"] = USER_EMAIL;
    json["user_password"] = USER_PASSWORD;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

#if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
    serializeJson(json, Serial);
    serializeJson(json, configFile);
#else
    json.printTo(Serial);
    json.printTo(configFile);
#endif
    configFile.close();
    //end save
  }
}

// Function to get current epoch time
unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return (0);
  }
  time(&now);
  return now;
}

void setup() {
  Serial.begin(115200);
  //Wire.begin(26, 27);

  // Initialize BME280 sensor
  initBME();
  // Initialize WiFi
  initWiFi();
  // Configure time
  configTime(0, 0, ntpServer);

  // Assign the api key (required)
  config.api_key = API_KEY;

  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;

  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);

  // Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback;

  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);

  // Update database path
  databasePath = "/UsersData/" + uid + "/readings";

}

void loop() {
  // Send new readings to database
  if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
& 
    //Get current timestamp
    timestamp = getTime();
    Serial.print("time: ");
    Serial.println(timestamp);

    parentPath = databasePath + "/" + String(timestamp);

    // Set JSON data
    json.set(tempPath.c_str(), String(bme.readTemperature()));
    json.set(humPath.c_str(), String(bme.readHumidity()));
    json.set(presPath.c_str(), String(bme.readPressure() / 100.0F));
    json.set(timePath, String(timestamp));

    // Print result of setting JSON data
    Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
  }
}


// ---------- Safety ----------