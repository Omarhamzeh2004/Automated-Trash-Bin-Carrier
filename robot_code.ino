/*
 * Elegoo Smart Car V4.0 - COMBINED MODE (Final Mission)
 * Operation: Line Following + Obstacle Overtake + Track Management
 * * Logic:
 * 1. Default: Follow the black line.
 * 2. Obstacle: If <15cm, Perform Box Overtake.
 * 3. Track End (T-Junction): Turn 180 degrees.
 * 4. Track Start (T-Junction): Stop.
 * * SAFETY:
 * - Uses MIN_LAP_TIME to prevent false finishes if the car wobbles early in the track.
 */

// --- 1. PIN DEFINITIONS ---
// Line Sensors
const int PIN_ITR_LEFT   = A2;
const int PIN_ITR_MIDDLE = A1;
const int PIN_ITR_RIGHT  = A0;

// Ultrasonic Sensor
const int PIN_TRIG = 13;
const int PIN_ECHO = 12;

// Motors (TB6612 Driver)
const int PIN_STBY = 3; 
const int PIN_PWMA = 5;  // Right Speed
const int PIN_AIN1 = 7;  // Right Dir
const int PIN_PWMB = 6;  // Left Speed
const int PIN_BIN1 = 8;  // Left Dir

// --- 2. SETTINGS (CALIBRATED) ---

// LINE FOLLOWER SETTINGS
const int THRESHOLD_LOW  = 200;  // White Floor
const int THRESHOLD_HIGH = 1100; // Dark Line
const int SPEED_LINE_FWD = 75;   // Cruising Speed on Line
const int SPEED_LINE_TURN = 120; // Corrective Turn Speed

// OBSTACLE SETTINGS
const int OBSTACLE_LIMIT = 10;   // Trigger distance (cm)

// MANEUVER SETTINGS
const int SPEED_MANEUVER    = 150; // Speed for turning/overtaking
const int TURN_TIME_90      = 370; // Calibrated for 90 degree turn
const int TURN_TIME_180     = 790; // Calibrated for 180 degree turn
const int DRIVE_SIDE_TIME   = 700; // Width of the box
const int DRIVE_PASS_TIME   = 1200;// Length of the box (to pass obstacle)

// SAFETY SETTINGS
const unsigned long MIN_LAP_TIME = 19500; // 5 Seconds. Ignore markers for this long after starting/turning.

// --- 3. GLOBAL VARIABLES ---
int lap_stage = 0; // 0 = Going Out, 1 = Returning, 2 = Finished
unsigned long stage_start_time = 0; // When did we start the current leg?

void setup() {
  Serial.begin(9600);

  // Init Sensors
  pinMode(PIN_ITR_LEFT, INPUT);
  pinMode(PIN_ITR_MIDDLE, INPUT);
  pinMode(PIN_ITR_RIGHT, INPUT);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  
  // Init Motors
  pinMode(PIN_STBY, OUTPUT);
  pinMode(PIN_PWMA, OUTPUT);
  pinMode(PIN_AIN1, OUTPUT);
  pinMode(PIN_PWMB, OUTPUT);
  pinMode(PIN_BIN1, OUTPUT);
  
  // Enable Motors
  digitalWrite(PIN_STBY, HIGH);
  
  Serial.println("--- Combined Mode Started ---");
  delay(2000); 

  // FORCE START
  moveCar(SPEED_LINE_FWD, SPEED_LINE_FWD);
  delay(500);
  
  // Start the timer for the first leg
  stage_start_time = millis();
}

void loop() {
  if (lap_stage == 2) {
    stopCar();
    return;
  }

  // 1. Check for Obstacles First (Safety Priority)
  int distance = getDistance();
  
  if (distance > 0 && distance < OBSTACLE_LIMIT) {
    // OBSTACLE DETECTED!
    performOvertake();
  } 
  else {
    // 2. Run Line Follower (with Marker Detection)
    runLineFollower();
  }
}

// --- CORE BEHAVIORS ---

