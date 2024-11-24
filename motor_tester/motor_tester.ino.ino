//Group 17 MREN 318 Pet Feeding Software
//website used for pet feeding values: https://www.petmd.com/dog/nutrition/are-you-feeding-your-dog-right-amount 


//defining pin values for sensors & actuators
#define CAMERA_DATA 1 //to adjust
#define CAMERA_POWER 2 //to adjust
#define POTENTIOMETER_DATA A2
#define POTENTIOMETER_POWER 14 //to adjust
#define WEIGHT_DATA A1
#define WEIGHT_POWER 8
#define INFRARED_DATA A0

// MOTOR SET UP
const int IN1 = 2;  // L298N IN1
const int IN2 = 3;  // L298N IN2
const int IN3 = 4; // L298N IN3
const int IN4 = 5; // L298N IN4
const int stepDelay = 5; // Adjust this value for speed control
// step pattern.
const int stepSequence[4][4] = {
  {1, 0, 1, 0}, // Step 1
  {0, 1, 1, 0}, // Step 2
  {0, 1, 0, 1}, // Step 3
  {1, 0, 0, 1}  // Step 4
};

#define LIMIT_SWITCH_PIN  15

//defining led pins
#define SLEEP_LED 13
#define FOOD_LOW_LED 12
#define DISPENSING_LED 11
#define CAMERA_LED 10

// defining adjustable values
#define DISTANCE 20 //can be adjusted (units in cm)
#define MAX_TIME_AWAKE 5000 //can be adjusted (units in milliseconds)
#define MIN_WEIGHT 3 //can be adjusted (units in lbs)
#define WEIGHT_DEVIANCE 5 //can be adjusted (units in lbs)
#define NOTCH_VOLUME 50 // can be adjusted (units in cm^3)

//Motor Values
#define MOTOR_STEPS_PER_REV 200 // Steps per revolution for the QSH4218 motor
#define NOTCH_STEPS (MOTOR_STEPS_PER_REV / 2) // Steps per notch

//for demo purposes - can be changed
#define DEMO_INTERVAL 1 //1 minutes for now
#define TIME_CONVERT 1.0/6000 //convert milliseconds to minutes (for final product would be hours) 
#define DEMO_SCALE 10 // adjustable 

//defining enums & structs we're going to use in code
enum machine_states {
  SLEEP, ACTIVE
};
enum weight_class { //might have to reduce sizes to S,M,L
  XS, S, SM, M, ML, L, XL 
};
struct pet_in_list{
  int weight;
  enum weight_class size; 
  int time_fed;
  struct pet_in_list *next_pet; 
};

//global variables (even thought they're bad practice (teehee))
enum machine_states state;
unsigned long time_last_active;
struct pet_in_list* pets;

//function declerations
bool check_list(int weight);
void add_pet(int weight);
struct pet_in_list* find_pet(int weight);
void activate_sleep();
void wake_up();
enum weight_class find_weight_class(int weight);
void check_camera();
void dispense_food(enum weight_class wc);
void homeRotaryValve();


void setup() {
  // put your setup code here, to run once:
  //setting up sensor pins
  pinMode(CAMERA_DATA, INPUT);
  pinMode(CAMERA_POWER, OUTPUT);

  pinMode(POTENTIOMETER_DATA, INPUT);
  pinMode(POTENTIOMETER_POWER, OUTPUT);

  pinMode(WEIGHT_DATA, INPUT);
  pinMode(WEIGHT_POWER, OUTPUT);

  pinMode(INFRARED_DATA, INPUT);
  pinMode(SLEEP_LED, OUTPUT);
  pinMode(FOOD_LOW_LED, OUTPUT); 
  pinMode(DISPENSING_LED, OUTPUT);
  
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);



  //initiate LEDS
  digitalWrite(SLEEP_LED, LOW);
  digitalWrite(FOOD_LOW_LED, LOW);
  digitalWrite(DISPENSING_LED, LOW);
  digitalWrite(CAMERA_LED, LOW);

  //initiate the sensors
  digitalWrite(WEIGHT_POWER, HIGH);

  //setup petfeeder to initiate in sleep mode
  activate_sleep();
  state = SLEEP; 
  time_last_active = 0;

  //initiating linked list (Matt Pan would be proud)
  pets = NULL; 

}

void loop() {
  // put your main code here, to run repeatedly:
  //update clock
  unsigned long time_now = millis();

  //variables to be used in code
  struct pet_in_list* pet_id; 

  //update IR sensor
  int proximity_raw = analogRead(INFRARED_DATA)*0.0048828125; // taken from lab1 part 2, which was taken from chat??
  int proximity = 13*pow(proximity_raw, -1); //taken from lab1 part 2, again, worked out from datasheet graph

  //update states
  if ( (proximity < DISTANCE) && (state == SLEEP)){
    //machine is sleeping, but senses animal
    wake_up();
    state = ACTIVE;
    time_last_active = time_now;
  }
  else if ( (state == ACTIVE) && ( (time_now - time_last_active) >= MAX_TIME_AWAKE)){
    //machine is awake, but times out
    activate_sleep();
    state = SLEEP;
  }
  else if ( (state == ACTIVE) && (proximity < DISTANCE) ){
    //machine is awake and animal is present
    time_last_active = time_now;
  }

  //state machine things
  if (state == ACTIVE){
    //check potentiometer - need one
    //desired_interval_raw = analogRead(POTENTIOMETER_DATA);
    //desired_interval = map(desired_interval_raw, 0, (max value (V)), min value, max value); 
    int desired_interval = DEMO_INTERVAL; 

    //check weight sensor
    int current_weight_raw = analogRead(WEIGHT_DATA);
    int current_weight = map(current_weight_raw, 0, 1023, 0, 150); //2o lbs is what the sensor supports, but we're just mapping it to be a bigger number for demo purposes

    //needs a debounce + way to get rid of noise
    if (current_weight > MIN_WEIGHT){
      if (!(check_list(current_weight))){
        add_pet(current_weight);
      }
      pet_id = find_pet(current_weight);

      //feed animal if the time is right
      if ( (time_now - pet_id->time_fed) > desired_interval){ 
        pet_id->time_fed = time_now;
        dispense_food(pet_id->size);
      }
    }

    check_camera();
  }

}

