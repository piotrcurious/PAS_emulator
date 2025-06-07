/*
  ADVANCED Bicycle PAS Sensor Emulator from Hall Throttle Input

  This version features:
  - An interpolated lookup table for a fully customizable throttle curve.
  - Non-blocking operation using micros() for timing.
  - Configurable signal duty cycle.
  - A state machine for robust signal generation.
  - Debug audio feedback with buzz tone modulated by throttle position.
  - Improved code structure and readability.

  - Throttle Input: Analog Pin A0
  - PAS Signal Output: Digital Pin D8
  - Debug Buzz Output: Digital Pin D9 (optional)

  By an AI assistant from Google
  Last updated: June 7, 2025
*/

// --- Pin Configuration ---
const int THROTTLE_PIN = A0;    // Analog input for the throttle signal wire
const int PAS_PIN = 8;          // Digital output for the emulated PAS signal
const int BUZZ_PIN = 9;         // Digital output for debug buzz tone

// --- Configuration Constants ---
const int DUTY_CYCLE = 25;      // PAS signal duty cycle in percent (0-100)
const int AUDIO_MULTIPLIER = 50; // Factor to convert PAS frequency to audible range
const bool DEBUG_MODE = true;   // Enable/disable debug buzz tone
const unsigned long SERIAL_INTERVAL = 250000; // Serial output interval in microseconds (250ms)

// --- Throttle Curve Lookup Table (LUT) ---
// Define your custom throttle response here.
// The throttle ADC values MUST be in increasing order.
// The arrays must have the same number of elements.

// Throttle ADC values (from 0-1023). The first value is your dead zone.
const int throttlePoints[] = { 220, 350, 500, 700, 820 };

// Corresponding PAS signal frequency in Hz for each throttle point.
const int frequencyPoints[] = { 0, 8, 14, 22, 25 };

// Automatically calculate the number of points in the table.
const int tableSize = sizeof(throttlePoints) / sizeof(throttlePoints[0]);

// --- State Machine Variables ---
struct PASState {
  bool outputHigh;
  unsigned long lastToggleMicros;
  unsigned long highDurationMicros;
  unsigned long lowDurationMicros;
  long currentFrequency;
} pas;

struct BuzzState {
  bool outputHigh;
  unsigned long lastToggleMicros;
  unsigned long periodMicros;
} buzz;

// --- Global Variables ---
int throttleValue = 0;
unsigned long lastSerialOutput = 0;

/**
 * @brief Performs linear interpolation based on a lookup table.
 * @param inputValue The current value to map (e.g., throttle ADC).
 * @param inputTable The array of input points (e.g., throttlePoints).
 * @param outputTable The array of corresponding output points (e.g., frequencyPoints).
 * @param tableLen The number of elements in the tables.
 * @return The calculated/interpolated output value.
 */
long interpolate(int inputValue, const int inputTable[], const int outputTable[], int tableLen) {
  // Boundary conditions
  if (inputValue <= inputTable[0]) {
    return outputTable[0];
  }
  if (inputValue >= inputTable[tableLen - 1]) {
    return outputTable[tableLen - 1];
  }

  // Find the segment where the inputValue falls and interpolate
  for (int i = 0; i < tableLen - 1; ++i) {
    if (inputValue >= inputTable[i] && inputValue <= inputTable[i + 1]) {
      // Linear interpolation formula
      float x0 = inputTable[i];
      float y0 = outputTable[i];
      float x1 = inputTable[i + 1];
      float y1 = outputTable[i + 1];
      
      float result = y0 + ((inputValue - x0) * (y1 - y0) / (x1 - x0));
      return (long)result;
    }
  }
  
  return outputTable[0]; // Fallback (should not reach here)
}

/**
 * @brief Updates PAS signal timing parameters based on frequency.
 * @param frequency The desired PAS frequency in Hz.
 */
void updatePASTimings(long frequency) {
  if (frequency > 0) {
    unsigned long totalPeriodMicros = 1000000UL / frequency;
    pas.highDurationMicros = totalPeriodMicros * DUTY_CYCLE / 100;
    pas.lowDurationMicros = totalPeriodMicros - pas.highDurationMicros;
  }
  pas.currentFrequency = frequency;
}

/**
 * @brief Updates buzz tone timing parameters based on frequency.
 * @param baseFrequency The PAS frequency to be converted to audible range.
 */
