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

// --- Paddle + ball constants ---
const uint8_t PADDLE_HEIGHT = 10;   // smaller paddle = stricter threshold
const uint8_t PADDLE_WIDTH  = 3;
const uint8_t BALL_RADIUS   = 2;

// Pong objects
int paddleLeftY  = 20;
int paddleRightY = 20;
int ballX = SCREEN_WIDTH / 2;
int ballY = SCREEN_HEIGHT / 2;
int ballDX = 1, ballDY = 1;

// Game state
int score = 0;
int lives = 3;

// ---- PONG HELPERS ----

void resetBallAndPaddles() {
  // Center the ball
  ballX = SCREEN_WIDTH / 2;
  ballY = SCREEN_HEIGHT / 2;

  // Random-ish starting direction
  do { ballDX = random(-1, 2); } while (ballDX == 0);
  do { ballDY = random(-1, 2); } while (ballDY == 0);

  // Random starting paddle positions (stay fully on-screen)
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

// Draw pong field + HUD
void drawPong() {
  display.clearDisplay();

  // HUD
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Score:");
  display.print(score);

  display.setCursor(80, 0);
  display.print("Lives:");
  display.print(lives);

  // Paddles
  display.fillRect(2, paddleLeftY, PADDLE_WIDTH, PADDLE_HEIGHT, SSD1306_WHITE);
  display.fillRect(SCREEN_WIDTH - 5, paddleRightY, PADDLE_WIDTH, PADDLE_HEIGHT, SSD1306_WHITE);

  // Ball
  display.fillCircle(ballX, ballY, BALL_RADIUS, SSD1306_WHITE);

  display.display();
}

/**
 * updateBall()
 *  - uses newX/newY like your CPU sketch
 *  - stricter hitbox using PADDLE_HEIGHT (smaller than before)
 *  - returns:
 *      0 -> normal play
 *      1 -> ball went out of bounds (life lost)
 */
int updateBall() {
  // Predict new position
  int newX = ballX + ballDX;
  int newY = ballY + ballDY;

  // Top / bottom collisions
  if (newY <= 0 || newY >= SCREEN_HEIGHT - 1) {
    ballDY = -ballDY;
    newY += ballDY * 2;  // nudge back inside
  }

  const int LEFT_PADDLE_FRONT = 5;
  if (newX <= LEFT_PADDLE_FRONT &&
      newY >= paddleLeftY &&
      newY <= paddleLeftY + PADDLE_HEIGHT) {
    ballDX = -ballDX;
    newX = LEFT_PADDLE_FRONT;
    score++;  // hit
  }

  const int RIGHT_PADDLE_FRONT = SCREEN_WIDTH - 5;
  if (newX >= RIGHT_PADDLE_FRONT &&
      newY >= paddleRightY &&
      newY <= paddleRightY + PADDLE_HEIGHT) {
    ballDX = -ballDX;
    newX = RIGHT_PADDLE_FRONT;
    score++;  // hit
  }

  if (newX <= 0 || newX >= SCREEN_WIDTH - 1) {
    return 1;  // life lost
  }

  // Commit move
  ballX = newX;
  ballY = newY;

  return 0;
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

  if (!rtc.begin()) {
    Serial.println("RTC not found");
    while (1);
  }

  // Uncomment ONCE to sync RTC to compile time, then comment again.
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  randomSeed(analogRead(0));
  resetBallAndPaddles();
}

void loop() {
  // Toggle between Time mode and Pong mode
  if (digitalRead(2) == LOW) {
    showTime = !showTime;
    delay(250); // debounce
  }

  // ----- TIME MODE -----
  if (showTime) {
    DateTime now = rtc.now();

    int hrs = now.hour();
    int mins = now.minute();
    int secs = now.second();

    display.clearDisplay();
    display.setCursor(0, 20);

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

  // ----- PONG MODE -----
  else {
    int result = updateBall();

    if (result == 1) {  // life lost
      lives--;
      showLifeLostScreen();

      if (lives <= 0) {
        showGameOverScreen();
        while (1) { } // freeze on game over
      }

      resetBallAndPaddles();
    }

    // Draw updated frame
    drawPong();

    // IMU-controlled paddles
    if (IMU.accelerationAvailable()) {
      IMU.readAcceleration(x, y, z);

      float pitch = atan2(x, sqrt(y * y + z * z)) * 180.0 / PI;
      float roll  = atan2(y, sqrt(x * x + z * z)) * 180.0 / PI;

      if (roll > 50)  paddleLeftY  -= 2;
      if (roll < -50) paddleLeftY  += 2;

      if (pitch > 50)  paddleRightY -= 2;
      if (pitch < -50) paddleRightY += 2;

      // Constrain using PADDLE_HEIGHT
      paddleLeftY  = constrain(paddleLeftY, 0, SCREEN_HEIGHT - PADDLE_HEIGHT);
      paddleRightY = constrain(paddleRightY, 0, SCREEN_HEIGHT - PADDLE_HEIGHT);
    }

    delay(30);
  }
}
