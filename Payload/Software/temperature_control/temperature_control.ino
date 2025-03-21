#include <math.h>

const int RPWM = 6; // Forward digital pin
const int LPWM = 5; // Reversal digital pin

const int thermistor[] = {A0, A1, A2}; // Analog pins for thermistors
const float R1 = 10000.0; // Resistor resistance (ohms) ??? 
const int numOfThermistors = sizeof(thermistor) / sizeof(thermistor[0]);

float thermistorWeight[] = {1, 1, 1}; // Weighting of each thermistor when calculating environment temp
bool thermistorIsValid[numOfThermistors];

float temp[numOfThermistors]; // Temperature in Celsius
float environmentTemp; // Temperature of the environment
float setpoint = 23.0; // Temperature goal
float tolerance = 1.0; // Acceptable range +/-
float errorTolerance = 8.0; // Acceptable difference from the median temp

void setup() {
  pinMode(RPWM, OUTPUT);
  pinMode(LPWM, OUTPUT);
  Serial.begin(9600);
  Serial.println("Start:");
}

void loop() {
  for (int i = 0; i < numOfThermistors; i++) { // Get indivudal temp readings
    temp[i] = getTemp(thermistor[i]);
    // Plot/Print temp data
    Serial.print("Thermistor_"); 
    Serial.print(thermistor[i]); 
    Serial.print(":"); 
    Serial.print(temp[i]);
    Serial.print("\t");
  }

  detectFaultySensors();

  // Find the weighted average temp
  environmentTemp = 0;
  float totalWeight = 0;
  for (int i = 0; i < numOfThermistors; i++) {
    if (thermistorIsValid[i]) {
      environmentTemp += temp[i] * thermistorWeight[i];
      totalWeight += thermistorWeight[i];
    }
  }
  environmentTemp /= totalWeight;

  Serial.print("Enviroment:"); 
  Serial.println(environmentTemp);

  controlPeltier(environmentTemp); // Adjust Peltier direction based on temp

  delay(500);

  if (Serial.available() > 0) { // Lets the change of the set point from the serial monitor
    float newSetpoint = Serial.parseFloat();
    while (Serial.available()) Serial.read(); // Clear buffer
    setpoint = newSetpoint;
    Serial.print("Updated Threshold: ");
    Serial.println(setpoint);
  }
}

float getTemp(int thermistorPin) {
  int Vo = analogRead(thermistorPin);
  if (Vo == 0) return -273.15; // Avoid division by zero error

  float logR2, R2, T;
  const float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;

  R2 = R1 * (1023.0 / Vo - 1.0);
  logR2 = log(R2);
  T = (1.0 / (c1 + c2 * logR2 + c3 * logR2 * logR2 * logR2)) - 273.15; // Convert from Kelvin to Celsius

  return T;
}

// Find the median and compares all the values to it.
// If outside the given range mark it as a faulty sensor
void detectFaultySensors() {
  float sorted[numOfThermistors];

  // Copy array
  for (int i = 0; i< numOfThermistors; i++){
    sorted[i] = temp[i];
  }

  // Sort to find median
  int minIndex;
  for (int i = 0; i< numOfThermistors; i++){
    minIndex = i;
    for (int j = i+1; j< numOfThermistors; j++){
      if (sorted[minIndex] > sorted[j]){
        minIndex = j;
      }
    }
    float tem = sorted[i];
    sorted[i] = sorted[minIndex];
    sorted[minIndex] = tem;
  }

  float median = sorted[numOfThermistors/2];

  for (int i = 0; i< numOfThermistors; i++){
    if(abs(temp[i] - median) > errorTolerance){ // Adjust accordingly
      thermistorIsValid[i] = false; // Mark as faulty
      Serial.print("Faulty Sensor Detected: Thermistor ");
      Serial.print(i);
      Serial.print(" | Reading: ");
      Serial.print(temp[i]);
      Serial.print(" C | Median: ");
      Serial.println(median);
    } else{
      thermistorIsValid[i] = true; // Mark as valid
    }
  }
}

void controlPeltier(float temperature) {
  if (temperature > setpoint + tolerance) { // Cooling
    digitalWrite(RPWM, HIGH);
    digitalWrite(LPWM, LOW);
  } else if (temperature < setpoint - tolerance) { // Heating
    digitalWrite(RPWM, LOW);
    digitalWrite(LPWM, HIGH);
  } else { // Idle
    digitalWrite(RPWM, LOW);
    digitalWrite(LPWM, LOW);
  }
}