const int ascPin = A1;
const float vcc = 5.0;
const int adcMax = 1023;

//const float sens = 0.185;  // 5A
//const float sens = 0.100;  // 20A
const float sens = 0.66;  // 30A

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

void setup() {
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(ascPin, INPUT);
  Serial.begin(9600);
}

void loop() {
  // H - Bridge forward
  digitalWrite(5, LOW);
  digitalWrite(6, HIGH);

  float cur = getAverageCurrent(100);
  //Serial.print("Current:");
  Serial.println(cur);
}