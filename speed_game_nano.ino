/*
 * Speed Game - Two Player Arduino Game
 * Hardware: Arduino Nano Every, 2x I2C 128x64 LCD (GME12864-11), RGB LED, 2x Push Buttons
 */

#include <Wire.h>
#include <U8g2lib.h>

// Pin Definitions
const int BUTTON_P1 = 7;
const int BUTTON_P2 = 8;
const int LED_RED = 3;
const int LED_GREEN = 5;
const int LED_BLUE = 6;

// LCD I2C Addresses
const int LCD_ADDR_P1 = 0x3C;
const int LCD_ADDR_P2 = 0x3D;

// Initialize displays
U8G2_SSD1306_128X64_NONAME_F_HW_I2C lcd1(U8G2_R0, U8X8_PIN_NONE);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C lcd2(U8G2_R0, U8X8_PIN_NONE);

// Win feedback (positive, 2-4 words)
const char* winMessages[] = {
  "LEGENDARY!", "ON FIRE!", "UNSTOPPABLE!", "BEAST MODE!", "CRUSHING IT!",
  "LIGHTNING FAST!", "DOMINATING!", "FLAWLESS!", "GODLIKE!", "INCREDIBLE!",
  "MONSTER!", "SAVAGE!", "INSANE!", "PERFECT!", "SUPREME!",
  "CLUTCH MASTER!", "TOO FAST!", "UNTOUCHABLE!", "RUTHLESS!", "BLAZING!",
  "SUPERHUMAN!", "KILLING IT!", "PHENOMENON!", "CHAMPION!", "ELITE!",
  "MASTERCLASS!", "POWERHOUSE!", "NUCLEAR!", "LEGENDARY!", "OUTSTANDING!",
  "WORLD CLASS!", "UNREAL!", "SPECTACULAR!", "MAGNIFICENT!", "BRILLIANT!",
  "STUNNING!", "PHENOMENAL!", "EXTRAORDINARY!", "SENSATIONAL!", "REMARKABLE!",
  "ANNIHILATING!", "DECIMATING!", "OBLITERATING!", "VAPORIZING!", "WRECKING!",
  "DESTROYING!", "MURDERING!", "SLAYING!", "BULLDOZING!", "STEAMROLLING!"
};

// Lose feedback (rude, 1-2 words)
const char* loseMessages[] = {
  "PATHETIC", "TRASH", "GARBAGE", "WEAK", "SCRUB",
  "EMBARRASSING", "USELESS", "TERRIBLE", "AWFUL", "HOPELESS",
  "SAD", "PITIFUL", "LAUGHABLE", "FAIL", "YIKES",
  "DISASTER", "JOKE", "CLOWN", "ROOKIE", "NOOB",
  "SHAMEFUL", "BRUTAL", "ROUGH", "OUCH", "WRECKED",
  "DESTROYED", "CRUSHED", "SMOKED", "OWNED", "DOMINATED",
  "BODIED", "TOASTED", "FRIED", "COOKED", "BURNT",
  "CHOKE ARTIST", "TOO SLOW", "BOZO", "CLOWN SHOW", "FUMBLED",
  "WASHED UP", "WASHED", "DONE", "FINISHED", "EXPOSED",
  "FIGURED OUT", "PREDICTABLE", "BASIC", "MID", "FRAUDULENT"
};

const int WIN_MSG_COUNT = sizeof(winMessages) / sizeof(winMessages[0]);
const int LOSE_MSG_COUNT = sizeof(loseMessages) / sizeof(loseMessages[0]);

// Game States
enum GameState {
  MENU, GAME_REACTION, GAME_BUTTON_MASH, GAME_REFLEX, GAME_SPEED_MATCH, GAME_OVER
};

GameState currentState = MENU;

// Menu System
const int MENU_ITEMS = 4;
int selectedMenuItem = 0;
unsigned long menuStartTime = 0;
const unsigned long MENU_TIMEOUT = 10000;

// Button Debouncing
unsigned long lastP1Press = 0;
unsigned long lastP2Press = 0;
unsigned long lastP1Click = 0;
const unsigned long DEBOUNCE_DELAY = 50;
const unsigned long DOUBLE_CLICK_TIME = 400;
bool p1Pressed = false;
bool p2Pressed = false;

