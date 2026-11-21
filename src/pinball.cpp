// Flipper manga. Bouton : flipper gauche. Joystick (n'importe quelle action) : flipper droit.
// Tourner l'encodeur secoue la table (nudge) : plus on tourne vite, plus la secousse et les effets manga sont
// forts (lignes de vitesse, tremblement, éclair). Trop secouer = TILT. Bouton de l'encodeur : pause / quitter.
#include "hw.h"
#include "save.h"
#include "activities.h"

namespace pinball {

// --- Réglages ---------------------------------------------------------------
constexpr float    GRAVITY      = 14.0f;   // cases/s²
constexpr float    MAX_SPEED    = 22.0f;   // cases/s
constexpr float    BALL_R       = 0.55f;   // rayon de la bille (cases)
constexpr float    FLIPPER_LEN  = 3.0f;   // laisse un passage d'1,6 case entre les flippers baissés
constexpr float    FLIP_DOWN    = 0.45f;   // angle au repos (radians, vers le bas)
constexpr float    FLIP_UP      = -0.5f;   // angle levé
constexpr float    FLIP_SPEED   = 16.0f;   // radians/s
constexpr float    NUDGE_POWER  = 7.0f;    // vitesse donnée à la bille par une secousse maximale
constexpr float    NUDGE_FULL   = 120.0f;  // impulsions d'encodeur par seconde pour une secousse maximale
constexpr float    TILT_LIMIT   = 2.6f;    // au-delà : TILT, les flippers ne répondent plus jusqu'à la bille perdue
constexpr int      BALLS        = 3;
constexpr int      STORY_RECORD = 3000;    // record du vieux flipper à battre dans l'histoire
// ---------------------------------------------------------------------------

const Note SND_BUMP[]   = {{880, 30}, {1320, 30}, {0, 0}};
const Note SND_FLIP[]   = {{120, 15}, {0, 0}};
const Note SND_TARGET[] = {{1568, 50}, {0, 0}};
const Note SND_MOCHI[]  = {{523, 70}, {659, 70}, {784, 70}, {1047, 70}, {1319, 250}, {0, 0}};
const Note SND_DRAIN[]  = {{400, 120}, {300, 120}, {200, 300}, {0, 0}};
const Note SND_TILT[]   = {{150, 200}, {0, 60}, {150, 400}, {0, 0}};
const Note SND_WIN[]    = {{523, 100}, {659, 100}, {784, 100}, {1047, 400}, {0, 0}};

struct Flipper { float px, py, angle; int side; bool up; float omega; };  // side -1 = gauche, 1 = droite
struct Bumper  { float x, y; uint8_t hue; uint32_t litUntil; };

Flipper  flippers[2];
Bumper   bumpers[3];
bool     targets[5];               // les lettres M O C H I en haut de la table
float    bx, by, vx, vy;
int      balls, score, best;
bool     story, bonus, paused, tilted, gameOver, ballWaiting;
float    tiltMeter, fx;            // fx = force des effets manga (0 à 1), retombe toute seule
uint32_t fxUntil, flashUntil, waitUntil, endUntil;
int32_t  encBase;
const char* message;
bool     dirty;

const int TARGET_X[5] = {2, 5, 8, 11, 14};
const char* const LETTERS = "MOCHI";

void resetBall(uint32_t now) {
  bx = 8; by = 2.5f; vx = random(-30, 31) / 10.0f; vy = 0;
  tilted = false; tiltMeter = 0;
  ballWaiting = true; waitUntil = now + 900;
}

// Rebond sur un segment (mur incliné ou flipper). kick = vitesse du segment à cet endroit (flipper qui se lève)
bool collideSegment(float ax, float ay, float ex, float ey, float bounce, float kick) {
  float sx = ex - ax, sy = ey - ay;
  float t = constrain(((bx - ax) * sx + (by - ay) * sy) / (sx * sx + sy * sy), 0.0f, 1.0f);
  float cx = ax + sx * t, cy = ay + sy * t;
  float nx = bx - cx, ny = by - cy, d = sqrtf(nx * nx + ny * ny);
  if (d >= BALL_R || d < 0.0001f) return false;
  nx /= d; ny /= d;
  float vn = vx * nx + vy * ny;
  if (vn < 0) { vx -= (1 + bounce) * vn * nx; vy -= (1 + bounce) * vn * ny; }
  vx += nx * kick * t; vy += ny * kick * t;   // le bout du flipper frappe plus fort que la base
  bx = cx + nx * BALL_R; by = cy + ny * BALL_R;
  return true;
}

void hitBumpers(uint32_t now) {
  for (Bumper& b : bumpers) {
    float nx = bx - b.x, ny = by - b.y, d = sqrtf(nx * nx + ny * ny);
    if (d >= 1.0f + BALL_R || d < 0.0001f) continue;
    nx /= d; ny /= d;
    float speed = max(12.0f, sqrtf(vx * vx + vy * vy));
    vx = nx * speed; vy = ny * speed;
    bx = b.x + nx * (1.0f + BALL_R); by = b.y + ny * (1.0f + BALL_R);
    score += 100 * (bonus ? 2 : 1);
    b.litUntil = now + 150;
    play(SND_BUMP);
    dirty = true;
  }
}

void hitTargets(uint32_t now) {
  if (by > 1.6f || vy > 0) return;
  for (int i = 0; i < 5; i++)
    if (!targets[i] && fabsf(bx - TARGET_X[i] - 0.5f) < 1.0f) {
      targets[i] = true;
      score += 250;
      play(SND_TARGET);
      dirty = true;
      bool all = true;
      for (bool t : targets) all &= t;
      if (all) {  // MOCHI complet : gros bonus et effet manga maximal
        score += 2000;
        for (bool& t : targets) t = false;
        fx = 1; fxUntil = now + 900; flashUntil = now + 300;
        message = "MOCHI !!";
        play(SND_MOCHI, true);
      }
    }
}

void physics(float dt, uint32_t now) {
  const int STEPS = 6;
  float h = dt / STEPS;
  for (int s = 0; s < STEPS; s++) {
    vy += GRAVITY * h;
    float sp = sqrtf(vx * vx + vy * vy);
    if (sp > MAX_SPEED) { vx *= MAX_SPEED / sp; vy *= MAX_SPEED / sp; }
    bx += vx * h; by += vy * h;
    if (bx < BALL_R)      { bx = BALL_R; vx = fabsf(vx) * 0.8f; }
    if (bx > 16 - BALL_R) { bx = 16 - BALL_R; vx = -fabsf(vx) * 0.8f; }
    if (by < BALL_R)      { by = BALL_R; vy = fabsf(vy) * 0.8f; }
    // Murs inclinés qui guident vers les flippers
    collideSegment(0, 10, 4.5f, 13.6f, 0.5f, 0);
    collideSegment(16, 10, 11.5f, 13.6f, 0.5f, 0);
    for (Flipper& f : flippers) {
      float ex = f.px - f.side * cosf(f.angle) * FLIPPER_LEN, ey = f.py + sinf(f.angle) * FLIPPER_LEN;
      float kick = f.omega < 0 ? -f.omega * FLIPPER_LEN : 0;  // le flipper se lève : il frappe la bille
      if (collideSegment(f.px, f.py, ex, ey, 0.3f, kick) && kick > 0) score += 10;
    }
    hitBumpers(now);
    hitTargets(now);
  }
}

void updateFlipper(Flipper& f, bool pressed, float dt) {
  float target = pressed && !tilted ? FLIP_UP : FLIP_DOWN;
  if (pressed && !f.up && !tilted) play(SND_FLIP);
  f.up = pressed && !tilted;
  float before = f.angle;
  f.angle += constrain(target - f.angle, -FLIP_SPEED * dt, FLIP_SPEED * dt);
  f.omega = (f.angle - before) / dt;  // négatif quand le flipper monte
}

void nudge(int32_t pulses, float dt, uint32_t now) {
  if (abs(pulses) < 2) return;
  float strength = min(1.0f, abs(pulses) / dt / NUDGE_FULL);
  vx += (pulses > 0 ? 1 : -1) * ENC_X_SIGN * strength * NUDGE_POWER;
  vy -= strength * NUDGE_POWER * 0.6f;
  tiltMeter += strength;
  fx = max(fx, strength);
  fxUntil = now + 250 + (uint32_t)(strength * 400);
  beep(200 + strength * 1200, 40);
  if (tiltMeter > TILT_LIMIT && !tilted) {
    tilted = true;
    message = "TILT !";
    play(SND_TILT, true);
    dirty = true;
  }
}

void drawOled() {
  oled.clearBuffer();
  oled.setFont(u8g2_font_6x10_tf);
  oled.setCursor(0, 10);
  oled.print(story ? "Le vieux flipper" : bonus ? "Flipper des étoiles" : "Flipper manga");
  oled.setFont(u8g2_font_logisoso24_tn);
  oled.setCursor(0, 40);
  oled.print(score);
  oled.setFont(u8g2_font_6x10_tf);
  for (int i = 0; i < balls; i++) oled.drawDisc(122 - i * 9, 30, 3);
  oled.setCursor(0, 62);
  if (paused) oled.print("PAUSE - axe : menu");
  else if (message) oled.print(message);
  else if (story) oled.printf("Record : %d", STORY_RECORD);
  else oled.printf("Meilleur : %d", best);
  oled.sendBuffer();
}

void render(uint32_t now) {
  // Tremblement manga : toute l'image se décale d'un pixel quand la secousse est forte
  int ox = 0, oy = 0;
  if (now < fxUntil && fx > 0.5f) { ox = random8(3) - 1; oy = random8(3) - 1; }
  auto px = [&](int x, int y, CRGB c) { setPixel(x + ox, y + oy, c); };

  FastLED.clear();
  for (int y = 0; y < 16; y++) { px(0, y, CRGB(40, 40, 60)); px(15, y, CRGB(40, 40, 60)); }
  for (int i = 0; i < 5; i++) px(TARGET_X[i], 0, targets[i] ? CRGB(255, 200, 0) : CRGB(60, 30, 0));
  for (Bumper& b : bumpers) {
    CRGB c = now < b.litUntil ? CRGB::White : CRGB(CHSV(b.hue, 255, bonus ? 255 : 200));
    px((int)b.x, (int)b.y, c); px((int)b.x - 1, (int)b.y, c); px((int)b.x, (int)b.y - 1, c); px((int)b.x - 1, (int)b.y - 1, c);
  }
  for (int i = 0; i <= 4; i++) {  // murs inclinés
    px((int)(i * 4.5f / 4), (int)(10 + i * 3.6f / 4), CRGB(40, 40, 60));
    px(15 - (int)(i * 4.5f / 4), (int)(10 + i * 3.6f / 4), CRGB(40, 40, 60));
  }
  for (Flipper& f : flippers)
    for (float r = 0; r <= FLIPPER_LEN; r += 0.5f) {
      float x = f.px - f.side * cosf(f.angle) * r;
      px((int)x, (int)(f.py + sinf(f.angle) * r), tilted ? CRGB(80, 0, 0) : CRGB(255, 60, 140));
    }
  // Effets manga proportionnels à la force de la secousse : lignes de vitesse en diagonale
  if (now < fxUntil) {
    int n = (int)(fx * 14);
    for (int i = 0; i < n; i++) {
      int x0 = random8(16), y0 = random8(16), len = 2 + random8(3);
      for (int k = 0; k < len; k++) px(x0 + k, y0 - k, CRGB(160, 160, 200));
    }
  } else {
    fx *= 0.9f;
  }
  if (now < flashUntil) fill_solid(leds, NUM_LEDS, CRGB(150, 150, 150));
  if (!ballWaiting || (now / 120) % 2) px((int)bx, (int)by, CRGB::White);
  if (gameOver) drawScroll("FIN", now % scrollDuration("FIN"), 5, CRGB(255, 200, 0));
  FastLED.show();

  if (tilted) setRgb(CRGB::Red);
  else if (now < fxUntil) setRgb(CHSV(now / 2, 255, 128 + fx * 127));  // éclair coloré
  else setRgb(CRGB(255, 60, 140).nscale8(80));
}

}  // namespace pinball

