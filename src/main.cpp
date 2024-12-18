#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// Function declaration
void uploadToGoogleSheets(String timestamp, float distance, String parkingStatus);

// WiFi Credentials
const char* WIFI_SSID = "your_wifi_name";
const char* WIFI_PASSWORD = "your_wifi_pass";

// Google Apps Script Web App URL
const String GOOGLE_SCRIPT_URL = "Put the google sheets web app url";

// Ultrasonic Sensor Pins
#define TRIG_PIN 2
#define ECHO_PIN 9
#define SOUND_SPEED 0.0343  // Speed of sound in cm/μs

// NTP Client setup
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600); // 3600 = UTC+1

// Variables
long duration;
float distanceCm;
unsigned long lastUploadTime = 0;
const long UPLOAD_INTERVAL = 30000;  // Upload every 30 seconds

void setup() {
  Serial.begin(115200);
  delay(1000); // Give serial time to initialize
  Serial.println("Starting...");
  
  // Ultrasonic Sensor Setup
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // WiFi Connection
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Initialize Time Client
  timeClient.begin();
  timeClient.setTimeOffset(3600); // 3600 seconds = 1 hour ahead of UTC
  Serial.println("NTP Client initialized");
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

  // Determine Parking Status
  String parkingStatus = (distanceCm < 20) ? "Occupied" : "Free";

  // Upload to Google Sheets periodically
  if (millis() - lastUploadTime > UPLOAD_INTERVAL) {
    uploadToGoogleSheets(formattedTime, distanceCm, parkingStatus);
    lastUploadTime = millis();
  }

  // Debug output
  Serial.print("Distance: ");
  Serial.print(distanceCm);
  Serial.print(" cm | Status: ");
  Serial.print(parkingStatus);
  Serial.print(" | Time: ");
  Serial.println(formattedTime);

  delay(1000);
}

void uploadToGoogleSheets(String timestamp, float distance, String parkingStatus) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(GOOGLE_SCRIPT_URL);
    http.addHeader("Content-Type", "application/json");

    // Prepare JSON payload
    String jsonPayload = "{\"timestamp\":\"" + timestamp + 
                         "\",\"distance\":" + String(distance) + 
                         ",\"parkingStatus\":\"" + parkingStatus + "\"}";

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