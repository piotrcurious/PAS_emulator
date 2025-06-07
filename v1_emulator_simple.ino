/*
  IMPROVED Bicycle PAS Sensor Emulator from Hall Throttle Input

  This version features:
  - Non-blocking operation using micros() for timing.
  - Configurable signal duty cycle.
  - A state machine for robust signal generation.

  This allows the Arduino to perform other tasks without interrupting the
  PAS signal generation.

  - Throttle Input: Analog Pin A0
  - PAS Signal Output: Digital Pin D8

  By an AI assistant from Google
  Last updated: June 7, 2025
*/

// --- Pin Configuration ---
const int THROTTLE_PIN = A0; // Analog input for the throttle signal wire
const int PAS_PIN = 8;       // Digital output for the emulated PAS signal

// --- Throttle Input Calibration (ADC Values: 0-1023) ---
const int THROTTLE_MIN = 220; // ADC value to start motor (provides a dead zone)
const int THROTTLE_MAX = 820; // ADC value for maximum throttle

// --- PAS Output Frequency Configuration (in Hertz) ---
const int MIN_FREQUENCY = 5;  // Frequency when throttle is just engaged
const int MAX_FREQUENCY = 25; // Frequency at full throttle

// --- PAS Output Duty Cycle Configuration ---
// The percentage of time the signal is HIGH during one period.
// 50 means a standard 50% square wave. 25 is often used for emulators.
const int DUTY_CYCLE = 25; // Duty cycle in percent (0-100)

// --- Global variables for the non-blocking state machine ---
int throttleValue = 0;           // Stores the current throttle reading
bool pasState = LOW;             // Tracks the current state of the PAS pin (HIGH or LOW)
unsigned long previousMicros = 0; // Stores the last time the PAS pin was toggled

void setup() {
  // Set the PAS signal pin as an output
  pinMode(PAS_PIN, OUTPUT);
  digitalWrite(PAS_PIN, LOW); // Start with the signal off

  // Initialize Serial Monitor for debugging and calibration
  Serial.begin(9600);
  Serial.println("Non-Blocking PAS Sensor Emulator Initialized.");
  Serial.println("-------------------------------------------");
}

void loop() {
  // Capture the current time at the start of the loop
  unsigned long currentMicros = micros();

  // Read the raw analog value from the throttle
  throttleValue = analogRead(THROTTLE_PIN);

  // Optional: Uncomment to print the ADC value for calibration
  // Serial.println(throttleValue);

  // Check if the throttle is within the active range
  if (throttleValue > THROTTLE_MIN) {
    // Map the throttle's ADC value to the desired frequency range
    long frequency = map(throttleValue, THROTTLE_MIN, THROTTLE_MAX, MIN_FREQUENCY, MAX_FREQUENCY);
    frequency = constrain(frequency, MIN_FREQUENCY, MAX_FREQUENCY);

    // Calculate the total period and the HIGH/LOW times in microseconds based on frequency and duty cycle
    unsigned long totalPeriodMicros = 1000000 / frequency;
    unsigned long highTimeMicros = totalPeriodMicros * (DUTY_CYCLE / 100.0);
    unsigned long lowTimeMicros = totalPeriodMicros - highTimeMicros;

    // --- State Machine Logic ---
    // Check if it's time to toggle the pin based on its current state
    if (pasState == HIGH && (currentMicros - previousMicros >= highTimeMicros)) {
      pasState = LOW; // Time to go LOW
      previousMicros = currentMicros; // Reset timer
      digitalWrite(PAS_PIN, LOW);
    } 
    else if (pasState == LOW && (currentMicros - previousMicros >= lowTimeMicros)) {
      pasState = HIGH; // Time to go HIGH
      previousMicros = currentMicros; // Reset timer
      digitalWrite(PAS_PIN, HIGH);
    }

  } else {
    // If throttle is not engaged, ensure the PAS signal is off and reset the state
    digitalWrite(PAS_PIN, LOW);
    pasState = LOW;
  }

  /*
    You can add other non-blocking code here.
    For example: read a brake sensor, update an LCD, etc.
    The loop will now run thousands of times per second.
  */
}
