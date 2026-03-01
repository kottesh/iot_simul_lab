#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>

// Initialize RTC and LCD
RTC_DS3231 rtc;
LiquidCrystal_I2C lcd(0x27, 16, 2); // Address 0x27, 16x2 LCD

// Traffic Light Pin Definitions
// Direction 1 (North)
const int N_RED = 2;
const int N_YELLOW = 3;
const int N_GREEN = 4;

// Direction 2 (South)
const int S_RED = 5;
const int S_YELLOW = 6;
const int S_GREEN = 7;

// Direction 3 (East)
const int E_RED = 8;
const int E_YELLOW = 9;
const int E_GREEN = 10;

// Direction 4 (West)
const int W_RED = 11;
const int W_YELLOW = 12;
const int W_GREEN = 13;

// Button Pins
const int BTN_MENU = A0;
const int BTN_UP = A1;
const int BTN_DOWN = A2;

// Timing Configuration (in seconds)
int peakGreenTime = 20;      // Green light during peak hours
int normalGreenTime = 30;    // Green light during normal hours
int yellowTime = 3;          // Yellow light duration
int allRedTime = 2;          // All red safety time

// Peak Hours Configuration (24-hour format)
int peakStartHour = 7;       // Peak hours start at 7:00 AM
int peakEndHour = 10;        // Peak hours end at 10:00 AM

// System Variables
unsigned long previousMillis = 0;
int currentPhase = 0;        // 0=N/S Green, 1=N/S Yellow, 2=All Red, 3=E/W Green, 4=E/W Yellow, 5=All Red
int phaseTimer = 0;
bool isPeakHour = false;
int currentGreenTime = normalGreenTime;

// Menu System
enum MenuState {
  MENU_MAIN,
  MENU_PEAK_GREEN,
  MENU_NORMAL_GREEN,
  MENU_YELLOW,
  MENU_PEAK_START,
  MENU_PEAK_END
};

MenuState currentMenu = MENU_MAIN;
bool inConfigMode = false;
unsigned long lastButtonPress = 0;
const unsigned long buttonDebounce = 200;

void setup() {
  Serial.begin(9600);
  
  // Initialize RTC
  if (!rtc.begin()) {
    Serial.println("RTC not found!");
    while (1);
  }
  
  // If RTC lost power, set to compile time
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Traffic System");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");
  delay(2000);
  
  // Initialize Traffic Light Pins
  pinMode(N_RED, OUTPUT);
  pinMode(N_YELLOW, OUTPUT);
  pinMode(N_GREEN, OUTPUT);
  pinMode(S_RED, OUTPUT);
  pinMode(S_YELLOW, OUTPUT);
  pinMode(S_GREEN, OUTPUT);
  pinMode(E_RED, OUTPUT);
  pinMode(E_YELLOW, OUTPUT);
  pinMode(E_GREEN, OUTPUT);
  pinMode(W_RED, OUTPUT);
  pinMode(W_YELLOW, OUTPUT);
  pinMode(W_GREEN, OUTPUT);
  
  // Initialize Button Pins with internal pull-up
  pinMode(BTN_MENU, INPUT_PULLUP);
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  
  // Start with all red
  allRed();
  
  lcd.clear();
}

void loop() {
  // Check buttons
  checkButtons();
  
  if (inConfigMode) {
    handleConfigMenu();
  } else {
    // Normal traffic light operation
    runTrafficSequence();
    displayStatus();
  }
}

