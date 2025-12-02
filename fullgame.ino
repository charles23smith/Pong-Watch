#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino_LSM6DS3.h>
#include <RTClib.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
RTC_DS3231 rtc;

// ======================
// --- IMU Variables ----
// ======================
float x, y, z;
bool showTime = true;   // Toggle between Time Mode and Game Mode

// ======================
// --- PONG CONSTANTS ---
// ======================
const uint8_t PADDLE_HEIGHT = 10;
const uint8_t PADDLE_WIDTH  = 3;
const uint8_t BALL_RADIUS   = 2;

int paddleLeftY  = 20;
int paddleRightY = 20;
int ballX = SCREEN_WIDTH / 2;
int ballY = SCREEN_HEIGHT / 2;
int ballDX = 1, ballDY = 1;

int score = 0;
int lives = 3;

// =====================================================================
//                             GAME HELPERS
// =====================================================================
void resetBallAndPaddles() {
  ballX = SCREEN_WIDTH / 2;
  ballY = SCREEN_HEIGHT / 2;

  do { ballDX = random(-1, 2); } while (ballDX == 0);
  do { ballDY = random(-1, 2); } while (ballDY == 0);

  paddleLeftY  = random(0, SCREEN_HEIGHT - PADDLE_HEIGHT);
  paddleRightY = random(0, SCREEN_HEIGHT - PADDLE_HEIGHT);
}

void showLifeLostScreen() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 10);
  display.print("LIFE -1");

  display.setTextSize(1);
  display.setCursor(10, 35);
  display.print("Lives left: ");
  display.print(lives);

  display.setCursor(10, 45);
  display.print("Score: ");
  display.print(score);

  display.display();
  delay(1500);
}

void showGameOverScreen() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(5, 10);
  display.print("GAME OVER");

  display.setTextSize(1);
  display.setCursor(10, 40);
  display.print("Final score: ");
  display.print(score);

  display.display();
}

// Draw everything for pong mode
void drawPong() {
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Score:");
  display.print(score);

  display.setCursor(80, 0);
  display.print("Lives:");
  display.print(lives);

  display.fillRect(2, paddleLeftY, PADDLE_WIDTH, PADDLE_HEIGHT, SSD1306_WHITE);
  display.fillRect(SCREEN_WIDTH - 5, paddleRightY, PADDLE_WIDTH, PADDLE_HEIGHT, SSD1306_WHITE);

  display.fillCircle(ballX, ballY, BALL_RADIUS, SSD1306_WHITE);

  display.display();
}

/**
 * updateBall()
 * Handles collisions + scoring
 */
int updateBall() {
  int newX = ballX + ballDX;
  int newY = ballY + ballDY;

  if (newY <= 0 || newY >= SCREEN_HEIGHT - 1) {
    ballDY = -ballDY;
    newY += ballDY * 2;
  }

  const int LEFT_PADDLE_FRONT = 5;
  if (newX <= LEFT_PADDLE_FRONT &&
      newY >= paddleLeftY &&
      newY <= paddleLeftY + PADDLE_HEIGHT) {
    ballDX = -ballDX;
    newX = LEFT_PADDLE_FRONT;
    score++;
  }

  const int RIGHT_PADDLE_FRONT = SCREEN_WIDTH - 5;
  if (newX >= RIGHT_PADDLE_FRONT &&
      newY >= paddleRightY &&
      newY <= paddleRightY + PADDLE_HEIGHT) {
    ballDX = -ballDX;
    newX = RIGHT_PADDLE_FRONT;
    score++;
  }

  if (newX <= 0 || newX >= SCREEN_WIDTH - 1) {
    return 1;
  }

  ballX = newX;
  ballY = newY;

  return 0;
}

// =====================================================================
//                               SETUP
// =====================================================================
void setup() {
  delay(500);
  Serial.begin(9600);
  delay(200);

  pinMode(2, INPUT_PULLUP); // Toggle button

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while (1);
  }
  display.clearDisplay();

  if (!IMU.begin()) {
    Serial.println("IMU not found");
    while (1);
  }

  if (!rtc.begin()) {
    Serial.println("RTC not found");
    while (1);
  }

  // Use this ONCE to set time, then comment it out
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  randomSeed(analogRead(0));
  resetBallAndPaddles();

  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(20, 25);
  display.print("Watch Ready");
  display.display();
  delay(1200);
}

// =====================================================================
//                                 LOOP
// =====================================================================
void loop() {

  // -------------------------------
  // Toggle between Time / Pong
  // -------------------------------
  if (digitalRead(2) == LOW) {
    showTime = !showTime;
    delay(250);
  }

  // ================================================================
  //                            CLOCK MODE
  // ================================================================
  if (showTime) {
    DateTime now = rtc.now();

    display.clearDisplay();

    // DATE
    display.setTextSize(1);
    display.setCursor(22, 8);
    display.print(now.month());
    display.print("/");
    display.print(now.day());
    display.print("/");
    display.print(now.year());

    // TIME
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
    return;
  }

  // ================================================================
  //                           PONG MODE
  // ================================================================
  int result = updateBall();

  if (result == 1) {
    lives--;
    showLifeLostScreen();

    if (lives <= 0) {
      showGameOverScreen();
      while (1);
    }

    resetBallAndPaddles();
  }

  drawPong();

  // IMU control for paddles
  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(x, y, z);

    float pitch = atan2(x, sqrt(y*y + z*z)) * 180.0 / PI;
    float roll  = atan2(y, sqrt(x*x + z*z)) * 180.0 / PI;

    if (roll > 50)  paddleLeftY  -= 2;
    if (roll < -50) paddleLeftY  += 2;

    if (pitch > 50)  paddleRightY -= 2;
    if (pitch < -50) paddleRightY += 2;

    paddleLeftY  = constrain(paddleLeftY, 0, SCREEN_HEIGHT - PADDLE_HEIGHT);
    paddleRightY = constrain(paddleRightY, 0, SCREEN_HEIGHT - PADDLE_HEIGHT);
  }

  delay(30);
}
