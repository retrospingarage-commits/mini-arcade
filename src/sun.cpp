// « 1, 2, 3, soleil ! » (Aventure, scène 10) : le garde mochi tourne le dos, la joueuse avance au joystick vers la
// porte ; quand il se retourne, il faut être immobile (joystick lâché).
// Le garde compte « un, deux, trois » (points et bips), puis un signal d'alerte (la tête clignote, la LED passe à
// l'orange), puis il se retourne (LED rouge). Après son demi-tour, un court délai de grâce permet de s'arrêter.
// Pris en train de bouger : on est renvoyée en arrière. Le temps est limité. Bouton du joystick : abandonner.
// La difficulté raccourcit le comptage, l'alerte et le délai de grâce, et renvoie plus loin en arrière.
#include "hw.h"
#include "save.h"
#include "activities.h"

namespace sun {

// --- Réglages ---------------------------------------------------------------
constexpr float    SPEED      = 3.5f;   // cases/s, joystick à fond
constexpr float    START_Y    = 14.0f;  // ligne de départ (en bas)
constexpr float    GOAL_Y     = 5.0f;   // ligne d'arrivée (la porte est derrière le garde)
constexpr float    MOVE_LEVEL = 0.35f;  // le joystick poussé plus que ça = on bouge
inline uint32_t beatMs()    { return (uint32_t)byLevel(520, 330, 230); }   // temps entre deux « un, deux, trois »
inline int      beatsMin()  { return (int)byLevel(3, 2, 1); }              // nombre de temps comptés avant l'alerte
inline int      beatsMax()  { return 3; }
inline uint32_t warnMs()    { return (uint32_t)byLevel(700, 150, 135); }   // alerte avant le demi-tour
inline uint32_t graceMs()   { return (uint32_t)byLevel(400, 80, 70); }     // délai de grâce après le demi-tour
inline uint32_t lookMinMs() { return (uint32_t)byLevel(900, 1000, 1100); } // temps où il regarde
inline float    pushBack()  { return byLevel(3.0f, 8.0f, 99.0f); }         // recul quand on est prise (99 = tout recommencer)
inline int      lives0()    { return (int)byLevel(99, 3, 3); }                // fois où on peut être prise (99 = sans limite)
inline uint32_t timeLimit() { return (uint32_t)byLevel(90, 50, 40) * 1000UL; }
// ---------------------------------------------------------------------------

enum State : uint8_t { INTRO, BACK, WARN, FRONT, CAUGHT, WIN, LOSE };
State    state;
float    px, py;
int      beats, beatsDone, lives;
uint32_t stateAt, nextBeat, startAt, lookFor, frozenUntil, endAt;
bool     dirty;
const char* message;

const Note SND_WARN[]   = {{1760, 50}, {0, 30}, {1760, 50}, {0, 0}};
const Note SND_SUN[]    = {{1047, 60}, {1568, 160}, {0, 0}};
const Note SND_CAUGHT[] = {{200, 120}, {140, 300}, {0, 0}};
const Note SND_WIN[]    = {{523, 100}, {659, 100}, {784, 100}, {1047, 300}, {784, 100}, {1047, 400}, {0, 0}};
const Note SND_LOSE[]   = {{392, 200}, {330, 200}, {262, 500}, {0, 0}};

void startBack(uint32_t now) {
  state = BACK; stateAt = now;
  beats = (int)random(beatsMin(), beatsMax() + 1);
  beatsDone = 0;
  nextBeat = now + beatMs();
  message = "Avance !";
  dirty = true;
}

void drawOled(uint32_t now) {
  oled.clearBuffer();
  oled.setFont(u8g2_font_6x10_tf);
  oled.setCursor(0, 11);  // bande jaune
  oled.print("1, 2, 3, soleil !");
  char buf[24];
  long left = max(0L, (long)(timeLimit() - (now - startAt)) / 1000);
  snprintf(buf, sizeof(buf), "Temps : %ld s", state == INTRO ? (long)(timeLimit() / 1000) : left);
  oled.setCursor(0, 26); oled.print(buf);
  int done = (int)(100 * (START_Y - py) / (START_Y - GOAL_Y));
  oled.drawFrame(0, 33, 128, 8);
  oled.drawBox(2, 35, 124 * constrain(done, 0, 100) / 100, 4);
  oled.setCursor(0, 53);
  oled.print(state == INTRO ? "Joystick : avance" : message ? message : "");
  oled.setCursor(0, 63);
  if (state == INTRO) oled.print("Fige-toi au soleil !");
  else if (lives < 10) { snprintf(buf, sizeof(buf), "Chances : %d", lives); oled.print(buf); }
  oled.sendBuffer();
}

void render(uint32_t now) {
  FastLED.clear();
  for (int y = 0; y < 16; y++)
    for (int x = 0; x < 16; x++) setPixel(x, y, ((x + y) & 1) ? CRGB(8, 8, 16) : CRGB(14, 14, 26));
  // Le garde : dos blanc, tête qui clignote pendant l'alerte, visage rouge quand il regarde
  CRGB body = CRGB(220, 220, 225);
  bool warn = state == WARN && (now / 60) % 2;
  for (int y = 0; y < 4; y++)
    for (int x = 5; x <= 10; x++) setPixel(x, y, warn && y == 0 ? CRGB(255, 200, 0) : body);
  if (state == WARN) { setPixel(5, 1, CRGB(255, 200, 0)); setPixel(5, 2, CRGB(255, 200, 0)); }  // un oeil commence à se montrer
  if (state == FRONT || state == CAUGHT) {
    CRGB eye = state == CAUGHT ? CRGB(255, 255, 0) : CRGB(255, 0, 0);
    setPixel(6, 1, eye); setPixel(9, 1, eye);
    for (int x = 6; x <= 9; x++) setPixel(x, 3, CRGB(80, 0, 0));  // bouche
  }
  // Compteur « un, deux, trois »
  for (int i = 0; i < 3; i++) {
    bool lit = state == BACK && i < beatsDone;
    setPixel(6 + i * 2, 4, state == FRONT ? CRGB(255, 0, 0) : lit ? CRGB::White : CRGB(30, 30, 30));
  }
  // Ligne d'arrivée
  for (int x = 0; x < 16; x += 2) setPixel(x, (int)GOAL_Y, CRGB(255, 200, 0).nscale8(140));
  // La joueuse (clignote quand elle est prise)
  if (state != CAUGHT || (now / 80) % 2) {
    setPixel((int)roundf(px), (int)roundf(py), CRGB(255, 60, 140));
    setPixel((int)roundf(px), (int)roundf(py) + 1, CRGB(255, 200, 150));
  }
  if (lives < 10) for (int i = 0; i < lives; i++) setPixel(1 + i * 2, 15, CRGB(255, 40, 80));  // chances restantes
  if (state == WIN) for (int i = 0; i < 25; i++) leds[random16(NUM_LEDS)] = CHSV(random8(), 200, 255);
  if (state == LOSE) fadeToBlackBy(leds, NUM_LEDS, 150);
  FastLED.show();
  if (state == BACK || state == INTRO) setRgb(CRGB(0, 160, 0));
  else if (state == WARN) setRgb((now / 80) % 2 ? CRGB(255, 120, 0) : CRGB(40, 20, 0));
  else if (state == FRONT) setRgb(CRGB(255, 0, 0));
  else if (state == CAUGHT || state == LOSE) setRgb((now / 100) % 2 ? CRGB(255, 0, 0) : CRGB(60, 0, 0));
  else setRgb(CHSV(now / 3, 255, 255));
}

}  // namespace sun