// Game Variables
int scoreP1 = 0;
int scoreP2 = 0;
int currentRound = 1;
const int MAX_ROUNDS = 5;

void setRGB(int r, int g, int b) {
  analogWrite(LED_RED, r);
  analogWrite(LED_GREEN, g);
  analogWrite(LED_BLUE, b);
}

void initDisplay(U8G2_SSD1306_128X64_NONAME_F_HW_I2C &display, uint8_t address) {
  display.setI2CAddress(address << 1);
  display.begin();
  display.setContrast(128);
}

void drawCenteredText(U8G2_SSD1306_128X64_NONAME_F_HW_I2C &display, int y, const char* text) {
  int width = display.getStrWidth(text);
  display.drawStr((128 - width) / 2, y, text);
}

void drawProgressBars(U8G2_SSD1306_128X64_NONAME_F_HW_I2C &disp, int val1, int val2, int maxVal) {
  // P1 bar (top)
  disp.drawStr(0, 10, "P1");
  disp.drawFrame(20, 2, 108, 10);
  int bar1 = map(val1, 0, maxVal, 0, 104);
  disp.drawBox(22, 4, bar1, 6);

  // P2 bar (bottom)
  disp.drawStr(0, 25, "P2");
  disp.drawFrame(20, 17, 108, 10);
  int bar2 = map(val2, 0, maxVal, 0, 104);
  disp.drawBox(22, 19, bar2, 6);
}

void setup() {
  pinMode(BUTTON_P1, INPUT_PULLUP);
  pinMode(BUTTON_P2, INPUT_PULLUP);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);

  initDisplay(lcd1, LCD_ADDR_P1);
  initDisplay(lcd2, LCD_ADDR_P2);

  setRGB(128, 0, 128);

  lcd1.clearBuffer();
  lcd1.setFont(u8g2_font_fur17_tr);
  drawCenteredText(lcd1, 30, "SPEED");
  drawCenteredText(lcd1, 50, "GAME!");
  lcd1.setFont(u8g2_font_9x15_tr);
  drawCenteredText(lcd1, 63, "Player 1");
  lcd1.sendBuffer();

  lcd2.clearBuffer();
  lcd2.setFont(u8g2_font_fur17_tr);
  drawCenteredText(lcd2, 30, "SPEED");
  drawCenteredText(lcd2, 50, "GAME!");
  lcd2.setFont(u8g2_font_9x15_tr);
  drawCenteredText(lcd2, 63, "Player 2");
  lcd2.sendBuffer();

  delay(2000);
  menuStartTime = millis();
  showMenu();
}

void loop() {
  readButtons();

  switch (currentState) {
    case MENU: handleMenu(); break;
    case GAME_REACTION: playReactionGame(); break;
    case GAME_BUTTON_MASH: playButtonMashGame(); break;
    case GAME_REFLEX: playReflexGame(); break;
    case GAME_SPEED_MATCH: playSpeedMatchGame(); break;
    case GAME_OVER: handleGameOver(); break;
  }
}

void readButtons() {
  bool p1State = digitalRead(BUTTON_P1) == LOW;
  bool p2State = digitalRead(BUTTON_P2) == LOW;
  unsigned long now = millis();

  if (p1State && !p1Pressed && (now - lastP1Press > DEBOUNCE_DELAY)) {
    p1Pressed = true;
    lastP1Press = now;
  } else if (!p1State && p1Pressed) {
    p1Pressed = false;
  }

  if (p2State && !p2Pressed && (now - lastP2Press > DEBOUNCE_DELAY)) {
    p2Pressed = true;
    lastP2Press = now;
  } else if (!p2State && p2Pressed) {
    p2Pressed = false;
  }
}

void showMenu() {
  const char* menuItems[MENU_ITEMS] = {"Reaction", "Button Mash", "Reflex", "Speed Match"};

  lcd1.clearBuffer();
  lcd1.setFont(u8g2_font_9x15_tr);
  lcd1.drawStr(0, 12, "SELECT GAME:");
  lcd1.setFont(u8g2_font_fur14_tr);
  drawCenteredText(lcd1, 40, menuItems[selectedMenuItem]);
  lcd1.sendBuffer();

  lcd2.clearBuffer();
  lcd2.setFont(u8g2_font_9x15_tr);
  lcd2.drawStr(0, 12, "P1:Click=Next");
  lcd2.drawStr(0, 28, "DblClick=Start");
  lcd2.sendBuffer();

  setRGB(0, 0, 255);
}

