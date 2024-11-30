//Group 17 MREN 318 Pet Feeding Software

#include <ArduCAM.h>
#include <SPI.h>
#include <Wire.h>

//defining pin values for sensors & actuators
#define POTENTIOMETER_DATA A2
#define WEIGHT_DATA A1
#define SENSORS_POWER 8
#define INFRARED_DATA A0

//camera definitons
#define CS_PIN 10  // Chip Select Pin for SPI (default for Uno R4)
ArduCAM myCAM(OV2640, CS_PIN);

// MOTOR SET UP
#define STEP_PIN 7
#define DIRECTION_PIN 6
#define MOTOR_NENABLE 5

//defining led pins
#define SLEEP_LED 0
#define FOOD_LED 1
#define DISPENSING_LED 2
#define TIME_LED 3
#define CAMERA_LED 4

// defining adjustable values
#define DISTANCE 20 //can be adjusted (units in cm)
#define MAX_TIME_AWAKE 10000 //can be adjusted (units in milliseconds)
#define MIN_WEIGHT 200 //jank, do not touch
#define WEIGHT_DEVIANCE 25 //can be adjusted (units in i dont know)
#define CAMERA_MIN 30 //can be adjusted

//Motor Values
#define MOTOR_STEPS_PER_REV 1050 // Steps per revolution for the QSH4218 motor

//for demo purposes - can be changed
#define DEMO_INTERVAL 30000 //1 minutes for now

//defining enums & structs we're going to use in code
enum machine_states {
  SLEEP, ACTIVE
};
enum weight_class {
  S, M, L
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
  pinMode(POTENTIOMETER_DATA, INPUT);
  pinMode(WEIGHT_DATA, INPUT);
  pinMode(SENSORS_POWER, OUTPUT);

  //LED pins
  pinMode(INFRARED_DATA, INPUT);
  pinMode(SLEEP_LED, OUTPUT);
  pinMode(FOOD_LED, OUTPUT); 
  pinMode(DISPENSING_LED, OUTPUT);
  pinMode(CAMERA_LED, OUTPUT);
  pinMode(TIME_LED, OUTPUT);
  
  //motor setup
  pinMode(DIRECTION_PIN, OUTPUT);
  pinMode(STEP_PIN, OUTPUT);
  pinMode(MOTOR_NENABLE, OUTPUT);

  digitalWrite(DIRECTION_PIN, LOW);
  digitalWrite(MOTOR_NENABLE, HIGH);
  
  //camera setup  
  Serial.begin(115200);

