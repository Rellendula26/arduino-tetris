#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

// ============================================================
// TFT pins
// ============================================================

#define TFT_CS   10
#define TFT_DC   8
#define TFT_RST  9

Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_RST);

// ============================================================
// Button and buzzer pins
// ============================================================

#define BTN_LEFT   2
#define BTN_RIGHT  3
#define BTN_UP     4
#define BTN_DOWN   5
#define BUZZER     6

// ============================================================
// Display and map settings
// ============================================================

#define SCREEN_W 128
#define SCREEN_H 160

#define TILE_SIZE 16
#define MAP_W 8
#define MAP_H 8

// The map uses the lower 128 pixels.
// The top 32 pixels are used as the status area.
//
// Since your physical top section is corrupted, rotate the display
// if necessary so the corrupted region is less disruptive.

#define HUD_H 32
#define MAP_Y 32

// ============================================================
// Game states
// ============================================================

enum GameState {
  STATE_WORLD,
  STATE_BATTLE,
  STATE_MESSAGE
};

GameState gameState = STATE_WORLD;

// ============================================================
// Tile types
// ============================================================

enum TileType {
  TILE_GRASS,
  TILE_TALL_GRASS,
  TILE_TREE,
  TILE_WATER,
  TILE_PATH,
  TILE_HEAL
};

// ============================================================
// Map
// ============================================================

const byte gameMap[MAP_H][MAP_W] PROGMEM = {
  {2, 2, 2, 2, 2, 2, 2, 2},
  {2, 0, 0, 1, 1, 0, 5, 2},
  {2, 0, 1, 1, 0, 0, 4, 2},
  {2, 4, 4, 4, 4, 4, 4, 2},
  {2, 0, 1, 0, 0, 3, 3, 2},
  {2, 1, 1, 0, 0, 3, 3, 2},
  {2, 0, 0, 0, 1, 0, 0, 2},
  {2, 2, 2, 2, 2, 2, 2, 2}
};

// ============================================================
// Player data
// ============================================================

int playerX = 1;
int playerY = 1;

int playerHP = 20;
int playerMaxHP = 20;
int playerLevel = 1;
int playerXP = 0;

int playerDirection = 0;

// ============================================================
// Enemy data
// ============================================================

int enemyHP = 0;
int enemyMaxHP = 0;
int enemyLevel = 1;

int battleSelection = 0;

// ============================================================
// Timing
// ============================================================

unsigned long lastInputTime = 0;
const unsigned long inputDelay = 160;

// ============================================================
// Colors
// ============================================================

#define COLOR_DARK_GREEN  0x0320
#define COLOR_GRASS       0x4640
#define COLOR_LIGHT_GRASS 0x6EC0
#define COLOR_PATH        0xC5A0
#define COLOR_TREE        0x2540
#define COLOR_WATER       0x04DF
#define COLOR_SKIN        0xFD20
#define COLOR_BROWN       0x8200
#define COLOR_DARK_BLUE   0x0010
#define COLOR_HP_GREEN    0x07E0
#define COLOR_HP_YELLOW   0xFFE0
#define COLOR_HP_RED      0xF800
#define COLOR_GRAY        0x8410

// ============================================================
// Setup
// ============================================================

void setup() {
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);

  pinMode(BUZZER, OUTPUT);

  /*
    Use REDTAB because it previously gave you the most normal
    brightness.

    If the colors are inverted, change false to true below.
  */
  tft.initR(INITR_REDTAB);
  tft.setRotation(0);
  tft.invertDisplay(false);
  tft.setTextWrap(false);

  randomSeed(analogRead(A0));

  showTitleScreen();

  drawWorld();
}

// ============================================================
// Main loop
// ============================================================

void loop() {
  switch (gameState) {
    case STATE_WORLD:
      handleWorldInput();
      break;

    case STATE_BATTLE:
      handleBattleInput();
      break;

    case STATE_MESSAGE:
      break;
  }
}

// ============================================================
// Title screen
// ============================================================

void showTitleScreen() {
  tft.fillScreen(ST77XX_BLACK);

  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(2);

  tft.setCursor(14, 48);
  tft.print(F("TINY"));

  tft.setCursor(14, 70);
  tft.print(F("QUEST"));

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);

  tft.setCursor(18, 105);
  tft.print(F("A MINI RPG"));

  playStartSound();

  delay(1800);
}

// ============================================================
// World input
// ============================================================