void handleMenu() {
  unsigned long now = millis();
  unsigned long elapsed = now - menuStartTime;

  if (elapsed >= MENU_TIMEOUT) {
    startSelectedGame();
    return;
  }

  static unsigned long lastBarUpdate = 0;
  if (now - lastBarUpdate > 100) {
    lastBarUpdate = now;

    lcd2.clearBuffer();
    lcd2.setFont(u8g2_font_9x15_tr);
    lcd2.drawStr(0, 12, "P1:Click=Next");
    lcd2.drawStr(0, 28, "DblClick=Start");

    int barWidth = map(elapsed, 0, MENU_TIMEOUT, 120, 0);
    lcd2.drawFrame(4, 45, 120, 12);
    lcd2.drawBox(6, 47, barWidth, 8);
    lcd2.sendBuffer();
  }

  if (p1Pressed && (now - lastP1Press < DEBOUNCE_DELAY + 10)) {
    if (now - lastP1Click < DOUBLE_CLICK_TIME) {
      startSelectedGame();
      lastP1Click = 0;
    } else {
      selectedMenuItem = (selectedMenuItem + 1) % MENU_ITEMS;
      showMenu();
      lastP1Click = now;
    }
  }
}

void startSelectedGame() {
  scoreP1 = 0;
  scoreP2 = 0;
  currentRound = 1;
  randomSeed(millis());

  switch (selectedMenuItem) {
    case 0: currentState = GAME_REACTION; break;
    case 1: currentState = GAME_BUTTON_MASH; break;
    case 2: currentState = GAME_REFLEX; break;
    case 3: currentState = GAME_SPEED_MATCH; break;
  }
  delay(500);
}

// ============== GAME 1: REACTION TIME ==============
void playReactionGame() {
  static int gamePhase = 0;
  static unsigned long phaseStartTime = 0;
  static unsigned long greenLightTime = 0;
  static bool roundWon = false;

  if (gamePhase == 0) {
    lcd1.clearBuffer();
    lcd2.clearBuffer();
    lcd1.setFont(u8g2_font_fur20_tr);
    lcd2.setFont(u8g2_font_fur20_tr);
    drawCenteredText(lcd1, 28, "REACT!");
    drawCenteredText(lcd2, 28, "REACT!");

    lcd1.setFont(u8g2_font_9x15_tr);
    lcd2.setFont(u8g2_font_9x15_tr);
    char buf[16];
    sprintf(buf, "Round %d/%d", currentRound, MAX_ROUNDS);
    drawCenteredText(lcd1, 45, buf);
    drawCenteredText(lcd2, 45, "Wait for");
    drawCenteredText(lcd2, 60, "GREEN!");

    // Progress bars
    drawProgressBars(lcd1, scoreP1, scoreP2, MAX_ROUNDS);
    drawProgressBars(lcd2, scoreP1, scoreP2, MAX_ROUNDS);

    lcd1.sendBuffer();
    lcd2.sendBuffer();

    setRGB(255, 0, 0);
    phaseStartTime = millis();
    gamePhase = 1;
    roundWon = false;
  }
  else if (gamePhase == 1) {
    if (millis() - phaseStartTime > random(1000, 2000)) {
      setRGB(255, 255, 0);
      phaseStartTime = millis();
      gamePhase = 2;
    }
    if (p1Pressed || p2Pressed) {
      handleFalseStart();
      gamePhase = 4;
    }
  }
  else if (gamePhase == 2) {
    if (millis() - phaseStartTime > random(500, 1500)) {
      setRGB(0, 255, 0);
      greenLightTime = millis();
      phaseStartTime = millis();
      gamePhase = 3;
    }
    if (p1Pressed || p2Pressed) {
      handleFalseStart();
      gamePhase = 4;
    }
  }
  else if (gamePhase == 3) {
    if (!roundWon) {
      if (p1Pressed && (millis() - lastP1Press < 100)) {
        scoreP1++;
        showReactionWinner(1, millis() - greenLightTime);
        roundWon = true;
        gamePhase = 4;
      }
      else if (p2Pressed && (millis() - lastP2Press < 100)) {
        scoreP2++;
        showReactionWinner(2, millis() - greenLightTime);
        roundWon = true;
        gamePhase = 4;
      }

      if (millis() - phaseStartTime > 3000) {
        lcd1.clearBuffer();
        lcd2.clearBuffer();
        lcd1.setFont(u8g2_font_fur20_tr);
        lcd2.setFont(u8g2_font_fur20_tr);
        drawCenteredText(lcd1, 35, "TOO");
        drawCenteredText(lcd1, 55, "SLOW!");
        drawCenteredText(lcd2, 35, "TOO");
        drawCenteredText(lcd2, 55, "SLOW!");
        lcd1.sendBuffer();
        lcd2.sendBuffer();
        setRGB(255, 0, 0);
        gamePhase = 4;
      }
    }
  }
  else if (gamePhase == 4) {
    delay(2000);
    currentRound++;
    if (currentRound > MAX_ROUNDS) {
      currentState = GAME_OVER;
    }
    gamePhase = 0;
  }
}

