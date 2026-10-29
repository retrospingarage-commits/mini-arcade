// Casse-briques façon Arkanoid : 6 niveaux de difficulté croissante (voir include/levels.h).
// Encodeur : raquette (avec inertie)   Bouton 1 ou bouton de l'encodeur : lancer   Bouton 2 : pause
// Réglages en direct via le moniteur série :  g <gain>   f <frottement>   m <vitesse max>   n <niveau>
#include <Arduino.h>
#include <FastLED.h>
#include <U8g2lib.h>
#include "config.h"
#include "font.h"
#include "levels.h"

CRGB leds[NUM_LEDS];
U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

// --- Réglages du ressenti ---------------------------------------------------
float gain     = 0.25f;  // cases/s de vitesse ajoutées par impulsion de l'encodeur
float friction = 0.97f;  // multiplicateur de vitesse à chaque image (plus petit = freine plus)
float vMax     = 30.0f;  // cases/s
// ---------------------------------------------------------------------------

// --- Réglages du jeu --------------------------------------------------------
constexpr uint8_t  START_LIVES      = 3;
constexpr float    SPEED_PER_BRICK  = 0.06f;  // la balle accélère à chaque brique cassée (pixels/s)
constexpr float    MAX_BOUNCE_ANGLE = 1.05f;  // angle max de rebond sur la raquette (~60°, en radians)
constexpr float    MIN_BOUNCE_ANGLE = 0.2f;   // angle min (~11°) : la balle ne repart jamais à la verticale
constexpr uint8_t  PADDLE_Y         = MATRIX_H - 1;
constexpr uint8_t  BRICK_TOP        = 1;      // ligne du haut du mur (la ligne 0 reste libre)
constexpr uint32_t MOVE_PERIOD      = 700;    // LISA-LOUISE : décalage des lignes (ms)
constexpr uint32_t FALL_PERIOD      = 8000;   // EMILIE-ROSE : descente du mur d'une ligne (ms)
constexpr uint8_t  FALL_LIMIT       = 13;     // EMILIE-ROSE : si le mur atteint cette ligne, vie perdue
constexpr uint32_t SCROLL_MS        = 45;     // vitesse du texte défilant (ms par pixel)
// ---------------------------------------------------------------------------

enum State { TITLE, SERVE, PLAY, PAUSE, CLEAR, GAME_OVER, VICTORY };
State    state = TITLE;
uint32_t stateSince = 0;

uint8_t  levelIdx = 0;
uint8_t  lives = START_LIVES;
uint32_t score = 0;
bool     dirty = true;  // l'OLED doit être redessiné

uint8_t  grid[BRICK_ROWS][BRICK_COLS];  // coups restants par brique (METAL = incassable)
uint8_t  glow[BRICK_ROWS][BRICK_COLS];  // éclat blanc quand une brique est touchée
uint8_t  brickTop = BRICK_TOP;
uint16_t bricksLeft = 0, bricksBroken = 0;
uint32_t lastMove = 0, lastFall = 0, hurtUntil = 0;

float   padPos = MATRIX_W / 2.0f, padVel = 0;
uint8_t padW = 5;
float   bx = 0, by = 0, vx = 0, vy = 0, speed = 0;

const Level& lvl() { return LEVELS[levelIdx]; }

// Encodeur en quadrature, lu par interruption (4 impulsions par cran sur un KY-040)
volatile int32_t encCount = 0;
volatile uint8_t encState = 0;
void IRAM_ATTR onEnc() {
  static const int8_t table[16] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};
  encState = ((encState << 2) | (digitalRead(PIN_ENC_A) << 1) | digitalRead(PIN_ENC_B)) & 0x0F;
  encCount += table[encState];
}

uint16_t XY(uint8_t x, uint8_t y) {
  if (MATRIX_SERPENTINE && (y & 1)) x = MATRIX_W - 1 - x;
  return y * MATRIX_W + x;
}

