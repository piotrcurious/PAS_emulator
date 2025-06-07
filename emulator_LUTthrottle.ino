/*
  ADVANCED Bicycle PAS Sensor Emulator from Hall Throttle Input

  This version features:
  - An interpolated lookup table for a fully customizable throttle curve.
  - Non-blocking operation using micros() for timing.
  - Configurable signal duty cycle.
  - A state machine for robust signal generation.

  - Throttle Input: Analog Pin A0
  - PAS Signal Output: Digital Pin D8

  By an AI assistant from Google
  Last updated: June 7, 2025
*/

// --- Pin Configuration ---
const int THROTTLE_PIN = A0; // Analog input for the throttle signal wire
const int PAS_PIN = 8;       // Digital output for the emulated PAS signal

// --- PAS Output Duty Cycle Configuration ---
const int DUTY_CYCLE = 25; // Duty cycle in percent (0-100)

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


// --- Global variables for the non-blocking state machine ---
int throttleValue = 0;
bool pasState = LOW;
unsigned long previousMicros = 0;

/**
 * @brief Performs linear interpolation based on a lookup table.
 * @param inputValue The current value to map (e.g., throttle ADC).
 * @param inputTable The array of input points (e.g., throttlePoints).
 * @param outputTable The array of corresponding output points (e.g., frequencyPoints).
 * @param tableLen The number of elements in the tables.
 * @return The calculated/interpolated output value.
 */
long interpolate(int inputValue, const int inputTable[], const int outputTable[], int tableLen) {
  // If the value is below the first point, return the first point's output
  if (inputValue <= inputTable[0]) {
    return outputTable[0];
  }

  // If the value is above the last point, return the last point's output
  if (inputValue >= inputTable[tableLen - 1]) {
    return outputTable[tableLen - 1];
  }

  // Find the segment where the inputValue falls
  for (int i = 0; i < tableLen - 1; ++i) {
    if (inputValue >= inputTable[i] && inputValue <= inputTable[i + 1]) {
      // Found the segment, now interpolate
      // Using floating point math for precision
      float x0 = inputTable[i];
      float y0 = outputTable[i];
      float x1 = inputTable[i + 1];
      float y1 = outputTable[i + 1];

      // The interpolation formula
      float result = y0 + ( (inputValue - x0) * (y1 - y0) / (x1 - x0) );
      
      return (long)result;
    }
  }

  // Should not happen if tables are sorted, but as a fallback
  return outputTable[0]; 
}


void setup() {
  pinMode(PAS_PIN, OUTPUT);
  digitalWrite(PAS_PIN, LOW);
  Serial.begin(9600);
  Serial.println("Advanced PAS Emulator with Interpolated LUT Initialized.");
  Serial.println("-----------------------------------------------------");
}

void loop() {
  unsigned long currentMicros = micros();
  throttleValue = analogRead(THROTTLE_PIN);

  // Optional: Uncomment to print the ADC value for calibration
  // Serial.println(throttleValue);

  // Check if throttle is past the dead zone (the first value in the table)
  if (throttleValue > throttlePoints[0]) {
    // Get frequency from our custom throttle curve
    long frequency = interpolate(throttleValue, throttlePoints, frequencyPoints, tableSize);

    // A frequency of 0 means we should do nothing.
    if (frequency > 0) {
      unsigned long totalPeriodMicros = 1000000 / frequency;
      unsigned long highTimeMicros = totalPeriodMicros * (DUTY_CYCLE / 100.0);
      unsigned long lowTimeMicros = totalPeriodMicros - highTimeMicros;

      // --- State Machine Logic ---
      if (pasState == HIGH && (currentMicros - previousMicros >= highTimeMicros)) {
        pasState = LOW;
        previousMicros = currentMicros;
        digitalWrite(PAS_PIN, LOW);
      } else if (pasState == LOW && (currentMicros - previousMicros >= lowTimeMicros)) {
        pasState = HIGH;
        previousMicros = currentMicros;
        digitalWrite(PAS_PIN, HIGH);
      }
    } else {
        // If frequency is zero but throttle is active, keep output low
        digitalWrite(PAS_PIN, LOW);
        pasState = LOW;
    }
    
  } else {
    // Throttle is in the dead zone, ensure PAS signal is off and reset the state
    digitalWrite(PAS_PIN, LOW);
    pasState = LOW;
  }
}