void runLineFollower() {
  int val_L = analogRead(PIN_ITR_LEFT);
  int val_M = analogRead(PIN_ITR_MIDDLE);
  int val_R = analogRead(PIN_ITR_RIGHT);

  // PRIORITY 1: INTERSECTION (All Black) -> CHECK LAP STAGE
  if (isBlack(val_L) && isBlack(val_M) && isBlack(val_R)) {
    
    // SAFETY CHECK: Have we been driving long enough?
    if (millis() - stage_start_time > MIN_LAP_TIME) {
      
      if (lap_stage == 0) {
        // We reached the END of the track -> Turn 180
        Serial.println("END REACHED -> TURNING 180");
        performUturn();
        
        // Reset Logic for Return Trip
        lap_stage = 1; 
        stage_start_time = millis(); // Reset timer so we don't detect the same line instantly
      }
      else if (lap_stage == 1) {
        // We reached the START of the track -> Stop
        Serial.println("HOME REACHED -> STOPPING");
        stopCar();
        lap_stage = 2; // Mission Complete
      }
    }
    else {
      // It's too early to be the finish line (false alarm or wobble). Drive over it.
      moveCar(SPEED_LINE_FWD, SPEED_LINE_FWD);
    }
  }
  // PRIORITY 2: CENTERED (Only Middle Black) -> GO FORWARD
  else if (isBlack(val_M) && !isBlack(val_L) && !isBlack(val_R)) {
    moveCar(SPEED_LINE_FWD, SPEED_LINE_FWD);
  }
  // PRIORITY 3: RIGHT SENSOR TRIGGERED -> TURN RIGHT
  else if (isBlack(val_R)) {
    turnRight(SPEED_LINE_TURN);
  }
  // PRIORITY 4: LEFT SENSOR TRIGGERED -> TURN LEFT
  else if (isBlack(val_L)) {
    turnLeft(SPEED_LINE_TURN);
  }
  // PRIORITY 5: LOOSE CENTER -> GO FORWARD
  else if (isBlack(val_M)) {
    moveCar(SPEED_LINE_FWD, SPEED_LINE_FWD);
  }
  // PRIORITY 6: LOST LINE -> STOP (Safety)
  else {
    stopCar();
  }
}

void performOvertake() {
  stopCar();
  delay(500); 
  
  // 1. Right 90
  turnRight(SPEED_MANEUVER); delay(TURN_TIME_90);
  
  // 2. Side
  moveCar(80, 80); delay(DRIVE_SIDE_TIME);
  
  // 3. Left 90
  turnLeft(SPEED_MANEUVER); delay(TURN_TIME_90);
  
  // 4. Pass
  moveCar(80, 80); delay(DRIVE_PASS_TIME);
  
  // 5. Left 90
  turnLeft(SPEED_MANEUVER); delay(TURN_TIME_90);
  
  // 6. Return Side
  moveCar(80, 80); delay(DRIVE_SIDE_TIME);
  
  // 7. Right 90
  turnRight(SPEED_MANEUVER); delay(TURN_TIME_90);
  
  stopCar();
  delay(500);
}

void performUturn() {
  stopCar();
  delay(500);
  
  // Spin 180
  digitalWrite(PIN_AIN1, HIGH); analogWrite(PIN_PWMA, SPEED_MANEUVER); // Right Fwd
  digitalWrite(PIN_BIN1, LOW);  analogWrite(PIN_PWMB, SPEED_MANEUVER); // Left Back
  
  delay(TURN_TIME_180);
  
  stopCar();
  delay(500);
  
  // Drive forward blindly a bit to clear the marker we just turned on
  moveCar(SPEED_LINE_FWD, SPEED_LINE_FWD);
  delay(300);
}

// --- HELPER FUNCTIONS ---

bool isBlack(int value) {
  return (value > THRESHOLD_LOW && value < THRESHOLD_HIGH);
}

int getDistance() {
  digitalWrite(PIN_TRIG, LOW); delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  long duration = pulseIn(PIN_ECHO, HIGH, 30000); 
  if (duration == 0) return 200; 
  return duration * 0.034 / 2;
}

// Unified Motor Control
void moveCar(int speedLeft, int speedRight) {
  digitalWrite(PIN_AIN1, HIGH); analogWrite(PIN_PWMA, speedRight);
  digitalWrite(PIN_BIN1, HIGH); analogWrite(PIN_PWMB, speedLeft);
}

void turnRight(int speed) {
  digitalWrite(PIN_AIN1, LOW);  analogWrite(PIN_PWMA, speed);
  digitalWrite(PIN_BIN1, HIGH); analogWrite(PIN_PWMB, speed);
}

void turnLeft(int speed) {
  digitalWrite(PIN_AIN1, HIGH); analogWrite(PIN_PWMA, speed);
  digitalWrite(PIN_BIN1, LOW);  analogWrite(PIN_PWMB, speed);
}

void stopCar() {
  analogWrite(PIN_PWMA, 0);
  analogWrite(PIN_PWMB, 0);
}