void setPixel(int x, int y, CRGB c) {
  if (x < 0 || x >= MATRIX_W || y < 0 || y >= MATRIX_H) return;
  leds[XY(x, y)] = c;
}

struct Button {
  uint8_t pin; bool last = true; uint32_t t = 0;
  bool pressed() {  // front descendant avec anti-rebond logiciel
    bool v = digitalRead(pin);
    bool p = (last && !v && millis() - t > 20);
    if (v != last) { last = v; t = millis(); }
    return p;
  }
};
Button btn1{PIN_BTN1}, btn2{PIN_BTN2}, btnEnc{PIN_ENC_SW};

// --- Texte défilant ---------------------------------------------------------
int textWidth(const char* s) { return strlen(s) * 4 - 1; }
uint32_t scrollDuration(const char* s) { return (MATRIX_W + textWidth(s)) * SCROLL_MS; }

void drawText(const char* s, int x, int y, CRGB col) {
  for (; *s; s++, x += 4) {
    const uint8_t* g = glyph(*s);
    for (int r = 0; r < 5; r++)
      for (int c = 0; c < 3; c++)
        if (g[r] & (4 >> c)) setPixel(x + c, y + r, col);
  }
}

void drawScroll(const char* s, uint32_t t, int y, CRGB col) {
  drawText(s, MATRIX_W - (int)(t / SCROLL_MS), y, col);
}

// --- Déroulement de la partie -----------------------------------------------
void setState(State s) {
  state = s;
  stateSince = millis();
  dirty = true;
}

void loadLevel(uint8_t i) {
  levelIdx = i;
  bricksLeft = bricksBroken = 0;
  brickTop = BRICK_TOP;
  for (int r = 0; r < BRICK_ROWS; r++)
    for (int c = 0; c < BRICK_COLS; c++) {
      char ch = LEVELS[i].rows[r][c];
      grid[r][c] = ch == '#' ? METAL : (ch >= '1' && ch <= '3') ? ch - '0' : 0;
      glow[r][c] = 0;
      if (grid[r][c] && grid[r][c] != METAL) bricksLeft++;
    }
  padW = LEVELS[i].paddle;
  setState(TITLE);
}

void newGame() {
  score = 0;
  lives = START_LIVES;
  loadLevel(0);
}

// Envoie la balle vers le haut avec un angle par rapport à la verticale (négatif = vers la gauche)
void sendUp(float a) {
  if (fabsf(a) < MIN_BOUNCE_ANGLE) a = (a < 0 || (a == 0 && random(2))) ? -MIN_BOUNCE_ANGLE : MIN_BOUNCE_ANGLE;
  vx = speed * sinf(a);
  vy = -speed * cosf(a);
}

void launch() {
  speed = lvl().speed + bricksBroken * SPEED_PER_BRICK;
  sendUp(random(-40, 41) / 100.0f);  // petit angle aléatoire au départ
  lastMove = lastFall = millis();
  setState(PLAY);
}

void resume() {  // on décale les minuteries pour que la pause ne compte pas
  uint32_t paused = millis() - stateSince;
  lastMove += paused;
  lastFall += paused;
  setState(PLAY);
}

void loseLife() {
  lives--;
  hurtUntil = millis() + 300;
  setState(lives ? SERVE : GAME_OVER);
}

// La balle garde sa direction mais prend la vitesse du moment
void updateSpeed() {
  speed = lvl().speed + bricksBroken * SPEED_PER_BRICK;
  float n = sqrtf(vx * vx + vy * vy);
  vx *= speed / n;
  vy *= speed / n;
}

// Petit écart aléatoire : la balle ne peut pas rester coincée dans une boucle entre des briques de métal
void nudge() {
  vx += speed * random(-15, 16) / 100.0f;
  if (fabsf(vy) < speed * 0.3f) vy = vy < 0 ? -speed * 0.3f : speed * 0.3f;
  updateSpeed();
}