void handleFalseStart() {
  lcd1.clearBuffer();
  lcd2.clearBuffer();
  lcd1.setFont(u8g2_font_fur17_tr);
  lcd2.setFont(u8g2_font_fur17_tr);
  drawCenteredText(lcd1, 25, "FALSE");
  drawCenteredText(lcd1, 45, "START!");
  drawCenteredText(lcd2, 25, "FALSE");
  drawCenteredText(lcd2, 45, "START!");

  if (p1Pressed) {
    lcd1.setFont(u8g2_font_9x15_tr);
    drawCenteredText(lcd1, 63, "P1 JUMPED!");
    if (scoreP1 > 0) scoreP1--;
  }
  if (p2Pressed) {
    lcd2.setFont(u8g2_font_9x15_tr);
    drawCenteredText(lcd2, 63, "P2 JUMPED!");
    if (scoreP2 > 0) scoreP2--;
  }

  lcd1.sendBuffer();
  lcd2.sendBuffer();
  setRGB(255, 0, 0);
}

void showReactionWinner(int player, unsigned long reactionTime) {
  const char* winMsg = winMessages[random(WIN_MSG_COUNT)];
  const char* loseMsg = loseMessages[random(LOSE_MSG_COUNT)];

  lcd1.clearBuffer();
  lcd2.clearBuffer();

  lcd1.setFont(u8g2_font_fur20_tr);
  lcd2.setFont(u8g2_font_fur20_tr);

  if (player == 1) {
    drawCenteredText(lcd1, 25, "P1 WINS!");
    drawCenteredText(lcd2, 25, "P1 WINS!");
    lcd1.setFont(u8g2_font_fur14_tr);
    lcd2.setFont(u8g2_font_fur14_tr);
    drawCenteredText(lcd1, 45, winMsg);
    drawCenteredText(lcd2, 45, loseMsg);
    setRGB(0, 255, 0);
  } else {
    drawCenteredText(lcd1, 25, "P2 WINS!");
    drawCenteredText(lcd2, 25, "P2 WINS!");
    lcd1.setFont(u8g2_font_fur14_tr);
    lcd2.setFont(u8g2_font_fur14_tr);
    drawCenteredText(lcd1, 45, loseMsg);
    drawCenteredText(lcd2, 45, winMsg);
    setRGB(0, 255, 255);
  }

  lcd1.setFont(u8g2_font_9x15_tr);
  lcd2.setFont(u8g2_font_9x15_tr);
  char timeText[16];
  sprintf(timeText, "%lums", reactionTime);
  drawCenteredText(lcd1, 63, timeText);
  drawCenteredText(lcd2, 63, timeText);

  lcd1.sendBuffer();
  lcd2.sendBuffer();
}

