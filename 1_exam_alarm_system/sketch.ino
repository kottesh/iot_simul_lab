#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>

// Initialize LCD (address 0x27, 16 columns, 2 rows)
LiquidCrystal_I2C lcd(0x27, 16, 2);
RTC_DS1307 rtc;

// Pin definitions
#define BUZZER_PIN 9
#define BTN_MODE 2
#define BTN_UP 3
#define BTN_DOWN 4
#define BTN_SET 5

// Configuration variables
enum AlarmPattern { HALF_HOUR, ONE_HOUR, CUSTOM, RANDOM };
AlarmPattern alarmPattern = HALF_HOUR;

enum DisplayMode { ELAPSED, REMAINING };
DisplayMode displayMode = ELAPSED;

enum SoundType { SINGLE_BEEP, DOUBLE_BEEP, LONG_BEEP };
SoundType soundType = SINGLE_BEEP;

// Timing variables
unsigned long examStartTime = 0;
unsigned long examDuration = 3600000; // Default 1 hour in milliseconds
unsigned long lastAlarmTime = 0;
unsigned long nextAlarmTime = 0;
bool examStarted = false;
bool alarmEnabled = true;

// Custom interval (in minutes)
int customInterval = 20; // Default 20 minutes

// Random interval configuration (in minutes)
int randomMinInterval = 15; // Minimum 15 minutes
int randomMaxInterval = 60; // Maximum 60 minutes

// Menu system
enum MenuState { MAIN_SCREEN, MENU_ALARM_PATTERN, MENU_CUSTOM_INTERVAL,
                 MENU_RANDOM_MIN, MENU_RANDOM_MAX, MENU_DISPLAY_MODE, 
                 MENU_SOUND_TYPE, MENU_EXAM_DURATION, MENU_START_EXAM };
MenuState menuState = MAIN_SCREEN;
int menuSelection = 0;

// Button debouncing
unsigned long lastButtonPress = 0;
const unsigned long debounceDelay = 200;

void setup() {
  Serial.begin(9600);
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  
  // Initialize RTC
  if (!rtc.begin()) {
    lcd.setCursor(0, 0);
    lcd.print("RTC Not Found!");
    while (1);
  }
  
  if (!rtc.isrunning()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  
  // Initialize pins
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BTN_MODE, INPUT_PULLUP);
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_SET, INPUT_PULLUP);
  
  digitalWrite(BUZZER_PIN, LOW);
  
  lcd.setCursor(0, 0);
  lcd.print("Exam Alarm Sys");
  lcd.setCursor(0, 1);
  lcd.print("Ready to Config");
  delay(2000);
}

void loop() {
  handleButtons();
  
  if (menuState == MAIN_SCREEN) {
    if (examStarted) {
      displayExamStatus();
      checkAlarm();
    } else {
      displayMainScreen();
    }
  } else {
    displayMenu();
  }
  
  delay(100);
}

void handleButtons() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastButtonPress < debounceDelay) {
    return;
  }
  
  if (digitalRead(BTN_MODE) == LOW) {
    lastButtonPress = currentTime;
    handleModeButton();
  } else if (digitalRead(BTN_UP) == LOW) {
    lastButtonPress = currentTime;
    handleUpButton();
  } else if (digitalRead(BTN_DOWN) == LOW) {
    lastButtonPress = currentTime;
    handleDownButton();
  } else if (digitalRead(BTN_SET) == LOW) {
    lastButtonPress = currentTime;
    handleSetButton();
  }
}

void handleModeButton() {
  if (examStarted) return; // Don't allow menu during exam
  
  if (menuState == MAIN_SCREEN) {
    menuState = MENU_ALARM_PATTERN;
    menuSelection = 0;
  } else {
    // Navigate to next menu
    int nextMenu = (int)menuState + 1;
    
    // Skip menus based on alarm pattern selection
    if (nextMenu == MENU_CUSTOM_INTERVAL && alarmPattern != CUSTOM) {
      nextMenu++; // Skip custom interval if not selected
    }
    if (nextMenu == MENU_RANDOM_MIN && alarmPattern != RANDOM) {
      nextMenu += 2; // Skip both random menus if not selected
    } else if (nextMenu == MENU_RANDOM_MAX && alarmPattern != RANDOM) {
      nextMenu++; // Skip random max if not selected
    }
    
    if (nextMenu > MENU_START_EXAM) {
      nextMenu = MENU_ALARM_PATTERN;
      menuSelection = 0;
    }
    
    menuState = (MenuState)nextMenu;
  }
  lcd.clear();
}