void handleWorldInput() {
  unsigned long now = millis();

  if (now - lastInputTime < inputDelay) {
    return;
  }

  int newX = playerX;
  int newY = playerY;

  if (digitalRead(BTN_LEFT) == LOW) {
    newX--;
    playerDirection = 3;
  }
  else if (digitalRead(BTN_RIGHT) == LOW) {
    newX++;
    playerDirection = 1;
  }
  else if (digitalRead(BTN_UP) == LOW) {
    newY--;
    playerDirection = 0;
  }
  else if (digitalRead(BTN_DOWN) == LOW) {
    newY++;
    playerDirection = 2;
  }
  else {
    return;
  }

  lastInputTime = now;

  if (canMoveTo(newX, newY)) {
    playerX = newX;
    playerY = newY;

    tone(BUZZER, 850, 15);

    byte tile = getTile(playerX, playerY);

    if (tile == TILE_HEAL) {
      healPlayer();
      return;
    }

    drawWorld();

    if (tile == TILE_TALL_GRASS) {
      checkEncounter();
    }
  } else {
    tone(BUZZER, 180, 35);
  }
}

// ============================================================
// Tile functions
// ============================================================

byte getTile(int x, int y) {
  if (x < 0 || x >= MAP_W || y < 0 || y >= MAP_H) {
    return TILE_TREE;
  }

  return pgm_read_byte(&gameMap[y][x]);
}

bool canMoveTo(int x, int y) {
  byte tile = getTile(x, y);

  if (tile == TILE_TREE) {
    return false;
  }

  if (tile == TILE_WATER) {
    return false;
  }

  return true;
}

// ============================================================
// World drawing
// ============================================================

void drawWorld() {
  tft.fillScreen(ST77XX_BLACK);

  drawHUD();

  for (int y = 0; y < MAP_H; y++) {
    for (int x = 0; x < MAP_W; x++) {
      drawTile(x, y, getTile(x, y));
    }
  }

  drawPlayer(playerX, playerY);
}

void drawHUD() {
  tft.fillRect(0, 0, SCREEN_W, HUD_H, ST77XX_BLACK);
  tft.drawFastHLine(0, HUD_H - 1, SCREEN_W, ST77XX_WHITE);

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);

  tft.setCursor(3, 3);
  tft.print(F("SPARK"));

  tft.setCursor(3, 14);
  tft.print(F("LV"));
  tft.print(playerLevel);

  tft.setCursor(42, 3);
  tft.print(F("HP"));

  drawHealthBar(
    42,
    13,
    80,
    8,
    playerHP,
    playerMaxHP
  );

  tft.setCursor(88, 23);
  tft.print(playerHP);
  tft.print('/');
  tft.print(playerMaxHP);
}

void drawTile(int mapX, int mapY, byte tile) {
  int x = mapX * TILE_SIZE;
  int y = MAP_Y + mapY * TILE_SIZE;

  switch (tile) {
    case TILE_GRASS:
      drawGrassTile(x, y);
      break;

    case TILE_TALL_GRASS:
      drawTallGrassTile(x, y);
      break;

    case TILE_TREE:
      drawTreeTile(x, y);
      break;

    case TILE_WATER:
      drawWaterTile(x, y);
      break;

    case TILE_PATH:
      drawPathTile(x, y);
      break;

    case TILE_HEAL:
      drawHealTile(x, y);
      break;
  }
}

void drawGrassTile(int x, int y) {
  tft.fillRect(x, y, TILE_SIZE, TILE_SIZE, COLOR_GRASS);

  tft.drawPixel(x + 3, y + 5, COLOR_LIGHT_GRASS);
  tft.drawPixel(x + 11, y + 10, COLOR_LIGHT_GRASS);
  tft.drawPixel(x + 6, y + 13, COLOR_DARK_GREEN);
}

void drawTallGrassTile(int x, int y) {
  tft.fillRect(x, y, TILE_SIZE, TILE_SIZE, COLOR_GRASS);

  for (int i = 1; i < 15; i += 4) {
    tft.drawLine(
      x + i,
      y + 14,
      x + i - 2,
      y + 8,
      COLOR_DARK_GREEN
    );

    tft.drawLine(
      x + i,
      y + 14,
      x + i + 2,
      y + 7,
      COLOR_LIGHT_GRASS
    );
  }
}

void drawTreeTile(int x, int y) {
  tft.fillRect(x, y, TILE_SIZE, TILE_SIZE, COLOR_DARK_GREEN);

  tft.fillRect(x + 6, y + 10, 4, 6, COLOR_BROWN);

  tft.fillCircle(x + 8, y + 7, 7, COLOR_TREE);
  tft.fillCircle(x + 5, y + 8, 4, COLOR_GRASS);
  tft.fillCircle(x + 11, y + 8, 4, COLOR_GRASS);
}