void runTrafficSequence() {
  DateTime now = rtc.now();
  
  // Check if current time is in peak hours
  int currentHour = now.hour();
  isPeakHour = (currentHour >= peakStartHour && currentHour < peakEndHour);
  currentGreenTime = isPeakHour ? peakGreenTime : normalGreenTime;
  
  unsigned long currentMillis = millis();
  
  // Update phase based on timer
  if (currentMillis - previousMillis >= 1000) { // Every second
    previousMillis = currentMillis;
    phaseTimer++;
    
    // Phase transitions
    switch (currentPhase) {
      case 0: // North/South Green
        if (phaseTimer >= currentGreenTime) {
          phaseTimer = 0;
          currentPhase = 1;
        }
        break;
        
      case 1: // North/South Yellow
        if (phaseTimer >= yellowTime) {
          phaseTimer = 0;
          currentPhase = 2;
        }
        break;
        
      case 2: // All Red (transition)
        if (phaseTimer >= allRedTime) {
          phaseTimer = 0;
          currentPhase = 3;
        }
        break;
        
      case 3: // East/West Green
        if (phaseTimer >= currentGreenTime) {
          phaseTimer = 0;
          currentPhase = 4;
        }
        break;
        
      case 4: // East/West Yellow
        if (phaseTimer >= yellowTime) {
          phaseTimer = 0;
          currentPhase = 5;
        }
        break;
        
      case 5: // All Red (transition)
        if (phaseTimer >= allRedTime) {
          phaseTimer = 0;
          currentPhase = 0;
        }
        break;
    }
    
    // Update lights based on current phase
    updateLights();
  }
}

void updateLights() {
  switch (currentPhase) {
    case 0: // North/South Green, East/West Red
      digitalWrite(N_GREEN, HIGH);
      digitalWrite(N_YELLOW, LOW);
      digitalWrite(N_RED, LOW);
      digitalWrite(S_GREEN, HIGH);
      digitalWrite(S_YELLOW, LOW);
      digitalWrite(S_RED, LOW);
      digitalWrite(E_RED, HIGH);
      digitalWrite(E_YELLOW, LOW);
      digitalWrite(E_GREEN, LOW);
      digitalWrite(W_RED, HIGH);
      digitalWrite(W_YELLOW, LOW);
      digitalWrite(W_GREEN, LOW);
      break;
      
    case 1: // North/South Yellow, East/West Red
      digitalWrite(N_GREEN, LOW);
      digitalWrite(N_YELLOW, HIGH);
      digitalWrite(S_GREEN, LOW);
      digitalWrite(S_YELLOW, HIGH);
      break;
      
    case 2: // All Red
      allRed();
      break;
      
    case 3: // East/West Green, North/South Red
      digitalWrite(N_RED, HIGH);
      digitalWrite(N_YELLOW, LOW);
      digitalWrite(N_GREEN, LOW);
      digitalWrite(S_RED, HIGH);
      digitalWrite(S_YELLOW, LOW);
      digitalWrite(S_GREEN, LOW);
      digitalWrite(E_GREEN, HIGH);
      digitalWrite(E_YELLOW, LOW);
      digitalWrite(E_RED, LOW);
      digitalWrite(W_GREEN, HIGH);
      digitalWrite(W_YELLOW, LOW);
      digitalWrite(W_RED, LOW);
      break;
      
    case 4: // East/West Yellow, North/South Red
      digitalWrite(E_GREEN, LOW);
      digitalWrite(E_YELLOW, HIGH);
      digitalWrite(W_GREEN, LOW);
      digitalWrite(W_YELLOW, HIGH);
      break;
      
    case 5: // All Red
      allRed();
      break;
  }
}

void allRed() {
  digitalWrite(N_RED, HIGH);
  digitalWrite(N_YELLOW, LOW);
  digitalWrite(N_GREEN, LOW);
  digitalWrite(S_RED, HIGH);
  digitalWrite(S_YELLOW, LOW);
  digitalWrite(S_GREEN, LOW);
  digitalWrite(E_RED, HIGH);
  digitalWrite(E_YELLOW, LOW);
  digitalWrite(E_GREEN, LOW);
  digitalWrite(W_RED, HIGH);
  digitalWrite(W_YELLOW, LOW);
  digitalWrite(W_GREEN, LOW);
}

