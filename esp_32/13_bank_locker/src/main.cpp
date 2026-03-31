/*
 * BANK LOCKER ACCESS SYSTEM — DUAL LOCKER
 * Two-Phase Auth + Dynamic OTP (MQTT) | Wokwi / ESP32
 *
 * PHASE 1 → Authority Master PIN (4 digits, shared)
 * PHASE 2 → Hirer PIN (4 dig) + MQTT OTP (2 dig) = 6 digits
 * OTP rotates every 60s → published to bank/locker/XX/otp
 *
 * Keypad:
 *   [1][2] - select locker
 *   [*]    - backspace
 *   [#]    - system reset
 *   [D]    - manual lock
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>

const char* WIFI_SSID  = "Wokwi-GUEST";
const char* WIFI_PASS  = "";
const char* MQTT_HOST  = "100.64.0.1";
const int   MQTT_PORT  = 1883;
const char* MQTT_CID   = "BankLocker_ESP32_A1";

const String MASTER_PIN   = "1234";
const String HIRER_PIN[3] = {"", "5678", "9012"};

const uint32_t OTP_INTERVAL    = 60000UL;   // 60s
const uint32_t UNLOCK_DURATION = 300000UL;  // 5 mins
const uint32_t ALARM_HOLD      = 10000UL;   // 10s
const uint32_t LCD_REFRESH     = 1000UL;

// GPIO
const byte ROWS = 4, COLS = 4;
byte rowPins[ROWS] = {13, 12, 14, 27};
byte colPins[COLS] = {26, 25, 33, 32};
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

const uint8_t SERVO_PIN[3] = {0, 18,  5};
const uint8_t GREEN_PIN[3] = {0,  2,  4};
const uint8_t RED_PIN[3]   = {0, 15, 16};
const uint8_t BUZZER_PIN   = 19;

// State
enum State { SEL_LOCKER, PHASE1, PHASE2, UNLOCKED, ALARM };
State   sysState  = SEL_LOCKER;
int     activeLk  = 0;
int     failCnt   = 0;
String  inputBuf  = "";

String   liveOTP[3]  = {"", "", ""};
uint32_t lastOtpMs   = 0;
uint32_t unlockMs    = 0;
uint32_t alarmMs     = 0;
uint32_t lcdMs       = 0;

// Objects
WiFiClient        wifiCli;
PubSubClient      mqtt(wifiCli);
Keypad            pad   = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo             srv[3];

// LCD
void lcdRow(uint8_t row, const char* text) {
  char buf[17];
  memset(buf, ' ', 16); buf[16] = '\0';
  strncpy(buf, text, strlen(text) < 16 ? strlen(text) : 16);
  lcd.setCursor(0, row);
  lcd.print(buf);
}

void lcdShow(const char* l1, const char* l2 = "") {
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(l1);
  lcd.setCursor(0, 1); lcd.print(l2);
}

// MQTT
void pub(const char* topic, String msg) {
  if (mqtt.connected()) {
    mqtt.publish(topic, msg.c_str(), true);
  }
  Serial.printf("[MQTT] %-38s  %s\n", topic, msg.c_str());
}

void mkTopic(char* buf, uint8_t sz, int locker, const char* evt) {
  snprintf(buf, sz, "bank/locker/%02d/%s", locker, evt);
}

// Network
void wifiConnect() {
  lcdShow("Connecting WiFi", "Please wait...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("WiFi");
  uint8_t tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries++ < 40) {
    delay(400); Serial.print('.');
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(" OK  " + WiFi.localIP().toString());
    lcdShow("WiFi Connected!", WiFi.localIP().toString().c_str());
    delay(900);
  } else {
    lcdShow("WiFi FAILED", "Check SSID/Pass");
    delay(2000);
  }
}

void mqttConnect() {
  while (!mqtt.connected()) {
    Serial.print("MQTT connect... ");
    if (sysState == SEL_LOCKER) {
      lcdShow("MQTT Connecting", "...");
    }
    if (mqtt.connect(MQTT_CID)) {
      Serial.println("OK");
      pub("bank/locker/system", "BankLockerSystem ONLINE | ESP32 ready");
      if (sysState == SEL_LOCKER) {
        lcdShow("Select Locker:", "  Press [1] [2] ");
      }
    } else {
      Serial.printf("fail (rc=%d) retrying\n", mqtt.state());
      delay(3000);
    }
  }
}

// OTP
void rotateOTPs() {
  char topic[40];
  for (int l = 1; l <= 2; l++) {
    liveOTP[l] = String(random(10, 100));
    mkTopic(topic, sizeof(topic), l, "otp");
    pub(topic,
        "SMS to Hirer-L#" + String(l) +
        ": Your OTP = [" + liveOTP[l] +
        "] | Append to your 4-digit PIN | Valid 60s");
  }
  lastOtpMs = millis();
  Serial.printf(">>> OTPs rotated  L1=%s  L2=%s\n",
                liveOTP[1].c_str(), liveOTP[2].c_str());
}

// Locker control
void openLocker(int l) {
  srv[l].write(90);
  digitalWrite(GREEN_PIN[l], HIGH);
  digitalWrite(RED_PIN[l],   LOW);

  char topic[40], line1[17];
  snprintf(line1, sizeof(line1), "Locker #%d  OPEN!", l);
  lcdShow(line1, "[D]Lock 5m-auto");

  mkTopic(topic, sizeof(topic), l, "sms");
  pub(topic, "SMS: Locker #" + String(l) + " OPENED successfully. Auto-lock in 5 mins.");
  mkTopic(topic, sizeof(topic), l, "status");
  pub(topic, "UNLOCKED");

  sysState = UNLOCKED;
  unlockMs = millis();
}

void closeLocker(int l) {
  srv[l].write(0);
  digitalWrite(GREEN_PIN[l], LOW);

  char topic[40];
  mkTopic(topic, sizeof(topic), l, "status");
  pub(topic, "SECURED");
  mkTopic(topic, sizeof(topic), l, "sms");
  pub(topic, "SMS: Locker #" + String(l) + " has been SECURED.");
}

void triggerAlarm(int l, const char* reason) {
  sysState = ALARM;
  alarmMs  = millis();

  digitalWrite(RED_PIN[l], HIGH);

  char line1[17], topic[40];
  snprintf(line1, sizeof(line1), " !! ALARM  L#%d !!", l);
  lcdShow(line1, reason);

  mkTopic(topic, sizeof(topic), l, "alarm");
  pub(topic, "ALERT on Locker #" + String(l) + ": " + String(reason));

  tone(BUZZER_PIN, 2000);
}

void resetSystem() {
  for (int l = 1; l <= 2; l++) {
    srv[l].write(0);
    digitalWrite(GREEN_PIN[l], LOW);
    digitalWrite(RED_PIN[l],   LOW);
  }
  noTone(BUZZER_PIN);
  inputBuf  = "";
  failCnt   = 0;
  activeLk  = 0;
  sysState  = SEL_LOCKER;
  pub("bank/locker/system", "System RESET by operator");
  lcdShow("System Reset OK", "Select: [1]  [2]");
}

// Input handling
String maskStr(const String& s) {
  String m = "";
  for (size_t i = 0; i < s.length(); i++) m += '*';
  return m;
}

void handleKey(char key) {
  if (key == '#') { resetSystem(); return; }

  if (key == 'D' && sysState == UNLOCKED) {
    char topic[40];
    mkTopic(topic, sizeof(topic), activeLk, "sms");
    pub(topic, "SMS: Locker #" + String(activeLk) + " MANUALLY LOCKED by user.");
    closeLocker(activeLk);
    sysState = SEL_LOCKER;
    lcdShow("Manual Lock OK", "Select: [1] [2]");
    return;
  }

  if (key == '*') {
    if ((sysState == PHASE1 || sysState == PHASE2) && inputBuf.length() > 0) {
      inputBuf.remove(inputBuf.length() - 1);
      lcdRow(1, maskStr(inputBuf).c_str());
    }
    return;
  }

  switch (sysState) {
    case SEL_LOCKER:
      if (key == '1' || key == '2') {
        activeLk = key - '0';
        failCnt  = 0;
        inputBuf = "";
        sysState = PHASE1;
        char hdr[17];
        snprintf(hdr, sizeof(hdr), "L#%d | Phase-1:", activeLk);
        lcdShow(hdr, "Authority PIN: _");
      }
      break;

    case PHASE1:
      if (inputBuf.length() < 4) {
        inputBuf += key;
        lcdRow(1, maskStr(inputBuf).c_str());
      }
      if (inputBuf.length() == 4) {
        delay(150);
        if (inputBuf == MASTER_PIN) {
          char topic[40], hdr[17];
          mkTopic(topic, sizeof(topic), activeLk, "auth");
          pub(topic, "Phase-1 PASSED — Authority verified for Locker #" + String(activeLk));
          failCnt  = 0;
          inputBuf = "";
          sysState = PHASE2;
          snprintf(hdr, sizeof(hdr), "L#%d | Phase-2:", activeLk);
          lcdShow(hdr, "PIN+OTP (6 dig):");
          lcdMs = millis();
        } else {
          failCnt++;
          inputBuf = "";
          if (failCnt >= 3) {
            triggerAlarm(activeLk, "3x Auth1 fails");
          } else {
            char buf[17];
            snprintf(buf, sizeof(buf), "Wrong! (%d left) ", 3 - failCnt);
            lcdShow(buf, "Authority PIN: _");
          }
        }
      }
      break;

    case PHASE2:
      if (inputBuf.length() < 6) {
        inputBuf += key;
        lcdRow(1, maskStr(inputBuf).c_str());
      }
      if (inputBuf.length() == 6) {
        delay(150);
        String expected = HIRER_PIN[activeLk] + liveOTP[activeLk];
        if (inputBuf == expected) {
          openLocker(activeLk);
        } else {
          failCnt++;
          inputBuf = "";
          if (failCnt >= 3) {
            triggerAlarm(activeLk, "3x Auth2 fails");
          } else {
            char buf[17];
            snprintf(buf, sizeof(buf), "Wrong! (%d left) ", 3 - failCnt);
            lcdShow(buf, "PIN+OTP (6 dig):");
          }
        }
      }
      break;

    default: break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n\n=== BANK LOCKER SYSTEM BOOT ===");

  // GPIO setup
  for (int l = 1; l <= 2; l++) {
    pinMode(GREEN_PIN[l], OUTPUT);
    pinMode(RED_PIN[l], OUTPUT);
    digitalWrite(GREEN_PIN[l], LOW);
    digitalWrite(RED_PIN[l], LOW);
  }
  
  noTone(BUZZER_PIN);

  // Servos
  srv[1].attach(SERVO_PIN[1]); srv[1].write(0);
  srv[2].attach(SERVO_PIN[2]); srv[2].write(0);

  lcd.init();
  lcd.backlight();
  lcdShow(" BANK LOCKER SYS", "  Initializing..");
  delay(800);

  wifiConnect();
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqttConnect();

  randomSeed(analogRead(34) ^ millis());
  rotateOTPs();

  lcdShow("Select Locker:", "  Press [1] [2] ");
  sysState = SEL_LOCKER;
  Serial.println("=== SYSTEM READY ===\n");
}

void loop() {
  if (!mqtt.connected()) mqttConnect();
  mqtt.loop();

  uint32_t now = millis();

  if (now - lastOtpMs >= OTP_INTERVAL) {
    rotateOTPs();
    if (sysState == PHASE2) {
      inputBuf = "";
      lcdRow(0, "OTP expired!Retry");
      lcdRow(1, "PIN+OTP (6 dig):");
      delay(1200);
      char hdr[17];
      snprintf(hdr, sizeof(hdr), "L#%d | Phase-2:", activeLk);
      lcdRow(0, hdr);
      lcdRow(1, "PIN+OTP (6 dig):");
    }
  }

  if (sysState == UNLOCKED && now - unlockMs >= UNLOCK_DURATION) {
    closeLocker(activeLk);
    sysState = SEL_LOCKER;
    lcdShow("Locker Secured.", "Select: [1] [2]");
  }

  if (sysState == ALARM && now - alarmMs >= ALARM_HOLD) {
    resetSystem();
  }

  if (sysState == PHASE2 && now - lcdMs >= LCD_REFRESH) {
    lcdMs = now;
    uint32_t elapsed   = (now - lastOtpMs) / 1000;
    uint32_t remaining = (elapsed < 60) ? (60 - elapsed) : 0;
    char buf[17];
    snprintf(buf, sizeof(buf), "L#%d Ph2 exp:%2lus", activeLk, (unsigned long)remaining);
    lcdRow(0, buf);
  }

  char key = pad.getKey();
  if (key) handleKey(key);
}