using namespace sun;

void sunBegin() {
  px = 8; py = START_Y;
  lives = lives0();
  state = INTRO;
  uint32_t now = millis();
  stateAt = now; startAt = now;
  message = nullptr;
  dirty = true;
}

GameResult sunLoop(uint32_t now) {
  static uint32_t lastFrame = 0;
  btn1.pressed();
  btnEnc.pressed();
  if (btn2.pressed()) return GAME_QUIT;
  if (now - lastFrame < 16) return GAME_RUNNING;
  float dt = min((now - lastFrame) / 1000.0f, 0.05f);
  lastFrame = now;

  float jx = joyX(), jy = joyY();
  bool moving = fmaxf(fabsf(jx), fabsf(jy)) > MOVE_LEVEL;
  bool canMove = state == BACK || state == WARN || state == FRONT;
  if (canMove) {
    px = constrain(px + jx * SPEED * dt, 0.0f, 15.0f);
    py = constrain(py + jy * SPEED * dt, GOAL_Y, START_Y);  // jy négatif = vers le haut
  }

  switch (state) {
    case INTRO:
      if (now - stateAt > 2500) { startAt = now; startBack(now); }
      break;
    case BACK:
      if (beatsDone < beats && now >= nextBeat) {  // « un », « deux », « trois »
        beatsDone++;
        static const Note TICK[] = {{900, 40}, {0, 0}};
        play(TICK);
        nextBeat = now + beatMs();
        dirty = true;
      } else if (beatsDone >= beats && now >= nextBeat) {  // alerte
        state = WARN; stateAt = now; message = "Attention...";
        play(SND_WARN, true);
        dirty = true;
      }
      break;
    case WARN:
      if (now - stateAt >= warnMs()) {
        state = FRONT; stateAt = now;
        lookFor = lookMinMs() + random(0, 700);
        message = "SOLEIL ! Fige-toi !";
        play(SND_SUN, true);
        dirty = true;
      }
      break;
    case FRONT:
      if (moving && now - stateAt > graceMs()) {  // pris en train de bouger
        state = CAUGHT; stateAt = now; frozenUntil = now + 1000;
        py = min(START_Y, py + pushBack());
        message = "Vu ! Tu recules";
        play(SND_CAUGHT, true);
        if (--lives <= 0) { state = LOSE; endAt = now + 2200; message = "Plus de chances !"; play(SND_LOSE, true); }
        dirty = true;
      } else if (now - stateAt >= lookFor) startBack(now);
      break;
    case CAUGHT:
      if (now >= frozenUntil) startBack(now);
      break;
    case WIN:
      if (now >= endAt) return GAME_WON;
      break;
    case LOSE:
      if (now >= endAt) return GAME_LOST;
      break;
  }
  // Arrivée et temps limité
  if (canMove && py <= GOAL_Y + 0.05f) {
    state = WIN; endAt = now + 2500; message = "Tu passes !";
    play(SND_WIN, true);
    dirty = true;
  } else if ((state == BACK || state == WARN || state == FRONT || state == CAUGHT) && now - startAt > timeLimit()) {
    state = LOSE; endAt = now + 2200; message = "Trop lent !";
    play(SND_LOSE, true);
    dirty = true;
  }
  static uint32_t lastSecond = 0;
  if (now / 1000 != lastSecond) { lastSecond = now / 1000; dirty = true; }
  render(now);
  if (dirty) { drawOled(now); dirty = false; }
  return GAME_RUNNING;
}
