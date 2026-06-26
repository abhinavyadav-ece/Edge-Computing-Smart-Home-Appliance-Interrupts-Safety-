# Project 4: Edge-Computing Smart Home Appliance
## Interrupts & Safety Override System

![Circuit Diagram](diagrams/circuit_diagram.svg)

---

## 📌 Overview

An intelligent **edge device** built on the Arduino Uno that handles human inputs and automated safety overrides **instantly** using hardware interrupts (ISRs) and priority-based state logic.

| Feature | Detail |
|---|---|
| **Board** | Arduino Uno / Nano (ATmega328P) |
| **Simulation** | [Wokwi Online Simulator](https://wokwi.com) |
| **Language** | C++ (Arduino framework) |
| **Key Concepts** | Hardware ISRs, multi-sensor edge logic, industrial safety fail-safes |

---

## 🎯 Project Goals

1. **Engineer an intelligent edge device** that handles human inputs and automated safety overrides instantly.
2. **Configure a PIR Motion Sensor** to control a smart lighting array using hardware Interrupts.
3. **Implement a secondary safety logic block** – if an analog Gas/Smoke sensor triggers, override all states immediately.
4. **Sound a physical buzzer and flash a red LED** to alert the user of a system fault.

---

## 🔌 Pin Map

| Pin | Component | Role |
|-----|-----------|------|
| D2 | PIR Motion Sensor | INT0 – Hardware interrupt (RISING) |
| D3 | Override Button | INT1 – Hardware interrupt (FALLING) |
| A0 | Gas / Smoke Sensor (MQ-2) | Analogue poll each loop |
| D8 | White LED Array | Smart lighting output |
| D9 | Red LED | Fault indicator |
| D10 | Active Buzzer | Fault alarm |

---

## 🧠 System Architecture

### Safety Priority (Highest → Lowest)

```
① Gas/Smoke OVERRIDE   → Lights OFF · Red LED ON · Buzzer ON  (blocks all other events)
② Manual Override ISR  → Toggles lights · Clears fault alarm
③ PIR Motion ISR       → Turns lights ON · Resets auto-off timer
④ Auto-off Timer       → Lights OFF after 10 s of no motion
```

### State Machine

![State Machine](diagrams/state_machine.svg)

---

## 🛠️ Key Implementation Details

### Hardware Interrupts (ISRs)

```cpp
// INT0 – PIR Motion (RISING edge)
attachInterrupt(digitalPinToInterrupt(PIN_PIR),      onMotion,   RISING);

// INT1 – Manual Override (FALLING edge, uses INPUT_PULLUP)
attachInterrupt(digitalPinToInterrupt(PIN_OVERRIDE), onOverride, FALLING);
```

Both ISRs use **software debouncing** (200 ms guard time) via a `static` timestamp inside each ISR to prevent spurious triggers.

### Gas / Smoke Safety Block

```cpp
int gasValue = analogRead(PIN_GAS);   // 0-1023
if (gasValue > GAS_THRESHOLD) {       // threshold = 400
    activateFaultAlarm();             // highest priority – runs immediately
    return;                           // skip all other logic
}
```

Because this check runs **first** in every `loop()` iteration, it acts as a hardware-level safety override that cannot be blocked by any ISR.

### Volatile Flag Pattern

```cpp
volatile bool motionDetected    = false;
volatile bool overrideTriggered = false;

// In main loop – atomic read & clear
noInterrupts();
bool motion = motionDetected;
motionDetected = false;
interrupts();
```

Flags are set in the ISR and consumed in the main loop, keeping ISRs short and safe.

---

## 🚀 Quick Start

### 1 – Simulate Online (Wokwi)

Click the link below to open the pre-wired simulation directly:

**[▶ Open in Wokwi Simulator]([https://wokwi.com/projects/new/arduino-uno)](https://wokwi.com/projects/467919522467017729)**

> Import `wokwi/diagram.json` via **File → Import Diagram** and paste `src/main.ino` into the editor.

### 2 – Flash to Real Hardware

```bash
# Install Arduino CLI
brew install arduino-cli          # macOS
# or: https://arduino.github.io/arduino-cli/

# Compile
arduino-cli compile --fqbn arduino:avr:uno src/main.ino

# Upload (change /dev/ttyUSB0 to your port)
arduino-cli upload -p /dev/ttyUSB0 --fqbn arduino:avr:uno src/main.ino

# Open Serial Monitor (9600 baud)
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=9600
```

### 3 – PlatformIO

```bash
pio run -e uno          # compile
pio run -e uno -t upload # flash
pio device monitor      # serial output
```

---

## 🧪 Testing Scenarios

| Test | How to trigger | Expected result |
|------|---------------|-----------------|
| Motion detection | Activate PIR / press PIR button in Wokwi | White LED turns ON |
| Auto-off | Wait 10 s after last motion | White LED turns OFF |
| Manual override | Press Override button (D3) | Light toggles |
| Gas alarm | Turn potentiometer > ~40% in Wokwi | Red LED + Buzzer ON, Light OFF |
| Gas clear | Return potentiometer to low | Red LED + Buzzer OFF, normal mode resumes |

---

## 📁 Project Structure

```
edge-smart-home/
├── src/
│   └── main.ino            ← Main Arduino sketch
├── wokwi/
│   ├── diagram.json        ← Wokwi circuit diagram
│   └── wokwi.toml          ← Wokwi project config
├── diagrams/
│   ├── circuit_diagram.svg ← Circuit schematic
│   └── state_machine.svg   ← System state machine
├── docs/
│   └── report.md           ← Technical report
└── README.md
```

---

## ⚙️ Configuration Constants

Edit these in `src/main.ino` to tune the system:

```cpp
constexpr int     GAS_THRESHOLD  = 400;   // ADC (0-1023); lower = more sensitive
constexpr uint16_t LIGHT_TIMEOUT = 10000; // ms before auto-off (default 10 s)
constexpr uint16_t DEBOUNCE_MS  = 200;    // ISR debounce guard time
```

---

## 🔑 Key Skills Demonstrated

- **Hardware Interrupts (ISRs)** — INT0/INT1, RISING/FALLING edge detection
- **Multi-sensor edge logic** — PIR (digital) + Gas/Smoke (analogue) fusion
- **Industrial safety fail-safes** — priority override that cannot be blocked
- **Volatile flag pattern** — safe ISR ↔ main-loop communication
- **Software debouncing** — inside ISRs without `delay()`

