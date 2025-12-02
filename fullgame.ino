#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino_LSM6DS3.h>
#include <RTClib.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
RTC_DS3231 rtc;

float x, y, z;
bool showTime = true;

// Pong objects
int paddleLeftY = 20;
int paddleRightY = 20;
int ballX = 64, ballY = 32;
int ballDX = 1, ballDY = 1;

void drawPong() {
  display.clearDisplay();
  display.fillRect(2, paddleLeftY, 3, 15, SSD1306_WHITE);
  display.fillRect(123, paddleRightY, 3, 15, SSD1306_WHITE);
  display.fillCircle(ballX, ballY, 2, SSD1306_WHITE);
  display.display();
}

void updateBall() {
  ballX += ballDX;
  ballY += ballDY;

  if (ballY <= 0 || ballY >= 63) ballDY = -ballDY;
  if (ballX <= 5 || ballX >= 123) ballDX = -ballDX;
}

void setup() {
  delay(500);
  Serial.begin(9600);
  delay(200);

  pinMode(2, INPUT_PULLUP);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while (1);
  }
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);

  if (!IMU.begin()) {
    while (1);
  }

  // -------- RTC ENABLED --------
  if (!rtc.begin()) {
    Serial.println("RTC not found");
    while (1);
  }

  // Uncomment once to sync RTC to code compile time:
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}

void loop() {

  if (digitalRead(2) == LOW) {
    showTime = !showTime;
    delay(250);
  }

  // TIME MODE (RTC)
  if (showTime) {
    DateTime now = rtc.now();   // <-- Read RTC time

    int hrs = now.hour();
    int mins = now.minute();
    int secs = now.second();

    display.clearDisplay();
    display.setCursor(0, 20);

    // Print HH:MM:SS with zero padding
    if (hrs < 10) display.print('0');
    display.print(hrs);
    display.print(":");

    if (mins < 10) display.print('0');
    display.print(mins);
    display.print(":");

    if (secs < 10) display.print('0');
    display.print(secs);

    display.display();
    delay(200);
  }

  // PONG MODE
  else {
    updateBall();
    drawPong();

    if (IMU.accelerationAvailable()) {
      IMU.readAcceleration(x, y, z);

      float pitch = atan2(x, sqrt(y*y + z*z)) * 180.0 / PI;
      float roll  = atan2(y, sqrt(x*x + z*z)) * 180.0 / PI;

      if (roll > 50) paddleLeftY -= 2;
      if (roll < -50) paddleLeftY += 2;

      if (pitch > 50) paddleRightY -= 2;
      if (pitch < -50) paddleRightY += 2;

      paddleLeftY = constrain(paddleLeftY, 0, 48);
      paddleRightY = constrain(paddleRightY, 0, 48);
    }

    delay(30);
  }
}