void drawWaterTile(int x, int y) {
  tft.fillRect(x, y, TILE_SIZE, TILE_SIZE, COLOR_WATER);

  tft.drawFastHLine(x + 2, y + 5, 6, ST77XX_CYAN);
  tft.drawFastHLine(x + 8, y + 11, 6, ST77XX_CYAN);
}

void drawPathTile(int x, int y) {
  tft.fillRect(x, y, TILE_SIZE, TILE_SIZE, COLOR_PATH);

  tft.drawPixel(x + 4, y + 4, COLOR_BROWN);
  tft.drawPixel(x + 12, y + 8, COLOR_BROWN);
  tft.drawPixel(x + 7, y + 13, COLOR_BROWN);
}

void drawHealTile(int x, int y) {
  tft.fillRect(x, y, TILE_SIZE, TILE_SIZE, ST77XX_WHITE);

  tft.fillRect(x + 6, y + 2, 4, 12, ST77XX_RED);
  tft.fillRect(x + 2, y + 6, 12, 4, ST77XX_RED);
}

// ============================================================
// Player sprite
// ============================================================

void drawPlayer(int mapX, int mapY) {
  int x = mapX * TILE_SIZE;
  int y = MAP_Y + mapY * TILE_SIZE;

  // Shadow
  tft.fillEllipse(x + 8, y + 14, 5, 2, COLOR_DARK_GREEN);

  // Legs
  tft.fillRect(x + 5, y + 11, 3, 4, COLOR_DARK_BLUE);
  tft.fillRect(x + 9, y + 11, 3, 4, COLOR_DARK_BLUE);

  // Body
  tft.fillRect(x + 4, y + 7, 8, 6, ST77XX_RED);

  // Head
  tft.fillRect(x + 5, y + 3, 7, 6, COLOR_SKIN);

  // Hair
  tft.fillRect(x + 5, y + 2, 7, 3, COLOR_BROWN);

  // Hat
  tft.fillRect(x + 4, y + 1, 8, 2, ST77XX_RED);
  tft.fillRect(x + 10, y + 3, 4, 2, ST77XX_RED);

  // Direction marker
  if (playerDirection == 0) {
    tft.drawPixel(x + 8, y + 2, ST77XX_WHITE);
  }
  else if (playerDirection == 1) {
    tft.drawPixel(x + 12, y + 6, ST77XX_WHITE);
  }
  else if (playerDirection == 2) {
    tft.drawPixel(x + 8, y + 9, ST77XX_WHITE);
  }
  else {
    tft.drawPixel(x + 4, y + 6, ST77XX_WHITE);
  }
}

// ============================================================
// Encounter system
// ============================================================

void checkEncounter() {
  // Approximately a 25% chance each step in tall grass.
  if (random(100) < 25) {
    startBattle();
  }
}

void startBattle() {
  enemyLevel = playerLevel + random(0, 2);
  enemyMaxHP = 8 + enemyLevel * 4;
  enemyHP = enemyMaxHP;

  battleSelection = 0;
  gameState = STATE_BATTLE;

  playEncounterSound();

  drawBattle();
}

// ============================================================
// Battle input
// ============================================================

void handleBattleInput() {
  unsigned long now = millis();

  if (now - lastInputTime < 180) {
    return;
  }

  if (digitalRead(BTN_LEFT) == LOW) {
    battleSelection = 0;
    lastInputTime = now;
    drawBattleMenu();
  }
  else if (digitalRead(BTN_RIGHT) == LOW) {
    battleSelection = 1;
    lastInputTime = now;
    drawBattleMenu();
  }
  else if (digitalRead(BTN_UP) == LOW) {
    lastInputTime = now;

    if (battleSelection == 0) {
      playerAttack();
    } else {
      attemptRun();
    }
  }
}

// ============================================================
// Battle drawing
// ============================================================

void drawBattle() {
  tft.fillScreen(ST77XX_WHITE);

  drawEnemyArea();
  drawPlayerBattleArea();
  drawBattleMenu();
}

void drawEnemyArea() {
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_BLACK);

  tft.setCursor(5, 5);
  tft.print(F("WILD SPROUT"));

  tft.setCursor(5, 15);
  tft.print(F("LV"));
  tft.print(enemyLevel);

  drawHealthBar(
    5,
    25,
    70,
    7,
    enemyHP,
    enemyMaxHP
  );

  drawEnemySprite(92, 47);
}

