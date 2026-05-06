#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

#define TFT_CS   10
#define TFT_DC   8
#define TFT_RST  9

#define BTN_LEFT   2
#define BTN_RIGHT  3
#define BTN_ROTATE 4
#define BTN_DOWN   5
#define BUZZER     6

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

#define GRID_W 10
#define GRID_H 18
#define BLOCK 6

#define OFFSET_X 4
#define OFFSET_Y 4

bool grid[GRID_W][GRID_H];
uint16_t gridColor[GRID_W][GRID_H];

int pieceX, pieceY;
int currentPiece, currentRot, nextPiece;
int score = 0;
int level = 1;
int linesCleared = 0;

unsigned long lastFall = 0;
unsigned long lastLeft = 0;
unsigned long lastRight = 0;
unsigned long lastRotate = 0;
unsigned long lastDown = 0;

int fallDelay = 500;

const uint16_t pieceColors[7] = {
  ST77XX_CYAN, ST77XX_YELLOW, ST77XX_MAGENTA,
  ST77XX_GREEN, ST77XX_RED, ST77XX_BLUE, ST77XX_ORANGE
};

const byte pieces[7][4][4][4] = {
  // I
  {
    {{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}},
    {{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,1,0}},
    {{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}},
    {{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,1,0}}
  },
  // O
  {
    {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
    {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
    {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
    {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}}
  },
  // T
  {
    {{0,1,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
    {{0,1,0,0},{0,1,1,0},{0,1,0,0},{0,0,0,0}},
    {{0,0,0,0},{1,1,1,0},{0,1,0,0},{0,0,0,0}},
    {{0,1,0,0},{1,1,0,0},{0,1,0,0},{0,0,0,0}}
  },
  // S
  {
    {{0,1,1,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}},
    {{0,1,0,0},{0,1,1,0},{0,0,1,0},{0,0,0,0}},
    {{0,1,1,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}},
    {{0,1,0,0},{0,1,1,0},{0,0,1,0},{0,0,0,0}}
  },
  // Z
  {
    {{1,1,0,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
    {{0,0,1,0},{0,1,1,0},{0,1,0,0},{0,0,0,0}},
    {{1,1,0,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
    {{0,0,1,0},{0,1,1,0},{0,1,0,0},{0,0,0,0}}
  },
  // J
  {
    {{1,0,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
    {{0,1,1,0},{0,1,0,0},{0,1,0,0},{0,0,0,0}},
    {{0,0,0,0},{1,1,1,0},{0,0,1,0},{0,0,0,0}},
    {{0,1,0,0},{0,1,0,0},{1,1,0,0},{0,0,0,0}}
  },
  // L
  {
    {{0,0,1,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
    {{0,1,0,0},{0,1,0,0},{0,1,1,0},{0,0,0,0}},
    {{0,0,0,0},{1,1,1,0},{1,0,0,0},{0,0,0,0}},
    {{1,1,0,0},{0,1,0,0},{0,1,0,0},{0,0,0,0}}
  }
};

// --- Buzzer functions ---
void beepMove() {
  tone(BUZZER, 1000, 30);
}

void beepRotate() {
  tone(BUZZER, 1200, 40);
}

void beepLock() {
  tone(BUZZER, 600, 60);
}

void beepLine(int count) {
  if (count == 4) {
    // tetris — special fanfare
    tone(BUZZER, 1000, 80); delay(90);
    tone(BUZZER, 1500, 80); delay(90);
    tone(BUZZER, 2000, 150);
  } else {
    tone(BUZZER, 1500, 80); delay(90);
    tone(BUZZER, 2000, 80);
  }
}

void beepGameOver() {
  tone(BUZZER, 400, 200); delay(250);
  tone(BUZZER, 300, 200); delay(250);
  tone(BUZZER, 200, 400);
}

void setup() {
  pinMode(BTN_LEFT,   INPUT_PULLUP);
  pinMode(BTN_RIGHT,  INPUT_PULLUP);
  pinMode(BTN_ROTATE, INPUT_PULLUP);
  pinMode(BTN_DOWN,   INPUT_PULLUP);
  pinMode(BUZZER,     OUTPUT);

  tft.initR(INITR_REDTAB);
  tft.setRotation(0);
  tft.fillScreen(ST77XX_BLACK);

  randomSeed(analogRead(A0));
  nextPiece = random(7);
  resetGame();
}

void loop() {
  handleButtons();

  if (millis() - lastFall > fallDelay) {
    if (!collides(pieceX, pieceY + 1, currentRot)) {
      pieceY++;
    } else {
      lockPiece();
      clearLines();
      spawnPiece();
    }
    lastFall = millis();
    drawGame();
  }
}

void resetGame() {
  for (int x = 0; x < GRID_W; x++)
    for (int y = 0; y < GRID_H; y++) {
      grid[x][y] = false;
      gridColor[x][y] = ST77XX_BLACK;
    }
  score = 0;
  level = 1;
  linesCleared = 0;
  fallDelay = 500;
  spawnPiece();
  drawGame();
}

void spawnPiece() {
  currentPiece = nextPiece;
  nextPiece = random(7);
  currentRot = 0;
  pieceX = 3;
  pieceY = 0;
  if (collides(pieceX, pieceY, currentRot)) gameOver();
}

void handleButtons() {
  bool moved = false;

  if (digitalRead(BTN_LEFT) == LOW && millis() - lastLeft > 120) {
    if (!collides(pieceX - 1, pieceY, currentRot)) {
      pieceX--;
      beepMove();
    }
    lastLeft = millis();
    moved = true;
  }

  if (digitalRead(BTN_RIGHT) == LOW && millis() - lastRight > 120) {
    if (!collides(pieceX + 1, pieceY, currentRot)) {
      pieceX++;
      beepMove();
    }
    lastRight = millis();
    moved = true;
  }

  if (digitalRead(BTN_ROTATE) == LOW && millis() - lastRotate > 150) {
    int newRot = (currentRot + 1) % 4;
    if (!collides(pieceX, pieceY, newRot)) {
      currentRot = newRot;
      beepRotate();
    } else if (!collides(pieceX - 1, pieceY, newRot)) {
      pieceX--;
      currentRot = newRot;
      beepRotate();
    } else if (!collides(pieceX + 1, pieceY, newRot)) {
      pieceX++;
      currentRot = newRot;
      beepRotate();
    }
    lastRotate = millis();
    moved = true;
  }

  if (digitalRead(BTN_DOWN) == LOW && millis() - lastDown > 50) {
    if (!collides(pieceX, pieceY + 1, currentRot)) {
      pieceY++;
      score += 1;
      lastFall = millis();
    } else {
      lockPiece();
      clearLines();
      spawnPiece();
    }
    lastDown = millis();
    moved = true;
  }

  if (moved) drawGame();
}

bool collides(int x, int y, int rot) {
  for (int py = 0; py < 4; py++)
    for (int px = 0; px < 4; px++)
      if (pieces[currentPiece][rot][py][px]) {
        int gx = x + px;
        int gy = y + py;
        if (gx < 0 || gx >= GRID_W) return true;
        if (gy >= GRID_H) return true;
        if (gy >= 0 && grid[gx][gy]) return true;
      }
  return false;
}

void lockPiece() {
  for (int py = 0; py < 4; py++)
    for (int px = 0; px < 4; px++)
      if (pieces[currentPiece][currentRot][py][px]) {
        int gx = pieceX + px;
        int gy = pieceY + py;
        if (gx >= 0 && gx < GRID_W && gy >= 0 && gy < GRID_H) {
          grid[gx][gy] = true;
          gridColor[gx][gy] = pieceColors[currentPiece];
        }
      }
  beepLock();
}

void clearLines() {
  int cleared = 0;
  for (int y = GRID_H - 1; y >= 0; y--) {
    bool full = true;
    for (int x = 0; x < GRID_W; x++)
      if (!grid[x][y]) { full = false; break; }

    if (full) {
      cleared++;
      for (int yy = y; yy > 0; yy--)
        for (int x = 0; x < GRID_W; x++) {
          grid[x][yy] = grid[x][yy - 1];
          gridColor[x][yy] = gridColor[x][yy - 1];
        }
      for (int x = 0; x < GRID_W; x++) {
        grid[x][0] = false;
        gridColor[x][0] = ST77XX_BLACK;
      }
      y++;
    }
  }

  const int pts[5] = {0, 100, 300, 500, 800};
  if (cleared > 0) {
    score += pts[min(cleared, 4)] * level;
    linesCleared += cleared;
    level = linesCleared / 10 + 1;
    fallDelay = max(100, 500 - (level - 1) * 40);
    beepLine(cleared);
  }
}

void drawGame() {
  tft.fillScreen(ST77XX_BLACK);
  drawBoard();
  drawLocked();
  drawPiece();
  drawSidebar();
}

void drawBoard() {
  tft.drawRect(
    OFFSET_X - 1, OFFSET_Y - 1,
    GRID_W * BLOCK + 2, GRID_H * BLOCK + 2,
    ST77XX_WHITE
  );
}

void drawLocked() {
  for (int x = 0; x < GRID_W; x++)
    for (int y = 0; y < GRID_H; y++)
      if (grid[x][y]) drawBlock(x, y, gridColor[x][y]);
}

void drawPiece() {
  for (int py = 0; py < 4; py++)
    for (int px = 0; px < 4; px++)
      if (pieces[currentPiece][currentRot][py][px])
        drawBlock(pieceX + px, pieceY + py, pieceColors[currentPiece]);
}

void drawSidebar() {
  int sx = OFFSET_X + GRID_W * BLOCK + 6;

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(sx, 4);
  tft.print("NEXT");

  for (int py = 0; py < 4; py++)
    for (int px = 0; px < 4; px++)
      if (pieces[nextPiece][0][py][px])
        tft.fillRect(sx + px * 5, 14 + py * 5, 4, 4, pieceColors[nextPiece]);

  tft.drawFastHLine(sx, 38, 40, ST77XX_WHITE);

  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(sx, 42);
  tft.print("SCORE");
  tft.setTextColor(ST77XX_GREEN);
  tft.setCursor(sx, 52);
  tft.print(score);

  tft.drawFastHLine(sx, 64, 40, ST77XX_WHITE);

  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(sx, 68);
  tft.print("LEVEL");
  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(sx, 78);
  tft.print(level);

  tft.drawFastHLine(sx, 90, 40, ST77XX_WHITE);

  tft.setTextColor(0x4208);
  tft.setCursor(sx, 94);
  tft.print("L:lft");
  tft.setCursor(sx, 104);
  tft.print("R:rgt");
  tft.setCursor(sx, 114);
  tft.print("M:rot");
  tft.setCursor(sx, 124);
  tft.print("D:drp");
}

void drawBlock(int gx, int gy, uint16_t color) {
  int x = OFFSET_X + gx * BLOCK;
  int y = OFFSET_Y + gy * BLOCK;
  tft.fillRect(x, y, BLOCK - 1, BLOCK - 1, color);
}

void gameOver() {
  beepGameOver();
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_RED);
  tft.setTextSize(2);
  tft.setCursor(18, 40);
  tft.print("GAME");
  tft.setCursor(18, 60);
  tft.print("OVER");
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(18, 90);
  tft.print("Score:");
  tft.setCursor(18, 102);
  tft.setTextColor(ST77XX_GREEN);
  tft.print(score);
  tft.setCursor(18, 114);
  tft.setTextColor(ST77XX_YELLOW);
  tft.print("Level: "); tft.print(level);
  delay(3000);
  resetGame();
}