#define WEIGHT_DATA A1
#define WEIGHT_POWER 7

#define MIN_WEIGHT 200 //can be adjusted (units in I have no idea)
#define WEIGHT_DEVIANCE 25 //can be adjusted (units in no clue)
#define STABILITY_TIME 2000

unsigned long last_stable_time = 0;  // Stores last time the weight was stable
int last_weight = 0;                  // Stores the last stable weight reading

#define DEMO_SCALE 10 // adjustable 
#define DEMO_INTERVAL 10000 //seconds for now

struct pet_in_list* pets;

enum weight_class {
  S, M, L 
};
struct pet_in_list{
  int weight;
  enum weight_class size; 
  int time_fed;
  struct pet_in_list *next_pet; 
};

enum machine_states {
  SLEEP, ACTIVE
};

bool check_list(int weight);
void add_pet(int weight);
struct pet_in_list* find_pet(int weight);
enum weight_class find_weight_class(int weight);

void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);

  pinMode(WEIGHT_DATA, INPUT);
  pinMode(WEIGHT_POWER, OUTPUT);

  digitalWrite(WEIGHT_POWER, HIGH);

  //initiating linked list (Matt Pan would be proud)
  pets = NULL; 

}

void loop() {
  // put your main code here, to run repeatedly:

  //variables to be used in code
  unsigned long time_now = millis();
  struct pet_in_list* pet_id; 

  int desired_interval = DEMO_INTERVAL; 

    //check weight sensor
    float current_weight_raw = analogRead(WEIGHT_DATA);
    float current_weight = map(current_weight_raw, 0, 1023, 0, 1000); //jank stuff. don't mess with it

    Serial.println(current_weight);

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

      if (current_weight > MIN_WEIGHT){
        if (!(check_list(current_weight))){
          add_pet(current_weight);
          Serial.print("new pet added, size: ");
          Serial.println(current_weight);
        }
        pet_id = find_pet(current_weight);

        //feed animal if the time is right
        if ( (time_now - pet_id->time_fed) > desired_interval){ 
          pet_id->time_fed = time_now;
          
          Serial.print("Feeding Pet with size: ");
          Serial.println(pet_id->size);
          Serial.print("Pet Weight: ");
          Serial.println(pet_id->weight);
          delay(500);
          
        }
        else{
          Serial.println("Pet already fed, please wait until feeding time");
          // DigitalWrite(TIME_LED, HIGH); // add new led for status
          delay(1000);
          // DigitalWrite(TIME_LED, LOW);
        }
      }

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

//sensor and actuator functions
enum weight_class find_weight_class(int weight){
  if (200 <= weight && weight < 500) {
    return S;
  }
  else if (500 <= weight && weight < 600){
    return M;
  }
  else if (600 <= weight){
    return L;
  }
}
