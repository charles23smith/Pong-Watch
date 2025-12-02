#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino_LSM6DS3.h>
#include <RTClib.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
RTC_DS3231 rtc;

enum GameState { WATCH, PONG, STICKMAN, GAME_OVER };
GameState gameState = WATCH;

const int BUTTON_PIN = 2;

float x, y, z;

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

const uint8_t MIN_SIZE = 1;
const uint8_t MAX_SIZE = 5;
uint8_t sizeLevel = MIN_SIZE;

float figureX = SCREEN_WIDTH / 2.0f;
int   figureDir = 1;

uint8_t gearLevel = 0;
int fullRotations = 0;

float speedForSize[MAX_SIZE + 1] = {
  0.0f,
  0.4f,
  0.6f,
  0.8f,
  1.0f,
  1.2f
};

unsigned long lastUpdate = 0;
const unsigned long FRAME_RATE = 50;

int stepCount = 0;
const int STEPS_PER_STAGE = 5;

float stepThreshold = 1.35;
unsigned long lastStepTime = 0;
const unsigned long STEP_COOLDOWN = 350;

float prevX = 0, prevY = 0, prevZ = 0;
bool hasPrev = false;

float targetX = SCREEN_WIDTH / 2;
float walkPhase = 0.0f;

bool isWaiting = false;
unsigned long waitUntil = 0;

bool backAndForthMode = false;
unsigned long modeUntil = 0;

const int GROUND_Y = SCREEN_HEIGHT - 5;

bool lastButtonState = HIGH;

void resetPong() {
  score = 0;
  lives = 3;
  paddleLeftY  = 20;
  paddleRightY = 20;
  ballX = SCREEN_WIDTH / 2;
  ballY = SCREEN_HEIGHT / 2;

  do { ballDX = random(-1, 2); } while (ballDX == 0);
  do { ballDY = random(-1, 2); } while (ballDY == 0);

  paddleLeftY  = random(0, SCREEN_HEIGHT - PADDLE_HEIGHT);
  paddleRightY = random(0, SCREEN_HEIGHT - PADDLE_HEIGHT);
}

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
  delay(1200);
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

  display.setCursor(10, 54);
  display.print("Press button");

  display.display();
}

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

void detectSteps() {
  float ax, ay, az;
  if (!IMU.accelerationAvailable()) return;

  IMU.readAcceleration(ax, ay, az);

  float mag = sqrt(ax*ax + ay*ay + az*az);

  float dx = ax - prevX;
  float dy = ay - prevY;
  float dz = az - prevZ;

  prevX = ax;
  prevY = ay;
  prevZ = az;

  if (!hasPrev) {
    hasPrev = true;
    return;
  }

  bool magSpike = mag > stepThreshold;
  bool verticalJerk = abs(dz) > 0.18;
  bool swayJerk = abs(dx) > 0.12 || abs(dy) > 0.12;

  bool stepDetected = magSpike || verticalJerk || swayJerk;

  if (stepDetected) {
    unsigned long now = millis();
    if (now - lastStepTime > STEP_COOLDOWN) {
      stepCount++;
      lastStepTime = now;
    }
  }
}

void drawGrass() {
  display.drawLine(0, GROUND_Y, SCREEN_WIDTH - 1, GROUND_Y, SSD1306_WHITE);
  for (int x = 0; x < SCREEN_WIDTH; x += 4) {
    display.drawLine(x, GROUND_Y, x, GROUND_Y - 2, SSD1306_WHITE);
  }
}

void drawSword(int baseX, int bodyBottom, uint8_t scale, bool pointingDown) {
  int bladeLength = 6 * scale;
  int bladeWidth  = max(1, scale / 2);
  int guardWidth  = 3 * scale;

  int bladeTop, bladeBottom;
  if (pointingDown) {
    bladeTop    = bodyBottom - bladeLength;
    bladeBottom = bodyBottom;
  } else {
    bladeTop    = bodyBottom;
    bladeBottom = bodyBottom + bladeLength;
  }

  for (int w = -bladeWidth; w <= bladeWidth; w++) {
    display.drawLine(baseX + w, bladeTop, baseX + w, bladeBottom, SSD1306_WHITE);
  }

  display.drawPixel(baseX,     bladeTop - 1, SSD1306_WHITE);
  display.drawPixel(baseX - 1, bladeTop,     SSD1306_WHITE);
  display.drawPixel(baseX + 1, bladeTop,     SSD1306_WHITE);

  display.drawLine(baseX - guardWidth / 2, bodyBottom - 1,
                   baseX + guardWidth / 2, bodyBottom - 1, SSD1306_WHITE);
}