// ============== GAME 2: BUTTON MASH ==============
void playButtonMashGame() {
  static int gamePhase = 0;
  static unsigned long gameStartTime = 0;
  static int countP1 = 0;
  static int countP2 = 0;
  const unsigned long GAME_DURATION = 10000;

  if (gamePhase == 0) {
    lcd1.clearBuffer();
    lcd2.clearBuffer();
    lcd1.setFont(u8g2_font_fur20_tr);
    lcd2.setFont(u8g2_font_fur20_tr);
    drawCenteredText(lcd1, 30, "MASH!");
    drawCenteredText(lcd2, 30, "MASH!");
    lcd1.setFont(u8g2_font_fur14_tr);
    lcd2.setFont(u8g2_font_fur14_tr);
    drawCenteredText(lcd1, 55, "Get ready");
    drawCenteredText(lcd2, 55, "Get ready");
    lcd1.sendBuffer();
    lcd2.sendBuffer();

    setRGB(255, 255, 0);
    delay(2000);

    countP1 = 0;
    countP2 = 0;
    gameStartTime = millis();
    gamePhase = 1;
    setRGB(0, 255, 0);
  }
  else if (gamePhase == 1) {
    unsigned long elapsed = millis() - gameStartTime;

    if (p1Pressed && (millis() - lastP1Press < 100)) {
      countP1++;
    }
    if (p2Pressed && (millis() - lastP2Press < 100)) {
      countP2++;
    }

    lcd1.clearBuffer();
    lcd2.clearBuffer();

    // Live progress bars
    int maxCount = max(countP1, countP2);
    if (maxCount < 10) maxCount = 10;
    drawProgressBars(lcd1, countP1, countP2, maxCount + 5);
    drawProgressBars(lcd2, countP1, countP2, maxCount + 5);

    // Big numbers
    lcd1.setFont(u8g2_font_fur30_tn);
    lcd2.setFont(u8g2_font_fur30_tn);
    char count1[8], count2[8];
    sprintf(count1, "%d", countP1);
    sprintf(count2, "%d", countP2);
    drawCenteredText(lcd1, 50, count1);
    drawCenteredText(lcd2, 50, count2);

    // Time bar at bottom
    int timeBar = map(elapsed, 0, GAME_DURATION, 120, 0);
    lcd1.drawFrame(4, 55, 120, 8);
    lcd1.drawBox(6, 57, timeBar, 4);
    lcd2.drawFrame(4, 55, 120, 8);
    lcd2.drawBox(6, 57, timeBar, 4);

    lcd1.sendBuffer();
    lcd2.sendBuffer();

    if (elapsed >= GAME_DURATION) {
      gamePhase = 2;
    }
  }
  else if (gamePhase == 2) {
    const char* p1Msg = (countP1 > countP2) ? winMessages[random(WIN_MSG_COUNT)] : loseMessages[random(LOSE_MSG_COUNT)];
    const char* p2Msg = (countP2 > countP1) ? winMessages[random(WIN_MSG_COUNT)] : loseMessages[random(LOSE_MSG_COUNT)];

    lcd1.clearBuffer();
    lcd2.clearBuffer();

    lcd1.setFont(u8g2_font_fur20_tr);
    lcd2.setFont(u8g2_font_fur20_tr);

    if (countP1 > countP2) {
      scoreP1++;
      drawCenteredText(lcd1, 22, "P1");
      drawCenteredText(lcd1, 42, "WINS!");
      drawCenteredText(lcd2, 22, "P1");
      drawCenteredText(lcd2, 42, "WINS!");
      setRGB(0, 255, 0);
    } else if (countP2 > countP1) {
      scoreP2++;
      drawCenteredText(lcd1, 22, "P2");
      drawCenteredText(lcd1, 42, "WINS!");
      drawCenteredText(lcd2, 22, "P2");
      drawCenteredText(lcd2, 42, "WINS!");
      setRGB(0, 255, 255);
    } else {
      drawCenteredText(lcd1, 30, "TIE!");
      drawCenteredText(lcd2, 30, "TIE!");
      setRGB(255, 255, 0);
      p1Msg = "EVEN";
      p2Msg = "EVEN";
    }

    lcd1.setFont(u8g2_font_fur11_tr);
    lcd2.setFont(u8g2_font_fur11_tr);
    drawCenteredText(lcd1, 58, p1Msg);
    drawCenteredText(lcd2, 58, p2Msg);

    lcd1.sendBuffer();
    lcd2.sendBuffer();

    delay(3000);
    currentRound++;
    if (currentRound > MAX_ROUNDS) {
      currentState = GAME_OVER;
    }
    gamePhase = 0;
  }
}

