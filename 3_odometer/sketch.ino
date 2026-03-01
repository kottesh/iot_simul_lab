float targetSpeed = 0.5;         // m/s (adjustable)
float maxSpeed = 5000.0;            // m/s (increased limit)
float minSpeed = 0.1;            // m/s
float speedStep = 0.2;           // m/s increment#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// === PIN DEFINITIONS ===
#define STEP_PIN 8
#define DIR_PIN 9
#define ENABLE_PIN 10
#define RESET_PIN 11

#define BTN1_PIN 2   // Start/Stop
#define BTN2_PIN 3   // Increase Speed
#define BTN3_PIN 4   // Decrease Speed
#define BTN4_PIN 5   // Reset Distance

// Track button states
volatile bool btn2Pressed = false;
volatile bool btn3Pressed = false;
volatile bool btn4Pressed = false;

// === STEPPER MOTOR PARAMETERS ===
#define STEPS_PER_REV 200        // 1.8° per step
#define MICROSTEPS 16            // A4988 microstepping (MS1, MS2, MS3 high)
#define WHEEL_DIAMETER 0.1       // 10cm diameter in meters
#define WHEEL_CIRCUMFERENCE (PI * WHEEL_DIAMETER)
#define STEPS_PER_METER (STEPS_PER_REV * MICROSTEPS) / WHEEL_CIRCUMFERENCE

// === LCD SETUP ===
LiquidCrystal_I2C lcd(0x27, 16, 2); // Address 0x27, 16x2 display

// === VARIABLES ===
volatile long totalSteps = 0;
volatile unsigned long lastStepTime = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastSpeedCalcTime = 0;

float currentSpeed = 0.0;        // m/s
float totalDistance = 0.0;       // meters


bool motorRunning = false;
bool speedChanged = false;

// Button debounce variables
unsigned long lastBtnTime[4] = {0, 0, 0, 0};
#define DEBOUNCE_DELAY 200

// === INTERRUPT HANDLERS ===
void btn1ISR() {
  if (millis() - lastBtnTime[0] < DEBOUNCE_DELAY) return;
  lastBtnTime[0] = millis();
  motorRunning = !motorRunning;
  
  if (motorRunning) {
    digitalWrite(ENABLE_PIN, LOW);  // Enable motor
  } else {
    digitalWrite(ENABLE_PIN, HIGH); // Disable motor
  }
}

void btn2ISR() {
  btn2Pressed = true;
}

void btn3ISR() {
  btn3Pressed = true;
}

void btn4ISR() {
  btn4Pressed = true;
}

// === SETUP ===
void setup() {
  Serial.begin(9600);
  
  // Initialize pins
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(ENABLE_PIN, OUTPUT);
  pinMode(RESET_PIN, OUTPUT);
  
  pinMode(BTN1_PIN, INPUT_PULLUP);
  pinMode(BTN2_PIN, INPUT_PULLUP);
  pinMode(BTN3_PIN, INPUT_PULLUP);
  pinMode(BTN4_PIN, INPUT_PULLUP);
  
  // Initialize motor control
  digitalWrite(RESET_PIN, HIGH);   // Release reset
  digitalWrite(ENABLE_PIN, HIGH);  // Disable motor initially
  digitalWrite(DIR_PIN, HIGH);     // Set direction (CCW)
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.print("Odometer Ready!");
  delay(2000);
  lcd.clear();
  
  // Attach interrupts
  attachInterrupt(digitalPinToInterrupt(BTN1_PIN), btn1ISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(BTN2_PIN), btn2ISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(BTN3_PIN), btn3ISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(BTN4_PIN), btn4ISR, FALLING);
  
  lastDisplayUpdate = millis();
  lastSpeedCalcTime = millis();
}

// === MAIN LOOP ===
void loop() {
  // Handle button presses with debouncing
  handleButtonInputs();
  
  // Step the motor if running
  if (motorRunning) {
    stepMotor();
  }
  
  // Calculate speed (steps per second to m/s)
  unsigned long currentTime = millis();
  if (currentTime - lastSpeedCalcTime >= 500) {
    updateSpeed(currentTime - lastSpeedCalcTime);
    lastSpeedCalcTime = currentTime;
  }
  
  // Update LCD display
  if (currentTime - lastDisplayUpdate >= 200) {
    updateLCD();
    lastDisplayUpdate = currentTime;
  }
}

