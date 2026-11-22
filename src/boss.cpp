// Combat final de l'Aventure contre le Roi Gluant.
// Le roi se balance en haut de la matrice et lance des bulles de mochi ; la joueuse (en bas) les esquive avec
// l'encodeur ou le joystick et lui renvoie des couleurs en appuyant sur le bouton (on peut le garder enfoncé).
// Chaque charme obtenu donne un pouvoir : Bol = bouclier qui revient, Feuille = tir triple, Note = bulles ralenties.
// Le roi a 3 phases : à chaque phase il va plus vite et lance plus de bulles (en zigzag à la dernière).
// Plus il perd de vie, plus les couleurs reviennent sur la matrice. Bouton du joystick : abandonner.
#include "hw.h"
#include "save.h"
#include "activities.h"

namespace boss {

// --- Réglages ---------------------------------------------------------------
constexpr int      KING_HP_EASY    = 60;      // vie du roi (facile ; ×1,25 en moyen, ×1,5 en difficile)
constexpr int      HEARTS          = 3;       // vies de la joueuse
constexpr int      KING_W          = 6;       // largeur du roi (cases)
constexpr float    PLAYER_SPEED    = 12.0f;   // cases/s au joystick
constexpr float    ENC_STEP        = 0.5f;    // cases par impulsion de l'encodeur
constexpr uint32_t SHOT_COOLDOWN   = 330;     // ms entre deux tirs
constexpr float    SHOT_SPEED      = 18.0f;   // cases/s
const float        KING_SPEED[3]   = {2.0f, 3.0f, 4.5f};   // cases/s, par phase
const float        BUBBLE_SPEED[3] = {4.5f, 5.5f, 6.5f};   // cases/s, par phase
const int          AIMED_CHANCE[3] = {20, 45, 60};         // % de bulles lancées vers la joueuse, par phase
const uint32_t     BUBBLE_EVERY[3] = {800, 600, 420};     // ms entre deux bulles, par phase
constexpr float    NOTE_SLOW       = 0.6f;    // pouvoir de la Note : vitesse des bulles × 0,6
constexpr uint32_t HURT_MS         = 1200;    // invincible un moment après une bulle reçue
constexpr uint32_t SHIELD_MS       = 9000;    // pouvoir du Bol : le bouclier revient après ce délai
// ---------------------------------------------------------------------------

const Note SND_SHOT[]   = {{1400, 15}, {0, 0}};
const Note SND_HIT[]    = {{330, 40}, {0, 0}};
const Note SND_HURT[]   = {{150, 250}, {0, 0}};
const Note SND_SHIELD[] = {{1047, 60}, {784, 120}, {0, 0}};
const Note SND_PHASE[]  = {{196, 150}, {185, 150}, {175, 400}, {0, 0}};
const Note SND_WIN[]    = {{523, 100}, {659, 100}, {784, 100}, {1047, 200}, {784, 100}, {1047, 500}, {0, 0}};
const Note SND_LOSE[]   = {{262, 250}, {247, 250}, {233, 250}, {220, 700}, {0, 0}};

struct Obj { float x, y, vx; bool on; };
Obj shots[9], bubbles[10];

enum State : uint8_t { FIGHT, WIN, LOSE };
State    state;
uint8_t  powers;  // charmes obtenus
float    px, kx, kdir;
int      hp, hearts, phaseIdx;
bool     shieldUp, dirty;
uint32_t shieldBack, hurtUntil, kingFlash, nextBubble, nextShot, endAt, messageUntil;
const char* message;
int32_t  encBase;

int KING_HP = KING_HP_EASY;
int phaseOf(int life) { return life > KING_HP * 2 / 3 ? 0 : life > KING_HP / 3 ? 1 : 2; }

void say(const char* m, uint32_t now) { message = m; messageUntil = now + 2000; dirty = true; }

void fire(uint32_t now) {
  int n = powers & CHARM_FEUILLE ? 3 : 1;
  nextShot = now + SHOT_COOLDOWN * (n == 3 ? 16 : 10) / 10;  // le tir triple recharge un peu plus lentement
  for (int k = 0; k < n; k++)
    for (Obj& s : shots)
      if (!s.on) { s = {px + 0.5f, 13.0f, n == 1 ? 0.0f : (k - 1) * 7.0f, true}; break; }
  play(SND_SHOT);
}

void spawnBubble() {
  for (Obj& b : bubbles)
    if (!b.on) {
      float x = kx + random(1, KING_W - 1) + 0.5f, vx = 0;
      if (random(100) < AIMED_CHANCE[phaseIdx])  // bulle visée : elle part vers la joueuse
        vx = constrain((px + 1 - x) / 11.0f * BUBBLE_SPEED[phaseIdx], -3.0f, 3.0f);
      else if (phaseIdx == 2) vx = random(2) ? 2.0f : -2.0f;  // dernière phase : en zigzag
      b = {x, 4.0f, vx, true};
      return;
    }
}

void hurt(uint32_t now) {
  if (shieldUp) {  // le bouclier du Bol encaisse la bulle
    shieldUp = false;
    shieldBack = now + SHIELD_MS;
    play(SND_SHIELD);
    say("Bouclier !", now);
    return;
  }
  hearts--;
  hurtUntil = now + HURT_MS;
  play(SND_HURT, true);
  dirty = true;
  if (hearts <= 0) { state = LOSE; endAt = now + 2500; play(SND_LOSE, true); say("Engluée...", now); }
}

void drawOled() {
  oled.clearBuffer();
  oled.setFont(u8g2_font_6x10_tf);
  const char* title = "Le Roi Gluant";
  oled.setCursor((128 - oled.getUTF8Width(title)) / 2, 11);  // bande jaune
  oled.print(title);
  oled.drawFrame(4, 19, 120, 8);  // vie du roi
  oled.drawBox(6, 21, 116 * max(hp, 0) / KING_HP, 4);
  oled.setCursor(0, 40);
  oled.print("Vies");
  for (int i = 0; i < hearts; i++) oled.drawDisc(36 + i * 10, 36, 3);
  if (powers & CHARM_BOL) { oled.setCursor(76, 40); oled.print(shieldUp ? "Bouclier" : "........"); }
  oled.setCursor(0, 58);
  if (message) oled.print(message);
  else {
    if (powers & CHARM_FEUILLE) oled.print("Tir triple ");
    if (powers & CHARM_NOTE) oled.print("Bulles lentes");
  }
  oled.sendBuffer();
}

void render(uint32_t now) {
  // Fond : gris au début, les couleurs reviennent à mesure que le roi perd de la vie
  uint8_t sat = 255 * (KING_HP - max(hp, 0)) / KING_HP;
  for (int y = 0; y < 16; y++)
    for (int x = 0; x < 16; x++) setPixel(x, y, CHSV(x * 16 + y * 4 + now / 30, sat, 22));
  // Le roi : couronne, corps de mochi, yeux (il clignote quand il est touché)
  int k = (int)(kx + 0.5f);
  bool flash = now < kingFlash;
  CRGB body = flash ? CRGB::White : phaseIdx == 2 ? CRGB(255, 90, 140) : CRGB(255, 170, 200);
  for (int x = 1; x < KING_W - 1; x += 2) setPixel(k + x, 0, CRGB(255, 200, 0));
  for (int y = 1; y <= 3; y++)
    for (int x = 0; x < KING_W; x++)
      if (!(y == 3 && (x == 0 || x == KING_W - 1))) setPixel(k + x, y, body);
  setPixel(k + 1, 2, CRGB::Black);
  setPixel(k + KING_W - 2, 2, CRGB::Black);
  // Bulles et tirs
  for (Obj& b : bubbles) if (b.on) setPixel((int)b.x, (int)b.y, CRGB(220, 220, 220));
  for (int i = 0; i < 9; i++) if (shots[i].on) setPixel((int)shots[i].x, (int)shots[i].y, CHSV(i * 28 + now / 4, 255, 255));
  // La joueuse (clignote quand elle est touchée), avec son bouclier
  if (now >= hurtUntil || (now / 100) % 2) {
    setPixel((int)px, 15, CRGB(255, 60, 140));
    setPixel((int)px + 1, 15, CRGB(255, 60, 140));
    setPixel((int)px, 14, CRGB(255, 200, 150));
    setPixel((int)px + 1, 14, CRGB(255, 200, 150));
  }
  if (shieldUp) for (int x = -1; x <= 2; x++) setPixel((int)px + x, 13, CRGB(0, 80, 160));
  // Fin du combat
  if (state == WIN) {
    for (int i = 0; i < 30; i++) leds[random16(NUM_LEDS)] = CHSV(random8(), 200, 255);
    drawScroll("BRAVO", (now - (endAt - 3000)) % scrollDuration("BRAVO"), 5, CRGB(255, 200, 0));
  }
  if (state == LOSE) fill_solid(leds, NUM_LEDS, CRGB(200, 200, 200));  // tout devient mochi
  FastLED.show();
  if (now < hurtUntil) setRgb(CRGB(255, 0, 0));
  else if (state == WIN) setRgb(CHSV(now / 3, 255, 255));
  else setRgb(shieldUp ? CRGB(0, 120, 255) : CRGB(255, 60, 140));
}

}  // namespace boss