void updateBuzzTimings(long baseFrequency) {
  if (baseFrequency > 0) {
    long audioFrequency = baseFrequency * AUDIO_MULTIPLIER;
    buzz.periodMicros = 1000000UL / audioFrequency / 2; // Half period for toggle
  } else {
    buzz.periodMicros = 0;
  }
}

/**
 * @brief Handles PAS signal generation state machine.
 * @param currentMicros Current microsecond timestamp.
 */
void updatePASSignal(unsigned long currentMicros) {
  if (pas.currentFrequency <= 0) {
    digitalWrite(PAS_PIN, LOW);
    pas.outputHigh = false;
    return;
  }

  if (pas.outputHigh) {
    if (currentMicros - pas.lastToggleMicros >= pas.highDurationMicros) {
      digitalWrite(PAS_PIN, LOW);
      pas.outputHigh = false;
      pas.lastToggleMicros = currentMicros;
    }
  } else {
    if (currentMicros - pas.lastToggleMicros >= pas.lowDurationMicros) {
      digitalWrite(PAS_PIN, HIGH);
      pas.outputHigh = true;
      pas.lastToggleMicros = currentMicros;
    }
  }
}

/**
 * @brief Handles debug buzz tone generation state machine.
 * @param currentMicros Current microsecond timestamp.
 */
void updateBuzzSignal(unsigned long currentMicros) {
  if (!DEBUG_MODE || buzz.periodMicros == 0) {
    digitalWrite(BUZZ_PIN, LOW);
    buzz.outputHigh = false;
    return;
  }

  if (currentMicros - buzz.lastToggleMicros >= buzz.periodMicros) {
    buzz.outputHigh = !buzz.outputHigh;
    digitalWrite(BUZZ_PIN, buzz.outputHigh);
    buzz.lastToggleMicros = currentMicros;
  }
}

/**
 * @brief Handles periodic serial debug output.
 * @param currentMicros Current microsecond timestamp.
 */
void updateSerialOutput(unsigned long currentMicros) {
  if (currentMicros - lastSerialOutput >= SERIAL_INTERVAL) {
    Serial.print("Throttle ADC: ");
    Serial.print(throttleValue);
    Serial.print(" | PAS Freq: ");
    Serial.print(pas.currentFrequency);
    Serial.print(" Hz | Audio Freq: ");
    Serial.print(pas.currentFrequency * AUDIO_MULTIPLIER);
    Serial.println(" Hz");
    
    lastSerialOutput = currentMicros;
  }
}

/**
 * @brief Processes throttle input and updates system state.
 */
void processThrottleInput() {
  throttleValue = analogRead(THROTTLE_PIN);
  
  if (throttleValue > throttlePoints[0]) {
    // Throttle is active - calculate frequency from lookup table
    long frequency = interpolate(throttleValue, throttlePoints, frequencyPoints, tableSize);
    updatePASTimings(frequency);
    updateBuzzTimings(frequency);
  } else {
    // Throttle is in dead zone - disable outputs
    updatePASTimings(0);
    updateBuzzTimings(0);
  }
}

void setup() {
  // Initialize pins
  pinMode(PAS_PIN, OUTPUT);
  pinMode(BUZZ_PIN, OUTPUT);
  digitalWrite(PAS_PIN, LOW);
  digitalWrite(BUZZ_PIN, LOW);
  
  // Initialize serial communication
  Serial.begin(9600);
  Serial.println("Advanced PAS Emulator with Debug Audio Initialized.");
  Serial.println("==================================================");
  Serial.print("Debug Mode: ");
  Serial.println(DEBUG_MODE ? "ENABLED" : "DISABLED");
  Serial.print("Audio Multiplier: ");
  Serial.println(AUDIO_MULTIPLIER);
  Serial.println();
  
  // Initialize state structures
  pas = {false, 0, 0, 0, 0};
  buzz = {false, 0, 0};
  lastSerialOutput = 0;
}

void loop() {
  unsigned long currentMicros = micros();
  
  // Process throttle input and update system parameters
  processThrottleInput();
  
  // Update signal generators
  updatePASSignal(currentMicros);
  updateBuzzSignal(currentMicros);
  
  // Handle debug output
  if (DEBUG_MODE) {
    updateSerialOutput(currentMicros);
  }
}