void drawPlayerBattleArea() {
  drawBackSprite(27, 103);

  tft.fillRect(55, 67, 71, 39, ST77XX_WHITE);
  tft.drawRect(55, 67, 71, 39, ST77XX_BLACK);

  tft.setTextColor(ST77XX_BLACK);
  tft.setTextSize(1);

  tft.setCursor(59, 71);
  tft.print(F("SPARK"));

  tft.setCursor(100, 71);
  tft.print(F("LV"));
  tft.print(playerLevel);

  tft.setCursor(59, 83);
  tft.print(F("HP"));

  drawHealthBar(
    75,
    83,
    46,
    7,
    playerHP,
    playerMaxHP
  );

  tft.setCursor(84, 94);
  tft.print(playerHP);
  tft.print('/');
  tft.print(playerMaxHP);
}

void drawEnemySprite(int centerX, int centerY) {
  // Shadow
  tft.fillEllipse(
    centerX,
    centerY + 16,
    18,
    5,
    COLOR_GRAY
  );

  // Body
  tft.fillCircle(
    centerX,
    centerY,
    14,
    COLOR_GRASS
  );

  // Leaves
  tft.fillTriangle(
    centerX,
    centerY - 12,
    centerX - 11,
    centerY - 24,
    centerX - 3,
    centerY - 7,
    COLOR_DARK_GREEN
  );

  tft.fillTriangle(
    centerX,
    centerY - 12,
    centerX + 11,
    centerY - 24,
    centerX + 3,
    centerY - 7,
    COLOR_DARK_GREEN
  );

  // Eyes
  tft.fillCircle(
    centerX - 5,
    centerY - 2,
    2,
    ST77XX_BLACK
  );

  tft.fillCircle(
    centerX + 5,
    centerY - 2,
    2,
    ST77XX_BLACK
  );

  // Mouth
  tft.drawFastHLine(
    centerX - 3,
    centerY + 6,
    7,
    ST77XX_BLACK
  );
}

void drawBackSprite(int centerX, int centerY) {
  // Shadow
  tft.fillEllipse(
    centerX,
    centerY + 18,
    22,
    5,
    COLOR_GRAY
  );

  // Body
  tft.fillCircle(
    centerX,
    centerY,
    17,
    ST77XX_YELLOW
  );

  // Ears
  tft.fillTriangle(
    centerX - 13,
    centerY - 11,
    centerX - 21,
    centerY - 27,
    centerX - 5,
    centerY - 16,
    ST77XX_YELLOW
  );

  tft.fillTriangle(
    centerX + 13,
    centerY - 11,
    centerX + 21,
    centerY - 27,
    centerX + 5,
    centerY - 16,
    ST77XX_YELLOW
  );

  // Back markings
  tft.fillRect(
    centerX - 10,
    centerY - 5,
    5,
    12,
    COLOR_BROWN
  );

  tft.fillRect(
    centerX + 5,
    centerY - 5,
    5,
    12,
    COLOR_BROWN
  );
}

void drawBattleMenu() {
  tft.fillRect(0, 118, 128, 42, ST77XX_BLACK);
  tft.drawRect(0, 118, 128, 42, ST77XX_WHITE);

  tft.setTextSize(1);

  if (battleSelection == 0) {
    tft.setTextColor(ST77XX_YELLOW);
  } else {
    tft.setTextColor(ST77XX_WHITE);
  }

  tft.setCursor(12, 132);
  tft.print(F("> FIGHT"));

  if (battleSelection == 1) {
    tft.setTextColor(ST77XX_YELLOW);
  } else {
    tft.setTextColor(ST77XX_WHITE);
  }

  tft.setCursor(72, 132);
  tft.print(F("> RUN"));

  tft.setTextColor(COLOR_GRAY);
  tft.setCursor(17, 148);
  tft.print(F("UP = SELECT"));
}

// ============================================================
// Battle actions
// ============================================================

void playerAttack() {
  int damage = random(3, 7) + playerLevel;

  enemyHP -= damage;

  if (enemyHP < 0) {
    enemyHP = 0;
  }

  tone(BUZZER, 1200, 80);

  showBattleMessage(F("SPARK ATTACKS!"));
  delay(600);

  drawBattle();

  if (enemyHP <= 0) {
    winBattle();
    return;
  }

  delay(350);

  enemyAttack();
}

void enemyAttack() {
  int damage = random(2, 5) + enemyLevel / 2;

  playerHP -= damage;

  if (playerHP < 0) {
    playerHP = 0;
  }

  tone(BUZZER, 300, 100);

  showBattleMessage(F("SPROUT ATTACKS!"));
  delay(600);

  if (playerHP <= 0) {
    playerFainted();
    return;
  }

  drawBattle();
}

