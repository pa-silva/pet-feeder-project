
#define MOTOR_STEP 7
#define MOTOR_DIR 8
#define MOTOR_EN 13

//Motor Values
#define MOTOR_STEPS_PER_REV 200 // Steps per revolution for the QSH4218 motor
#define NOTCH_STEPS (MOTOR_STEPS_PER_REV / 2) // Steps per notch

void setup() {
  // put your setup code here, to run once:
    pinMode(MOTOR_STEP, OUTPUT);
    pinMode(MOTOR_DIR, OUTPUT);
    pinMode(MOTOR_EN, OUTPUT);

    pinMode(SLEEP_LED, OUTPUT);
    pinMode(FOOD_LOW_LED, OUTPUT); 
    pinMode(DISPENSING_LED, OUTPUT);

    //initiate LEDS
    digitalWrite(SLEEP_LED, LOW);
    digitalWrite(FOOD_LOW_LED, LOW);
    digitalWrite(DISPENSING_LED, LOW);
    digitalWrite(CAMERA_LED, LOW);

    //initiate the motor
    digitalWrite(MOTOR_STEP, LOW);
    digitalWrite(MOTOR_DIR, HIGH); // Default direction

    //setup petfeeder to initiate in sleep mode
    activate_sleep();
    state = SLEEP; 
    time_last_active = 0;

    //initiating linked list (Matt Pan would be proud)
    pets = NULL; 
}

void loop() {
  // put your main code here, to run repeatedly:

}

void dispense_food(enum weight_class wc){
  digitalWrite(DISPENSING_LED, HIGH);
  // Define food amounts for each weight class
  int food_volume;
  
  switch (wc) {
    case XS: 
      food_volume = 118.5 / DEMO_SCALE;  // ½ cup = 118.5 cm³
      break; 
    case S: 
      food_volume = 296.25 / DEMO_SCALE;  // 1¼ cups = 296.25 cm³
      break;
    case SM: 
      food_volume = 396.75 / DEMO_SCALE;  // 1⅔ cups = 396.75 cm³
      break;
    case M: 
      food_volume = 557.25 / DEMO_SCALE;  // 2⅓ cups = 557.25 cm³
      break;
    case ML:
      food_volume = 711 / DEMO_SCALE;  // 3 cups = 711 cm³
      break;
    case L: 
      food_volume = 890.25 / DEMO_SCALE;  // 3¾ cups = 890.25 cm³
      break;
    case XL:
      food_volume = 1115.5 / DEMO_SCALE;  // 4⅔ cups = 1115.5 cm³
      break;
  }

  // Calculate the number of notches needed to dispense the desired volume
  int notches_needed = food_volume / NOTCH_VOLUME;  // Total number of notches based on volume

  // Enable the motor
  digitalWrite(MOTOR_EN, LOW); // Active low for enabling A4988

  // Rotate the motor to dispense the food
  for (int i = 0; i < notches_needed; i++) {
    digitalWrite(MOTOR_STEP, HIGH);
    delayMicroseconds(800); // Step pulse width (adjust for motor speed)
    digitalWrite(MOTOR_STEP, LOW);
    delayMicroseconds(800); // Step interval
  }

  // Disable the motor
  digitalWrite(MOTOR_EN, HIGH); // Disable motor when idle
  digitalWrite(DISPENSING_LED, LOW);
}

void homeRotaryValve() { 
  while (digitalRead(LIMIT_SWITCH_PIN) == HIGH) { // Assuming LOW when triggered
    digitalWrite(MOTOR_DIR, LOW); // Rotate backward
    digitalWrite(MOTOR_STEP, HIGH);
    delayMicroseconds(1000);
    digitalWrite(MOTOR_STEP, LOW);
    delayMicroseconds(1000);
  }
  // Stop motor
  digitalWrite(MOTOR_EN, HIGH);
}