// --- Briques ----------------------------------------------------------------
bool brickAt(float x, float y) {
  if (x < 0 || y < 0) return false;
  int r = (int)y - brickTop, c = (int)x / 2;
  if (r < 0 || r >= BRICK_ROWS || c >= BRICK_COLS) return false;
  return grid[r][c] != 0;
}

void hitBrick(float x, float y) {
  int r = (int)y - brickTop, c = (int)x / 2;
  if (!grid[r][c]) return;
  glow[r][c] = 255;
  if (grid[r][c] == METAL) { nudge(); return; }
  score += 10;
  dirty = true;
  if (--grid[r][c] == 0) {
    bricksLeft--;
    bricksBroken++;
    updateSpeed();
    if (!bricksLeft) setState(CLEAR);
  }
}

int lowestRow() {
  for (int r = BRICK_ROWS - 1; r >= 0; r--)
    for (int c = 0; c < BRICK_COLS; c++)
      if (grid[r][c]) return r;
  return 0;
}

void rotateRow(uint8_t* row, bool right) {
  if (right) { uint8_t t = row[BRICK_COLS - 1]; memmove(row + 1, row, BRICK_COLS - 1); row[0] = t; }
  else       { uint8_t t = row[0]; memmove(row, row + 1, BRICK_COLS - 1); row[BRICK_COLS - 1] = t; }
}

void updateSpecial(uint32_t now) {
  if (lvl().special == SP_MOVING && now - lastMove >= MOVE_PERIOD) {
    lastMove = now;
    for (int r = 0; r < BRICK_ROWS; r++) {
      rotateRow(grid[r], r % 2 == 0);
      rotateRow(glow[r], r % 2 == 0);
    }
  }
  if (lvl().special == SP_FALLING && now - lastFall >= FALL_PERIOD) {
    lastFall = now;
    brickTop++;
    if (brickTop + lowestRow() >= FALL_LIMIT) {  // le mur a rattrapé la raquette
      brickTop = BRICK_TOP;
      loseLife();
    }
  }
}

// --- Raquette et balle ------------------------------------------------------
void updatePaddle(float dt, bool active) {
  static int32_t lastEnc = 0;
  noInterrupts(); int32_t c = encCount; interrupts();
  if (!active) { lastEnc = c; padVel = 0; return; }  // on ignore l'encodeur hors du jeu
  padVel += (c - lastEnc) * gain;
  lastEnc = c;
  padVel = constrain(padVel * friction, -vMax, vMax);
  padPos += padVel * dt;
  float lo = padW / 2.0f, hi = MATRIX_W - padW / 2.0f;
  if (padPos < lo) { padPos = lo; padVel = -padVel * 0.5f; }
  if (padPos > hi) { padPos = hi; padVel = -padVel * 0.5f; }
}

void updateBall(float dt) {
  int steps = max(1, (int)ceilf(speed * dt / 0.4f));  // petits pas pour ne pas traverser une brique
  float h = dt / steps;
  for (int i = 0; i < steps && state == PLAY; i++) {
    float nx = bx + vx * h, ny = by + vy * h;
    if (nx < 0)         { nx = -nx; vx = -vx; }
    if (nx >= MATRIX_W) { nx = 2 * MATRIX_W - nx - 0.01f; vx = -vx; }
    if (ny < 0)         { ny = -ny; vy = -vy; }

    if (brickAt(nx, ny)) {  // on regarde de quel côté la balle arrive sur la brique
      bool hitV = brickAt(bx, ny), hitH = brickAt(nx, by);
      if (hitV) { hitBrick(bx, ny); vy = -vy; }
      if (hitH) { hitBrick(nx, by); vx = -vx; }
      if (!hitV && !hitH) { hitBrick(nx, ny); vx = -vx; vy = -vy; }  // coin
      continue;
    }

    if (vy > 0 && by < PADDLE_Y && ny >= PADDLE_Y) {
      float left = padPos - padW / 2.0f;
      if (nx >= left - 0.5f && nx < left + padW + 0.5f) {
        // plus la balle tape loin du centre, plus elle repart en biais
        float off = constrain((nx - padPos) / (padW / 2.0f + 0.5f), -1.0f, 1.0f);
        sendUp(off * MAX_BOUNCE_ANGLE);
        ny = PADDLE_Y - 0.01f;
      }
    }
    if (ny >= MATRIX_H) { loseLife(); return; }
    bx = nx;
    by = ny;
  }
}

