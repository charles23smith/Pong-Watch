#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define UP_BUTTON 2
#define DOWN_BUTTON 3

// Gameplay constants
const unsigned long PADDLE_RATE = 33;
const unsigned long BALL_RATE = 20;
const uint8_t PADDLE_HEIGHT = 10;
const uint8_t BALL_SIZE = 2;
int MAX_SCORE = 8;

int CPU_SCORE = 0;
int PLAYER_SCORE = 0;

uint8_t ball_x = 64, ball_y = 32;
int8_t ball_dir_x = 1, ball_dir_y = 1;

bool gameIsRunning = true;
bool resetBall = false;

unsigned long ball_update;
unsigned long paddle_update;

const uint8_t CPU_X = 8;
uint8_t cpu_y = 16;

const uint8_t PLAYER_X = 118;
uint8_t player_y = 16;

void setup() {
  pinMode(UP_BUTTON, INPUT_PULLUP);
  pinMode(DOWN_BUTTON, INPUT_PULLUP);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // 0x3C = I2C address
    for (;;); // Loop forever if display not found
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(25, 25);
  display.print("PONG");
  display.display();
  delay(1500);

  display.clearDisplay();
  drawCourt();
  delay(1000);

  ball_update = millis();
  paddle_update = ball_update;
  randomSeed(analogRead(0));
  ball_x = random(25, 65);
  ball_y = random(3, 63);
}

void loop() {
  unsigned long time = millis();
  static bool up_state = false;
  static bool down_state = false;

  up_state |= (digitalRead(UP_BUTTON) == LOW);
  down_state |= (digitalRead(DOWN_BUTTON) == LOW);

  if (resetBall) {
    ball_x = random(25, 100);
    ball_y = random(3, 63);
    do { ball_dir_x = random(-1, 2); } while (ball_dir_x == 0);
    do { ball_dir_y = random(-1, 2); } while (ball_dir_y == 0);
    resetBall = false;
  }

  if (time > ball_update && gameIsRunning) {
    uint8_t new_x = ball_x + ball_dir_x;
    uint8_t new_y = ball_y + ball_dir_y;

    if (new_x <= 0) { // Player scores
      PLAYER_SCORE++;
      if (PLAYER_SCORE == MAX_SCORE) gameOver(true);
      else showScore();
    }

    if (new_x >= SCREEN_WIDTH - 1) { // CPU scores
      CPU_SCORE++;
      if (CPU_SCORE == MAX_SCORE) gameOver(false);
      else showScore();
    }

    if (new_y <= 0 || new_y >= SCREEN_HEIGHT - 1) {
      ball_dir_y = -ball_dir_y;
      new_y += ball_dir_y * 2;
    }

    if (new_x == CPU_X && new_y >= cpu_y && new_y <= cpu_y + PADDLE_HEIGHT)
      ball_dir_x = -ball_dir_x;

    if (new_x == PLAYER_X && new_y >= player_y && new_y <= player_y + PADDLE_HEIGHT)
      ball_dir_x = -ball_dir_x;

    ball_x = new_x;
    ball_y = new_y;
    ball_update += BALL_RATE;
  }

  if (time > paddle_update && gameIsRunning) {
    paddle_update += PADDLE_RATE;

    const uint8_t half_paddle = PADDLE_HEIGHT >> 1;
    if (cpu_y + half_paddle > ball_y) cpu_y--;
    if (cpu_y + half_paddle < ball_y) cpu_y++;
    if (cpu_y < 1) cpu_y = 1;
    if (cpu_y + PADDLE_HEIGHT > SCREEN_HEIGHT - 1) cpu_y = SCREEN_HEIGHT - 1 - PADDLE_HEIGHT;

    if (up_state) player_y--;
    if (down_state) player_y++;
    up_state = down_state = false;

    if (player_y < 1) player_y = 1;
    if (player_y + PADDLE_HEIGHT > SCREEN_HEIGHT - 1) player_y = SCREEN_HEIGHT - 1 - PADDLE_HEIGHT;
  }

  // Draw the frame
  display.clearDisplay();
  drawCourt();
  display.fillRect(CPU_X, cpu_y, 2, PADDLE_HEIGHT, SSD1306_WHITE);
  display.fillRect(PLAYER_X, player_y, 2, PADDLE_HEIGHT, SSD1306_WHITE);
  display.fillRect(ball_x, ball_y, BALL_SIZE, BALL_SIZE, SSD1306_WHITE);
  display.display();
}

void drawCourt() {
  display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
}

void gameOver(bool playerWon) {
  gameIsRunning = false;
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(20, 20);
  if (playerWon)
    display.print("You Win!");
  else
    display.print("CPU Wins");
  display.display();
  delay(2000);
  PLAYER_SCORE = CPU_SCORE = 0;
  gameIsRunning = true;
  resetBall = true;
}

void showScore() {
  gameIsRunning = false;
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(25, 20);
  display.print("CPU: ");
  display.print(CPU_SCORE);
  display.setCursor(25, 35);
  display.print("YOU: ");
  display.print(PLAYER_SCORE);
  display.display();
  delay(1500);
  gameIsRunning = true;
  resetBall = true;
}
