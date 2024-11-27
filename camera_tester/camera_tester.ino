#include <ArduCAM.h>
#include <SPI.h>
#include <Wire.h>

#define FOOD_LED 9
#define CAMERA_LED 10

// Define the camera module
#define CS_PIN 10  // Chip Select Pin for SPI (default for Uno R4)
ArduCAM myCAM(OV2640, CS_PIN);

void setup() {
  Serial.begin(115200);
  Serial.println("Arducam Light Intensity Test on Uno R4");

  // Initialize SPI and camera
  Wire.begin(); // SDA and SCL (near AREF on Uno R4)
  SPI.begin();
  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);

  // Initialize camera
  myCAM.write_reg(0x07, 0x80); // Reset the camera
  delay(100);
  myCAM.write_reg(0x07, 0x00);
  delay(100);
  myCAM.InitCAM();
  myCAM.set_format(JPEG);
  myCAM.InitCAM();
  delay(100);

  // Set resolution to 320x240
  myCAM.OV2640_set_JPEG_size(OV2640_320x240);
  delay(100);

  //
  pinMode(FOOD_LED, OUTPUT);
  pinMode(CAMERA_LED, OUTPUT);

  digitalWrite(FOOD_LED, LOW);
  digitalWrite(CAMERA_LED, HIGH);

}

void loop() {
  check_camera();
}

void check_camera(){
  // Capture a frame
  myCAM.flush_fifo();
  myCAM.clear_fifo_flag();
  myCAM.start_capture();

  // Wait for the capture to complete
  while (!myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK));

  // Read frame data
  uint8_t light_sum = 0;
  uint32_t pixel_count = 0;

  myCAM.CS_LOW();
  myCAM.set_fifo_burst();

  // Analyze pixel data from the frame buffer
  while (true) {
    uint8_t data = SPI.transfer(0x00);

    // Check for the end of the frame (JPEG EOI marker 0xFFD9)
    if (data == 0xD9 && SPI.transfer(0x00) == 0xFF) break;

    // Sum pixel intensity (simplified grayscale approximation)
    light_sum += data;
    pixel_count++;
  }
  myCAM.CS_HIGH();

  // Calculate the average light intensity
  float avg_light = (float)light_sum / pixel_count;

  // Decide food level based on light intensity
  if (avg_light > 150) { // Adjust threshold as needed
    Serial.println("Low Food Level");
    digitalWrite(FOOD_LED, HIGH);
  } else {
    Serial.println("Sufficient Food Level");
    digitalWrite(FOOD_LED, LOW);
  }

  delay(1000); // Wait for 1 second before the next capture
}
