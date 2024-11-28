#include <ArduCAM.h>
#include <SPI.h>
#include <Wire.h>

#define FOOD_LED 4
#define CAMERA_LED 1
#define CAMERA_POWER 9

// Define the camera module
#define CS_PIN 10  // Chip Select Pin for SPI (default for Uno R4)
ArduCAM myCAM(OV2640, CS_PIN);

void setup() {
  Serial.begin(115200);
  Serial.println("Arducam Light Intensity Test on Uno R4");

  pinMode(CAMERA_POWER, OUTPUT);
  digitalWrite(CAMERA_POWER, HIGH);
  // Initialize SPI and camera
  Wire.begin(); // SDA and SCL (near AREF on Uno R4)
  SPI.begin();
  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);

  // Power the camera

  // Allow time for the camera to power on
  delay(500);

  // Initialize camera
  Serial.println("Starting camera initialization");
  myCAM.write_reg(0x07, 0x80);  // Reset the camera
  delay(500);                    // Increased delay for reset
  myCAM.write_reg(0x07, 0x00);  // End reset
  delay(500);                    // Increased delay for reset completion

  // Set the camera format to RGB (RAW)
  myCAM.set_format(RAW);  // Use RGB565 (16-bit color)
  delay(100);

  // Set resolution to 320x240
  myCAM.OV2640_set_JPEG_size(OV2640_320x240);  // Same resolution for consistency
  delay(100);

  myCAM.InitCAM();
  Serial.println("Camera initialized");

  // Setup LED pins
  pinMode(FOOD_LED, OUTPUT);
  pinMode(CAMERA_LED, OUTPUT);

  digitalWrite(FOOD_LED, LOW);
  digitalWrite(CAMERA_LED, HIGH);

  Serial.println("Setup Complete");
}

void loop() {
  check_camera();
}

void check_camera() {
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
  if (avg_light > 70) { // Adjust threshold as needed based on environment
    Serial.println("Low Food Level");
    digitalWrite(FOOD_LED, HIGH);
  } else {
    Serial.println("Sufficient Food Level");
    digitalWrite(FOOD_LED, LOW);
  }

  delay(1000); // Wait for 1 second before the next capture
}

