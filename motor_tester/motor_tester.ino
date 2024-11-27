#define WEIGHT_DATA A1

// Define motor control pins
const int IN1 = 2;  // L298N IN1
const int IN2 = 3;  // L298N IN2
const int IN3 = 4;  // L298N IN3
const int IN4 = 5;  // L298N IN4
const int stepDelay = 5; // Adjust for speed control (milliseconds)

// Define step sequence
const int stepSequence[4][4] = {
  {1, 0, 1, 0}, // Step 1
  {0, 1, 1, 0}, // Step 2
  {0, 1, 0, 1}, // Step 3
  {1, 0, 0, 1}  // Step 4
};

// Function to set motor pins
void setMotorPins(int a, int b, int c, int d) {
  digitalWrite(IN1, a);
  digitalWrite(IN2, b);
  digitalWrite(IN3, c);
  digitalWrite(IN4, d);
}

// Function to rotate the motor one step
void rotateMotorStep(int direction) {
  for (int j = 0; j < 4; j++) {
    int index = (direction == 1) ? j : (3 - j); // Adjust direction
    setMotorPins(stepSequence[index][0], stepSequence[index][1], 
                 stepSequence[index][2], stepSequence[index][3]);
    delay(stepDelay); // Control speed
  }
}

// Function to stop the motor
void stopMotor() {
  setMotorPins(0, 0, 0, 0); // Turn off all motor pins
}

void setup() {
  // Set motor pins as output
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // Set weight sensor as input
  pinMode(WEIGHT_DATA, INPUT);

  Serial.begin(9600);
  Serial.println("Starting motor rotation...");
}

void loop() {
  // Read weight sensor value
  int current_weight_raw = analogRead(WEIGHT_DATA);
  int current_weight = map(current_weight_raw, 0, 1023, 0, 1000); 
  Serial.print("Current weight: ");
  Serial.println(current_weight);

  // If weight exceeds threshold, stop the motor
  if (current_weight > 100) {
    Serial.println("Weight limit exceeded. Stopping motor...");
    stopMotor();
    delay(500); // Pause for a moment
    return;
  }

  // Rotate motor in counterclockwise direction
  rotateMotorStep(1); // Use 1 for clockwise, -1 for counterclockwise
}
