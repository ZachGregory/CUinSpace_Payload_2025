// Code for the H-Bridge testing module 

int potVal;
const byte potentiometer = A6;
const int R_PWM = 3;
const int R_En = 2;
const int L_PWM = 1;
const int L_En = 0;

const int midVal = 500;
const int deadZone = 10;

const int leftHigh = 893;
const int rightHigh = 125;

void setup() {
  Serial.begin(115200);
  while(!Serial){}

  pinMode(potentiometer, INPUT);
  pinMode(R_PWM, OUTPUT);
  pinMode(R_En, OUTPUT);
  pinMode(L_PWM, OUTPUT);
  pinMode(L_En, OUTPUT);
  digitalWrite(R_En, HIGH);
  digitalWrite(L_En, HIGH);

  delay(500);
}

void loop() {
  potVal = analogRead(potentiometer);
  //Serial.println(potVal);


  if (potVal <= midVal-deadZone){
    //Right
    potVal = map(potVal, midVal-deadZone, rightHigh, 0, 255);
    potVal = constrain(potVal, 0, 255);
    analogWrite(R_PWM, potVal);
    digitalWrite(L_PWM, 0);

  } else if (potVal >= midVal+deadZone){
    //Left
    potVal = map(potVal, midVal+deadZone, leftHigh, 0, 255);
    potVal = constrain(potVal, 0, 255);
    analogWrite(L_PWM, potVal);
    digitalWrite(R_PWM, 0);

  } else {
    potVal = 0;
    analogWrite(R_PWM, potVal);
    digitalWrite(L_PWM, potVal);
  }

  Serial.println(potVal);

  delay(1);
}
