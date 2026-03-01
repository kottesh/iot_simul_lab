const int PIR_PIN = 2;
const int PHOTO_DIGITAL_PIN = 3;
const int PHOTO_ANALOG_PIN = A0;

const int MOTION_LED = 8;   // White LED
const int ANALOG_LED = 9;   // Blue LED
const int DIGITAL_LED = 10; // Orange LED

const int LIGHT_THRESHOLD = 512;

void setup()
{
  Serial.begin(9600);

  pinMode(PIR_PIN, INPUT);
  pinMode(PHOTO_DIGITAL_PIN, INPUT);
  pinMode(MOTION_LED, OUTPUT);
  pinMode(ANALOG_LED, OUTPUT);
  pinMode(DIGITAL_LED, OUTPUT);

  Serial.println("========================================");
  Serial.println("Motion & Photoresistor Sensor Demo");
  Serial.println("========================================");
  Serial.println("White LED (Pin 8)  - Motion Detection");
  Serial.println("Blue LED (Pin 9)   - Analog Light Level");
  Serial.println("Orange LED (Pin 10) - Digital Light Status");
  Serial.println("========================================");
  delay(2000);
  Serial.println("System Ready!\n");
}

void loop()
{
  Serial.println("========================================");

  // PIR Motion Sensor
  int motionDetected = digitalRead(PIR_PIN);

  if (motionDetected == HIGH)
  {
    digitalWrite(MOTION_LED, HIGH);
    Serial.println(">>> MOTION DETECTED! <<<");
  }
  else
  {
    digitalWrite(MOTION_LED, LOW);
    Serial.println("Motion: None");
  }

  // Photoresistor Analog Output
  int analogValue = analogRead(PHOTO_ANALOG_PIN);

  // Exponential curve for visible brightness changes
  float normalizedValue = (1023.0 - analogValue) / 1023.0;
  float gamma = 2.5;
  float correctedValue = pow(normalizedValue, gamma);
  int ledBrightness = (int)(correctedValue * 255.0);
  ledBrightness = constrain(ledBrightness, 0, 255);

  analogWrite(ANALOG_LED, ledBrightness);

  // Convert to lux
  const float GAMMA_LUX = 0.7;
  const float RL10 = 50;
  float voltage = analogValue / 1024.0 * 5.0;
  float resistance = 2000.0 * voltage / (1.0 - voltage / 5.0);
  float lux = pow(RL10 * 1e3 * pow(10, GAMMA_LUX) / resistance, (1.0 / GAMMA_LUX));

  Serial.println("\n--- ANALOG OUTPUT (Blue LED) ---");
  Serial.print("Raw Value: ");
  Serial.print(analogValue);
  Serial.print(" | Voltage: ");
  Serial.print(voltage, 2);
  Serial.print("V | Light: ");

  if (isfinite(lux))
  {
    Serial.print((int)lux);
    Serial.println(" lux");
  }
  else
  {
    Serial.println("Very Bright");
  }

  Serial.print("Brightness: ");
  Serial.print(ledBrightness);
  Serial.print("/255 (");
  Serial.print((ledBrightness * 100) / 255);
  Serial.print("%) ");

  if (ledBrightness < 25)
  {
    Serial.println("- OFF/VERY DIM");
  }
  else if (ledBrightness < 80)
  {
    Serial.println("- DIM");
  }
  else if (ledBrightness < 150)
  {
    Serial.println("- MEDIUM");
  }
  else if (ledBrightness < 220)
  {
    Serial.println("- BRIGHT");
  }
  else
  {
    Serial.println("- VERY BRIGHT");
  }

  Serial.print("Bar: [");
  int bars = map(ledBrightness, 0, 255, 0, 20);
  for (int i = 0; i < bars; i++)
    Serial.print("█");
  for (int i = bars; i < 20; i++)
    Serial.print("░");
  Serial.println("]");

  // Photoresistor Digital Output
  int digitalValue = digitalRead(PHOTO_DIGITAL_PIN);

  if (digitalValue == LOW)
  {
    digitalWrite(DIGITAL_LED, HIGH);
    Serial.println("\n--- DIGITAL OUTPUT (Orange LED) ---");
    Serial.println("Status: BRIGHT | Orange LED: ON");
  }
  else
  {
    digitalWrite(DIGITAL_LED, LOW);
    Serial.println("\n--- DIGITAL OUTPUT (Orange LED) ---");
    Serial.println("Status: DARK | Orange LED: OFF");
  }

  // Custom Threshold
  Serial.println("\n--- CUSTOM THRESHOLD ---");
  if (analogValue > LIGHT_THRESHOLD)
  {
    Serial.print("Value (");
    Serial.print(analogValue);
    Serial.print(") > (");
    Serial.print(LIGHT_THRESHOLD);
    Serial.println(") = DARK");
  }
  else
  {
    Serial.print("Value (");
    Serial.print(analogValue);
    Serial.print(") ≤ (");
    Serial.print(LIGHT_THRESHOLD);
    Serial.println(") = BRIGHT");
  }

  Serial.println("========================================\n");
  delay(1000);
}
