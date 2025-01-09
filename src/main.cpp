#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>

// Function declarations
void uploadToGoogleSheets(String timestamp, float distance, String parkingStatus, float temperature, float humidity, String weatherDesc);
String getWeatherData();

// WiFi Credentials - replace as needed
const char* WIFI_SSID = "--"; 
const char* WIFI_PASSWORD = "--";

// Google Apps Script Web App URL
const String GOOGLE_SCRIPT_URL = "--"; //Replace with your Web APP URL key

// OpenWeatherMap API Configuration
const String WEATHER_API_KEY = "--"; // Replace with your API key
const String CITY = "London"; // Replace with your city
const String COUNTRY_CODE = "UK"; // Replace with your country code
const String WEATHER_URL = "http://api.openweathermap.org/data/2.5/weather?q=" + CITY + "," + COUNTRY_CODE + "&APPID=" + WEATHER_API_KEY + "&units=metric";

// Ultrasonic Sensor Pins
#define TRIG_PIN 2
#define ECHO_PIN 9
#define SOUND_SPEED 0.0343

// NTP Client setup
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600);

// Variables
long duration;
float distanceCm;
unsigned long lastUploadTime = 0;
unsigned long lastWeatherCheck = 0;
const long UPLOAD_INTERVAL = 30000;    // Upload every 30 seconds
const long WEATHER_CHECK_INTERVAL = 300000; // Check weather every 5 minutes
float currentTemp = 0;
float currentHumidity = 0;
String currentWeatherDesc = "";

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\nStarting Smart Parking System...");
  
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected Successfully!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal Strength (RSSI): ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    Serial.println("\nWiFi Connection FAILED!");
    Serial.println("Restarting ESP32...");
    delay(1000);
    ESP.restart();
  }

  timeClient.begin();
  timeClient.setTimeOffset(3600);
  Serial.println("NTP Client initialized");
  
  // Initial weather check
  Serial.println("Getting initial weather data...");
  String weatherData = getWeatherData();
  Serial.println("Setup complete!");
}

void loop() {
  // Measure Distance
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  duration = pulseIn(ECHO_PIN, HIGH);
  distanceCm = (duration * SOUND_SPEED) / 2;

  // Update Time
  timeClient.update();
  String formattedTime = timeClient.getFormattedTime();

  // Check weather periodically
  if (millis() - lastWeatherCheck >= WEATHER_CHECK_INTERVAL) {
    getWeatherData();
    lastWeatherCheck = millis();
  }

  // Determine Parking Status
  String parkingStatus = (distanceCm < 20) ? "Occupied" : "Free";

  // Upload to Google Sheets periodically
  if (millis() - lastUploadTime > UPLOAD_INTERVAL) {
    uploadToGoogleSheets(formattedTime, distanceCm, parkingStatus, currentTemp, currentHumidity, currentWeatherDesc);
    lastUploadTime = millis();
  }

  // Debug output
  Serial.print("Distance: ");
  Serial.print(distanceCm);
  Serial.print(" cm | Status: ");
  Serial.print(parkingStatus);
  Serial.print(" | Time: ");
  Serial.print(formattedTime);
  Serial.print(" | Temp: ");
  Serial.print(currentTemp);
  Serial.print("°C | Weather: ");
  Serial.println(currentWeatherDesc);

  delay(1000);
}

String getWeatherData() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(WEATHER_URL);
    
    int httpResponseCode = http.GET();
    
    if (httpResponseCode > 0) {
      String payload = http.getString();
      
      // Parse JSON response
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, payload);
      
      if (!error) {
        currentTemp = doc["main"]["temp"].as<float>();
        currentHumidity = doc["main"]["humidity"].as<float>();
        currentWeatherDesc = doc["weather"][0]["description"].as<String>();
        
        Serial.println("Weather data updated successfully");
      }
      
      return payload;
    } else {
      Serial.print("Error getting weather data: ");
      Serial.println(httpResponseCode);
      return "Error";
    }
    
    http.end();
  }
  return "Not connected";
}

void uploadToGoogleSheets(String timestamp, float distance, String parkingStatus, float temperature, float humidity, String weatherDesc) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(GOOGLE_SCRIPT_URL);
    http.addHeader("Content-Type", "application/json");

    // Prepare JSON payload with weather data
    String jsonPayload = "{\"timestamp\":\"" + timestamp + 
                         "\",\"distance\":" + String(distance) + 
                         ",\"parkingStatus\":\"" + parkingStatus + 
                         "\",\"temperature\":" + String(temperature) + 
                         ",\"humidity\":" + String(humidity) + 
                         ",\"weather\":\"" + weatherDesc + "\"}";

    int httpResponseCode = http.POST(jsonPayload);
    
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("HTTP Response: " + response);
    } else {
      Serial.print("Error on sending POST: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  }
}