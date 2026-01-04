# Speed Game - Two Player Arduino Game

A competitive two-player speed game featuring 4 different game modes, dual LCD displays, RGB LED feedback, and an interactive menu system.

## Hardware Requirements

- **Arduino Nano Every**
- **2x I2C LCD Display** (16x2, GY-1284 modules with PCF8574 backpack)
- **1x RGB LED Module** (common cathode)
- **2x Push Buttons** (normally open)
- Breadboard and jumper wires
- Power supply (USB or external 5V)

## Wiring Diagram

### Push Buttons
```
Player 1 Button: D7 → Button → GND (internal pullup enabled)
Player 2 Button: D8 → Button → GND (internal pullup enabled)
```

### RGB LED Module
```
Red Anode:   → D3 (PWM)
Green Anode: → D5 (PWM)
Blue Anode:  → D6 (PWM)
Cathode (-)  → GND
```

**Note:** RGB LED should be common cathode type (long leg to ground). If using common anode, invert the values in `setRGB()` function.

### LCD Displays (I2C)
```
LCD Module 1 (Player 1):           LCD Module 2 (Player 2):
VCC → 5V                           VCC → 5V
GND → GND                          GND → GND
SDA → A4                           SDA → A4
SCL → A5                           SCL → A5
I2C Address: 0x3C                  I2C Address: 0x3D
```

**Note:** Both LCDs share the same I2C bus (A4/A5) but have unique addresses.

## Visual Wiring Reference

```
           Arduino Nano Every
       ┌─────────────────────┐
       │                     │
    D7 │●  ← Button P1       │
    D8 │●  ← Button P2       │
       │                     │
    D3 │●  → LED Red         │
    D5 │●  → LED Green       │
    D6 │●  → LED Blue        │
       │                     │
    A4 │●  ↔ SDA (both LCDs) │
    A5 │●  ↔ SCL (both LCDs) │
       │                     │
   GND │●  ← Common Ground   │
    5V │●  → Power (LCDs)    │
       └─────────────────────┘
```

## I2C Address Configuration

The LCD modules use I2C addresses `0x3C` (Player 1) and `0x3D` (Player 2). If your displays have different addresses:

1. **Check your LCD address** using an I2C scanner sketch
2. **Update addresses** in the code:
   ```cpp
   const int LCD_ADDR_P1 = 0x3C; // Change if needed
   const int LCD_ADDR_P2 = 0x3D; // Change if needed
   ```

Common I2C LCD addresses: `0x27`, `0x3C`, `0x3D`, `0x3F`

## Installation

### 1. Install Arduino IDE or Arduino CLI

