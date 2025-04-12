#include <math.h>

const int ascPin = A0; // Current Sensor
const float vcc = 5.0;
const int adcMax = 1023;
const float sens = 0.66;  // 30A

const int RPWM = 6; // Forward digital pin
const int LPWM = 5; // Reversal digital pin

float environmentTemp; // Temperature of the environment
float setpoint = 23.0; // Temperature goal
float tolerance = 1.0; // Acceptable range +/-

long lastTime;

float P, I = 0, D;
float kd = 10, kp = 0, ki = 0;
float lastError = 0;

void setup() {
  pinMode(RPWM, OUTPUT);
  pinMode(LPWM, OUTPUT);
  Serial.begin(9600);
  Serial.println("Start:");

  lastTime = millis();
}

void loop() {

  long currentTime = millis();
  long dt = currentTime - lastTime;
  lastTime = currentTime;

  environmentTemp = 0;

  controlPeltier(environmentTemp,255);
  //controlPeltierPID(environmentTemp,dt);

  float current = getAverageCurrent(100);

  Serial.print("dt:");
  Serial.print(dt);
  Serial.print("\t");
  Serial.print("Current:");
  Serial.println(current);

  delay(500);

  if (Serial.available() > 0) { // Lets the change of the set point from the serial monitor
    float newSetpoint = Serial.parseFloat();
    while (Serial.available()) Serial.read(); // Clear buffer
    setpoint = newSetpoint;
    Serial.print("Updated Threshold: ");
    Serial.println(setpoint);
  }
}

float getAverageCurrent(int nSamples) {
  float val = 0;
  for (int i = 0; i < nSamples; i++) {
    val += analogRead(ascPin);
    delay(1);
  }
  //return val / nSamples;
  float avgOutput = val / adcMax / nSamples;
  return (vcc * avgOutput - vcc / 2) / sens * 10;
}

void controlPeltier(float temperature, int PWM) {
  if (temperature > setpoint + tolerance) { // Cooling
    digitalWrite(RPWM, PWM);
    digitalWrite(LPWM, 0);
  } else if (temperature < setpoint - tolerance) { // Heating
    digitalWrite(RPWM, 0);
    digitalWrite(LPWM, PWM);
  } else { // Idle
    digitalWrite(RPWM, 0);
    digitalWrite(LPWM, 0);
  }
}

void controlPeltierPID(float temperature, long dtMillis){
  float error = setpoint - temperature;
  float dt = dtMillis / 1000.0;

  P = kp * error;
  I += ki * error * dt;
  if (dt != 0){D = kd * (error - lastError) / dt;} // Avoid divsion by 0
  else {dt = 0;}

  float PID = P + I + D;
  int PWM = (int)max(abs(PID),255);

  if (PID>0){
    digitalWrite(RPWM, PWM);
    digitalWrite(LPWM, 0);
  } else {
    digitalWrite(RPWM, 0);
    digitalWrite(LPWM, PWM);
  }
}