//Matt Pan functions
bool check_list(int weight){
  //check if the pet exists
  struct pet_in_list* temp = pets;

  while (temp != NULL){
    if ( (weight < (temp->weight + WEIGHT_DEVIANCE)) && (weight > (temp->weight - WEIGHT_DEVIANCE))){
      return true;
    }
    else {
      temp = temp->next_pet;
    }
  }
  return false;

}

void add_pet(int weight){
  //if pet doesn't exist, add it. 
  struct pet_in_list* new_pet = (struct pet_in_list*)malloc(sizeof(struct pet_in_list));
  if (new_pet != NULL){
    new_pet->weight = weight;
    new_pet->size = find_weight_class(weight);
    new_pet->time_fed = 999; //for logic reasons
    new_pet->next_pet = NULL;
  }

  if (pets == NULL){
    //special case for the first in the list
    pets = new_pet; 
  }
  else{
    struct pet_in_list* temp = pets;
    while ( temp->next_pet != NULL ){
      temp = temp->next_pet;
    }  
    if (temp->next_pet == NULL){
      temp->next_pet = new_pet;
    }
  }
}

struct pet_in_list* find_pet(int weight){
  //return a pointer to the pet in the list
  struct pet_in_list* temp = pets;
  while ( temp != NULL ){
    if ((weight < (temp->weight + WEIGHT_DEVIANCE)) && (weight > (temp->weight - WEIGHT_DEVIANCE))){
      return temp;
    }
    else {
      temp = temp->next_pet;
    }
  }  
  return NULL;
}

//state machine functions
void activate_sleep() {
  //status led 
  digitalWrite(SLEEP_LED, HIGH);
  //control power to sensors being unused
  digitalWrite(CAMERA_POWER, LOW);
  digitalWrite(POTENTIOMETER_POWER, LOW);
  digitalWrite(WEIGHT_POWER, LOW);
  digitalWrite(CAMERA_LED, LOW);
}

void wake_up(){
  //status led
  digitalWrite(SLEEP_LED, LOW);
  //control power to sensors that will be used
  digitalWrite(CAMERA_POWER, HIGH);
  digitalWrite(POTENTIOMETER_POWER, HIGH);
  digitalWrite(WEIGHT_POWER, HIGH);
  digitalWrite(CAMERA_LED, HIGH);
}

//sensor and actuator functions
enum weight_class find_weight_class(int weight){
  if (3 >= weight && weight < 13) {
    return XS;
  }
  else if (13 >= weight && weight < 21){
    return S;
  }
  else if (21 >= weight && weight < 35){
    return SM;
  }
  else if (35 >= weight && weight < 51){
    return M;
  }
  else if (51 >= weight && weight < 75){
    return ML;
  }
  else if (75 >= weight && weight < 100){
    return L;
  }
  else {
    return XL;
  }
}

void check_camera(){
  //add code here
  /*
  if ( low food ){
    digitalWrite(FOOD_LOW_LED, HIGH);
  }
  */
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
  //digitalWrite(MOTOR_EN, LOW); // Active low for enabling A4988

  // Rotate the motor to dispense the food
  stepMotor(notches_needed,1);

  // Disable the motor
  digitalWrite(DISPENSING_LED, LOW);
}



// void homeRotaryValve() { 
//   while (digitalRead(LIMIT_SWITCH_PIN) == HIGH) { // Assuming LOW when triggered
//     digitalWrite(MOTOR_DIR, LOW); // Rotate backward
//     digitalWrite(MOTOR_STEP, HIGH);
//     delayMicroseconds(1000);
//     digitalWrite(MOTOR_STEP, LOW);
//     delayMicroseconds(1000);
//   }
//   // Stop motor
//   digitalWrite(MOTOR_EN, HIGH);
// }

// Function to set motor pins
void setMotorPins(int a, int b, int c, int d) {
  digitalWrite(IN1, a);
  digitalWrite(IN2, b);
  digitalWrite(IN3, c);
  digitalWrite(IN4, d);
}

void stepMotor(int steps, int direction) {
  for (int i = 0; i < steps; i++) {
    // Loop through the step sequence
    for (int j = 0; j < 4; j++) {
      int index = (direction == 1) ? j : (3 - j); // Reverse direction if needed
      setMotorPins(stepSequence[index][0], stepSequence[index][1],
                   stepSequence[index][2], stepSequence[index][3]);
      delay(stepDelay);
    }
  }
}

