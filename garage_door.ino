/*
 * Elegoo Smart Car V4.0 - COMBINED MODE (Final Mission)
 * Operation: Line Following + Obstacle Overtake + Track Management
 *
 * MISSION LOGIC:
 * 1. Start: Follow the line.
 * 2. Obstacle: If <10cm, perform Box Overtake.
 * 3. Track End (T-Junction): Turn 180 degrees -> STOP for 5 seconds -> Return.
 * 4. Track Start (T-Junction): Stop completely.
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
const int THRESHOLD_LOW   = 200;   // White Floor
const int THRESHOLD_HIGH  = 1100;  // Dark Line
const int SPEED_LINE_FWD  = 80;    // Cruising Speed on Line
const int SPEED_LINE_TURN = 120;   // Corrective Turn Speed

// OBSTACLE SETTINGS
const int OBSTACLE_LIMIT = 10;     // Trigger distance (cm)

// MANEUVER SETTINGS
const int SPEED_MANEUVER  = 150;   // Speed for turning/overtaking
const int TURN_TIME_90    = 365;   // Calibrated for 90 degree turn
const int TURN_TIME_180   = 790;   // Calibrated for 180 degree turn
const int DRIVE_SIDE_TIME = 600;   // Width of the box
const int DRIVE_PASS_TIME = 1200;  // Length of the box

// SAFETY SETTINGS
// Time to ignore markers after starting/turning (19.5 seconds)
const unsigned long MIN_LAP_TIME = 19500; 

// --- 3. GLOBAL VARIABLES ---
int lap_stage = 0; // 0 = Going Out, 1 = Returning, 2 = Finished
unsigned long stage_start_time = 0; // Timer for the current leg

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

  // FORCE START: Drive forward blindly to clear Start Line
  moveCar(SPEED_LINE_FWD, SPEED_LINE_FWD);
  delay(500);
  
  // Start the timer
  stage_start_time = millis();
}

void loop() {
  // Check if mission is complete
  if (lap_stage == 2) {
    stopCar();
    return;
  }

  // 1. Check for Obstacles First (Safety Priority)
  int distance = getDistance();
  
  if (distance > 0 && distance < OBSTACLE_LIMIT) {
    performOvertake();
  } 
  else {
    // 2. Run Line Follower logic
    runLineLogic();
  }
}

// --- CORE BEHAVIORS ---

void runLineLogic() {
  int val_L = analogRead(PIN_ITR_LEFT);
  int val_M = analogRead(PIN_ITR_MIDDLE);
  int val_R = analogRead(PIN_ITR_RIGHT);

  // PRIORITY 1: INTERSECTION (All Black) -> CHECK LAP STAGE
  if (isBlack(val_L) && isBlack(val_M) && isBlack(val_R)) {
    
    // SAFETY CHECK: Have we been driving long enough?
    if (millis() - stage_start_time > MIN_LAP_TIME) {
      
      if (lap_stage == 0) {
        // --- END OF TRACK REACHED ---
        Serial.println("END REACHED -> TURNING 180");
        performUturn();
        
        // ** NEW: PAUSE FOR 5 SECONDS **
        stopCar();
        delay(5000);
        // ******************************
        
        // Reset Logic for Return Trip
        lap_stage = 1; 
        stage_start_time = millis(); 
        
        // Drive forward slightly to ensure we don't re-detect the same line
        moveCar(SPEED_LINE_FWD, SPEED_LINE_FWD);
        delay(300);
      }
      else if (lap_stage == 1) {
        // --- START OF TRACK REACHED (FINISH) ---
        Serial.println("HOME REACHED -> STOPPING");
        stopCar();
        lap_stage = 2; // Mission Complete
      }
    }
    else {
      // Too early to be a real finish line (or car wobbled). Drive over it.
      moveCar(SPEED_LINE_FWD, SPEED_LINE_FWD);
    }
  }
  // PRIORITY 2: CENTERED (Only Middle Black)
  else if (isBlack(val_M) && !isBlack(val_L) && !isBlack(val_R)) {
    moveCar(SPEED_LINE_FWD, SPEED_LINE_FWD);
  }
  // PRIORITY 3: CORRECTIONS
  else if (isBlack(val_R)) {
    turnRight(SPEED_LINE_TURN);
  }
  else if (isBlack(val_L)) {
    turnLeft(SPEED_LINE_TURN);
  }
  // PRIORITY 4: LOOSE CENTER
  else if (isBlack(val_M)) {
    moveCar(SPEED_LINE_FWD, SPEED_LINE_FWD);
  }
  // PRIORITY 5: LOST LINE -> STOP (Safety)
  else {
    stopCar();
  }
}

void performOvertake() {
  stopCar();
  delay(500); 
  
  // 1. Turn Right 90
  turnRight(SPEED_MANEUVER); delay(TURN_TIME_90);
  
  // 2. Drive Side
  moveCar(80, 80); delay(DRIVE_SIDE_TIME);
  
  // 3. Turn Left 90
  turnLeft(SPEED_MANEUVER); delay(TURN_TIME_90);
  
  // 4. Drive Past
  moveCar(80, 80); delay(DRIVE_PASS_TIME);
  
  // 5. Turn Left 90
  turnLeft(SPEED_MANEUVER); delay(TURN_TIME_90);
  
  // 6. Drive Side (Return)
  moveCar(80, 80); delay(DRIVE_SIDE_TIME);
  
  // 7. Turn Right 90 (Face forward)
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
  // Note: No delay here anymore because we have the 5s delay in the main logic
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