// ============== GAME 3: REFLEX CHALLENGE ==============
void playReflexGame() {
  static int gamePhase = 0;
  static unsigned long colorChangeTime = 0;
  static int currentColor = 0;
  static int roundsPlayed = 0;
  const int COLORS_PER_ROUND = 10;

  if (gamePhase == 0) {
    lcd1.clearBuffer();
    lcd2.clearBuffer();
    lcd1.setFont(u8g2_font_fur20_tr);
    lcd2.setFont(u8g2_font_fur20_tr);
    drawCenteredText(lcd1, 25, "REFLEX!");
    drawCenteredText(lcd2, 25, "REFLEX!");
    lcd1.setFont(u8g2_font_fur11_tr);
    lcd2.setFont(u8g2_font_fur11_tr);
    drawCenteredText(lcd1, 42, "Press on");
    drawCenteredText(lcd2, 42, "Press on");
    drawCenteredText(lcd1, 58, "GREEN only!");
    drawCenteredText(lcd2, 58, "GREEN only!");
    lcd1.sendBuffer();
    lcd2.sendBuffer();

    delay(2000);
    roundsPlayed = 0;
    gamePhase = 1;
  }
  else if (gamePhase == 1) {
    currentColor = random(0, 3);

    if (currentColor == 0) setRGB(255, 0, 0);
    else if (currentColor == 1) setRGB(0, 255, 0);
    else setRGB(0, 0, 255);

    colorChangeTime = millis();
    gamePhase = 2;
  }
  else if (gamePhase == 2) {
    if (p1Pressed && (millis() - lastP1Press < 100)) {
      if (currentColor == 1) scoreP1++;
      else if (scoreP1 > 0) scoreP1--;
    }

    if (p2Pressed && (millis() - lastP2Press < 100)) {
      if (currentColor == 1) scoreP2++;
      else if (scoreP2 > 0) scoreP2--;
    }

    lcd1.clearBuffer();
    lcd2.clearBuffer();

    // Progress bars
    drawProgressBars(lcd1, scoreP1, scoreP2, COLORS_PER_ROUND);
    drawProgressBars(lcd2, scoreP1, scoreP2, COLORS_PER_ROUND);

    // Big scores
    lcd1.setFont(u8g2_font_fur30_tn);
    lcd2.setFont(u8g2_font_fur30_tn);
    char s1[8], s2[8];
    sprintf(s1, "%d", scoreP1);
    sprintf(s2, "%d", scoreP2);
    drawCenteredText(lcd1, 55, s1);
    drawCenteredText(lcd2, 55, s2);

    lcd1.sendBuffer();
    lcd2.sendBuffer();

    if (millis() - colorChangeTime > random(500, 1500)) {
      roundsPlayed++;
      if (roundsPlayed >= COLORS_PER_ROUND) {
        gamePhase = 3;
      } else {
        gamePhase = 1;
      }
    }
  }
  else if (gamePhase == 3) {
    const char* p1Msg = (scoreP1 > scoreP2) ? winMessages[random(WIN_MSG_COUNT)] : loseMessages[random(LOSE_MSG_COUNT)];
    const char* p2Msg = (scoreP2 > scoreP1) ? winMessages[random(WIN_MSG_COUNT)] : loseMessages[random(LOSE_MSG_COUNT)];

    lcd1.clearBuffer();
    lcd2.clearBuffer();
    lcd1.setFont(u8g2_font_fur20_tr);
    lcd2.setFont(u8g2_font_fur20_tr);

    if (scoreP1 > scoreP2) {
      drawCenteredText(lcd1, 30, "WIN!");
      drawCenteredText(lcd2, 30, "LOSE!");
    } else if (scoreP2 > scoreP1) {
      drawCenteredText(lcd1, 30, "LOSE!");
      drawCenteredText(lcd2, 30, "WIN!");
    } else {
      drawCenteredText(lcd1, 30, "TIE!");
      drawCenteredText(lcd2, 30, "TIE!");
      p1Msg = "EVEN";
      p2Msg = "EVEN";
    }

    lcd1.setFont(u8g2_font_fur11_tr);
    lcd2.setFont(u8g2_font_fur11_tr);
    drawCenteredText(lcd1, 50, p1Msg);
    drawCenteredText(lcd2, 50, p2Msg);

    lcd1.sendBuffer();
    lcd2.sendBuffer();

    setRGB(255, 255, 255);
    delay(3000);
    currentState = GAME_OVER;
  }
}