void drawStickFigureOnGround(int x, uint8_t scale, uint8_t gear) {
  int headR   = 2 * scale;
  int bodyLen = 5 * scale;
  int legLen  = 4 * scale;
  int armLen  = 4 * scale;

  float swing = sin(walkPhase) * (2 * scale);
  int swayX = (int)(sin(walkPhase * 0.5f) * 1);
  x += swayX;

  int bodyBottom   = GROUND_Y - 1;
  int bodyTop      = bodyBottom - bodyLen;
  int headCenterY  = bodyTop - headR;

  int armY = bodyTop + bodyLen / 3;
  int rightHandX = x + armLen + swing;
  int leftHandX  = x - armLen - swing;

  display.drawCircle(x, headCenterY, headR, SSD1306_WHITE);
  display.drawLine(x, bodyTop, x, bodyBottom, SSD1306_WHITE);

  display.drawLine(x, armY, leftHandX,  armY + scale, SSD1306_WHITE);
  display.drawLine(x, armY, rightHandX, armY + scale, SSD1306_WHITE);

  int leftFootX  = x - legLen;
  int rightFootX = x + legLen;
  display.drawLine(x, bodyBottom, leftFootX,  GROUND_Y, SSD1306_WHITE);
  display.drawLine(x, bodyBottom, rightFootX, GROUND_Y, SSD1306_WHITE);

  if (gear >= 1) {
    int swordX = rightHandX + 2 * scale;
    drawSword(swordX, bodyBottom, scale, true);
  }

  if (gear >= 2) {
    int headTop = headCenterY - headR;
    int brimLeft  = x - headR - 2 * scale;
    int brimRight = x + headR + 2 * scale;
    display.drawLine(brimLeft, headTop, brimRight, headTop, SSD1306_WHITE);
    int crownHeight = headR + 2 * scale;
    int crownTop    = headTop - crownHeight;
    display.fillRect(x - headR, crownTop, 2 * headR, crownHeight, SSD1306_WHITE);
  }

  if (gear >= 3) {
    int shoeWidth  = 2 * scale;
    int shoeHeight = 1 * scale;
    display.fillRect(leftFootX - shoeWidth / 2,
                     GROUND_Y - shoeHeight,
                     shoeWidth, shoeHeight, SSD1306_WHITE);
    display.fillRect(rightFootX - shoeWidth / 2,
                     GROUND_Y - shoeHeight,
                     shoeWidth, shoeHeight, SSD1306_WHITE);
  }

  if (gear >= 4) {
    int swordX = leftHandX - 2 * scale;
    drawSword(swordX, bodyBottom, scale, true);
  }
}

int figureMargin(uint8_t scale) {
  int halfWidth = 7 * scale;
  return halfWidth + 2;
}

void setup() {
  delay(500);
  Serial.begin(9600);
  delay(200);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) while (1);
  display.clearDisplay();

  if (!IMU.begin()) while (1);
  if (!rtc.begin()) while (1);

  randomSeed(analogRead(0));
  resetPong();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(20, 25);
  display.print("Smartwatch");
  display.display();
  delay(1200);

  figureX   = SCREEN_WIDTH / 2.0f;
  figureDir = 1;
  gearLevel = 0;
  sizeLevel = MIN_SIZE;
  targetX = random(15, SCREEN_WIDTH - 15);
}

