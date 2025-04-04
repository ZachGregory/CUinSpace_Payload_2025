const float vcc = 5.0;
const int adcMax = 1023;

const float sens = 0.185;  // 5A
//const float sens = 0.100;  // 20A
//const float sens = 0.66;  // 30A

float getAverageCurrent(int nSamples) {
  float val = 0;
  for (int i = 0; i < nSamples; i++) {
    val += analogRead(A0);
    delay(1);
  }
  float avgOutput = val / adcMax / nSamples;
  return (vcc / 2 - vcc * avgOutput) / sens;
}

void setup() {
  Serial.begin(9600);
}

void loop() {
  float cur = getAverageCurrent(100);
  Serial.print("Current:");
  Serial.println(cur);
}