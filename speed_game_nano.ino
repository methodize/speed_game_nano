/*
 * Speed Game - Horizontal Split Screen Edition
 * Top half = You, Bottom half = Opponent, divided by line
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

// Win feedback (short, clean fonts)
const char* winMessages[] = {
  "BEAST", "FIRE", "INSANE", "SAVAGE", "GODLY",
  "ELITE", "LEGEND", "CLUTCH", "PERFECT", "MONSTER",
  "SICK", "NASTY", "NUCLEAR", "FLAWLESS", "EPIC",
  "SUPREME", "CRUSHING", "RUTHLESS", "DEADLY", "KILLING",
  "UNSTOP", "BLAZING", "DEMON", "KING", "GOAT",
  "CRACKED", "CHAMP", "ACE", "BOSS", "ALPHA",
  "APEX", "ON FIRE", "TOO GOOD", "BUILT", "HIM"
};

// Lose feedback (BRUTAL)
const char* loseMessages[] = {
  "TRASH", "GARBAGE", "PATHETIC", "WEAK", "SCRUB",
  "AWFUL", "TERRIBLE", "USELESS", "HOPELESS", "SAD",
  "SHAMEFUL", "DISASTER", "JOKE", "CLOWN", "NOOB",
  "WRECKED", "DESTROYED", "OWNED", "BODIED", "SMOKED",
  "FRIED", "COOKED", "BURNT", "TOASTED", "ROASTED",
  "CHOKED", "FUMBLED", "EXPOSED", "FINISHED", "DONE",
  "WASHED", "MID", "BASIC", "BOT", "NPC",
  "GET GOOD", "QUIT", "UNINSTALL", "DELETE", "RETIRE",
  "YIKES", "CRINGE", "L BOZO", "RATIO", "CRY",
  "COPE", "SEETHE", "MAD?", "TILTED", "BRONZE",
  "PLASTIC", "HARDSTUCK", "STUCK", "SKILL GAP", "DOG",
  "BOT DIFF", "GAPPED", "DIFF", "FREE", "EZ",
  "TOO EZ", "EASY", "NO CHANCE", "CHILD", "BABY",
  "SLOW", "TURTLE", "SNAIL", "FROZEN", "BLIND",
  "ASLEEP", "AFK", "LUCKY", "CARRIED", "BOOSTED",
  "SIT DOWN", "PIPE DOWN", "QUIET", "HUSH", "SILENCE",
  "TOUCH GRASS", "GO OUTSIDE", "QUIT NOW", "REFUND", "BROKEN",
  "DAMAGED", "BUSTED", "FAULTY", "FAKE", "PHONY",
  "PRETENDER", "IMPOSTER", "READABLE", "OBVIOUS", "SEEN",
  "ONE-TRICK", "LUCK", "FLUKE", "ACCIDENT", "WASHED UP",
  "PAST PRIME", "OLD", "DUSTY", "ANCIENT", "PAYCHECK"
};

const int WIN_MSG_COUNT = sizeof(winMessages) / sizeof(winMessages[0]);
const int LOSE_MSG_COUNT = sizeof(loseMessages) / sizeof(loseMessages[0]);

enum GameState {
  MENU, GAME_REACTION, GAME_BUTTON_MASH, GAME_REFLEX, GAME_SPEED_MATCH, GAME_OVER
};

GameState currentState = MENU;

const int MENU_ITEMS = 4;
const char* menuItems[MENU_ITEMS] = {"Reaction", "Button Mash", "Reflex", "Speed Match"};
int selectedMenuItem = 0;
unsigned long menuStartTime = 0;
const unsigned long MENU_TIMEOUT = 10000;

unsigned long lastP1Press = 0;
unsigned long lastP2Press = 0;
unsigned long lastP1Click = 0;
const unsigned long DEBOUNCE_DELAY = 50;
const unsigned long DOUBLE_CLICK_TIME = 400;
bool p1Pressed = false;
bool p2Pressed = false;
bool p1NewPress = false;  // Flag for new button press
bool p2NewPress = false;  // Flag for new button press

int scoreP1 = 0;
int scoreP2 = 0;
int currentRound = 1;
const int MAX_ROUNDS = 10; // Best of 10 for React

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

// Draw horizontal divider line at y=32
void drawDivider(U8G2_SSD1306_128X64_NONAME_F_HW_I2C &disp) {
  disp.drawHLine(0, 32, 128);
}

// Draw centered text in top or bottom half
void drawCenteredInHalf(U8G2_SSD1306_128X64_NONAME_F_HW_I2C &disp, int y, const char* text, bool topHalf) {
  int width = disp.getStrWidth(text);
  int x = (128 - width) / 2;
  if (!topHalf) y += 33; // Shift to bottom half
  disp.drawStr(x, y, text);
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

  // Welcome - P1 screen
  lcd1.clearBuffer();
  lcd1.setFont(u8g2_font_helvB14_tr);
  drawCenteredInHalf(lcd1, 12, "SPEED", true);
  drawCenteredInHalf(lcd1, 28, "GAME", true);
  drawDivider(lcd1);
  lcd1.setFont(u8g2_font_6x12_tr);
  drawCenteredInHalf(lcd1, 12, "Player 1", false);
  lcd1.sendBuffer();

  // Welcome - P2 screen
  lcd2.clearBuffer();
  lcd2.setFont(u8g2_font_helvB14_tr);
  drawCenteredInHalf(lcd2, 12, "SPEED", true);
  drawCenteredInHalf(lcd2, 28, "GAME", true);
  drawDivider(lcd2);
  lcd2.setFont(u8g2_font_6x12_tr);
  drawCenteredInHalf(lcd2, 12, "Player 2", false);
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

  // Clear new press flags from previous loop
  p1NewPress = false;
  p2NewPress = false;

  // Player 1 button edge detection
  if (p1State && !p1Pressed && (now - lastP1Press > DEBOUNCE_DELAY)) {
    p1Pressed = true;
    p1NewPress = true;  // Set flag for NEW press only
    lastP1Press = now;
  } else if (!p1State && p1Pressed) {
    p1Pressed = false;
  }

  // Player 2 button edge detection
  if (p2State && !p2Pressed && (now - lastP2Press > DEBOUNCE_DELAY)) {
    p2Pressed = true;
    p2NewPress = true;  // Set flag for NEW press only
    lastP2Press = now;
  } else if (!p2State && p2Pressed) {
    p2Pressed = false;
  }
}

void showMenu() {
  lcd1.clearBuffer();
  lcd1.setFont(u8g2_font_6x12_tr);
  lcd1.drawStr(0, 8, "SELECT:");
  lcd1.setFont(u8g2_font_helvB10_tr);
  drawCenteredInHalf(lcd1, 24, menuItems[selectedMenuItem], true);
  drawDivider(lcd1);
  lcd1.setFont(u8g2_font_6x10_tr);
  lcd1.drawStr(0, 42, "Click: Next");
  lcd1.drawStr(0, 54, "2xClick: Start");
  lcd1.sendBuffer();

  lcd2.clearBuffer();
  lcd2.setFont(u8g2_font_6x12_tr);
  lcd2.drawStr(0, 8, "SELECT:");
  lcd2.setFont(u8g2_font_helvB10_tr);
  drawCenteredInHalf(lcd2, 24, menuItems[selectedMenuItem], true);
  drawDivider(lcd2);
  lcd2.setFont(u8g2_font_6x10_tr);
  lcd2.drawStr(0, 42, "P1 Controls");
  lcd2.drawStr(0, 54, "the Menu");
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

    // Update time bar
    int barWidth = map(elapsed, 0, MENU_TIMEOUT, 120, 0);

    lcd2.clearBuffer();
    lcd2.setFont(u8g2_font_6x12_tr);
    lcd2.drawStr(0, 8, "SELECT:");
    lcd2.setFont(u8g2_font_helvB10_tr);
    drawCenteredInHalf(lcd2, 24, menuItems[selectedMenuItem], true);
    drawDivider(lcd2);
    lcd2.setFont(u8g2_font_6x10_tr);
    lcd2.drawStr(0, 42, "P1 Controls");
    lcd2.drawStr(0, 54, "the Menu");
    lcd2.drawFrame(4, 57, 120, 6);
    lcd2.drawBox(6, 59, barWidth, 2);
    lcd2.sendBuffer();
  }

  if (p1NewPress) {
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
  static bool greenLightActive = false;

  if (gamePhase == 0) {
    // Show round info
    lcd1.clearBuffer();
    lcd2.clearBuffer();

    lcd1.setFont(u8g2_font_helvB10_tr);
    drawCenteredInHalf(lcd1, 10, "REACT!", true);
    lcd1.setFont(u8g2_font_6x10_tr);
    char buf[16];
    sprintf(buf, "Round %d/%d", currentRound, MAX_ROUNDS);
    drawCenteredInHalf(lcd1, 24, buf, true);
    drawDivider(lcd1);
    lcd1.setFont(u8g2_font_6x10_tr);
    lcd1.drawStr(0, 42, "You:");
    lcd1.drawStr(40, 42, itoa(scoreP1, buf, 10));
    lcd1.drawStr(0, 54, "Them:");
    lcd1.drawStr(40, 54, itoa(scoreP2, buf, 10));
    lcd1.sendBuffer();

    lcd2.clearBuffer();
    lcd2.setFont(u8g2_font_helvB10_tr);
    drawCenteredInHalf(lcd2, 10, "REACT!", true);
    lcd2.setFont(u8g2_font_6x10_tr);
    sprintf(buf, "Round %d/%d", currentRound, MAX_ROUNDS);
    drawCenteredInHalf(lcd2, 24, buf, true);
    drawDivider(lcd2);
    lcd2.setFont(u8g2_font_6x10_tr);
    lcd2.drawStr(0, 42, "You:");
    lcd2.drawStr(40, 42, itoa(scoreP2, buf, 10));
    lcd2.drawStr(0, 54, "Them:");
    lcd2.drawStr(40, 54, itoa(scoreP1, buf, 10));
    lcd2.sendBuffer();

    setRGB(255, 0, 0);
    phaseStartTime = millis();
    gamePhase = 1;
    roundWon = false;
    greenLightActive = false;
    delay(500);
  }
  else if (gamePhase == 1) {
    // Red light
    if (millis() - phaseStartTime > random(1500, 2500)) {
      setRGB(255, 255, 0);
      phaseStartTime = millis();
      gamePhase = 2;
    } else if (p1NewPress || p2NewPress) {
      // Only check for false start if we haven't changed phase yet
      handleFalseStart();
      gamePhase = 4;
    }
  }
  else if (gamePhase == 2) {
    // Yellow light
    if (millis() - phaseStartTime > random(800, 1500)) {
      setRGB(0, 255, 0);
      greenLightTime = millis();
      greenLightActive = true;
      gamePhase = 3;
    } else if (p1NewPress || p2NewPress) {
      // Only check for false start if we haven't gone green yet
      handleFalseStart();
      gamePhase = 4;
    }
  }
  else if (gamePhase == 3) {
    // Green light - check for new button presses
    if (!roundWon && greenLightActive) {
      unsigned long timeSinceGreen = millis() - greenLightTime;

      if (p1NewPress) {
        scoreP1++;
        showReactionWinner(1);
        roundWon = true;
        gamePhase = 4;
      }
      else if (p2NewPress) {
        scoreP2++;
        showReactionWinner(2);
        roundWon = true;
        gamePhase = 4;
      }

      if (timeSinceGreen > 3000) {
        lcd1.clearBuffer();
        lcd2.clearBuffer();
        lcd1.setFont(u8g2_font_helvB12_tr);
        lcd2.setFont(u8g2_font_helvB12_tr);
        drawCenteredInHalf(lcd1, 18, "TOO SLOW!", true);
        drawCenteredInHalf(lcd2, 18, "TOO SLOW!", true);
        drawDivider(lcd1);
        drawDivider(lcd2);
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
  lcd1.setFont(u8g2_font_6x10_tr);
  lcd2.setFont(u8g2_font_6x10_tr);

  if (p1NewPress) {
    drawCenteredInHalf(lcd1, 8, "FALSE", true);
    drawCenteredInHalf(lcd1, 20, "START!", true);
    drawCenteredInHalf(lcd2, 8, "P1", false);
    drawCenteredInHalf(lcd2, 20, "JUMPED!", false);
    if (scoreP1 > 0) scoreP1--;
  }
  if (p2NewPress) {
    drawCenteredInHalf(lcd2, 8, "FALSE", true);
    drawCenteredInHalf(lcd2, 20, "START!", true);
    drawCenteredInHalf(lcd1, 8, "P2", false);
    drawCenteredInHalf(lcd1, 20, "JUMPED!", false);
    if (scoreP2 > 0) scoreP2--;
  }

  drawDivider(lcd1);
  drawDivider(lcd2);
  lcd1.sendBuffer();
  lcd2.sendBuffer();
  setRGB(255, 0, 0);
}

void showReactionWinner(int player) {
  const char* winMsg = winMessages[random(WIN_MSG_COUNT)];
  const char* loseMsg = loseMessages[random(LOSE_MSG_COUNT)];

  lcd1.clearBuffer();
  lcd2.clearBuffer();

  lcd1.setFont(u8g2_font_6x10_tr);
  lcd2.setFont(u8g2_font_6x10_tr);

  if (player == 1) {
    drawCenteredInHalf(lcd1, 8, "YOU WIN!", true);
    drawCenteredInHalf(lcd1, 20, winMsg, true);
    drawCenteredInHalf(lcd2, 8, "YOU LOSE", true);
    drawCenteredInHalf(lcd2, 20, loseMsg, true);
    setRGB(0, 255, 0);
  } else {
    drawCenteredInHalf(lcd1, 8, "YOU LOSE", true);
    drawCenteredInHalf(lcd1, 20, loseMsg, true);
    drawCenteredInHalf(lcd2, 8, "YOU WIN!", true);
    drawCenteredInHalf(lcd2, 20, winMsg, true);
    setRGB(0, 255, 255);
  }

  drawDivider(lcd1);
  drawDivider(lcd2);

  // Show current scores
  char scoreText[16];
  sprintf(scoreText, "Score: %d-%d", scoreP1, scoreP2);
  drawCenteredInHalf(lcd1, 12, scoreText, false);
  drawCenteredInHalf(lcd2, 12, scoreText, false);

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
    lcd1.setFont(u8g2_font_helvB12_tr);
    lcd2.setFont(u8g2_font_helvB12_tr);
    drawCenteredInHalf(lcd1, 18, "MASH!", true);
    drawCenteredInHalf(lcd2, 18, "MASH!", true);
    drawDivider(lcd1);
    drawDivider(lcd2);
    lcd1.setFont(u8g2_font_6x10_tr);
    lcd2.setFont(u8g2_font_6x10_tr);
    drawCenteredInHalf(lcd1, 12, "Get ready...", false);
    drawCenteredInHalf(lcd2, 12, "Get ready...", false);
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

    if (p1NewPress) {
      countP1++;
    }
    if (p2NewPress) {
      countP2++;
    }

    // Update displays only every 50ms to reduce flicker
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate > 50) {
      lastUpdate = millis();

      lcd1.clearBuffer();
      lcd2.clearBuffer();

      // Top half - your count (use 3-digit padding to prevent artifacts)
      lcd1.setFont(u8g2_font_helvB18_tn);
      lcd2.setFont(u8g2_font_helvB18_tn);
      char cnt1[8], cnt2[8];
      sprintf(cnt1, "%3d", countP1);  // Right-aligned with padding
      sprintf(cnt2, "%3d", countP2);
      drawCenteredInHalf(lcd1, 24, cnt1, true);
      drawCenteredInHalf(lcd2, 24, cnt2, true);

      drawDivider(lcd1);
      drawDivider(lcd2);

      // Bottom half - opponent count
      lcd1.setFont(u8g2_font_helvB10_tn);
      lcd2.setFont(u8g2_font_helvB10_tn);
      drawCenteredInHalf(lcd1, 18, cnt2, false);
      drawCenteredInHalf(lcd2, 18, cnt1, false);

      // Time bar
      int timeBar = map(elapsed, 0, GAME_DURATION, 100, 0);
      lcd1.drawFrame(14, 50, 100, 6);
      lcd1.drawBox(16, 52, timeBar, 2);
      lcd2.drawFrame(14, 50, 100, 6);
      lcd2.drawBox(16, 52, timeBar, 2);

      lcd1.sendBuffer();
      lcd2.sendBuffer();
    }

    if (elapsed >= GAME_DURATION) {
      gamePhase = 2;
    }
  }
  else if (gamePhase == 2) {
    showGameWinner(countP1, countP2, "MASH");
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
  const int COLORS_PER_ROUND = 20; // Longer game

  if (gamePhase == 0) {
    lcd1.clearBuffer();
    lcd2.clearBuffer();
    lcd1.setFont(u8g2_font_helvB10_tr);
    lcd2.setFont(u8g2_font_helvB10_tr);
    drawCenteredInHalf(lcd1, 10, "REFLEX!", true);
    drawCenteredInHalf(lcd2, 10, "REFLEX!", true);
    lcd1.setFont(u8g2_font_6x10_tr);
    lcd2.setFont(u8g2_font_6x10_tr);
    drawCenteredInHalf(lcd1, 24, "Press on", true);
    drawCenteredInHalf(lcd2, 24, "Press on", true);
    lcd1.drawStr(38, 30, "GREEN only!");
    lcd2.drawStr(38, 30, "GREEN only!");
    drawDivider(lcd1);
    drawDivider(lcd2);
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
    if (p1NewPress) {
      if (currentColor == 1) scoreP1++;
      else if (scoreP1 > 0) scoreP1--;
    }

    if (p2NewPress) {
      if (currentColor == 1) scoreP2++;
      else if (scoreP2 > 0) scoreP2--;
    }

    // Update displays every 100ms
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate > 100 || p1NewPress || p2NewPress) {
      lastUpdate = millis();

      lcd1.clearBuffer();
      lcd2.clearBuffer();

      // Top - your score (use padding to prevent artifacts)
      lcd1.setFont(u8g2_font_helvB18_tn);
      lcd2.setFont(u8g2_font_helvB18_tn);
      char s1[8], s2[8];
      sprintf(s1, "%3d", scoreP1);  // 3-digit padding
      sprintf(s2, "%3d", scoreP2);
      drawCenteredInHalf(lcd1, 24, s1, true);
      drawCenteredInHalf(lcd2, 24, s2, true);

      drawDivider(lcd1);
      drawDivider(lcd2);

      // Bottom - opponent score
      lcd1.setFont(u8g2_font_helvB10_tn);
      lcd2.setFont(u8g2_font_helvB10_tn);
      drawCenteredInHalf(lcd1, 18, s2, false);
      drawCenteredInHalf(lcd2, 18, s1, false);

      lcd1.sendBuffer();
      lcd2.sendBuffer();
    }

    if (millis() - colorChangeTime > random(600, 1200)) {
      roundsPlayed++;
      if (roundsPlayed >= COLORS_PER_ROUND) {
        gamePhase = 3;
      } else {
        gamePhase = 1;
      }
    }
  }
  else if (gamePhase == 3) {
    showGameWinner(scoreP1, scoreP2, "REFLEX");
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
    lcd1.setFont(u8g2_font_helvB10_tr);
    lcd2.setFont(u8g2_font_helvB10_tr);
    drawCenteredInHalf(lcd1, 10, "SPEED", true);
    drawCenteredInHalf(lcd1, 24, "MATCH!", true);
    drawCenteredInHalf(lcd2, 10, "SPEED", true);
    drawCenteredInHalf(lcd2, 24, "MATCH!", true);
    drawDivider(lcd1);
    drawDivider(lcd2);
    lcd1.setFont(u8g2_font_6x10_tr);
    lcd2.setFont(u8g2_font_6x10_tr);
    char buf[16];
    sprintf(buf, "Round %d/%d", currentRound, MAX_ROUNDS);
    drawCenteredInHalf(lcd1, 12, buf, false);
    drawCenteredInHalf(lcd2, 12, buf, false);
    lcd1.sendBuffer();
    lcd2.sendBuffer();

    delay(2000);

    patternLength = currentRound + 2;
    p1Presses = 0;
    p2Presses = 0;
    gamePhase = 1;
  }
  else if (gamePhase == 1) {
    // Show pattern
    for (int i = 0; i < patternLength; i++) {
      int r = random(0, 2) * 255;
      int g = random(0, 2) * 255;
      int b = random(0, 2) * 255;
      setRGB(r, g, b);

      lcd1.clearBuffer();
      lcd2.clearBuffer();
      lcd1.setFont(u8g2_font_helvB18_tn);
      lcd2.setFont(u8g2_font_helvB18_tn);
      char num[4];
      sprintf(num, "%d", i + 1);
      drawCenteredInHalf(lcd1, 22, num, true);
      drawCenteredInHalf(lcd2, 22, num, true);
      drawDivider(lcd1);
      drawDivider(lcd2);
      lcd1.sendBuffer();
      lcd2.sendBuffer();

      delay(500);
      setRGB(0, 0, 0);
      delay(250);
    }

    showStartTime = millis();
    gamePhase = 2;
    setRGB(0, 255, 0);
  }
  else if (gamePhase == 2) {
    if (p1NewPress) {
      p1Presses++;
    }
    if (p2NewPress) {
      p2Presses++;
    }

    // Update displays every 50ms or on button press
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate > 50 || p1NewPress || p2NewPress) {
      lastUpdate = millis();

      lcd1.clearBuffer();
      lcd2.clearBuffer();

      // Top - your count (use padding)
      lcd1.setFont(u8g2_font_helvB18_tn);
      lcd2.setFont(u8g2_font_helvB18_tn);
      char cnt1[8], cnt2[8];
      sprintf(cnt1, "%3d", p1Presses);  // 3-digit padding
      sprintf(cnt2, "%3d", p2Presses);
      drawCenteredInHalf(lcd1, 22, cnt1, true);
      drawCenteredInHalf(lcd2, 22, cnt2, true);

      drawDivider(lcd1);
      drawDivider(lcd2);

      // Bottom - target and opponent
      lcd1.setFont(u8g2_font_6x10_tr);
      lcd2.setFont(u8g2_font_6x10_tr);
      char target[16];
      sprintf(target, "Goal: %2d", patternLength);
      lcd1.drawStr(4, 44, target);
      lcd2.drawStr(4, 44, target);
      sprintf(target, "Them: %2d", p2Presses);
      lcd1.drawStr(4, 56, target);
      sprintf(target, "Them: %2d", p1Presses);
      lcd2.drawStr(4, 56, target);

      lcd1.sendBuffer();
      lcd2.sendBuffer();
    }

    // Win if within 10 clicks, lose if more than 10 behind
    int p1Diff = abs(p1Presses - patternLength);
    int p2Diff = abs(p2Presses - patternLength);

    if ((p1Presses == patternLength) || (p2Presses == patternLength) ||
        (millis() - showStartTime > 10000) ||
        (p1Diff > 10 && p2Diff > 10)) {
      gamePhase = 3;
    }
  }
  else if (gamePhase == 3) {
    int p1Diff = abs(p1Presses - patternLength);
    int p2Diff = abs(p2Presses - patternLength);

    if (p1Diff < p2Diff) {
      scoreP1++;
    } else if (p2Diff < p1Diff) {
      scoreP2++;
    }

    showGameWinner(p1Presses, p2Presses, "SPEED");
    delay(3000);
    currentRound++;
    if (currentRound > MAX_ROUNDS) {
      currentState = GAME_OVER;
    }
    gamePhase = 0;
  }
}

void showGameWinner(int val1, int val2, const char* game) {
  const char* p1Msg = (val1 > val2) ? winMessages[random(WIN_MSG_COUNT)] :
                      (val1 < val2) ? loseMessages[random(LOSE_MSG_COUNT)] : "TIE";
  const char* p2Msg = (val2 > val1) ? winMessages[random(WIN_MSG_COUNT)] :
                      (val2 < val1) ? loseMessages[random(LOSE_MSG_COUNT)] : "TIE";

  lcd1.clearBuffer();
  lcd2.clearBuffer();

  lcd1.setFont(u8g2_font_6x10_tr);
  lcd2.setFont(u8g2_font_6x10_tr);

  if (val1 > val2) {
    scoreP1++;
    drawCenteredInHalf(lcd1, 8, "YOU WIN!", true);
    drawCenteredInHalf(lcd1, 20, p1Msg, true);
    drawCenteredInHalf(lcd2, 8, "YOU LOSE", true);
    drawCenteredInHalf(lcd2, 20, p2Msg, true);
    setRGB(0, 255, 0);
  } else if (val2 > val1) {
    scoreP2++;
    drawCenteredInHalf(lcd1, 8, "YOU LOSE", true);
    drawCenteredInHalf(lcd1, 20, p1Msg, true);
    drawCenteredInHalf(lcd2, 8, "YOU WIN!", true);
    drawCenteredInHalf(lcd2, 20, p2Msg, true);
    setRGB(0, 255, 255);
  } else {
    drawCenteredInHalf(lcd1, 8, "TIE!", true);
    drawCenteredInHalf(lcd1, 20, p1Msg, true);
    drawCenteredInHalf(lcd2, 8, "TIE!", true);
    drawCenteredInHalf(lcd2, 20, p2Msg, true);
    setRGB(255, 255, 0);
  }

  drawDivider(lcd1);
  drawDivider(lcd2);

  char score[16];
  sprintf(score, "You: %d", val1);
  lcd1.drawStr(4, 44, score);
  sprintf(score, "Them: %d", val2);
  lcd1.drawStr(4, 56, score);

  sprintf(score, "You: %d", val2);
  lcd2.drawStr(4, 44, score);
  sprintf(score, "Them: %d", val1);
  lcd2.drawStr(4, 56, score);

  lcd1.sendBuffer();
  lcd2.sendBuffer();
}

// ============== GAME OVER ==============
void handleGameOver() {
  const char* p1Msg = (scoreP1 > scoreP2) ? winMessages[random(WIN_MSG_COUNT)] :
                      (scoreP1 < scoreP2) ? loseMessages[random(LOSE_MSG_COUNT)] : "TIE GAME";
  const char* p2Msg = (scoreP2 > scoreP1) ? winMessages[random(WIN_MSG_COUNT)] :
                      (scoreP2 < scoreP1) ? loseMessages[random(LOSE_MSG_COUNT)] : "TIE GAME";

  lcd1.clearBuffer();
  lcd2.clearBuffer();

  lcd1.setFont(u8g2_font_6x10_tr);
  lcd2.setFont(u8g2_font_6x10_tr);
  drawCenteredInHalf(lcd1, 8, "GAME", true);
  drawCenteredInHalf(lcd1, 20, "OVER!", true);
  drawCenteredInHalf(lcd2, 8, "GAME", true);
  drawCenteredInHalf(lcd2, 20, "OVER!", true);

  drawDivider(lcd1);
  drawDivider(lcd2);

  if (scoreP1 > scoreP2) {
    setRGB(0, 255, 0);
  } else if (scoreP2 > scoreP1) {
    setRGB(0, 255, 255);
  } else {
    setRGB(255, 255, 0);
  }

  char s[16];
  sprintf(s, "You: %d", scoreP1);
  lcd1.drawStr(4, 44, s);
  sprintf(s, "Them: %d", scoreP2);
  lcd1.drawStr(4, 56, s);

  sprintf(s, "You: %d", scoreP2);
  lcd2.drawStr(4, 44, s);
  sprintf(s, "Them: %d", scoreP1);
  lcd2.drawStr(4, 56, s);

  lcd1.sendBuffer();
  lcd2.sendBuffer();

  delay(2000);

  // Show final feedback
  lcd1.clearBuffer();
  lcd2.clearBuffer();
  drawCenteredInHalf(lcd1, 18, p1Msg, true);
  drawCenteredInHalf(lcd2, 18, p2Msg, true);
  drawDivider(lcd1);
  drawDivider(lcd2);
  lcd1.sendBuffer();
  lcd2.sendBuffer();

  delay(3000);

  currentState = MENU;
  selectedMenuItem = 0;
  menuStartTime = millis();
  showMenu();
}
