#define INFRARED_DATA A0

#define SLEEP_LED 13

#define DISTANCE 20 //can be adjusted (units in cm)
#define MAX_TIME_AWAKE 5000 //can be adjusted (units in milliseconds)

enum machine_states {
  SLEEP, ACTIVE
};

enum machine_states state;
unsigned long time_last_active;


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
    
  pinMode(SLEEP_LED, OUTPUT);
  pinMode(INFRARED_DATA, INPUT);

  digitalWrite(SLEEP_LED, LOW);
  //setup petfeeder to initiate in sleep mode
  activate_sleep();
  state = SLEEP; 
  time_last_active = 0;

  Serial.println("Setup complete");
}

void loop() {
  // put your main code here, to run repeatedly:
  unsigned long time_now = millis();

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

  if (state == ACTIVE){
    Serial.println("active");
    delay(500);
    Serial.println(proximity);
  }
  else {
    Serial.println("asleep");
    delay(500);
  }
}

void activate_sleep() {
  //status led 
  digitalWrite(SLEEP_LED, HIGH);
  //control power to sensors being unused
  // digitalWrite(CAMERA_POWER, LOW);
  // digitalWrite(POTENTIOMETER_POWER, LOW);
  // digitalWrite(WEIGHT_POWER, LOW);
  // digitalWrite(MOTOR_EN, HIGH);
  // digitalWrite(CAMERA_LED, LOW);
}

void wake_up(){
  //status led
  digitalWrite(SLEEP_LED, LOW);
  //control power to sensors that will be used
  // digitalWrite(CAMERA_POWER, HIGH);
  // digitalWrite(POTENTIOMETER_POWER, HIGH);
  // digitalWrite(WEIGHT_POWER, HIGH);
  // digitalWrite(MOTOR_EN, LOW);
  // digitalWrite(CAMERA_LED, HIGH);
}