// ============== GAME 4: SPEED MATCH ==============
void playSpeedMatchGame() {
  static int gamePhase = 0;
  static int patternLength = 0;
  static unsigned long showStartTime = 0;
  static int p1Presses = 0;
  static int p2Presses = 0;

  if (gamePhase == 0) {
    lcd1.clearBuffer();
    lcd2.clearBuffer();
    lcd1.setFont(u8g2_font_fur17_tr);
    lcd2.setFont(u8g2_font_fur17_tr);
    drawCenteredText(lcd1, 25, "SPEED");
    drawCenteredText(lcd1, 45, "MATCH!");
    drawCenteredText(lcd2, 25, "SPEED");
    drawCenteredText(lcd2, 45, "MATCH!");

    lcd1.setFont(u8g2_font_9x15_tr);
    lcd2.setFont(u8g2_font_9x15_tr);
    char buf[16];
    sprintf(buf, "Round %d/%d", currentRound, MAX_ROUNDS);
    drawCenteredText(lcd1, 63, buf);
    drawCenteredText(lcd2, 63, "Watch!");
    lcd1.sendBuffer();
    lcd2.sendBuffer();

    delay(2000);

    patternLength = currentRound + 2;
    p1Presses = 0;
    p2Presses = 0;
    gamePhase = 1;
  }
  else if (gamePhase == 1) {
    for (int i = 0; i < patternLength; i++) {
      int r = random(0, 2) * 255;
      int g = random(0, 2) * 255;
      int b = random(0, 2) * 255;
      setRGB(r, g, b);

      lcd1.clearBuffer();
      lcd2.clearBuffer();
      lcd1.setFont(u8g2_font_fur30_tn);
      lcd2.setFont(u8g2_font_fur30_tn);
      char num[4];
      sprintf(num, "%d", i + 1);
      drawCenteredText(lcd1, 45, num);
      drawCenteredText(lcd2, 45, num);
      lcd1.sendBuffer();
      lcd2.sendBuffer();

      delay(400);
      setRGB(0, 0, 0);
      delay(200);
    }

    showStartTime = millis();
    gamePhase = 2;
    setRGB(0, 255, 0);
  }
  else if (gamePhase == 2) {
    if (p1Pressed && (millis() - lastP1Press < 100)) {
      p1Presses++;
    }
    if (p2Pressed && (millis() - lastP2Press < 100)) {
      p2Presses++;
    }

    lcd1.clearBuffer();
    lcd2.clearBuffer();

    // Progress bars
    drawProgressBars(lcd1, p1Presses, p2Presses, patternLength + 2);
    drawProgressBars(lcd2, p1Presses, p2Presses, patternLength + 2);

    // Target and counts
    lcd1.setFont(u8g2_font_fur20_tn);
    lcd2.setFont(u8g2_font_fur20_tn);
    char target[16];
    sprintf(target, "=%d", patternLength);
    drawCenteredText(lcd1, 42, target);
    drawCenteredText(lcd2, 42, target);

    lcd1.setFont(u8g2_font_fur25_tn);
    lcd2.setFont(u8g2_font_fur25_tn);
    char cnt1[8], cnt2[8];
    sprintf(cnt1, "%d", p1Presses);
    sprintf(cnt2, "%d", p2Presses);
    drawCenteredText(lcd1, 63, cnt1);
    drawCenteredText(lcd2, 63, cnt2);

    lcd1.sendBuffer();
    lcd2.sendBuffer();

    if (p1Presses == patternLength || p2Presses == patternLength || millis() - showStartTime > 5000) {
      gamePhase = 3;
    }
  }
  else if (gamePhase == 3) {
    int p1Diff = abs(p1Presses - patternLength);
    int p2Diff = abs(p2Presses - patternLength);

    const char* p1Msg;
    const char* p2Msg;

    lcd1.clearBuffer();
    lcd2.clearBuffer();
    lcd1.setFont(u8g2_font_fur20_tr);
    lcd2.setFont(u8g2_font_fur20_tr);

    if (p1Diff < p2Diff) {
      scoreP1++;
      drawCenteredText(lcd1, 25, "P1");
      drawCenteredText(lcd1, 45, "WINS!");
      drawCenteredText(lcd2, 25, "P1");
      drawCenteredText(lcd2, 45, "WINS!");
      p1Msg = winMessages[random(WIN_MSG_COUNT)];
      p2Msg = loseMessages[random(LOSE_MSG_COUNT)];
      setRGB(0, 255, 0);
    } else if (p2Diff < p1Diff) {
      scoreP2++;
      drawCenteredText(lcd1, 25, "P2");
      drawCenteredText(lcd1, 45, "WINS!");
      drawCenteredText(lcd2, 25, "P2");
      drawCenteredText(lcd2, 45, "WINS!");
      p1Msg = loseMessages[random(LOSE_MSG_COUNT)];
      p2Msg = winMessages[random(WIN_MSG_COUNT)];
      setRGB(0, 255, 255);
    } else {
      drawCenteredText(lcd1, 30, "TIE!");
      drawCenteredText(lcd2, 30, "TIE!");
      p1Msg = "EVEN";
      p2Msg = "EVEN";
      setRGB(255, 255, 0);
    }

    lcd1.setFont(u8g2_font_fur11_tr);
    lcd2.setFont(u8g2_font_fur11_tr);
    drawCenteredText(lcd1, 63, p1Msg);
    drawCenteredText(lcd2, 63, p2Msg);

    lcd1.sendBuffer();
    lcd2.sendBuffer();

    delay(3000);
    currentRound++;
    if (currentRound > MAX_ROUNDS) {
      currentState = GAME_OVER;
    }
    gamePhase = 0;
  }
}