// === BUTTON HANDLER ===
void handleButtonInputs() {
  static unsigned long lastBtn2Time = 0;
  static unsigned long lastBtn3Time = 0;
  static unsigned long lastBtn4Time = 0;
  unsigned long currentTime = millis();
  
  // Handle button 2 (Increase Speed)
  if (btn2Pressed) {
    if (currentTime - lastBtn2Time >= DEBOUNCE_DELAY) {
      targetSpeed += speedStep;
      if (targetSpeed > maxSpeed) targetSpeed = maxSpeed;
      speedChanged = true;
      lastBtn2Time = currentTime;
    }
    btn2Pressed = false;
  }
  
  // Handle button 3 (Decrease Speed)
  if (btn3Pressed) {
    if (currentTime - lastBtn3Time >= DEBOUNCE_DELAY) {
      targetSpeed -= speedStep;
      if (targetSpeed < minSpeed) targetSpeed = minSpeed;
      speedChanged = true;
      lastBtn3Time = currentTime;
    }
    btn3Pressed = false;
  }
  
  // Handle button 4 (Reset Distance)
  if (btn4Pressed) {
    if (currentTime - lastBtn4Time >= DEBOUNCE_DELAY) {
      totalSteps = 0;
      totalDistance = 0.0;
      currentSpeed = 0.0;
      lastBtn4Time = currentTime;
    }
    btn4Pressed = false;
  }
  
  // Print speed if changed
  if (speedChanged) {
    Serial.print("Target Speed: ");
    Serial.print(targetSpeed, 2);
    Serial.println(" m/s");
    speedChanged = false;
  }
}

// === STEPPER MOTOR CONTROL ===
void stepMotor() {
  // Calculate delay based on target speed
  // Speed (m/s) = steps/second / steps_per_meter
  // steps/second = speed * steps_per_meter
  float stepsPerSecond = targetSpeed * STEPS_PER_METER;
  float delayMicros = 1000000.0 / stepsPerSecond;
  
  unsigned long currentMicros = micros();
  
  // Only step if enough time has passed
  if (currentMicros - lastStepTime >= delayMicros) {
    // Pulse the STEP pin
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(5);
    digitalWrite(STEP_PIN, LOW);
    
    lastStepTime = currentMicros;
    totalSteps++;
    
    // Update distance (in meters)
    totalDistance = (float)totalSteps / STEPS_PER_METER;
  }
}

// === SPEED CALCULATION ===
void updateSpeed(unsigned long deltaTime) {
  // Current speed based on recent step rate
  // This is smoothed over the last update interval
  float deltaSeconds = deltaTime / 1000.0;
  
  // If moving, report target speed; otherwise 0
  if (motorRunning && targetSpeed > 0) {
    currentSpeed = targetSpeed;
  } else {
    currentSpeed = 0.0;
  }
}

// === LCD UPDATE ===
void updateLCD() {
  // Line 1: Speed
  lcd.setCursor(0, 0);
  lcd.print("Speed:");
  lcd.setCursor(7, 0);
  lcd.print(currentSpeed, 2);
  lcd.print(" m/s ");
  
  // Line 2: Distance
  lcd.setCursor(0, 1);
  lcd.print("Dist:");
  lcd.setCursor(6, 1);
  lcd.print(totalDistance, 2);
  lcd.print("m   ");
  
  // Debug output to Serial
  Serial.print("Speed: ");
  Serial.print(currentSpeed, 2);
  Serial.print(" m/s | Distance: ");
  Serial.print(totalDistance, 2);
  Serial.print(" m | Running: ");
  Serial.println(motorRunning ? "YES" : "NO");
}

// === UTILITY FUNCTIONS ===
void displayStatus(const char* msg) {
  lcd.clear();
  lcd.print(msg);
  delay(1000);
}