// --- Affichage --------------------------------------------------------------
CRGB brickColor(uint8_t hp) {
  if (hp == METAL) return CRGB(90, 90, 110);
  return CHSV(lvl().hue[hp - 1], 255, 255);
}

void drawBricks() {
  for (int r = 0; r < BRICK_ROWS; r++)
    for (int c = 0; c < BRICK_COLS; c++) {
      int y = brickTop + r;
      CRGB col = grid[r][c] ? brickColor(grid[r][c]) : CRGB::Black;
      if (lvl().special == SP_DARK && grid[r][c]) {  // seule la balle éclaire
        float d = hypotf(c * 2 + 1 - bx, y + 0.5f - by);
        col.nscale8(d < 1.5f ? 255 : d > 5.5f ? 0 : (uint8_t)(255 * (5.5f - d) / 4));
      }
      if (glow[r][c]) {
        nblend(col, CRGB::White, glow[r][c]);
        glow[r][c] = qsub8(glow[r][c], 25);
      }
      setPixel(c * 2, y, col);
      setPixel(c * 2 + 1, y, col);
    }
}

void drawGame(uint32_t now) {
  if (now < hurtUntil) fill_solid(leds, NUM_LEDS, CRGB(60, 0, 0));
  drawBricks();
  int left = (int)roundf(padPos - padW / 2.0f);
  for (int i = 0; i < padW; i++)
    setPixel(left + i, PADDLE_Y, (i == 0 || i == padW - 1) ? CRGB::Red : CRGB(150, 150, 150));
  setPixel((int)bx, (int)by, lvl().special == SP_DARK ? CRGB(255, 200, 40) : CRGB::White);
}

void render(uint32_t now) {
  uint32_t t = now - stateSince;
  FastLED.clear();
  switch (state) {
    case TITLE: {
      char num[2] = {char('1' + levelIdx), 0};
      drawText(num, 6, 1, CRGB::White);
      drawScroll(lvl().name, t, 8, CHSV(lvl().hue[0], 255, 255));
      break;
    }
    case SERVE:
    case PLAY:
      drawGame(now);
      break;
    case PAUSE:
      drawGame(now);
      fadeToBlackBy(leds, NUM_LEDS, 170);
      break;
    case CLEAR:  // le mur clignote aux couleurs du niveau
      if ((t / 150) % 2) fill_solid(leds, NUM_LEDS, CHSV(lvl().hue[0], 255, 120));
      break;
    case GAME_OVER:
      drawScroll("GAME OVER", t % scrollDuration("GAME OVER"), 5, CRGB::Red);
      break;
    case VICTORY:
      if (random8() < 80) leds[random16(NUM_LEDS)] = CHSV(random8(), 200, 255);  // feu d'artifice
      drawScroll("BRAVO !", t % scrollDuration("BRAVO !"), 5, CHSV(now / 8, 255, 255));
      break;
  }
  FastLED.show();
}