**Option A: Arduino IDE**
- Download from [arduino.cc](https://www.arduino.cc/en/software)
- Install and open

**Option B: Arduino CLI (recommended for Nano Every)**
```bash
# Already installed on your system
arduino-cli version
```

### 2. Install Required Library

The project requires the `LiquidCrystal_I2C` library:

```bash
# Using Arduino CLI
arduino-cli lib install "LiquidCrystal I2C"

# Using Arduino IDE
# Sketch → Include Library → Manage Libraries → Search "LiquidCrystal I2C" → Install
```

### 3. Install Arduino Nano Every Board Support

```bash
# Using Arduino CLI
arduino-cli core install arduino:megaavr

# Using Arduino IDE
# Tools → Board → Boards Manager → Search "megaAVR" → Install
```

### 4. Upload the Sketch

```bash
# Using Arduino CLI
arduino-cli compile --fqbn arduino:megaavr:nona4809 speed_game_nano
arduino-cli upload -p /dev/ttyACM0 --fqbn arduino:megaavr:nona4809 speed_game_nano

# Using Arduino IDE
# 1. Open speed_game_nano.ino
# 2. Select Board: Tools → Board → Arduino megaAVR Boards → Arduino Nano Every
# 3. Select Port: Tools → Port → /dev/ttyACM0 (or appropriate port)
# 4. Click Upload
```

**Nano Every Upload Note:** You may need to press the reset button on the Nano Every just before uploading to enter bootloader mode.

## Game Modes

### Menu System
- On boot, the menu appears with a 10-second timeout timer
- **Player 1 Button:**
  - Single click: Cycle through game modes
  - Double click: Select current game mode
- If no selection is made within 10 seconds, the current game mode auto-starts

### 1. Reaction Time
Classic reflex game where speed is everything!

**Rules:**
- Watch the RGB LED: Red → Yellow → Green
- Press your button as fast as possible when LED turns green
- Fastest button press wins the round
- **False start penalty:** Pressing before green costs you a point!
- Best of 5 rounds wins

**Scoring:** First to press after green = +1 point

### 2. Button Mash
Pure speed clicking competition!

**Rules:**
- When LED turns green, press your button as many times as possible
- 10 second time limit shown as depleting bar
- Player with most button presses wins
- Best of 5 rounds wins

**Scoring:** Highest press count = +1 point

### 3. Reflex Challenge
Test your selective reflexes!

**Rules:**
- RGB LED flashes random colors (red, green, or blue)
- **Only press when LED is GREEN**
- Pressing on wrong color = penalty (lose 1 point)
- 10 color flashes per round
- Player with highest score wins

**Scoring:**
- Correct press (on green): +1 point
- Wrong press (on red/blue): -1 point

### 4. Speed Match
Memory meets speed!

**Rules:**
- Watch RGB LED flash a pattern of colors
- Count the number of flashes (difficulty increases each round)
- After pattern, press your button exactly that many times
- Closest to the target number wins
- 5 rounds with increasing difficulty (3→7 flashes)

**Scoring:** Closest match to pattern length = +1 point

## LED Color Codes

| Color          | Meaning                    |
|----------------|----------------------------|
| Purple         | Startup/Welcome            |
| Blue           | Menu selection             |
| Red            | Wait/Stop (Reaction Game)  |
| Yellow         | Get ready (Reaction Game)  |
| Green          | GO! / Action required      |
| Cyan           | Player 2 wins              |
| White          | Round complete             |

## Display Layout

**Player 1 Screen (0x3C):**
- Row 1: Game status / Round info
- Row 2: Player 1 score / Instructions

**Player 2 Screen (0x3D):**
- Row 1: Game status / Round info
- Row 2: Player 2 score / Instructions

## Troubleshooting

### LCDs not displaying
1. **Check I2C addresses:**
   ```bash
   # Run an I2C scanner sketch to find actual addresses
   # Update LCD_ADDR_P1 and LCD_ADDR_P2 in code
   ```

2. **Check wiring:**
   - Verify SDA → A4, SCL → A5
   - Ensure both LCDs powered (VCC → 5V, GND → GND)
   - Check for loose connections

3. **LCD backlight on but no text:**
   - Adjust contrast potentiometer on LCD backpack
   - Try different I2C address in code

### Buttons not responding
1. Check button wiring (D7/D8 to GND when pressed)
2. Verify internal pullups are enabled (pinMode INPUT_PULLUP)
3. Adjust DEBOUNCE_DELAY or DOUBLE_CLICK_TIME if needed

### RGB LED wrong colors
1. **Common cathode vs common anode:**
   - This code assumes common cathode (cathode to GND)
   - For common anode, invert PWM values: `analogWrite(pin, 255 - value)`

2. **Check wiring:**
   - Red → D3, Green → D5, Blue → D6
   - Verify cathode connected to GND

### Double-click not working
- Adjust `DOUBLE_CLICK_TIME` constant (default 400ms)
- Try clicking faster (< 400ms between clicks)
- Increase timeout if needed: `const unsigned long DOUBLE_CLICK_TIME = 600;`

### Upload fails
- Press reset button on Nano Every just before upload
- Check correct port selected (`/dev/ttyACM0` or similar)
- Verify board is "Arduino Nano Every" not "Arduino Nano"

## Customization

### Adjust Game Difficulty
```cpp
// Menu timeout
const unsigned long MENU_TIMEOUT = 10000; // 10 seconds (change as needed)

// Number of rounds
const int MAX_ROUNDS = 5; // Best of 5 (change to 3, 7, etc.)

// Button Mash duration
const unsigned long GAME_DURATION = 10000; // 10 seconds

// Reflex Challenge colors per round
const int COLORS_PER_ROUND = 10; // 10 flashes

// Speed Match timeout
if (millis() - showStartTime > 5000) // 5 second limit
```

### Change LED Colors
Modify the `setRGB()` calls throughout the code:
```cpp
setRGB(255, 0, 0);     // Full red
setRGB(0, 255, 0);     // Full green
setRGB(0, 0, 255);     // Full blue
setRGB(255, 255, 0);   // Yellow (red + green)
setRGB(255, 0, 255);   // Magenta (red + blue)
setRGB(0, 255, 255);   // Cyan (green + blue)
setRGB(128, 128, 128); // Dim white (50% brightness)
```

### Add More Game Modes
1. Add new enum value to `GameState`
2. Increment `MENU_ITEMS` constant
3. Add menu item name to array
4. Create new game function
5. Add case to `startSelectedGame()` switch
6. Add case to main `loop()` switch

## Technical Details

**Microcontroller:** ATmega4809 (Arduino Nano Every)
- Flash: 48KB
- RAM: 6KB
- EEPROM: 256 bytes
- Clock: 16 MHz

**I2C Bus Speed:** 100 kHz (standard mode)

**Button Debouncing:** 50ms hardware debounce

**Double-Click Detection:** 400ms window

## Future Enhancements

- [ ] Add sound effects using piezo buzzer
- [ ] Implement EEPROM high score storage
- [ ] Add difficulty levels (easy/medium/hard)
- [ ] Create tournament mode (best of 10)
- [ ] Add team mode (co-op challenges)
- [ ] Implement combo multipliers
- [ ] Add calibration mode for button sensitivity
- [ ] Create attract mode with demo gameplay

## License

Open source - feel free to modify and share!

## Credits

Created for Arduino Nano Every with dual I2C LCD displays.
Hardware: RGB LED, 2x push buttons, 2x GY-1284 LCD modules.

---

**Have fun and may the fastest player win!**
