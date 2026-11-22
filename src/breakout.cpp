// Casse-briques façon Arkanoid : 6 niveaux de difficulté croissante (voir include/levels.h).
// Encodeur ou joystick (gauche/droite) : raquette   Bouton 1, bouton de l'encodeur ou joystick vers le haut : lancer
// Bouton du joystick : pause, puis bouton de l'encodeur pour revenir au menu
#include "hw.h"
#include "levels.h"
#include "breakout.h"
#include "save.h"

// --- Réglages du ressenti (modifiables en direct par le moniteur série) -----
float direct   = 0.25f;  // cases parcourues tout de suite par impulsion (4 impulsions = 1 cran = 1 case)
float gain     = 0.1f;   // élan : vitesse donnée par un tour rapide (cases/s par impulsion/s), comme un volant
float friction = 0.985f; // multiplicateur de vitesse à chaque image : proche de 1 = l'élan dure longtemps
float vMax     = 30.0f;  // cases/s
// ---------------------------------------------------------------------------

namespace breakout {

constexpr float JOY_SPEED   = 16.0f;  // cases/s de la raquette, joystick poussé à fond
constexpr float FLING_SPEED = 20.0f;  // impulsions/s : en dessous, l'encodeur déplace sans donner d'élan

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

// 6 niveaux, plus MOCHI-MAGE quand la fin légendaire a été trouvée (mode secret)
uint8_t levelCount() { return progress.endings & END_LEGENDAIRE ? NUM_LEVELS + 1 : NUM_LEVELS; }

const Note SND_LAUNCH[]   = {{660, 40}, {990, 40}, {0, 0}};
const Note SND_PADDLE[]   = {{440, 20}, {0, 0}};
const Note SND_HIT[]      = {{880, 20}, {0, 0}};
const Note SND_BREAK[]    = {{1320, 30}, {0, 0}};
const Note SND_METAL[]    = {{220, 30}, {0, 0}};
const Note SND_LOSE[]     = {{392, 120}, {330, 120}, {262, 250}, {0, 0}};
const Note SND_CLEAR[]    = {{523, 90}, {659, 90}, {784, 90}, {1047, 250}, {0, 0}};
const Note SND_GAMEOVER[] = {{392, 200}, {370, 200}, {349, 200}, {330, 500}, {0, 0}};
const Note SND_VICTORY[]  = {{523, 120}, {523, 120}, {784, 120}, {1047, 300}, {0, 80}, {784, 120}, {1047, 400}, {0, 0}};

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

bool story = false;  // défi du tanuki dans l'Aventure : un seul niveau, puis retour à l'histoire

void newGame(uint8_t level) {
  score = 0;
  lives = START_LIVES;
  loadLevel(level);
}

// Envoie la balle vers le haut avec un angle par rapport à la verticale (négatif = vers la gauche)
void sendUp(float a) {
  if (fabsf(a) < MIN_BOUNCE_ANGLE) a = (a < 0 || (a == 0 && random(2))) ? -MIN_BOUNCE_ANGLE : MIN_BOUNCE_ANGLE;
  vx = speed * sinf(a);
  vy = -speed * cosf(a);
}

void launch() {
  speed = lvl().speed * byLevel(1.0f, 1.15f, 1.3f) + bricksBroken * SPEED_PER_BRICK;
  sendUp(random(-40, 41) / 100.0f);  // petit angle aléatoire au départ
  lastMove = lastFall = millis();
  play(SND_LAUNCH);
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
  play(lives ? SND_LOSE : SND_GAMEOVER, true);
  setState(lives ? SERVE : GAME_OVER);
}

// La balle garde sa direction mais prend la vitesse du moment
void updateSpeed() {
  speed = lvl().speed * byLevel(1.0f, 1.15f, 1.3f) + bricksBroken * SPEED_PER_BRICK;
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
  if (grid[r][c] == METAL) { nudge(); play(SND_METAL); return; }
  score += levelPoints(10);
  dirty = true;
  if (--grid[r][c] == 0) {
    bricksLeft--;
    bricksBroken++;
    updateSpeed();
    play(SND_BREAK);
    if (!bricksLeft) { play(SND_CLEAR, true); setState(CLEAR); }
  } else {
    play(SND_HIT);
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
  static float   encSpeed = 0;  // vitesse de rotation lissée (impulsions/s)
  int32_t c = encoderCount();
  if (!active) { lastEnc = c; padVel = 0; encSpeed = 0; return; }  // on ignore l'encodeur hors du jeu
  int32_t pulses = (c - lastEnc) * ENC_X_SIGN;
  lastEnc = c;
  padPos += pulses * direct;  // la raquette suit l'encodeur tout de suite, sans temps mort
  encSpeed = encSpeed * 0.7f + pulses / dt * 0.3f;
  if (pulses && fabsf(encSpeed) > FLING_SPEED) padVel = encSpeed * gain;  // tour rapide : la raquette garde de l'élan
  float jx = joyX();
  if (jx) padVel = jx * JOY_SPEED;  // le joystick commande directement la vitesse
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
        play(SND_PADDLE);
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

void renderRgb(uint32_t now) {
  CRGB c;
  switch (state) {
    case SERVE:
    case PLAY:      c = now < hurtUntil ? CRGB(CRGB::Red) : CRGB(CHSV(lvl().hue[0], 255, 255)); break;
    case TITLE:
    case PAUSE:     c = CHSV(lvl().hue[0], 255, sin8(now / 4)); break;  // respire doucement
    case CLEAR:     c = (now / 150) % 2 ? CRGB(CRGB::Green) : CRGB(CRGB::Black); break;
    case GAME_OVER: c = CRGB::Red; break;
    case VICTORY:   c = CHSV(now / 8, 255, 255); break;
  }
  setRgb(c);
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
  renderRgb(now);
}

void drawOled() {
  oled.clearBuffer();
  oled.setFont(u8g2_font_6x10_tf);
  oled.setCursor(0, 10);
  if (story) oled.print("Défi du tanuki");
  else oled.printf("%u/%u %s", levelIdx + 1, levelCount(), lvl().name);
  oled.setFont(u8g2_font_logisoso24_tn);
  oled.setCursor(0, 42);
  oled.print(score);
  for (int i = 0; i < lives; i++) oled.drawDisc(123 - i * 9, 30, 3);  // vies restantes
  oled.setFont(u8g2_font_6x10_tf);
  oled.setCursor(0, 62);
  switch (state) {
    case TITLE:     oled.print("Pret ?"); break;
    case SERVE:     oled.print("Bouton 1 : lancer"); break;
    case PAUSE:     oled.print("PAUSE - axe : menu"); break;
    case CLEAR:     oled.print("Niveau reussi !"); break;
    case GAME_OVER: oled.print(story ? "Perdu ! Bouton : suite" : "Perdu ! Bouton : menu"); break;
    case VICTORY:   oled.print(story ? "Gagné ! Bouton : suite" : "Bravo ! Bouton : menu"); break;
    default: break;
  }
  oled.sendBuffer();
}

}  // namespace breakout

using namespace breakout;

void breakoutBegin(uint8_t level) {
  story = false;
  newGame(level);
}

void breakoutBeginStory() {
  story = true;
  newGame(STORY_LEVEL);
}

bool breakoutWon() { return state == VICTORY; }

bool breakoutLoop(uint32_t now) {
  static uint32_t lastFrame = 0;

  // Les boutons sont lus à chaque tour pour ne rater aucun appui
  bool encBtn = btnEnc.pressed();
  bool fire = btn1.pressed();
  fire |= encBtn;
  fire |= joyUpPressed();
  bool pauseBtn = btn2.pressed();
  switch (state) {
    case TITLE:     if (fire) setState(SERVE); break;
    case SERVE:     if (fire) launch(); break;
    case PLAY:      if (pauseBtn) setState(PAUSE); break;
    case PAUSE:
      if (encBtn) return false;  // bouton de l'encodeur : retour au menu
      if (fire || pauseBtn) resume();
      break;
    case GAME_OVER:
    case VICTORY:   if (fire && now - stateSince > 1000) return false; break;
    default: break;
  }

  if (now - lastFrame < 16) return true;      // ~60 images/s
  float dt = min((now - lastFrame) / 1000.0f, 0.05f);
  lastFrame = now;

  updatePaddle(dt, state == SERVE || state == PLAY);
  if (state == SERVE) { bx = padPos; by = PADDLE_Y - 0.5f; }  // la balle attend sur la raquette
  if (state == PLAY)  { updateBall(dt); if (state == PLAY) updateSpecial(now); }
  if (state == TITLE && now - stateSince > scrollDuration(lvl().name)) setState(SERVE);
  if (state == CLEAR && now - stateSince > 1500) {
    score += levelPoints(100 * (levelIdx + 1));
    if (!story && levelIdx + 1 < levelCount()) loadLevel(levelIdx + 1);
    else { play(SND_VICTORY, true); setState(VICTORY); }
  }

  render(now);
  if (dirty) { drawOled(); dirty = false; }
  return true;
}