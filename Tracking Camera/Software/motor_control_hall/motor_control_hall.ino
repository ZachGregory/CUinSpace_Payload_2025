const int HallA = 2;  // Hall sensor A connected to digital pin 2
const int HallB = 3;  // Hall sensor B connected to digital pin 3

volatile int lastHallA = 0; // Stores previous Hall A state
volatile int lastHallB = 0; // Stores previous Hall B state
volatile long updates = 0;  // Hall state updates (+ if forward, - if backward)
volatile float angle = 0; // The angle of the motor shaft (360 is one full rotation)
volatile float rpm = 0; // Full rotations of the shaft per min
volatile unsigned long lastTime = 0; // Last time a pulse was detected

void setup() {
  Serial.begin(9600);
  pinMode(HallA, INPUT_PULLUP);
  pinMode(HallB, INPUT_PULLUP);

  // Attach interrupts for fast detection
  attachInterrupt(digitalPinToInterrupt(HallA), updateMotor, CHANGE);
  attachInterrupt(digitalPinToInterrupt(HallB), updateMotor, CHANGE);
}

void loop() {
  Serial.print("Angle: ");
  Serial.print(angle); 
  Serial.print("\tRPM: ");
  Serial.println(rpm);
  rpm = 0; // Sets rpm to 0 when not moving
  
  delay(100);
}

void updateMotor() {
  int hallA_state = digitalRead(HallA);
  int hallB_state = digitalRead(HallB);
  
  // Determine direction based on quadrature encoding
  // Forward pattern goes (AB): 00 10 11 01
  if ((lastHallA == 0 && lastHallB == 0 && hallA_state == 1 && hallB_state == 0) || 
      (lastHallA == 1 && lastHallB == 0 && hallA_state == 1 && hallB_state == 1) || 
      (lastHallA == 1 && lastHallB == 1 && hallA_state == 0 && hallB_state == 1) || 
      (lastHallA == 0 && lastHallB == 1 && hallA_state == 0 && hallB_state == 0)) {
    updates++;  // Moving forward
  } 
  else updates--;  // Moving backward

  // For every 3375.5 hall sensor updates it does one rotation (I have no idea where this number comes from. I just guessed and checked)
  angle = updates/3375.5 * 360; 
  // Normalizes angle to be 0-360
  while (angle > 360) angle -= 360;
  while (angle < 0) angle += 360;

  if(updates % 4 == 0){
    unsigned long currentTime = micros();  // Use micros() for better accuracy
    unsigned long timeDiff = currentTime - lastTime;
    lastTime = currentTime;
    // 4: number of steps between last check, 60: to make it per min, 1e6: to convert micro seconds, 3375.5: updates per rotation
    if (timeDiff != 0) rpm = 4 * 60.0 * 1e6 / (3375.5 * timeDiff); 
  }

  // Store last states
  lastHallA = hallA_state;
  lastHallB = hallB_state;
}
