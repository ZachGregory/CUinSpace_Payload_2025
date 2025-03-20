#include <math.h>

const int ThermistorPin = A0; // Analog pin for thermistor
const float R1 = 10000.0; // Resistor resistance (ohms)

const int RPWM = 6; // Forward digital pin
const int LPWM = 5; // Reversal digital pin

float temp; // Temperature in Celsius
float threshold; // Temperature goal
float tolerance; // Acceptable range + or -

void setup() {
  threshold = 30; // Temperature goal
  tolerance = 1; // Acceptable range + or -

  pinMode(RPWM, OUTPUT);
  pinMode(LPWM, OUTPUT);
  
  Serial.begin(9600);
}

void loop() {
  temp = getTemp();

  if (temp > threshold + tolerance) { // Cooling
    digitalWrite(RPWM, HIGH);
    digitalWrite(LPWM, LOW);
    Serial.print("Cooling");
  }
  else if (temp < threshold - tolerance) { // Heating
    digitalWrite(RPWM, LOW);
    digitalWrite(LPWM, HIGH);
    Serial.print("Heating");
  }
  else { // Idle
    digitalWrite(RPWM, LOW);
    digitalWrite(LPWM, LOW);
    Serial.print("Idle");
  }

  Serial.print("\t| Temperature:\t");
  Serial.print(temp);
  Serial.print(" C");
  Serial.print("\t| Goal:\t");
  Serial.print(threshold);
  Serial.println(" C");

  delay(500);


  if (Serial.available() > 0) {  
    float newThreshold = Serial.parseFloat();
    while (Serial.available()) Serial.read(); // Clear buffer
    if (newThreshold != 0) {
        threshold = newThreshold;
        Serial.print("Updated Threshold: ");
        Serial.println(threshold);
    }
  }

}

float getTemp() {
  int Vo = analogRead(ThermistorPin);
  if (Vo == 0) return -273.15; // Avoid division by zero error

  float logR2, R2, T;
  const float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;

  R2 = R1 * (1023.0 / Vo - 1.0);
  logR2 = log(R2);
  T = (1.0 / (c1 + c2 * logR2 + c3 * logR2 * logR2 * logR2));
  T = T - 273.15; // Convert from Kelvin to Celsius

  return T;
}