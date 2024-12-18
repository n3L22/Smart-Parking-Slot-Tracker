
#include <Arduino.h>

#define TRIG_PIN 2
#define ECHO_PIN 9
#define SOUND_SPEED 0.0343  // Speed of sound in cm/μs

long duration;
float distanceCm;
unsigned long lastPrintTime = 0;  // Store the last print time
bool isOn = false; // Current state flag

void setup() {
  Serial.begin(115200);  // Initialize Serial Monitor
  pinMode(TRIG_PIN, OUTPUT);  // TRIG as Output
  pinMode(ECHO_PIN, INPUT);   // ECHO as Input
}

void loop() {
  // Clear the TRIG pin
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  // Trigger the ultrasonic pulse
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Measure the pulse duration
  duration = pulseIn(ECHO_PIN, HIGH);

  // Calculate the distance in cm
  distanceCm = (duration * SOUND_SPEED) / 2;

  // Print the calculated distance
  Serial.print("Distance (Cm): ");
  Serial.println(distanceCm);

  delay(1000);  // Wait for 1 second
}
