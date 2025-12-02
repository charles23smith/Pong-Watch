#include <Arduino_LSM6DS3.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "RTClib.h"

// ======================
// --- OLED SETTINGS ----
// ======================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C
#define OLED_RST 4

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

// ======================
// ------- RTC ----------
// ======================
RTC_DS3231 rtc;

// ======================
// ---- IMU SETTINGS -----
// ======================
float x, y, z;
const float DEAD_ZONE = 50;   // sensitivity threshold

// =====================================================
// SETUP
// =====================================================
void setup() {
  Serial.begin(9600);
  while (!Serial);

  // --- I2C + OLED ---
  Wire.begin();
  Wire.setClock(100000);
  delay(200);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("SSD1306 allocation failed");
    while (1);
  }

  // --- RTC INIT ---
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  // Uncomment ONCE to set current time, upload, then comment again:
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  // --- IMU INIT ---
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }
  Serial.println("IMU ready.");

  // Startup screen
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(20, 25);
  display.println("Watch Ready");
  display.display();
  delay(1000);
}

// =====================================================
// LOOP
// =====================================================
void loop() {
  // =============================
  // 1. Read RTC and draw clock
  // =============================
  DateTime now = rtc.now();

  display.clearDisplay();

  // --- DATE ---
  display.setTextSize(1);
  display.setCursor(22, 8);
  display.print(now.month());
  display.print("/");
  display.print(now.day());
  display.print("/");
  display.print(now.year());

  // --- TIME ---
  display.setTextSize(2);
  display.setCursor(10, 28);

  if (now.hour() < 10) display.print("0");
  display.print(now.hour());
  display.print(":");
  if (now.minute() < 10) display.print("0");
  display.print(now.minute());
  display.print(":");
  if (now.second() < 10) display.print("0");
  display.print(now.second());

  display.display();

  // =============================
  // 2. IMU Tilt Detection
  // =============================
  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(x, y, z);

    float pitch = atan2(x, sqrt(y*y + z*z)) * 180.0 / PI;
    float roll  = atan2(y, sqrt(x*x + z*z)) * 180.0 / PI;

    if (abs(roll) >= DEAD_ZONE) {
      if (roll > DEAD_ZONE)  Serial.println("Left board down");
      if (roll < -DEAD_ZONE) Serial.println("Left board up");
    }

    if (abs(pitch) >= DEAD_ZONE) {
      if (pitch > DEAD_ZONE)  Serial.println("Right board up");
      if (pitch < -DEAD_ZONE) Serial.println("Right board down");
    }
  }

  delay(200);
}