void drawOled() {
  oled.clearBuffer();
  oled.setFont(u8g2_font_6x10_tf);
  oled.setCursor(0, 10);
  oled.printf("%u/%u %s", levelIdx + 1, NUM_LEVELS, lvl().name);
  oled.setFont(u8g2_font_logisoso24_tn);
  oled.setCursor(0, 42);
  oled.print(score);
  for (int i = 0; i < lives; i++) oled.drawDisc(123 - i * 9, 30, 3);  // vies restantes
  oled.setFont(u8g2_font_6x10_tf);
  oled.setCursor(0, 62);
  switch (state) {
    case TITLE:     oled.print("Pret ?"); break;
    case SERVE:     oled.print("Bouton 1 : lancer"); break;
    case PAUSE:     oled.print("PAUSE"); break;
    case CLEAR:     oled.print("Niveau reussi !"); break;
    case GAME_OVER: oled.print("Perdu - bouton 1"); break;
    case VICTORY:   oled.print("Bravo ! - bouton 1"); break;
    default: break;
  }
  oled.sendBuffer();
}

void handleSerial() {
  static String line;
  while (Serial.available()) {
    char c = Serial.read();
    if (c != '\n') { line += c; continue; }
    float v = line.substring(2).toFloat();
    switch (line[0]) {
      case 'g': gain = v; break;
      case 'f': friction = constrain(v, 0.80f, 0.999f); break;
      case 'm': vMax = v; break;
      case 'n': if (v >= 1 && v <= NUM_LEVELS) { lives = START_LIVES; loadLevel((uint8_t)v - 1); } break;
    }
    Serial.printf("gain=%.2f friction=%.3f vmax=%.0f niveau=%u\n", gain, friction, vMax, levelIdx + 1);
    line = "";
  }
}

void setup() {
  Serial.begin(115200);
  FastLED.addLeds<WS2812B, PIN_LED_DATA, GRB>(leds, NUM_LEDS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, MAX_MILLIAMPS);
  FastLED.setBrightness(40);

  pinMode(PIN_ENC_A, INPUT_PULLUP);
  pinMode(PIN_ENC_B, INPUT_PULLUP);
  pinMode(PIN_ENC_SW, INPUT_PULLUP);
  pinMode(PIN_BTN1, INPUT_PULLUP);
  pinMode(PIN_BTN2, INPUT);  // GPIO36 : pull-up externe uniquement
  attachInterrupt(PIN_ENC_A, onEnc, CHANGE);
  attachInterrupt(PIN_ENC_B, onEnc, CHANGE);

  oled.setBusClock(400000);  // I2C rapide : l'OLED bloque moins longtemps le jeu
  oled.begin();
  newGame();
  Serial.println("Pret. Commandes : g <gain>  f <frottement>  m <vmax>  n <niveau 1-6>");
}

void loop() {
  static uint32_t lastFrame = 0;
  uint32_t now = millis();
  handleSerial();

  // Les boutons sont lus à chaque tour pour ne rater aucun appui
  bool fire = btn1.pressed();
  fire |= btnEnc.pressed();
  bool pauseBtn = btn2.pressed();
  switch (state) {
    case TITLE:     if (fire) setState(SERVE); break;
    case SERVE:     if (fire) launch(); break;
    case PLAY:      if (pauseBtn) setState(PAUSE); break;
    case PAUSE:     if (fire || pauseBtn) resume(); break;
    case GAME_OVER:
    case VICTORY:   if (fire && now - stateSince > 1000) newGame(); break;
    default: break;
  }

  if (now - lastFrame < 16) return;           // ~60 images/s
  float dt = min((now - lastFrame) / 1000.0f, 0.05f);
  lastFrame = now;

  updatePaddle(dt, state == SERVE || state == PLAY);
  if (state == SERVE) { bx = padPos; by = PADDLE_Y - 0.5f; }  // la balle attend sur la raquette
  if (state == PLAY)  { updateBall(dt); if (state == PLAY) updateSpecial(now); }
  if (state == TITLE && now - stateSince > scrollDuration(lvl().name)) setState(SERVE);
  if (state == CLEAR && now - stateSince > 1500) {
    score += 100 * (levelIdx + 1);
    if (levelIdx + 1 < NUM_LEVELS) loadLevel(levelIdx + 1);
    else setState(VICTORY);
  }

  render(now);
  if (dirty) { drawOled(); dirty = false; }
}