  // Initialize SPI and camera
  Wire.begin(); // SDA and SCL (near AREF on Uno R4)
  SPI.begin();
  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);

  // Allow time for the camera to power on
  delay(500);

  // Initialize camera
  myCAM.write_reg(0x07, 0x80);  // Reset the camera
  delay(500);                    // Increased delay for reset
  myCAM.write_reg(0x07, 0x00);  // End reset
  delay(500);                    // Increased delay for reset completion

  // Set the camera format to RGB (RAW)
  myCAM.set_format(RAW);
  delay(100);

  // Set resolution to 320x240
  myCAM.OV2640_set_JPEG_size(OV2640_320x240);
  delay(100);

  myCAM.InitCAM();

  //initiate LEDS
  digitalWrite(SLEEP_LED, LOW);
  digitalWrite(FOOD_LED, LOW);
  digitalWrite(DISPENSING_LED, LOW);
  digitalWrite(CAMERA_LED, LOW);
  digitalWrite(TIME_LED, LOW);

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
  int proximity = abs(13*pow(proximity_raw, -1)); //taken from lab1 part 2, again, worked out from datasheet graph

  //update states
  if ( (proximity < DISTANCE) && (state == SLEEP)){
    //machine is sleeping, but senses animal
            
      // Start debounce period
      delay(500);

      // Collect multiple readings
      int num_samples = 5;
      int total_prox = 0;
      for (int i = 0; i < num_samples; i++) {
          delay(100);
          int temp_prox = analogRead(INFRARED_DATA)*0.0048828125;;
          temp_prox = abs(13*pow(temp_prox, -1));
          total_prox += temp_prox;
          delay(100); // Small delay between samples
      }

      // Calculate the average weight
      int avg_prox = total_prox / num_samples;

      // Check if average weight is stable
      if (avg_prox < proximity - 5 || avg_prox > proximity + 5) {
          return; // Not stable, exit and retry
      }

      // Use avg_weight as the final stabilized weight
      proximity = avg_prox;
      //end debounce period

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
    int desired_interval_raw = analogRead(POTENTIOMETER_DATA);
    int desired_interval = map(desired_interval_raw, 0, 1023, 0, 30); 
    desired_interval = DEMO_INTERVAL + desired_interval*1000; //30 sec + max 30 sec 

    //check weight sensor
    int current_weight_raw = analogRead(WEIGHT_DATA);
    int current_weight = map(current_weight_raw, 0, 1023, 0, 1000); //2o lbs is what the sensor supports, but we're just mapping it to be a bigger number for demo purposes
    Serial.println(current_weight);
    //needs a debounce + way to get rid of noise
    if (current_weight > MIN_WEIGHT){

      // Start debounce period
      delay(500);

      // Collect multiple readings
      int num_samples = 5;
      int total_weight = 0;
      for (int i = 0; i < num_samples; i++) {
          delay(100);
          int temp = analogRead(WEIGHT_DATA);
          temp = map(temp, 0, 1023, 0, 1000);
          total_weight += temp;
          delay(100); // Small delay between samples
      }

      // Calculate the average weight
      int avg_weight = total_weight / num_samples;

      // Check if average weight is stable
      if (avg_weight < current_weight - WEIGHT_DEVIANCE || avg_weight > current_weight + WEIGHT_DEVIANCE) {
          return; // Not stable, exit and retry
      }

      // Use avg_weight as the final stabilized weight
      current_weight = avg_weight;
      //end debounce period

      if (!(check_list(current_weight))){
        add_pet(current_weight);
      }
      pet_id = find_pet(current_weight);

      //feed animal if the time is right
      if ( (time_now - pet_id->time_fed) > desired_interval){ 
        pet_id->time_fed = time_now;
        dispense_food(pet_id->size);
      }
      else{
        digitalWrite(TIME_LED, HIGH);
        delay(500);
        digitalWrite(TIME_LED, LOW);
        delay(500);
        digitalWrite(TIME_LED, HIGH);
        delay(500);
        digitalWrite(TIME_LED, LOW);
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
    new_pet->time_fed = 99999; //for logic reasons
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
  digitalWrite(SENSORS_POWER, LOW);
  digitalWrite(CAMERA_LED, LOW);
}

void wake_up(){
  //status led
  digitalWrite(SLEEP_LED, LOW);
  //control power to sensors that will be used
  digitalWrite(SENSORS_POWER, HIGH);
  digitalWrite(CAMERA_LED, HIGH);
}

//sensor and actuator functions
enum weight_class find_weight_class(int weight){
  if (100 <= weight && weight < 300) {
    return S;
  }
  else if (300 <= weight && weight < 600){
    return M;
  }
  else if (600 <= weight){
    return L;
  }
}

void check_camera(){
  // Capture a frame
  myCAM.flush_fifo();
  myCAM.clear_fifo_flag();
  myCAM.start_capture();

  // Wait for the capture to complete
  while (!myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK));

  // Read frame data
  uint32_t pixel_count = 0;
  uint32_t light_sum = 0;

  myCAM.CS_LOW();
  myCAM.set_fifo_burst();

  // Read a fixed number of pixels based on the resolution (320x240 for example)
  for (uint32_t i = 0; i < 320 * 240; i++) {
    uint8_t data1 = SPI.transfer(0x00);  // Read first byte of pixel
    uint8_t data2 = SPI.transfer(0x00);  // Read second byte of pixel (for RGB565)

    // Combine two bytes to form the RGB565 value (16-bit)
    uint16_t rgb565 = (data1 << 8) | data2;

    // Extract the RGB components from RGB565
    uint8_t r = (rgb565 >> 11) & 0x1F;   // Red (5 bits)
    uint8_t g = (rgb565 >> 5) & 0x3F;    // Green (6 bits)
    uint8_t b = rgb565 & 0x1F;           // Blue (5 bits)

    // Optionally, normalize the components to 0-255 range if needed
    r = map(r, 0, 31, 0, 255);  // Scale to 8 bits
    g = map(g, 0, 63, 0, 255);  // Scale to 8 bits
    b = map(b, 0, 31, 0, 255);  // Scale to 8 bits

    // Focus on the red channel for intensity calculation
    uint8_t intensity = r;  // Use the red channel as the light intensity

    // Accumulate intensity
    light_sum += intensity;
    pixel_count++;
  }

  myCAM.CS_HIGH();

  // Calculate the average light intensity
  float avg_light = (float)light_sum / pixel_count;
  Serial.println(avg_light);

  // Decide food level based on light intensity (focus on red)
  if (avg_light > CAMERA_MIN) { // Adjust threshold as needed based on environment
    digitalWrite(FOOD_LED, HIGH);
  } else {
    digitalWrite(FOOD_LED, LOW);
  }

  delay(1000); // Wait for 1 second before the next capture
}

void dispense_food(enum weight_class wc){
  digitalWrite(DISPENSING_LED, HIGH);
  // Define food amounts for each weight class
  float food_volume;
  int food_steps;

  switch (wc) {
    case S: 
      food_volume = 0.5;  // demo
      break;
    case M: 
      food_volume = 1;  // demo
      break;
    case L: 
      food_volume = 2;  // demo
      break;
  }
  //convert rotations to steps
  food_steps = food_volume*MOTOR_STEPS_PER_REV;

  // Enable the motor
  digitalWrite(MOTOR_NENABLE, LOW);

  // Rotate the motor to dispense the food
  stepMotor(food_steps);

  //bounce the motor in case of jamming
    delay(100);
    digitalWrite(DIRECTION_PIN, HIGH);
    stepMotor(100);
    delay(100);
    digitalWrite(DIRECTION_PIN, LOW);
    stepMotor(100);
    delay(100);

  // Disable the motor
  digitalWrite(MOTOR_NENABLE, HIGH);

  digitalWrite(DISPENSING_LED, LOW);
}

void stepMotor(int steps) {
  for(int i = 0; i < steps; i++){
    delayMicroseconds(3000);
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(3000);
    digitalWrite(STEP_PIN, LOW);

  }
}