void handleUpButton() {
  if (menuState == MAIN_SCREEN) return;
  
  switch (menuState) {
    case MENU_ALARM_PATTERN:
      alarmPattern = (AlarmPattern)((alarmPattern + 1) % 4);
      break;
      
    case MENU_CUSTOM_INTERVAL:
      customInterval += 1; // Increment by 5 minutes
      if (customInterval > 180) customInterval = 180; // Max 3 hours
      break;
      
    case MENU_RANDOM_MIN:
      randomMinInterval += 5; // Increment by 5 minutes
      if (randomMinInterval > 120) randomMinInterval = 120; // Max 2 hours
      if (randomMinInterval >= randomMaxInterval) {
        randomMaxInterval = randomMinInterval + 5;
      }
      break;
      
    case MENU_RANDOM_MAX:
      randomMaxInterval += 5; // Increment by 5 minutes
      if (randomMaxInterval > 180) randomMaxInterval = 180; // Max 3 hours
      break;
      
    case MENU_DISPLAY_MODE:
      displayMode = (DisplayMode)((displayMode + 1) % 2);
      break;
      
    case MENU_SOUND_TYPE:
      soundType = (SoundType)((soundType + 1) % 3);
      break;
      
    case MENU_EXAM_DURATION:
      examDuration += 300000; // Add 5 minutes
      if (examDuration > 14400000) examDuration = 14400000; // Max 4 hours
      break;
  }
}

void handleDownButton() {
  if (menuState == MAIN_SCREEN) return;
  
  switch (menuState) {
    case MENU_ALARM_PATTERN:
      alarmPattern = (AlarmPattern)((alarmPattern - 1 + 4) % 4);
      break;
      
    case MENU_CUSTOM_INTERVAL:
      if (customInterval > 0) {
        customInterval -= 1; // Decrement by 5 minutes
      }
      break;
      
    case MENU_RANDOM_MIN:
      if (randomMinInterval > 5) {
        randomMinInterval -= 5; // Decrement by 5 minutes
      }
      break;
      
    case MENU_RANDOM_MAX:
      randomMaxInterval -= 5; // Decrement by 5 minutes
      if (randomMaxInterval < randomMinInterval + 5) {
        randomMaxInterval = randomMinInterval + 5;
      }
      if (randomMaxInterval < 10) randomMaxInterval = 10;
      break;
      
    case MENU_DISPLAY_MODE:
      displayMode = (DisplayMode)((displayMode - 1 + 2) % 2);
      break;
      
    case MENU_SOUND_TYPE:
      soundType = (SoundType)((soundType - 1 + 3) % 3);
      break;
      
    case MENU_EXAM_DURATION:
      if (examDuration > 300000) {
        examDuration -= 300000; // Subtract 5 minutes
      }
      break;
  }
}

void handleSetButton() {
  if (!examStarted) {
    startExam();
    menuState = MAIN_SCREEN;
  } else if (examStarted) {
    stopExam();
  }
  lcd.clear();
}

void displayMainScreen() {
  DateTime now = rtc.now();
  
  lcd.setCursor(0, 0);
  lcd.print("Time: ");
  printTime(now.hour(), now.minute(), now.second());
  
  lcd.setCursor(0, 1);
  lcd.print("Press MODE menu");
}

void displayMenu() {
  lcd.setCursor(0, 0);
  
  switch (menuState) {
    case MENU_ALARM_PATTERN:
      lcd.print("Alarm Pattern: ");
      lcd.setCursor(0, 1);
      if (alarmPattern == HALF_HOUR) lcd.print("Every Half Hour");
      else if (alarmPattern == ONE_HOUR) lcd.print("Every One Hour ");
      else if (alarmPattern == CUSTOM) lcd.print("Custom Interval");
      else lcd.print("Random Interval");
      break;
      
    case MENU_CUSTOM_INTERVAL:
      lcd.print("Custom Interval:");
      lcd.setCursor(0, 1);
      lcd.print(customInterval);
      lcd.print(" Minutes      ");
      break;
      
    case MENU_RANDOM_MIN:
      lcd.print("Random Min Int:");
      lcd.setCursor(0, 1);
      lcd.print(randomMinInterval);
      lcd.print(" Minutes      ");
      break;
      
    case MENU_RANDOM_MAX:
      lcd.print("Random Max Int:");
      lcd.setCursor(0, 1);
      lcd.print(randomMaxInterval);
      lcd.print(" Minutes      ");
      break;
      
    case MENU_DISPLAY_MODE:
      lcd.print("Display Mode:  ");
      lcd.setCursor(0, 1);
      if (displayMode == ELAPSED) lcd.print("Elapsed Time   ");
      else lcd.print("Remaining Time ");
      break;
      
    case MENU_SOUND_TYPE:
      lcd.print("Sound Type:    ");
      lcd.setCursor(0, 1);
      if (soundType == SINGLE_BEEP) lcd.print("Single Beep    ");
      else if (soundType == DOUBLE_BEEP) lcd.print("Double Beep    ");
      else lcd.print("Long Beep      ");
      break;
      
    case MENU_EXAM_DURATION:
      lcd.print("Exam Duration: ");
      lcd.setCursor(0, 1);
      unsigned long hours = examDuration / 3600000;
      unsigned long mins = (examDuration % 3600000) / 60000;
      lcd.print(hours);
      lcd.print(" Hrs ");
      lcd.print(mins);
      lcd.print(" Mins  ");
      break;
  }
}