using namespace boss;

void bossBegin(uint8_t charms) {
  powers = charms;
  state = FIGHT;
  px = 7; kx = 5; kdir = 1;
  KING_HP = (int)(KING_HP_EASY * byLevel(1.0f, 1.25f, 1.5f));
  hp = KING_HP; hearts = HEARTS + (charms & CHARM_PINCEAU ? 1 : 0); phaseIdx = 0;  // Pinceau : une vie de plus
  shieldUp = charms & CHARM_BOL;
  for (Obj& s : shots) s.on = false;
  for (Obj& b : bubbles) b.on = false;
  uint32_t now = millis();
  nextBubble = now + 1500; nextShot = now; hurtUntil = kingFlash = 0;
  encBase = encoderCount();
  say("Viens te battre !", now);
}

GameResult bossLoop(uint32_t now) {
  static uint32_t lastFrame = 0;
  btn1.pressed();
  btnEnc.pressed();
  bool fireHeld = btn1.held() || btnEnc.held();
  if (btn2.pressed()) return GAME_LOST;
  if (now - lastFrame < 16) return GAME_RUNNING;
  float dt = min((now - lastFrame) / 1000.0f, 0.05f);
  lastFrame = now;

  int32_t pulses = encoderCount() - encBase;
  encBase += pulses;
  if (message && now >= messageUntil) { message = nullptr; dirty = true; }

  if (state != FIGHT) {
    render(now);
    if (dirty) { drawOled(); dirty = false; }
    if (now >= endAt) return state == WIN ? GAME_WON : GAME_LOST;
    return GAME_RUNNING;
  }

  // Joueuse
  px = constrain(px + pulses * ENC_STEP * ENC_X_SIGN + joyX() * PLAYER_SPEED * dt, 0.0f, 14.0f);
  if (fireHeld && now >= nextShot) fire(now);
  if ((powers & CHARM_BOL) && !shieldUp && now >= shieldBack && shieldBack) { shieldUp = true; dirty = true; }

  // Roi
  kx += kdir * KING_SPEED[phaseIdx] * dt;
  if (kx < 0) { kx = 0; kdir = 1; }
  if (kx > 16 - KING_W) { kx = 16 - KING_W; kdir = -1; }
  if (now >= nextBubble) { spawnBubble(); nextBubble = now + BUBBLE_EVERY[phaseIdx] + random(0, 300); }

  // Tirs : touchent le roi
  for (Obj& s : shots) {
    if (!s.on) continue;
    s.y -= SHOT_SPEED * dt;
    s.x += s.vx * dt;
    int k = (int)(kx + 0.5f);
    if (s.y <= 3.5f && s.x >= k && s.x < k + KING_W) {
      s.on = false;
      static uint8_t hits = 0;
      hp -= (powers & CHARM_PINCEAU) && (++hits & 1) ? 2 : 1;  // Pinceau : un tir sur deux compte double
      kingFlash = now + 80;
      play(SND_HIT);
      dirty = true;
      if (hp <= 0) { state = WIN; endAt = now + 3000; play(SND_WIN, true); say("Le roi est vaincu !", now); break; }
      int p = phaseOf(hp);
      if (p != phaseIdx) { phaseIdx = p; play(SND_PHASE, true); say(p == 1 ? "Le roi gonfle !" : "Le roi est furieux !", now); }
    } else if (s.y < 0 || s.x < 0 || s.x >= 16) s.on = false;
  }

  // Bulles : tombent sur la joueuse
  float speed = BUBBLE_SPEED[phaseIdx] * (powers & CHARM_NOTE ? NOTE_SLOW : 1.0f) * byLevel(1.0f, 1.15f, 1.3f);
  for (Obj& b : bubbles) {
    if (!b.on) continue;
    b.y += speed * dt;
    b.x += b.vx * dt;
    if (b.x < 0 || b.x >= 16) { b.vx = -b.vx; b.x = constrain(b.x, 0.0f, 15.99f); }
    int bx = (int)b.x, p = (int)px;
    if (b.y >= 13.5f && b.y < 15.5f && (bx == p || bx == p + 1) && now >= hurtUntil && state == FIGHT) {
      b.on = false;
      hurt(now);
    } else if (b.y >= 16) b.on = false;
  }

  render(now);
  if (dirty) { drawOled(); dirty = false; }
  return GAME_RUNNING;
}
