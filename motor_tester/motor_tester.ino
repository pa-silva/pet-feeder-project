#define DIR 6
#define STEP 7
// #define 

#define ROTATION 200

void setup() {
  // put your setup code here, to run once:
  pinMode(DIR, OUTPUT);
  pinMode(STEP, OUTPUT);

  digitalWrite(DIR, LOW);

  Serial.begin(9600);

}

void loop() {
  // put your main code here, to run repeatedly:
  if (1){
    char incoming = Serial.read();
    if (1){
      Serial.println("I like to move it move it, I like to move it move it, I like to move it");
      Serial.println("- King Julien");
      move_step(500);
    }
  }
}

void move_step(int step) {
  for(int i = 0; i < step; i++){
    delayMicroseconds(3000);
    digitalWrite(STEP, HIGH);
    delayMicroseconds(3000);
    digitalWrite(STEP, LOW);

  }
}