// ============== GAME OVER ==============
void handleGameOver() {
  const char* p1Msg = (scoreP1 > scoreP2) ? winMessages[random(WIN_MSG_COUNT)] : loseMessages[random(LOSE_MSG_COUNT)];
  const char* p2Msg = (scoreP2 > scoreP1) ? winMessages[random(WIN_MSG_COUNT)] : loseMessages[random(LOSE_MSG_COUNT)];

  lcd1.clearBuffer();
  lcd2.clearBuffer();

  lcd1.setFont(u8g2_font_fur20_tr);
  lcd2.setFont(u8g2_font_fur20_tr);
  drawCenteredText(lcd1, 22, "GAME");
  drawCenteredText(lcd1, 42, "OVER!");
  drawCenteredText(lcd2, 22, "GAME");
  drawCenteredText(lcd2, 42, "OVER!");

  lcd1.setFont(u8g2_font_fur17_tn);
  lcd2.setFont(u8g2_font_fur17_tn);
  char s1[16], s2[16];
  sprintf(s1, "P1:%d", scoreP1);
  sprintf(s2, "P2:%d", scoreP2);
  drawCenteredText(lcd1, 60, s1);
  drawCenteredText(lcd2, 60, s2);

  if (scoreP1 > scoreP2) {
    setRGB(0, 255, 0);
  } else if (scoreP2 > scoreP1) {
    setRGB(0, 255, 255);
  } else {
    setRGB(255, 255, 0);
    p1Msg = "TIE GAME";
    p2Msg = "TIE GAME";
  }

  lcd1.sendBuffer();
  lcd2.sendBuffer();

  delay(2000);

  // Show feedback
  lcd1.clearBuffer();
  lcd2.clearBuffer();
  lcd1.setFont(u8g2_font_fur14_tr);
  lcd2.setFont(u8g2_font_fur14_tr);
  drawCenteredText(lcd1, 35, p1Msg);
  drawCenteredText(lcd2, 35, p2Msg);
  lcd1.sendBuffer();
  lcd2.sendBuffer();

  delay(3000);

  currentState = MENU;
  selectedMenuItem = 0;
  menuStartTime = millis();
  showMenu();
}
