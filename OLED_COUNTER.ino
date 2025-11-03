#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "RTClib.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C
#define OLED_RST 4   // tie to D4 or use -1 if OLED reset is wired to Arduino reset

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);
RTC_DS3231 rtc;

void setup() {
  Wire.begin();
  Wire.setClock(100000);
  Serial.begin(9600);
  delay(1000);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("SSD1306 allocation failed"));
    while (true);
  }

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (true);
  }

  // Uncomment ONCE to set time, then comment again
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(20, 25);
  display.println("RTC Clock Ready");
  display.display();
  delay(1000);
}

void loop() {
  DateTime now = rtc.now();

  display.clearDisplay();

  // === DATE LINE ===
  display.setTextSize(1);
  display.setCursor(22, 8);
  display.print(now.month());
  display.print("/");
  display.print(now.day());
  display.print("/");
  display.print(now.year());

  // === TIME LINE ===
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
  delay(200);
}
