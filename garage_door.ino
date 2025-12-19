#include <Servo.h>

// --- Pin Definitions ---
// Sensor A 
const int trigPinA = 9;
const int echoPinA = 10;

// Sensor B 
const int trigPinB = 2;
const int echoPinB = 5;

// Servo
const int servoPin = 3;

// --- Configuration ---
const int detectionDistance = 20; // Trigger distance in cm

// Objects and Variables
Servo myServo; 
bool isOpen = false;      // Tracks if servo is at 170 (true) or 53 (false)
int openedBySensor = 0;   // 0 = None, 1 = Sensor A, 2 = Sensor B

void setup() {
  Serial.begin(9600);

  // Sensor A Setup
  pinMode(trigPinA, OUTPUT);
  pinMode(echoPinA, INPUT);

  // Sensor B Setup
  pinMode(trigPinB, OUTPUT);
  pinMode(echoPinB, INPUT);

  // Servo Setup
  myServo.attach(servoPin);
  myServo.write(53); // Start at original position
  delay(500);
}

void loop() {
  // Read distances from both sensors
  int distA = getDistance(trigPinA, echoPinA);
  delay(10); // Small delay to prevent ultrasonic interference
  int distB = getDistance(trigPinB, echoPinB);

  // --- LOGIC BLOCK ---

  // SCENARIO 1: Servo is currently CLOSED (53 degrees)
  if (!isOpen) {
    if (distA > 0 && distA < detectionDistance) {
      // Sensor A triggered opening
      openGate(1); // 1 represents Sensor A
    } 
    else if (distB > 0 && distB < detectionDistance) {
      // Sensor B triggered opening
      openGate(2); // 2 represents Sensor B
    }
  }
  
  // SCENARIO 2: Servo is currently OPEN (170 degrees)
  else {
    // If opened by A, wait for B to close it
    if (openedBySensor == 1) {
      if (distB > 0 && distB < detectionDistance) {
        closeGate();
      }
    }
    // If opened by B, wait for A to close it
    else if (openedBySensor == 2) {
      if (distA > 0 && distA < detectionDistance) {
        closeGate();
      }
    }
  }

  delay(100); // Check every 0.1 seconds
}

// --- Helper Functions ---
void openGate(int sensorID) {
  myServo.write(170);
  isOpen = true;
  openedBySensor = sensorID; // Remember who opened it
  delay(1000); // Debounce delay to let the person move past the first sensor
}

void closeGate() {  
  myServo.write(53);
  isOpen = false;
  openedBySensor = 0; // Reset
  delay(1000); // Debounce delay
}

// Function to calculate distance for any sensor
int getDistance(int trig, int echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  long duration = pulseIn(echo, HIGH);
  int distance = duration * 0.034 / 2;
  return distance;
}