void attemptRun() {
  if (random(100) < 70) {
    tone(BUZZER, 1000, 60);

    showBattleMessage(F("GOT AWAY!"));
    delay(700);

    gameState = STATE_WORLD;
    drawWorld();
  } else {
    showBattleMessage(F("CANNOT ESCAPE!"));
    delay(700);

    drawBattle();
    enemyAttack();
  }
}

void winBattle() {
  int gainedXP = 5 + enemyLevel * 3;

  playerXP += gainedXP;

  playVictorySound();

  showBattleMessage(F("YOU WON!"));
  delay(700);

  if (playerXP >= playerLevel * 10) {
    playerXP -= playerLevel * 10;
    playerLevel++;

    playerMaxHP += 4;
    playerHP = playerMaxHP;

    showBattleMessage(F("LEVEL UP!"));
    playLevelSound();

    delay(900);
  }

  gameState = STATE_WORLD;
  drawWorld();
}

void playerFainted() {
  playFaintSound();

  showBattleMessage(F("SPARK FAINTED!"));
  delay(1000);

  playerHP = playerMaxHP;
  playerX = 6;
  playerY = 1;

  gameState = STATE_WORLD;

  drawWorld();
}

// ============================================================
// Healing
// ============================================================

void healPlayer() {
  playerHP = playerMaxHP;

  tft.fillScreen(ST77XX_BLACK);

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);

  tft.setCursor(14, 60);
  tft.print(F("YOUR CREATURE"));

  tft.setCursor(20, 76);
  tft.print(F("WAS HEALED!"));

  playHealSound();

  delay(1200);

  drawWorld();
}

// ============================================================
// Messages
// ============================================================

void showBattleMessage(const __FlashStringHelper *message) {
  tft.fillRect(0, 118, 128, 42, ST77XX_BLACK);
  tft.drawRect(0, 118, 128, 42, ST77XX_WHITE);

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);

  tft.setCursor(8, 135);
  tft.print(message);
}

// ============================================================
// Health bar
// ============================================================

void drawHealthBar(
  int x,
  int y,
  int width,
  int height,
  int hp,
  int maxHP
) {
  tft.drawRect(x, y, width, height, ST77XX_BLACK);

  int insideWidth = width - 2;

  int fillWidth = 0;

  if (maxHP > 0) {
    fillWidth = ((long)hp * insideWidth) / maxHP;
  }

  if (fillWidth < 0) {
    fillWidth = 0;
  }

  if (fillWidth > insideWidth) {
    fillWidth = insideWidth;
  }

  uint16_t healthColor;

  if (hp > maxHP / 2) {
    healthColor = COLOR_HP_GREEN;
  }
  else if (hp > maxHP / 4) {
    healthColor = COLOR_HP_YELLOW;
  }
  else {
    healthColor = COLOR_HP_RED;
  }

  tft.fillRect(
    x + 1,
    y + 1,
    insideWidth,
    height - 2,
    COLOR_GRAY
  );

  tft.fillRect(
    x + 1,
    y + 1,
    fillWidth,
    height - 2,
    healthColor
  );
}

// ============================================================
// Sounds
// ============================================================

void playStartSound() {
  tone(BUZZER, 600, 100);
  delay(120);

  tone(BUZZER, 800, 100);
  delay(120);

  tone(BUZZER, 1100, 180);
  delay(200);
}

void playEncounterSound() {
  tone(BUZZER, 900, 70);
  delay(80);

  tone(BUZZER, 500, 70);
  delay(80);

  tone(BUZZER, 1000, 120);
  delay(130);
}

void playVictorySound() {
  tone(BUZZER, 700, 100);
  delay(120);

  tone(BUZZER, 900, 100);
  delay(120);

  tone(BUZZER, 1200, 180);
  delay(200);
}

void playLevelSound() {
  tone(BUZZER, 700, 80);
  delay(90);

  tone(BUZZER, 900, 80);
  delay(90);

  tone(BUZZER, 1100, 80);
  delay(90);

  tone(BUZZER, 1400, 180);
  delay(190);
}

void playHealSound() {
  tone(BUZZER, 800, 80);
  delay(100);

  tone(BUZZER, 1000, 80);
  delay(100);

  tone(BUZZER, 1200, 160);
  delay(170);
}

void playFaintSound() {
  tone(BUZZER, 500, 150);
  delay(170);

  tone(BUZZER, 350, 150);
  delay(170);

  tone(BUZZER, 200, 300);
  delay(310);
}