using namespace pinball;

void pinballBegin(bool storyMode) {
  story = storyMode;
  bonus = !story && (progress.endings & END_LEGENDAIRE);  // mode secret : bumpers étoilés à points doublés
  flippers[0] = {4.5f, 13.6f, FLIP_DOWN, -1, false, 0};
  flippers[1] = {11.5f, 13.6f, FLIP_DOWN, 1, false, 0};
  bumpers[0] = {5, 4, 0, 0};
  bumpers[1] = {11, 4, 96, 0};
  bumpers[2] = {8, 7, 160, 0};
  for (bool& t : targets) t = false;
  balls = BALLS; score = 0; fx = 0; fxUntil = flashUntil = 0;
  paused = gameOver = false; message = nullptr;
  best = progress.pinballBest;
  encBase = encoderCount();
  resetBall(millis());
  dirty = true;
}

GameResult pinballLoop(uint32_t now) {
  static uint32_t lastFrame = 0;
  if (btnEnc.pressed()) {  // pause ; en pause, le bouton de l'encodeur quitte
    if (paused) return story ? GAME_LOST : GAME_QUIT;
    paused = true; dirty = true;
  }
  if (paused && btn1.pressed()) { paused = false; dirty = true; lastFrame = now; }
  btn2.pressed();  // le bouton du joystick sert de flipper droit (lu par held())

  if (now - lastFrame < 16) return GAME_RUNNING;
  float dt = min((now - lastFrame) / 1000.0f, 0.04f);
  lastFrame = now;
  int32_t pulses = encoderCount() - encBase;
  encBase += pulses;

  if (gameOver) {
    if (now >= endUntil) {
      if (!story && score > progress.pinballBest) { progress.pinballBest = min(score, 65535); progressSave(); }
      if (story) return score >= STORY_RECORD ? GAME_WON : GAME_LOST;
      return GAME_QUIT;
    }
  } else if (!paused) {
    updateFlipper(flippers[0], btn1.held(), dt);
    updateFlipper(flippers[1], joyActive(), dt);
    if (ballWaiting) {
      if (now >= waitUntil) { ballWaiting = false; message = nullptr; dirty = true; }
    } else {
      nudge(pulses, dt, now);
      tiltMeter = max(0.0f, tiltMeter - 0.8f * dt);
      physics(dt, now);
      if (by > 16.5f) {  // bille perdue entre les flippers
        balls--;
        play(SND_DRAIN, true);
        if (balls <= 0 || (story && score >= STORY_RECORD)) {
          gameOver = true;
          endUntil = now + 2500;
          bool won = !story || score >= STORY_RECORD;
          message = story ? (won ? "Record battu !" : "Raté...") : "Partie finie";
          if (won) play(SND_WIN, true);
        } else {
          message = "Bille suivante";
          resetBall(now);
        }
        dirty = true;
      }
      if (story && score >= STORY_RECORD && !gameOver) {  // record battu : on s'arrête là
        gameOver = true; endUntil = now + 2500; message = "Record battu !"; play(SND_WIN, true); dirty = true;
      }
    }
  }

  render(now);
  static int lastScore = -1;
  if (score != lastScore) { lastScore = score; dirty = true; }
  if (dirty) { drawOled(); dirty = false; }
  return GAME_RUNNING;
}
