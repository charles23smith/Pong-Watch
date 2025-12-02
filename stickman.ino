#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino_LSM6DS3.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET   -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define BUTTON_PIN 2
const int GROUND_Y = SCREEN_HEIGHT - 5;

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

float targetX = SCREEN_WIDTH / 2;
float walkPhase = 0.0f;

bool isWaiting = false;
unsigned long waitUntil = 0;

bool backAndForthMode = false;
unsigned long modeUntil = 0;

void detectSteps() {
  float x, y, z;
  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(x, y, z);
    float mag = sqrt(x*x + y*y + z*z);
    if (mag > stepThreshold) {
      unsigned long now = millis();
      if (now - lastStepTime > STEP_COOLDOWN) {
        stepCount++;
        lastStepTime = now;
      }
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

void drawStickFigureOnGround(int x, uint8_t scale, uint8_t gearLevel) {
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

  if (gearLevel >= 1) {
    int swordX = rightHandX + 2 * scale;
    drawSword(swordX, bodyBottom, scale, true);
  }

  if (gearLevel >= 2) {
    int headTop = headCenterY - headR;
    int brimLeft  = x - headR - 2 * scale;
    int brimRight = x + headR + 2 * scale;
    display.drawLine(brimLeft, headTop, brimRight, headTop, SSD1306_WHITE);
    int crownHeight = headR + 2 * scale;
    int crownTop    = headTop - crownHeight;
    display.fillRect(x - headR, crownTop, 2 * headR, crownHeight, SSD1306_WHITE);
  }

  if (gearLevel >= 3) {
    int shoeWidth  = 2 * scale;
    int shoeHeight = 1 * scale;
    display.fillRect(leftFootX - shoeWidth / 2,
                     GROUND_Y - shoeHeight,
                     shoeWidth, shoeHeight, SSD1306_WHITE);
    display.fillRect(rightFootX - shoeWidth / 2,
                     GROUND_Y - shoeHeight,
                     shoeWidth, shoeHeight, SSD1306_WHITE);
  }

  if (gearLevel >= 4) {
    int swordX = leftHandX - 2 * scale;
    drawSword(swordX, bodyBottom, scale, true);
  }
}

int figureMargin(uint8_t scale) {
  int halfWidth = 7 * scale;
  return halfWidth + 2;
}

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  IMU.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while (1);
  }

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(4, 20);
  display.print("STICK HERO");
  display.display();
  delay(1500);

  figureX   = SCREEN_WIDTH / 2.0f;
  figureDir = 1;
  gearLevel = 0;
  sizeLevel = MIN_SIZE;

  targetX = random(15, SCREEN_WIDTH - 15);
}

void loop() {
  detectSteps();

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

  unsigned long now = millis();
  if (now - lastUpdate < FRAME_RATE) return;
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
  display.print("Score:");
  display.print(fullRotations);

  drawGrass();
  drawStickFigureOnGround((int)(figureX + 0.5f), sizeLevel, gearLevel);

  display.display();
}
