#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <DHT.h>

// Function declarations
void uploadToGoogleSheets(String timestamp, float distance, String parkingStatus, float temperature, float humidity);

// Initialize DHT sensor
#define DHTPIN 4       // DHT11 connected to GPIO 4
#define DHTTYPE DHT11  // Changed to DHT11
DHT dht(DHTPIN, DHTTYPE);

// WiFi Credentials
const char* WIFI_SSID = "---";     // Replace with your WiFi name
const char* WIFI_PASSWORD = "---";   // Replace with your WiFi password

// Google Apps Script Web App URL
const String GOOGLE_SCRIPT_URL = "----";  // Replace with your new URL

// Ultrasonic Sensor Pins
#define TRIG_PIN 2
#define ECHO_PIN 9
#define SOUND_SPEED 0.0343 

// NTP Client setup
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "uk.pool.ntp.org", 0);  // 3600 = UTC+1 for British summer time

// Variables
long duration;
float distanceCm;
unsigned long lastUploadTime = 0;
const long UPLOAD_INTERVAL = 30000;    // Upload every 30 seconds

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting...");
  
  // Initialize DHT sensor
  dht.begin();
  Serial.println("DHT11 sensor initialized");
  
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Initialize NTP Client with more detailed setup
  timeClient.begin();
  timeClient.setTimeOffset(0);     // UTC+0 for London in winter
  timeClient.setUpdateInterval(60000); // Update every minute
  
  // Force time update
  while(!timeClient.update()) {
    timeClient.forceUpdate();
    delay(500);
  }
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

  // Read DHT11 sensor
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  
  // Update Time
  timeClient.update();
  String formattedTime = timeClient.getFormattedTime();

  // Determine Parking Status
  String parkingStatus = (distanceCm < 20) ? "Occupied" : "Free";

  // Check if DHT reading failed
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
  } else {
    // Upload to Google Sheets periodically
    if (millis() - lastUploadTime > UPLOAD_INTERVAL) {
      uploadToGoogleSheets(formattedTime, distanceCm, parkingStatus, temperature, humidity);
      lastUploadTime = millis();
    }

    // Debug output
    Serial.print("Time: ");
    Serial.print(formattedTime);
    Serial.print(" | Distance: ");
    Serial.print(distanceCm);
    Serial.print(" cm | Status: ");
    Serial.print(parkingStatus);
    Serial.print(" | Temp: ");
    Serial.print(temperature);
    Serial.print("°C | Humidity: ");
    Serial.print(humidity);
    Serial.println("%");
  }

  delay(2000); // Wait for 2 seconds between measurements
}

void uploadToGoogleSheets(String timestamp, float distance, String parkingStatus, float temperature, float humidity) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.begin(GOOGLE_SCRIPT_URL);
    http.addHeader("Content-Type", "application/json");

    // Fixed JSON format (removed extra quote)
    String jsonPayload = "{\"timestamp\":\"" + timestamp + 
                         "\",\"distance\":" + String(distance, 2) + 
                         ",\"parkingStatus\":\"" + parkingStatus + 
                         "\",\"temperature\":" + String(temperature, 2) + 
                         ",\"humidity\":" + String(humidity, 2) + 
                         "}";

    Serial.print("\nSending: ");
    Serial.println(jsonPayload);

    int httpResponseCode = http.POST(jsonPayload);
    
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Response: " + response);
    } else {
      Serial.print("Error: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  }
}