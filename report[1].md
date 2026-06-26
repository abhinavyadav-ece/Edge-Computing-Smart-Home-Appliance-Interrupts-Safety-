# Technical Report – Project 4
## Edge-Computing Smart Home Appliance (Interrupts & Safety)

---

### 1. Introduction

This project implements an edge-computing smart home appliance on the Arduino Uno. The system demonstrates real-world embedded design patterns: hardware-driven interrupts for instant response, priority-based state management, and an unbypassable safety override for hazardous conditions.

---

### 2. Hardware Architecture

#### 2.1 Interrupt-Driven Inputs

| Sensor | Interrupt Type | Rationale |
|--------|---------------|-----------|
| PIR Motion (D2/INT0) | RISING | Detects the rising edge when motion is first detected |
| Override Button (D3/INT1) | FALLING | Uses INPUT_PULLUP; button press pulls pin LOW |

Hardware interrupts guarantee **sub-millisecond response** regardless of what the main loop is doing.

#### 2.2 Analogue Safety Sensor

The Gas/Smoke MQ-2 sensor is read every loop iteration via `analogRead(A0)`. Polling is preferred over an interrupt here because:
- The fault condition must be **continuously monitored** (gas doesn't produce an edge)
- The check must run **before** any other logic to enforce highest priority

#### 2.3 Outputs

| Output | Pin | Active |
|--------|-----|--------|
| White LED Array | D8 | HIGH |
| Red Fault LED | D9 | HIGH |
| Active Buzzer | D10 | HIGH |

---

### 3. Software Design

#### 3.1 ISR Design Principles

Both ISRs are kept extremely short – they only set a `volatile bool` flag and perform debounce checking. No `Serial.print`, no `delay`, no complex logic. This is critical: ISRs run in interrupt context where other interrupts are disabled.

```
ISR executes → sets volatile flag → returns immediately
Main loop reads flag → clears flag atomically → executes response
```

#### 3.2 Priority Enforcement

The main loop enforces safety priority through sequential if-return logic:

```
loop() {
    1. Check Gas/Smoke → if fault: activate alarm, RETURN (skip everything else)
    2. Check override flag → toggle light
    3. Check motion flag → turn light on
    4. Check auto-off timer → turn light off
}
```

This structure makes the Gas/Smoke override **architecturally impossible to bypass** – even if an ISR fires while the fault is active, the flags are simply ignored until gas clears.

#### 3.3 Debouncing Strategy

Hardware buttons produce multiple electrical transitions per press. The solution:

```cpp
void IRAM_ATTR onOverride() {
    static unsigned long lastTrigger = 0;
    unsigned long now = millis();
    if (now - lastTrigger > DEBOUNCE_MS) {  // 200 ms guard
        overrideTriggered = true;
        lastTrigger = now;
    }
}
```

Using a `static` variable inside the ISR maintains state between calls without global variable pollution.

---

### 4. State Machine

The system has four logical states:

| State | Entry Condition | Active Outputs |
|-------|----------------|----------------|
| IDLE | Boot / gas cleared / timer expired | All off |
| LIGHT_ON | PIR motion or override | White LED |
| OVERRIDE | Button press while in any state | White LED toggled |
| FAULT | Gas ADC > 400 | Red LED + Buzzer; white LED OFF |

State transitions always check FAULT first, ensuring it takes effect within one loop cycle (~50 ms maximum).

---

### 5. Simulation (Wokwi)

The Wokwi simulation uses:
- **wokwi-pir-motion-sensor** – simulates the HC-SR501
- **wokwi-pushbutton** – simulates the override button
- **wokwi-potentiometer** – simulates MQ-2 analogue output (turn to ~40%+ to trigger fault)
- **wokwi-led** (white + red) – simulates LED outputs
- **wokwi-buzzer** – simulates the active buzzer

---

### 6. Test Results

| Test Case | Trigger | Observed Output | Pass |
|-----------|---------|----------------|------|
| Motion ON | PIR RISING | White LED ON within 50ms | ✅ |
| Auto-off | 10 s elapsed | White LED OFF | ✅ |
| Manual override | Button FALLING | Light toggles | ✅ |
| Gas fault | ADC > 400 | Red LED + Buzzer ON, White LED OFF | ✅ |
| Gas clear | ADC < 400 | Red LED + Buzzer OFF, normal mode | ✅ |
| Fault during light-on | ADC > 400 | Light OFF immediately | ✅ |
| ISR debounce | Rapid button press | Single toggle per press | ✅ |

---

### 7. Conclusions

The project successfully demonstrates:
1. **Hardware ISRs** provide deterministic, low-latency response to physical events
2. **Priority-ordered polling** enforces an unbypassable safety hierarchy
3. **Volatile flag pattern** is the safe, correct way to share data between ISR and main context
4. **Software debouncing inside ISRs** is viable and keeps the main loop clean

These patterns are directly applicable to industrial control systems, IoT edge devices, and any safety-critical embedded application.
