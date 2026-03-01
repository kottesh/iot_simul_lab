#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED Configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET     -1 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int gasSensorPin = A0;
const int buzzerPin = 8;
const int redPin = 11;
const int greenPin = 10;
const int bluePin = 9;
const int relayPin = 6;    // NEW: Relay Control Pin

const int SAFE_LIMIT = 350;      
const int WARNING_LIMIT = 600;   

int gasValue = 0;

void setup() {
  Serial.begin(9600);
  
  pinMode(gasSensorPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  pinMode(relayPin, OUTPUT); // NEW: Set Relay as output
  
  // Start with Relay OFF (Active LOW usually means LOW = ON, but let's assume HIGH = OFF)
  digitalWrite(relayPin, HIGH); 
  
  // Initialize OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("Sys Initialize...");
  display.println("Exhaust: OFF");
  display.display();
  delay(2000);
  
  setColor(0, 255, 0);
  digitalWrite(buzzerPin, LOW);
}

void loop() {
  gasValue = analogRead(gasSensorPin);
  
  display.clearDisplay(); 
  display.setCursor(0,0);
  
  // Header
  display.setTextSize(1);
  display.println("Gas Safety Unit");
  display.println("----------------");
  display.print("Gas Val: ");
  display.print(gasValue);
  display.println("   ");
  
  if (gasValue >= WARNING_LIMIT) {
    // --- DANGER: ACTIVATE SAFETY SYSTEMS ---
    
    display.setTextColor(BLACK, WHITE); 
    display.println(" ALERT: CRITICAL");
    display.setTextColor(WHITE);
    
    // SAFETY ACTION: Turn Relay ON (Active LOW logic used here)
    digitalWrite(relayPin, LOW); 
    display.println(">> EXHAUST: ON <<"); // Fan is spinning
    
    setColor(255, 0, 0); 
    
    // Alarm
    tone(buzzerPin, 1000);
    delay(200);
    noTone(buzzerPin);
    delay(200);
    
  } else if (gasValue >= SAFE_LIMIT) {
    // --- WARNING ---
    
    display.println(" ALERT: WARNING");
    
    // Keep Exhaust OFF to save power (or ON if you prefer constant ventilation)
    digitalWrite(relayPin, HIGH); 
    display.println("Exhaust: STANDBY");
    
    setColor(255, 255, 0); 
    
    tone(buzzerPin, 800);
    delay(150);
    noTone(buzzerPin);
    delay(850);
    
  } else {
    // --- SAFE ---
    
    display.println(" STATUS: SAFE");
    
    // SAFETY ACTION: Turn Relay OFF
    digitalWrite(relayPin, HIGH); 
    display.println("Exhaust: OFF");
    
    setColor(0, 255, 0); 
    
    digitalWrite(buzzerPin, LOW);
    delay(500);
  }
  
  display.display();
}

void setColor(int redValue, int greenValue, int blueValue) {
  analogWrite(redPin, 255 - redValue);
  analogWrite(greenPin, 255 - greenValue);
  analogWrite(bluePin, 255 - blueValue);
}