void displayExamStatus() {
  unsigned long currentTime = millis();
  unsigned long elapsed = currentTime - examStartTime;
  
  lcd.setCursor(0, 0);
  
  if (displayMode == ELAPSED) {
    lcd.print("Elapsed: ");
    printDuration(elapsed);
  } else {
    lcd.print("Remain:  ");
    if (elapsed < examDuration) {
      printDuration(examDuration - elapsed);
    } else {
      lcd.print("00:00   ");
    }
  }
  
  lcd.setCursor(0, 1);
  DateTime now = rtc.now();
  lcd.print("Time: ");
  printTime(now.hour(), now.minute(), now.second());
  
  // Check if exam is over
  if (elapsed >= examDuration) {
    examFinished();
  }
}

void printTime(int h, int m, int s) {
  if (h < 10) lcd.print("0");
  lcd.print(h);
  lcd.print(":");
  if (m < 10) lcd.print("0");
  lcd.print(m);
  lcd.print(":");
  if (s < 10) lcd.print("0");
  lcd.print(s);
}

void printDuration(unsigned long duration) {
  unsigned long hours = duration / 3600000;
  unsigned long mins = (duration % 3600000) / 60000;
  
  if (hours < 10) lcd.print("0");
  lcd.print(hours);
  lcd.print(":");
  if (mins < 10) lcd.print("0");
  lcd.print(mins);
  lcd.print("   ");
}

void startExam() {
  examStarted = true;
  examStartTime = millis();
  lastAlarmTime = examStartTime;
  
  // Set next alarm based on pattern
  if (alarmPattern == HALF_HOUR) {
    nextAlarmTime = examStartTime + 1800000; // 30 minutes
  } else if (alarmPattern == ONE_HOUR) {
    nextAlarmTime = examStartTime + 3600000; // 60 minutes
  } else if (alarmPattern == CUSTOM) {
    nextAlarmTime = examStartTime + (customInterval * 60000UL); // Custom minutes
  } else { // RANDOM
    unsigned long minMs = randomMinInterval * 60000UL;
    unsigned long maxMs = randomMaxInterval * 60000UL;
    nextAlarmTime = examStartTime + random(minMs, maxMs);
  }
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Exam Started!");
  lcd.setCursor(0, 1);
  lcd.print("Good Luck!");
  delay(2000);
  lcd.clear();
}

void stopExam() {
  examStarted = false;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Exam Stopped!");
  delay(2000);
  lcd.clear();
}

void examFinished() {
  examStarted = false;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Exam Finished!");
  lcd.setCursor(0, 1);
  lcd.print("Well Done!");
  playSound();
  delay(3000);
  lcd.clear();
}

void checkAlarm() {
  unsigned long currentTime = millis();
  
  if (currentTime >= nextAlarmTime && alarmEnabled) {
    triggerAlarm();
    
    // Set next alarm
    lastAlarmTime = currentTime;
    if (alarmPattern == HALF_HOUR) {
      nextAlarmTime = currentTime + 1800000;
    } else if (alarmPattern == ONE_HOUR) {
      nextAlarmTime = currentTime + 3600000;
    } else if (alarmPattern == CUSTOM) {
      nextAlarmTime = currentTime + (customInterval * 60000UL);
    } else { // RANDOM
      unsigned long minMs = randomMinInterval * 60000UL;
      unsigned long maxMs = randomMaxInterval * 60000UL;
      nextAlarmTime = currentTime + random(minMs, maxMs);
    }
  }
}

void triggerAlarm() {
  playSound();
}

void playSound() {
  switch (soundType) {
    case SINGLE_BEEP:
      tone(BUZZER_PIN, 1000, 200);
      break;
      
    case DOUBLE_BEEP:
      tone(BUZZER_PIN, 1000, 200);
      delay(800);
      tone(BUZZER_PIN, 1000, 200);
      break;
      
    case LONG_BEEP:
      tone(BUZZER_PIN, 1000, 1000);
      break;
  }
}