void loop() {
  if (gameState != PONG) {
    detectSteps();
  }

  if (stepCount >= STEPS_PER_STAGE) {
    stepCount = 0;
    sizeLevel++;
    if (sizeLevel > MAX_SIZE) {
      sizeLevel = MIN_SIZE;
      fullRotations++;
      gearLevel = (gearLevel + 1) % 5;
      figureX   = SCREEN_WIDTH / 2.0f;
      figureDir = 1;
    }
  }

  bool buttonPressed = (digitalRead(BUTTON_PIN) == LOW);
  if (lastButtonState == HIGH && buttonPressed == LOW) {
    if (gameState == GAME_OVER) {
      gameState = WATCH;
      resetPong();
    } else {
      if (gameState == WATCH) gameState = PONG;
      else if (gameState == PONG) gameState = STICKMAN;
      else if (gameState == STICKMAN) gameState = WATCH;
    }
  }
  lastButtonState = buttonPressed;

  if (gameState == WATCH) {
    DateTime now = rtc.now();

    display.clearDisplay();

    display.setTextSize(1);
    display.setCursor(22, 8);
    display.print(now.month());
    display.print("/");
    display.print(now.day());
    display.print("/");
    display.print(now.year());

    int hour12 = now.hour() % 12;
    if (hour12 == 0) hour12 = 12;

    display.setTextSize(2);
    display.setCursor(0, 28);

    display.print(hour12);
    display.print(":");

    if (now.minute() < 10) display.print("0");
    display.print(now.minute());
    display.print(":");

    if (now.second() < 10) display.print("0");
    display.print(now.second());

    display.setTextSize(1);
    display.print(" ");
    display.print(now.hour() < 12 ? "AM" : "PM");

    display.display();
    delay(200);
    return;
  }

  if (gameState == GAME_OVER) {
    showGameOverScreen();
    delay(100);
    return;
  }

  if (gameState == PONG) {
    int result = updateBall();

    if (result == 1) {
      lives--;
      showLifeLostScreen();

      if (lives <= 0) {
        gameState = GAME_OVER;
        return;
      }

      resetBallAndPaddles();
    }

    drawPong();

    if (IMU.accelerationAvailable()) {
      IMU.readAcceleration(x, y, z);

      float pitch = atan2(x, sqrt(y*y + z*z)) * 180.0 / PI;
      float roll  = atan2(y, sqrt(x*x + z*z)) * 180.0 / PI;

      const float threshold = 8.0;

      if (roll > threshold) {
        paddleLeftY -= 3;
      } else if (roll < -threshold) {
        paddleLeftY += 3;
      }

      if (pitch > threshold) {
        paddleRightY -= 3;
      } else if (pitch < -threshold) {
        paddleRightY += 3;
      }

      paddleLeftY  = constrain(paddleLeftY, 0, SCREEN_HEIGHT - PADDLE_HEIGHT);
      paddleRightY = constrain(paddleRightY, 0, SCREEN_HEIGHT - PADDLE_HEIGHT);
    }

    delay(30);
    return;
  }

  if (gameState == STICKMAN) {
    unsigned long now = millis();
    if (now - lastUpdate < FRAME_RATE) {
      return;
    }
    lastUpdate = now;

    float speed = speedForSize[sizeLevel];
    walkPhase += 0.15f * speed;

    if (backAndForthMode) {
      if (millis() > modeUntil) {
        backAndForthMode = false;
        targetX = random(15, SCREEN_WIDTH - 15);
      }
    }

    if (isWaiting) {
      if (millis() >= waitUntil) {
        isWaiting = false;
        if (random(0, 10) == 0) {
          backAndForthMode = true;
          modeUntil = millis() + random(2000, 5000);
        } else {
          targetX = random(15, SCREEN_WIDTH - 15);
        }
      }
    } else {
      if (backAndForthMode) {
        int margin = figureMargin(sizeLevel);
        figureX += figureDir * speed;
        if (figureX >= SCREEN_WIDTH - margin || figureX <= margin) {
          figureDir = -figureDir;
        }
      } else {
        if (abs(figureX - targetX) < 1.5f) {
          isWaiting = true;
          waitUntil = millis() + random(1200, 3500);
        } else {
          figureDir = (figureX < targetX) ? 1 : -1;
          figureX += figureDir * speed;
        }
      }
    }

    display.clearDisplay();

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print("Rot:");
    display.print(fullRotations);

    drawGrass();
    drawStickFigureOnGround((int)(figureX + 0.5f), sizeLevel, gearLevel);

    display.display();
  }
}
