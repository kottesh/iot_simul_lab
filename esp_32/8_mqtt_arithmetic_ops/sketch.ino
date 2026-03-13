#include <WiFi.h>
#include <PubSubClient.h>

const char* WIFI_SSID     = "YOUR_SSID";
const char* WIFI_PASS     = "YOUR_PASSWORD";
const char* MQTT_BROKER   = "192.168.1.100";
const int   MQTT_PORT     = 1883;
const char* MQTT_CLIENT   = "esp32-calc";
const char* TOPIC_INPUT   = "calc/input";
const char* TOPIC_RESULT  = "calc/result";


WiFiClient   wifiClient;
PubSubClient mqtt(wifiClient);


static const char* expr_ptr = nullptr;   // current position in expression string


long parseExpr();
long parseTerm();
long parseFactor();
long compute(const char* expression, bool* error);
void parser(const char* raw, char* resultBuf, size_t bufLen);

/*
 * Grammar (handles precedence & parentheses):
 *   Expr   = Term   { ('+'|'-') Term   }
 *   Term   = Factor { ('*'|'/') Factor }
 *   Factor = '(' Expr ')' | ['-'] Number
 */

static void skipSpaces() {
    while (*expr_ptr == ' ') expr_ptr++;
}

/* Lowest pecedence: addition & subtraction */
long parseExpr() {
    long result = parseTerm();
    skipSpaces();
    while (*expr_ptr == '+' || *expr_ptr == '-') {
        char op = *expr_ptr++;
        long rhs = parseTerm();
        result = (op == '+') ? result + rhs : result - rhs;
        skipSpaces();
    }
    return result;
}

/* Middle precedence: multiplication & division */
long parseTerm() {
    long result = parseFactor();
    skipSpaces();
    while (*expr_ptr == '*' || *expr_ptr == '/') {
        char op = *expr_ptr++;
        long rhs = parseFactor();
        if (op == '/' && rhs == 0) {
            // Signal divide-by-zero by poisoning the pointer
            expr_ptr = nullptr;
            return 0;
        }
        result = (op == '*') ? result * rhs : result / rhs;
        skipSpaces();
    }
    return result;
}

/* Highest precedence: parentheses & integer literals */
long parseFactor() {
    skipSpaces();
    if (expr_ptr == nullptr) return 0;

    // Parenthesised sub-expression
    if (*expr_ptr == '(') {
        expr_ptr++;                  // consume '('
        long result = parseExpr();
        skipSpaces();
        if (*expr_ptr == ')') expr_ptr++;   // consume ')'
        return result;
    }

    // Unary minus
    int sign = 1;
    if (*expr_ptr == '-') { sign = -1; expr_ptr++; }

    // Integer literal
    long num = 0;
    bool hasDigit = false;
    while (*expr_ptr >= '0' && *expr_ptr <= '9') {
        num = num * 10 + (*expr_ptr++ - '0');
        hasDigit = true;
    }

    if (!hasDigit) expr_ptr = nullptr;   // unexpected character → error
    return sign * num;
}

// ── compute() ────────────────────────────────────────────────────────────────
/*
 * Entry point for evaluation.
 * Sets *error = true and returns 0 on any parse/math fault.
 */
long compute(const char* expression, bool* error) {
    expr_ptr = expression;
    *error   = false;

    long result = parseExpr();

    // Leftover characters or a poisoned pointer = malformed expression
    if (expr_ptr == nullptr || (*expr_ptr != '\0' && *expr_ptr != '\n')) {
        *error = true;
        return 0;
    }
    return result;
}

/*
 * Strips whitespace from raw MQTT payload, drives compute(),
 * and serialises the outcome into resultBuf.
 */
void parser(const char* raw, char* resultBuf, size_t bufLen) {
    // Compact: copy non-space chars into a clean buffer
    char clean[128] = {};
    size_t j = 0;
    for (size_t i = 0; raw[i] && j < sizeof(clean) - 1; i++) {
        if (raw[i] != ' ') clean[j++] = raw[i];
    }

    bool error = false;
    long result = compute(clean, &error);

    if (error) {
        snprintf(resultBuf, bufLen, "ERROR: invalid expression [%s]", raw);
    } else {
        snprintf(resultBuf, bufLen, "%s = %ld", raw, result);
    }
}

// ── MQTT callback ─────────────────────────────────────────────────────────────
void onMessage(char* topic, byte* payload, unsigned int length) {
    // Null-terminate payload
    char msg[128] = {};
    size_t len = min((unsigned int)(sizeof(msg) - 1), length);
    memcpy(msg, payload, len);

    Serial.printf("[MQTT] RX on '%s': %s\n", topic, msg);

    char resultBuf[160] = {};
    parser(msg, resultBuf, sizeof(resultBuf));

    Serial.printf("[MQTT] TX → %s\n", resultBuf);

    mqtt.publish(TOPIC_RESULT, resultBuf);
}

// ── WiFi ─────────────────────────────────────────────────────────────────────
void connectWiFi() {
    Serial.printf("Connecting to WiFi '%s'", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
    Serial.printf("\nWiFi OK — IP: %s\n", WiFi.localIP().toString().c_str());
}

// ── MQTT ─────────────────────────────────────────────────────────────────────
void connectMQTT() {
    while (!mqtt.connected()) {
        Serial.print("Connecting to MQTT broker… ");
        if (mqtt.connect(MQTT_CLIENT)) {
            Serial.println("connected");
            mqtt.subscribe(TOPIC_INPUT);
            Serial.printf("Subscribed to '%s'\n", TOPIC_INPUT);
        } else {
            Serial.printf("failed (rc=%d), retry in 3 s\n", mqtt.state());
            delay(3000);
        }
    }
}

// ── Arduino entry points ─────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    connectWiFi();
    mqtt.setServer(MQTT_BROKER, MQTT_PORT);
    mqtt.setCallback(onMessage);
    connectMQTT();
}

void loop() {
    if (!mqtt.connected()) connectMQTT();
    mqtt.loop();
}