/*
 * Speed Game - Two Player Arduino Game
 * Hardware: Arduino Nano Every, 2x I2C LCD (GY-1284), RGB LED, 2x Push Buttons
 *
 * Features:
 * - Menu selection with 10-second timeout
 * - 4 game modes: Reaction Time, Button Masher, Reflex Challenge, Speed Match
 * - Dual LCD displays for each player
 * - RGB LED for visual feedback
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Pin Definitions
const int BUTTON_P1 = 7;      // Player 1 button (D7 to GND)
const int BUTTON_P2 = 8;      // Player 2 button (D8 to GND)
const int LED_RED = 3;        // RGB LED Red
const int LED_GREEN = 5;      // RGB LED Green
const int LED_BLUE = 6;       // RGB LED Blue

// LCD I2C Addresses
const int LCD_ADDR_P1 = 0x3C; // Player 1 screen
const int LCD_ADDR_P2 = 0x3D; // Player 2 screen

// Initialize LCDs (16x2 displays)
LiquidCrystal_I2C lcd1(LCD_ADDR_P1, 16, 2);
LiquidCrystal_I2C lcd2(LCD_ADDR_P2, 16, 2);

// Game States
enum GameState {
  MENU,
  GAME_REACTION,
  GAME_BUTTON_MASH,
  GAME_REFLEX,
  GAME_SPEED_MATCH,
  GAME_OVER
};

GameState currentState = MENU;

// Menu System
const int MENU_ITEMS = 4;
int selectedMenuItem = 0;
unsigned long menuStartTime = 0;
const unsigned long MENU_TIMEOUT = 10000; // 10 seconds

// Button Debouncing
unsigned long lastP1Press = 0;
unsigned long lastP2Press = 0;
unsigned long lastP1Click = 0;
const unsigned long DEBOUNCE_DELAY = 50;
const unsigned long DOUBLE_CLICK_TIME = 400; // 400ms for double-click

bool p1Pressed = false;
bool p2Pressed = false;

// Game Variables
int scoreP1 = 0;
int scoreP2 = 0;
int currentRound = 1;
const int MAX_ROUNDS = 5;

// LED Control
void setRGB(int r, int g, int b) {
  analogWrite(LED_RED, r);
  analogWrite(LED_GREEN, g);
  analogWrite(LED_BLUE, b);
}

void setup() {
  // Initialize pins
  pinMode(BUTTON_P1, INPUT_PULLUP);
  pinMode(BUTTON_P2, INPUT_PULLUP);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);

  // Initialize I2C and LCDs
  Wire.begin();
  lcd1.init();
  lcd2.init();
  lcd1.backlight();
  lcd2.backlight();

  // Welcome screen
  setRGB(128, 0, 128); // Purple
  lcd1.clear();
  lcd2.clear();
  lcd1.setCursor(0, 0);
  lcd1.print("SPEED GAME!");
  lcd2.setCursor(0, 0);
  lcd2.print("SPEED GAME!");
  lcd1.setCursor(0, 1);
  lcd1.print("Player 1");
  lcd2.setCursor(0, 1);
  lcd2.print("Player 2");
  delay(2000);

  // Start menu
  menuStartTime = millis();
  showMenu();
}

void loop() {
  readButtons();

  switch (currentState) {
    case MENU:
      handleMenu();
      break;
    case GAME_REACTION:
      playReactionGame();
      break;
    case GAME_BUTTON_MASH:
      playButtonMashGame();
      break;
    case GAME_REFLEX:
      playReflexGame();
      break;
    case GAME_SPEED_MATCH:
      playSpeedMatchGame();
      break;
    case GAME_OVER:
      handleGameOver();
      break;
  }
}

void readButtons() {
  // Read button states with debouncing
  bool p1State = digitalRead(BUTTON_P1) == LOW;
  bool p2State = digitalRead(BUTTON_P2) == LOW;

  unsigned long now = millis();

  // Player 1 button
  if (p1State && !p1Pressed && (now - lastP1Press > DEBOUNCE_DELAY)) {
    p1Pressed = true;
    lastP1Press = now;
    onP1ButtonPress();
  } else if (!p1State && p1Pressed) {
    p1Pressed = false;
  }

  // Player 2 button
  if (p2State && !p2Pressed && (now - lastP2Press > DEBOUNCE_DELAY)) {
    p2Pressed = true;
    lastP2Press = now;
    onP2ButtonPress();
  } else if (!p2State && p2Pressed) {
    p2Pressed = false;
  }
}

void onP1ButtonPress() {
  // Handled differently depending on game state
}

void onP2ButtonPress() {
  // Handled differently depending on game state
}

void showMenu() {
  const char* menuItems[MENU_ITEMS] = {
    "Reaction Time",
    "Button Mash",
    "Reflex Challenge",
    "Speed Match"
  };

  lcd1.clear();
  lcd2.clear();

  // Player 1 screen shows menu
  lcd1.setCursor(0, 0);
  lcd1.print("SELECT GAME:");
  lcd1.setCursor(0, 1);
  lcd1.print(">");
  lcd1.print(menuItems[selectedMenuItem]);

  // Player 2 screen shows instructions
  lcd2.setCursor(0, 0);
  lcd2.print("P1: Click=Next");
  lcd2.setCursor(0, 1);
  lcd2.print("DblClick=Select");

  setRGB(0, 0, 255); // Blue for menu
}

void handleMenu() {
  unsigned long now = millis();
  unsigned long elapsed = now - menuStartTime;

  // Check timeout
  if (elapsed >= MENU_TIMEOUT) {
    // Auto-select current item
    startSelectedGame();
    return;
  }

  // Update time bar on Player 2 screen (bottom row)
  int barLength = map(elapsed, 0, MENU_TIMEOUT, 16, 0);
  lcd2.setCursor(0, 1);
  for (int i = 0; i < 16; i++) {
    if (i < barLength) {
      lcd2.print("=");
    } else {
      lcd2.print(" ");
    }
  }

  // Check for button presses
  if (p1Pressed && (now - lastP1Press < DEBOUNCE_DELAY + 10)) {
    // Check for double-click
    if (now - lastP1Click < DOUBLE_CLICK_TIME) {
      // Double-click detected - select game
      startSelectedGame();
      lastP1Click = 0; // Reset
    } else {
      // Single click - next menu item
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

  switch (selectedMenuItem) {
    case 0:
      currentState = GAME_REACTION;
      break;
    case 1:
      currentState = GAME_BUTTON_MASH;
      break;
    case 2:
      currentState = GAME_REFLEX;
      break;
    case 3:
      currentState = GAME_SPEED_MATCH;
      break;
  }

  delay(500); // Brief pause before starting
}

// ============== GAME 1: REACTION TIME ==============
void playReactionGame() {
  static int gamePhase = 0;
  static unsigned long phaseStartTime = 0;
  static unsigned long greenLightTime = 0;
  static bool roundActive = false;
  static bool roundWon = false;

  if (gamePhase == 0) {
    // Initialize round
    lcd1.clear();
    lcd2.clear();
    lcd1.setCursor(0, 0);
    lcd1.print("REACTION TIME!");
    lcd2.setCursor(0, 0);
    lcd2.print("REACTION TIME!");
    lcd1.setCursor(0, 1);
    lcd1.print("Round ");
    lcd1.print(currentRound);
    lcd1.print("/");
    lcd1.print(MAX_ROUNDS);
    lcd2.setCursor(0, 1);
    lcd2.print("Wait for green!");

    setRGB(255, 0, 0); // Red
    phaseStartTime = millis();
    gamePhase = 1;
    roundActive = false;
    roundWon = false;
  }
  else if (gamePhase == 1) {
    // Red light phase (1-2 seconds)
    if (millis() - phaseStartTime > random(1000, 2000)) {
      setRGB(255, 255, 0); // Yellow
      phaseStartTime = millis();
      gamePhase = 2;
    }
    // Check for false start
    if (p1Pressed || p2Pressed) {
      handleFalseStart();
      gamePhase = 4;
    }
  }
  else if (gamePhase == 2) {
    // Yellow light phase (0.5-1.5 seconds)
    if (millis() - phaseStartTime > random(500, 1500)) {
      setRGB(0, 255, 0); // Green!
      greenLightTime = millis();
      phaseStartTime = millis();
      gamePhase = 3;
      roundActive = true;
    }
    // Check for false start
    if (p1Pressed || p2Pressed) {
      handleFalseStart();
      gamePhase = 4;
    }
  }
  else if (gamePhase == 3) {
    // Green light - wait for button press
    if (!roundWon) {
      if (p1Pressed && (millis() - lastP1Press < 100)) {
        // Player 1 wins!
        scoreP1++;
        showReactionWinner(1, millis() - greenLightTime);
        roundWon = true;
        gamePhase = 4;
      }
      else if (p2Pressed && (millis() - lastP2Press < 100)) {
        // Player 2 wins!
        scoreP2++;
        showReactionWinner(2, millis() - greenLightTime);
        roundWon = true;
        gamePhase = 4;
      }

      // Timeout after 3 seconds
      if (millis() - phaseStartTime > 3000) {
        lcd1.setCursor(0, 1);
        lcd1.print("Too slow!       ");
        lcd2.setCursor(0, 1);
        lcd2.print("Too slow!       ");
        setRGB(255, 0, 0);
        gamePhase = 4;
      }
    }
  }
  else if (gamePhase == 4) {
    // Round over - wait and continue
    delay(2000);
    currentRound++;
    if (currentRound > MAX_ROUNDS) {
      currentState = GAME_OVER;
    }
    gamePhase = 0; // Reset for next round
  }
}

void handleFalseStart() {
  lcd1.clear();
  lcd2.clear();
  lcd1.setCursor(0, 0);
  lcd1.print("FALSE START!");
  lcd2.setCursor(0, 0);
  lcd2.print("FALSE START!");

  if (p1Pressed) {
    lcd1.setCursor(0, 1);
    lcd1.print("P1 jumped!");
    if (scoreP1 > 0) scoreP1--;
  }
  if (p2Pressed) {
    lcd2.setCursor(0, 1);
    lcd2.print("P2 jumped!");
    if (scoreP2 > 0) scoreP2--;
  }

  setRGB(255, 0, 0);
}

void showReactionWinner(int player, unsigned long reactionTime) {
  lcd1.clear();
  lcd2.clear();

  lcd1.setCursor(0, 0);
  lcd1.print("Player ");
  lcd1.print(player);
  lcd1.print(" wins!");
  lcd2.setCursor(0, 0);
  lcd2.print("Player ");
  lcd2.print(player);
  lcd2.print(" wins!");

  lcd1.setCursor(0, 1);
  lcd1.print("Time: ");
  lcd1.print(reactionTime);
  lcd1.print("ms");
  lcd2.setCursor(0, 1);
  lcd2.print("Time: ");
  lcd2.print(reactionTime);
  lcd2.print("ms");

  if (player == 1) {
    setRGB(0, 255, 0);
  } else {
    setRGB(0, 255, 255);
  }
}

// ============== GAME 2: BUTTON MASH ==============
void playButtonMashGame() {
  static int gamePhase = 0;
  static unsigned long gameStartTime = 0;
  static int countP1 = 0;
  static int countP2 = 0;
  const unsigned long GAME_DURATION = 10000; // 10 seconds

  if (gamePhase == 0) {
    // Initialize
    lcd1.clear();
    lcd2.clear();
    lcd1.setCursor(0, 0);
    lcd1.print("BUTTON MASH!");
    lcd2.setCursor(0, 0);
    lcd2.print("BUTTON MASH!");
    lcd1.setCursor(0, 1);
    lcd1.print("Get ready...");
    lcd2.setCursor(0, 1);
    lcd2.print("Get ready...");

    setRGB(255, 255, 0); // Yellow
    delay(2000);

    countP1 = 0;
    countP2 = 0;
    gameStartTime = millis();
    gamePhase = 1;

    setRGB(0, 255, 0); // Green - GO!
    lcd1.setCursor(0, 1);
    lcd1.print("GO! GO! GO!    ");
    lcd2.setCursor(0, 1);
    lcd2.print("GO! GO! GO!    ");
  }
  else if (gamePhase == 1) {
    // Game active
    unsigned long elapsed = millis() - gameStartTime;

    // Count button presses
    if (p1Pressed && (millis() - lastP1Press < 100)) {
      countP1++;
    }
    if (p2Pressed && (millis() - lastP2Press < 100)) {
      countP2++;
    }

    // Update displays
    lcd1.setCursor(0, 0);
    lcd1.print("P1: ");
    lcd1.print(countP1);
    lcd1.print("        ");

    lcd2.setCursor(0, 0);
    lcd2.print("P2: ");
    lcd2.print(countP2);
    lcd2.print("        ");

    // Time remaining bar
    int timeLeft = map(elapsed, 0, GAME_DURATION, 16, 0);
    lcd1.setCursor(0, 1);
    lcd2.setCursor(0, 1);
    for (int i = 0; i < 16; i++) {
      if (i < 16 - timeLeft) {
        lcd1.print("=");
        lcd2.print("=");
      } else {
        lcd1.print(" ");
        lcd2.print(" ");
      }
    }

    // Check if time expired
    if (elapsed >= GAME_DURATION) {
      gamePhase = 2;
    }
  }
  else if (gamePhase == 2) {
    // Game over - show winner
    lcd1.clear();
    lcd2.clear();

    if (countP1 > countP2) {
      scoreP1++;
      lcd1.setCursor(0, 0);
      lcd1.print("P1 WINS!");
      lcd2.setCursor(0, 0);
      lcd2.print("P1 WINS!");
      setRGB(0, 255, 0);
    } else if (countP2 > countP1) {
      scoreP2++;
      lcd1.setCursor(0, 0);
      lcd1.print("P2 WINS!");
      lcd2.setCursor(0, 0);
      lcd2.print("P2 WINS!");
      setRGB(0, 255, 255);
    } else {
      lcd1.setCursor(0, 0);
      lcd1.print("TIE!");
      lcd2.setCursor(0, 0);
      lcd2.print("TIE!");
      setRGB(255, 255, 0);
    }

    lcd1.setCursor(0, 1);
    lcd1.print("P1:");
    lcd1.print(countP1);
    lcd1.print(" P2:");
    lcd1.print(countP2);
    lcd2.setCursor(0, 1);
    lcd2.print("P1:");
    lcd2.print(countP1);
    lcd2.print(" P2:");
    lcd2.print(countP2);

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
  static int currentColor = 0; // 0=red, 1=green, 2=blue
  static int targetColor = 1; // Green is target
  static int roundsPlayed = 0;
  const int COLORS_PER_ROUND = 10;

  if (gamePhase == 0) {
    // Initialize
    lcd1.clear();
    lcd2.clear();
    lcd1.setCursor(0, 0);
    lcd1.print("REFLEX GAME!");
    lcd2.setCursor(0, 0);
    lcd2.print("REFLEX GAME!");
    lcd1.setCursor(0, 1);
    lcd1.print("Press on GREEN!");
    lcd2.setCursor(0, 1);
    lcd2.print("Press on GREEN!");

    delay(2000);
    roundsPlayed = 0;
    gamePhase = 1;
  }
  else if (gamePhase == 1) {
    // Show random color
    currentColor = random(0, 3);

    if (currentColor == 0) {
      setRGB(255, 0, 0); // Red
    } else if (currentColor == 1) {
      setRGB(0, 255, 0); // Green - TARGET!
    } else {
      setRGB(0, 0, 255); // Blue
    }

    colorChangeTime = millis();
    gamePhase = 2;
  }
  else if (gamePhase == 2) {
    // Wait for button press or timeout
    bool p1Correct = false;
    bool p2Correct = false;

    if (p1Pressed && (millis() - lastP1Press < 100)) {
      if (currentColor == targetColor) {
        scoreP1++;
        p1Correct = true;
      } else {
        if (scoreP1 > 0) scoreP1--;
      }
    }

    if (p2Pressed && (millis() - lastP2Press < 100)) {
      if (currentColor == targetColor) {
        scoreP2++;
        p2Correct = true;
      } else {
        if (scoreP2 > 0) scoreP2--;
      }
    }

    // Update scores on display
    lcd1.setCursor(0, 0);
    lcd1.print("P1:");
    lcd1.print(scoreP1);
    lcd1.print(" P2:");
    lcd1.print(scoreP2);
    lcd1.print("    ");
    lcd2.setCursor(0, 0);
    lcd2.print("P1:");
    lcd2.print(scoreP1);
    lcd2.print(" P2:");
    lcd2.print(scoreP2);
    lcd2.print("    ");

    // Color duration: 0.5-1.5 seconds
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
    // Round over
    lcd1.clear();
    lcd2.clear();
    lcd1.setCursor(0, 0);
    lcd1.print("ROUND OVER!");
    lcd2.setCursor(0, 0);
    lcd2.print("ROUND OVER!");
    lcd1.setCursor(0, 1);
    lcd1.print("P1:");
    lcd1.print(scoreP1);
    lcd1.print(" P2:");
    lcd1.print(scoreP2);
    lcd2.setCursor(0, 1);
    lcd2.print("P1:");
    lcd2.print(scoreP1);
    lcd2.print(" P2:");
    lcd2.print(scoreP2);

    setRGB(255, 255, 255); // White
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
    // Initialize
    lcd1.clear();
    lcd2.clear();
    lcd1.setCursor(0, 0);
    lcd1.print("SPEED MATCH!");
    lcd2.setCursor(0, 0);
    lcd2.print("SPEED MATCH!");
    lcd1.setCursor(0, 1);
    lcd1.print("Round ");
    lcd1.print(currentRound);
    lcd1.print("/");
    lcd1.print(MAX_ROUNDS);
    lcd2.setCursor(0, 1);
    lcd2.print("Watch pattern!");

    delay(2000);

    // Generate pattern length (increasing difficulty)
    patternLength = currentRound + 2; // 3-7 flashes
    p1Presses = 0;
    p2Presses = 0;
    gamePhase = 1;
  }
  else if (gamePhase == 1) {
    // Show pattern (flash LED)
    lcd1.clear();
    lcd2.clear();
    lcd1.setCursor(0, 0);
    lcd1.print("Watch pattern:");
    lcd2.setCursor(0, 0);
    lcd2.print("Watch pattern:");

    for (int i = 0; i < patternLength; i++) {
      // Random color each flash
      int r = random(0, 2) * 255;
      int g = random(0, 2) * 255;
      int b = random(0, 2) * 255;

      setRGB(r, g, b);
      lcd1.setCursor(0, 1);
      lcd1.print("Flash ");
      lcd1.print(i + 1);
      lcd1.print("/");
      lcd1.print(patternLength);
      lcd1.print("    ");
      lcd2.setCursor(0, 1);
      lcd2.print("Flash ");
      lcd2.print(i + 1);
      lcd2.print("/");
      lcd2.print(patternLength);
      lcd2.print("    ");

      delay(400);
      setRGB(0, 0, 0);
      delay(200);
    }

    showStartTime = millis();
    gamePhase = 2;

    setRGB(0, 255, 0); // Green - GO!
    lcd1.clear();
    lcd2.clear();
    lcd1.setCursor(0, 0);
    lcd1.print("PRESS ");
    lcd1.print(patternLength);
    lcd1.print(" TIMES!");
    lcd2.setCursor(0, 0);
    lcd2.print("PRESS ");
    lcd2.print(patternLength);
    lcd2.print(" TIMES!");
  }
  else if (gamePhase == 2) {
    // Count presses
    if (p1Pressed && (millis() - lastP1Press < 100)) {
      p1Presses++;
    }
    if (p2Pressed && (millis() - lastP2Press < 100)) {
      p2Presses++;
    }

    lcd1.setCursor(0, 1);
    lcd1.print("P1:");
    lcd1.print(p1Presses);
    lcd1.print(" P2:");
    lcd1.print(p2Presses);
    lcd1.print("    ");
    lcd2.setCursor(0, 1);
    lcd2.print("P1:");
    lcd2.print(p1Presses);
    lcd2.print(" P2:");
    lcd2.print(p2Presses);
    lcd2.print("    ");

    // Check if someone matched exactly
    if (p1Presses == patternLength || p2Presses == patternLength) {
      gamePhase = 3;
    }

    // Timeout after 5 seconds
    if (millis() - showStartTime > 5000) {
      gamePhase = 3;
    }
  }
  else if (gamePhase == 3) {
    // Evaluate results
    lcd1.clear();
    lcd2.clear();

    int p1Diff = abs(p1Presses - patternLength);
    int p2Diff = abs(p2Presses - patternLength);

    if (p1Diff < p2Diff) {
      scoreP1++;
      lcd1.setCursor(0, 0);
      lcd1.print("P1 WINS!");
      lcd2.setCursor(0, 0);
      lcd2.print("P1 WINS!");
      setRGB(0, 255, 0);
    } else if (p2Diff < p1Diff) {
      scoreP2++;
      lcd1.setCursor(0, 0);
      lcd1.print("P2 WINS!");
      lcd2.setCursor(0, 0);
      lcd2.print("P2 WINS!");
      setRGB(0, 255, 255);
    } else {
      lcd1.setCursor(0, 0);
      lcd1.print("TIE!");
      lcd2.setCursor(0, 0);
      lcd2.print("TIE!");
      setRGB(255, 255, 0);
    }

    lcd1.setCursor(0, 1);
    lcd1.print("Target:");
    lcd1.print(patternLength);
    lcd2.setCursor(0, 1);
    lcd2.print("P1:");
    lcd2.print(p1Presses);
    lcd2.print(" P2:");
    lcd2.print(p2Presses);

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
  lcd1.clear();
  lcd2.clear();

  lcd1.setCursor(0, 0);
  lcd1.print("GAME OVER!");
  lcd2.setCursor(0, 0);
  lcd2.print("GAME OVER!");

  lcd1.setCursor(0, 1);
  lcd1.print("P1:");
  lcd1.print(scoreP1);
  lcd2.setCursor(0, 1);
  lcd2.print("P2:");
  lcd2.print(scoreP2);

  if (scoreP1 > scoreP2) {
    setRGB(0, 255, 0); // Green for P1
    lcd1.setCursor(6, 1);
    lcd1.print("WINNER!");
  } else if (scoreP2 > scoreP1) {
    setRGB(0, 255, 255); // Cyan for P2
    lcd2.setCursor(6, 1);
    lcd2.print("WINNER!");
  } else {
    setRGB(255, 255, 0); // Yellow for tie
    lcd1.setCursor(6, 1);
    lcd1.print("TIE!");
    lcd2.setCursor(6, 1);
    lcd2.print("TIE!");
  }

  delay(5000);

  // Return to menu
  currentState = MENU;
  selectedMenuItem = 0;
  menuStartTime = millis();
  showMenu();
}
