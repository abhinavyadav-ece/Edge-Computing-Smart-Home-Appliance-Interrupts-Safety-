/*
 * ============================================================
 * Project 4: Edge-Computing Smart Home Appliance
 *            (Interrupts & Safety)
 * ============================================================
 * Board   : Arduino Uno / Nano
 * Author  : Edge-Smart-Home Project
 *
 * Pin Map
 * -------
 *  D2  – PIR Motion Sensor  (INT0 – hardware interrupt)
 *  D3  – Manual Override    (INT1 – hardware interrupt)
 *  A0  – Gas / Smoke Sensor (MQ-2 analogue output)
 *  D8  – White LED array    (smart lighting)
 *  D9  – Red LED            (fault indicator)
 *  D10 – Buzzer             (active fault alarm)
 *
 * Safety priority (highest → lowest):
 *   1. Gas/Smoke OVERRIDE  – shuts lights, sounds alarm
 *   2. Manual Override ISR – toggles lights, clears fault
 *   3. PIR Motion ISR      – turns lights ON
 *   4. Auto-off timer      – turns lights OFF after LIGHT_TIMEOUT ms
 * ============================================================
 */

#include <Arduino.h>

// ── Pin definitions ──────────────────────────────────────────
constexpr uint8_t PIN_PIR        = 2;   // INT0
constexpr uint8_t PIN_OVERRIDE   = 3;   // INT1
constexpr uint8_t PIN_GAS        = A0;
constexpr uint8_t PIN_LIGHT      = 8;
constexpr uint8_t PIN_RED_LED    = 9;
constexpr uint8_t PIN_BUZZER     = 10;

// ── Tunable constants ────────────────────────────────────────
constexpr int     GAS_THRESHOLD   = 400;   // ADC counts (0-1023)
constexpr uint16_t LIGHT_TIMEOUT  = 10000; // ms – auto-off after motion
constexpr uint16_t DEBOUNCE_MS    = 200;   // ms – ISR debounce

// ── Volatile state shared with ISRs ─────────────────────────
volatile bool motionDetected   = false;
volatile bool overrideTriggered = false;

// ── Normal state ─────────────────────────────────────────────
bool systemFault   = false;   // true while gas/smoke is above threshold
bool lightOn       = false;
unsigned long lightOnTime = 0;

// ── ISR: PIR Motion Sensor (INT0) ───────────────────────────
void IRAM_ATTR onMotion() {
  static unsigned long lastTrigger = 0;
  unsigned long now = millis();
  if (now - lastTrigger > DEBOUNCE_MS) {
    motionDetected = true;
    lastTrigger = now;
  }
}

// ── ISR: Manual Override Button (INT1) ──────────────────────
void IRAM_ATTR onOverride() {
  static unsigned long lastTrigger = 0;
  unsigned long now = millis();
  if (now - lastTrigger > DEBOUNCE_MS) {
    overrideTriggered = true;
    lastTrigger = now;
  }
}

// ── Helpers ──────────────────────────────────────────────────
void setLight(bool state) {
  lightOn = state;
  digitalWrite(PIN_LIGHT, state ? HIGH : LOW);
  if (state) lightOnTime = millis();
}

void activateFaultAlarm() {
  // Lights OFF, red LED + buzzer ON
  setLight(false);
  digitalWrite(PIN_RED_LED, HIGH);
  digitalWrite(PIN_BUZZER, HIGH);
}

void clearFaultAlarm() {
  digitalWrite(PIN_RED_LED, LOW);
  digitalWrite(PIN_BUZZER, LOW);
}

void printStatus() {
  Serial.print(F("[STATUS] Light="));
  Serial.print(lightOn ? F("ON ") : F("OFF"));
  Serial.print(F("  Fault="));
  Serial.print(systemFault ? F("YES") : F("NO "));
  Serial.print(F("  Gas="));
  Serial.println(analogRead(PIN_GAS));
}

// ── Setup ────────────────────────────────────────────────────
void setup() {
  Serial.begin(9600);
  Serial.println(F("=== Edge Smart Home Appliance BOOT ==="));

  pinMode(PIN_PIR,      INPUT);
  pinMode(PIN_OVERRIDE, INPUT_PULLUP);
  pinMode(PIN_LIGHT,    OUTPUT);
  pinMode(PIN_RED_LED,  OUTPUT);
  pinMode(PIN_BUZZER,   OUTPUT);

  // Attach hardware interrupts
  attachInterrupt(digitalPinToInterrupt(PIN_PIR),      onMotion,   RISING);
  attachInterrupt(digitalPinToInterrupt(PIN_OVERRIDE), onOverride, FALLING);

  // Safe initial state
  setLight(false);
  clearFaultAlarm();

  Serial.println(F("System ready. Monitoring..."));
}

// ── Main loop ────────────────────────────────────────────────
void loop() {
  // ── 1. HIGHEST PRIORITY: Gas / Smoke safety check ─────────
  int gasValue = analogRead(PIN_GAS);
  if (gasValue > GAS_THRESHOLD) {
    if (!systemFault) {
      systemFault = true;
      Serial.print(F("[ALERT] Gas/Smoke detected! ADC="));
      Serial.println(gasValue);
    }
    activateFaultAlarm();
    // While fault active – ignore all other events
    printStatus();
    delay(500);
    return;
  } else {
    // Gas cleared
    if (systemFault) {
      systemFault = false;
      clearFaultAlarm();
      Serial.println(F("[INFO] Gas/Smoke cleared. Resuming normal operation."));
    }
  }

  // ── 2. Manual Override ISR flag ───────────────────────────
  if (overrideTriggered) {
    noInterrupts();
    overrideTriggered = false;
    interrupts();
    // Toggle light
    setLight(!lightOn);
    Serial.print(F("[OVERRIDE] Light toggled → "));
    Serial.println(lightOn ? F("ON") : F("OFF"));
  }

  // ── 3. PIR Motion ISR flag ────────────────────────────────
  if (motionDetected) {
    noInterrupts();
    motionDetected = false;
    interrupts();
    if (!lightOn) {
      setLight(true);
      Serial.println(F("[PIR] Motion detected → Light ON"));
    } else {
      // Refresh timer
      lightOnTime = millis();
      Serial.println(F("[PIR] Motion refresh → timer reset"));
    }
  }

  // ── 4. Auto-off timer ─────────────────────────────────────
  if (lightOn && (millis() - lightOnTime >= LIGHT_TIMEOUT)) {
    setLight(false);
    Serial.println(F("[TIMER] No motion – Light OFF"));
  }

  // ── Periodic status print ─────────────────────────────────
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint >= 2000) {
    lastPrint = millis();
    printStatus();
  }

  delay(50);
}