void displayStatus() {
  DateTime now = rtc.now();
  
  lcd.setCursor(0, 0);
  lcd.print(now.hour() < 10 ? "0" : "");
  lcd.print(now.hour());
  lcd.print(":");
  lcd.print(now.minute() < 10 ? "0" : "");
  lcd.print(now.minute());
  lcd.print(" ");
  lcd.print(isPeakHour ? "PEAK" : "NORM");
  lcd.print("   ");
  
  lcd.setCursor(0, 1);
  switch (currentPhase) {
    case 0:
      lcd.print("N/S:GRN E/W:RED ");
      break;
    case 1:
      lcd.print("N/S:YEL E/W:RED ");
      break;
    case 2:
      lcd.print("ALL RED        ");
      break;
    case 3:
      lcd.print("E/W:GRN N/S:RED ");
      break;
    case 4:
      lcd.print("E/W:YEL N/S:RED ");
      break;
    case 5:
      lcd.print("ALL RED        ");
      break;
  }
}

void checkButtons() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastButtonPress < buttonDebounce) {
    return;
  }
  
  if (digitalRead(BTN_MENU) == LOW) {
    lastButtonPress = currentTime;
    if (!inConfigMode) {
      inConfigMode = true;
      currentMenu = MENU_MAIN;
      lcd.clear();
    } else {
      // Cycle through menu options
      currentMenu = (MenuState)((currentMenu + 1) % 6);
      
      if (currentMenu == MENU_MAIN) {
        inConfigMode = false;
        lcd.clear();
      }
    }
  }
  
  if (inConfigMode) {
    if (digitalRead(BTN_UP) == LOW) {
      lastButtonPress = currentTime;
      adjustValue(1);
    }
    
    if (digitalRead(BTN_DOWN) == LOW) {
      lastButtonPress = currentTime;
      adjustValue(-1);
    }
  }
}

void adjustValue(int direction) {
  switch (currentMenu) {
    case MENU_PEAK_GREEN:
      peakGreenTime = constrain(peakGreenTime + direction, 5, 120);
      break;
    case MENU_NORMAL_GREEN:
      normalGreenTime = constrain(normalGreenTime + direction, 5, 120);
      break;
    case MENU_YELLOW:
      yellowTime = constrain(yellowTime + direction, 1, 10);
      break;
    case MENU_PEAK_START:
      peakStartHour = constrain(peakStartHour + direction, 0, 23);
      break;
    case MENU_PEAK_END:
      peakEndHour = constrain(peakEndHour + direction, 0, 23);
      break;
    default:
      break;
  }
}

void handleConfigMenu() {
  lcd.setCursor(0, 0);
  
  switch (currentMenu) {
    case MENU_MAIN:
      lcd.print("Config Menu     ");
      lcd.setCursor(0, 1);
      lcd.print("Press Menu Btn  ");
      break;
      
    case MENU_PEAK_GREEN:
      lcd.print("Peak Green Time ");
      lcd.setCursor(0, 1);
      lcd.print(peakGreenTime);
      lcd.print(" sec    ");
      break;
      
    case MENU_NORMAL_GREEN:
      lcd.print("Normal Grn Time ");
      lcd.setCursor(0, 1);
      lcd.print(normalGreenTime);
      lcd.print(" sec    ");
      break;
      
    case MENU_YELLOW:
      lcd.print("Yellow Time     ");
      lcd.setCursor(0, 1);
      lcd.print(yellowTime);
      lcd.print(" sec    ");
      break;
      
    case MENU_PEAK_START:
      lcd.print("Peak Start Hour ");
      lcd.setCursor(0, 1);
      lcd.print(peakStartHour);
      lcd.print(":00    ");
      break;
      
    case MENU_PEAK_END:
      lcd.print("Peak End Hour   ");
      lcd.setCursor(0, 1);
      lcd.print(peakEndHour);
      lcd.print(":00    ");
      break;
  }
}