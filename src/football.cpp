// Football manga : tirs au but face à un gardien et un défenseur qui accélèrent à chaque niveau.
// Encodeur ou joystick : se déplacer. Maintenir le bouton : charger le tir (la LED passe du jaune au rouge),
// relâcher : tirer. Pendant le tir, tourner l'encodeur courbe le ballon. Bouton du joystick : quitter.
#include "hw.h"
#include "save.h"
#include "activities.h"

namespace football {

// --- Réglages ---------------------------------------------------------------
constexpr int      SHOTS_PER_ROUND = 5;
constexpr int      GOALS_TO_WIN    = 3;
constexpr float    PLAYER_SPEED    = 10.0f;  // cases/s au joystick
constexpr float    SHOT_SPEED_MIN  = 10.0f;  // cases/s, tir sans charge
constexpr float    SHOT_SPEED_MAX  = 24.0f;  // cases/s, tir chargé à fond
constexpr uint32_t CHARGE_MS       = 1000;   // temps pour charger à fond
constexpr float    CURVE_PER_PULSE = 0.6f;   // courbe ajoutée par impulsion de l'encodeur pendant le tir
constexpr int      GOAL_LEFT = 5, GOAL_RIGHT = 10;  // largeur de la cage (colonnes)
// ---------------------------------------------------------------------------

const Note SND_KICK[]  = {{180, 40}, {0, 0}};
const Note SND_GOAL[]  = {{523, 80}, {659, 80}, {784, 80}, {1047, 300}, {0, 0}};
const Note SND_SAVE[]  = {{300, 120}, {200, 200}, {0, 0}};
const Note SND_WIN[]   = {{392, 150}, {523, 150}, {659, 150}, {784, 300}, {659, 150}, {784, 400}, {0, 0}};
const Note SND_LOSE[]  = {{392, 200}, {349, 200}, {330, 500}, {0, 0}};

enum Phase : uint8_t { AIM, SHOT, RESULT, END };
Phase    phase;
bool     story, bonus;
int      level, shots, goals, score;
float    px, bx, by, bvx, bvy, kx, dx, ddir;  // joueuse, ballon, gardien, défenseur
uint32_t chargeStart, phaseUntil, flashUntil;
float    power;
bool     charging, lastWasGoal;
const char* message;
int32_t  encBase;
bool     dirty;

float keeperSpeed()   { return 2.5f + level * 1.0f + (bonus ? 1.5f : 0); }
float defenderSpeed() { return 3.0f + level * 1.0f; }

void newRound() {
  shots = goals = 0;
  phase = AIM;
  message = story ? "La finale !" : "À toi !";
  dirty = true;
}

void endShot(uint32_t now, bool goal, const char* msg) {
  phase = RESULT;
  lastWasGoal = goal;
  message = msg;
  phaseUntil = now + (goal ? 1600 : 1100);
  if (goal) { goals++; score += 100 * (level + 1) * (bonus ? 2 : 1); flashUntil = now + 600; play(SND_GOAL, true); }
  else play(SND_SAVE, true);
  dirty = true;
}

void drawOled() {
  oled.clearBuffer();
  oled.setFont(u8g2_font_6x10_tf);
  oled.setCursor(0, 10);
  if (story) oled.print("Football - la finale");
  else oled.printf("Football  niveau %d%s", level + 1, bonus ? " *" : "");
  oled.setCursor(0, 26);
  oled.printf("Buts %d/%d   Tirs %d/%d", goals, GOALS_TO_WIN, shots, SHOTS_PER_ROUND);
  if (!story) { oled.setCursor(0, 40); oled.printf("Score %d  (record %d)", score, progress.footballBest); }
  if (message) { oled.setCursor((128 - oled.getUTF8Width(message)) / 2, 60); oled.print(message); }
  oled.sendBuffer();
}

void render(uint32_t now) {
  FastLED.clear();
  // Pelouse à bandes, cage et lignes
  for (int y = 0; y < 16; y++)
    for (int x = 0; x < 16; x++) setPixel(x, y, (y / 2) % 2 ? CRGB(0, 40, 0) : CRGB(0, 60, 0));
  for (int x = GOAL_LEFT - 1; x <= GOAL_RIGHT + 1; x++) setPixel(x, 0, CRGB(200, 200, 200));
  setPixel(GOAL_LEFT - 1, 1, CRGB(200, 200, 200));
  setPixel(GOAL_RIGHT + 1, 1, CRGB(200, 200, 200));
  for (int x = 0; x < 16; x++) setPixel(x, 8, CRGB(30, 90, 30));
  // Gardien, défenseur et joueuse
  setPixel((int)kx, 1, CRGB(0, 60, 255));
  setPixel((int)kx + 1, 1, CRGB(0, 60, 255));
  setPixel((int)dx, 7, CRGB(0, 60, 255));
  setPixel((int)dx + 1, 7, CRGB(0, 60, 255));
  setPixel((int)px, 14, CRGB(255, 60, 140));
  setPixel((int)px, 13, CRGB(255, 200, 150));
  // Ballon, avec lignes de vitesse manga pendant le tir
  CRGB ball = power > 0.8f && phase == SHOT ? CRGB(255, 120, 0) : CRGB::White;
  if (phase == SHOT) {
    for (int i = 1; i <= 2 + (int)(power * 4); i++) setPixel((int)bx, (int)by + i, CRGB(255, 255 - i * 40, 0).nscale8(200 - i * 25));
    if (power > 0.5f)
      for (int i = 0; i < (int)(power * 8); i++) setPixel(random8(16), random8(16), CRGB(90, 90, 90));
  }
  if (phase == AIM && charging && (now / 80) % 2) ball = CRGB(255, 200, 0);
  if (phase != RESULT || !lastWasGoal) setPixel((int)bx, (int)by, ball);
  // But : flash blanc puis feu d'artifice de couleurs
  if (now < flashUntil) {
    if (flashUntil - now > 400) fill_solid(leds, NUM_LEDS, CRGB(120, 120, 120));
    else for (int i = 0; i < 25; i++) leds[random16(NUM_LEDS)] = CHSV(random8(), 200, 255);
  }
  if (phase == END) drawScroll(goals >= GOALS_TO_WIN ? "GAGNE" : "PERDU", now % scrollDuration("GAGNE"), 5,
                               goals >= GOALS_TO_WIN ? CRGB(255, 200, 0) : CRGB::Red);
  FastLED.show();
  // LED : charge du tir (jaune → rouge), arc-en-ciel après un but
  if (now < flashUntil + 800 && lastWasGoal) setRgb(CHSV(now / 3, 255, 255));
  else if (charging) setRgb(blend(CRGB(255, 200, 0), CRGB(255, 0, 0), power * 255));
  else setRgb(CRGB(0, 120, 0));
}

}  // namespace football

using namespace football;

void footballBegin(bool storyMode) {
  story = storyMode;
  bonus = !story && (progress.endings & END_LEGENDAIRE);  // mode secret : gardien plus rapide, points doublés
  level = story ? 1 : 0;
  score = 0;
  px = 8; kx = 7; dx = 2; ddir = 1;
  bx = px; by = 12;
  charging = false; power = 0; lastWasGoal = false; flashUntil = 0;
  encBase = encoderCount();
  newRound();
}

GameResult footballLoop(uint32_t now) {
  static uint32_t lastFrame = 0;
  static bool firePressed = false;  // un appui lu entre deux images est gardé pour l'image suivante
  firePressed |= btn1.pressed() | btnEnc.pressed();
  bool fireHeld = btn1.held() || btnEnc.held();
  if (btn2.pressed()) return story ? GAME_LOST : GAME_QUIT;

  if (now - lastFrame < 16) return GAME_RUNNING;
  float dt = min((now - lastFrame) / 1000.0f, 0.05f);
  lastFrame = now;

  int32_t pulses = encoderCount() - encBase;
  encBase += pulses;

  // Le défenseur va et vient, le gardien suit le ballon
  dx += ddir * defenderSpeed() * dt;
  if (dx < 1) { dx = 1; ddir = 1; }
  if (dx > 13) { dx = 13; ddir = -1; }
  float target = phase == SHOT && by < 9 ? bx - 0.5f : 7.0f;  // le gardien réagit quand le ballon passe le milieu
  float ks = keeperSpeed() * (phase == SHOT ? 1.0f - power * 0.4f : 1.0f);  // un tir puissant surprend le gardien
  kx += constrain(target - kx, -ks * dt, ks * dt);
  kx = constrain(kx, GOAL_LEFT - 1.0f, GOAL_RIGHT * 1.0f);

  switch (phase) {
    case AIM:
      px = constrain(px + pulses * 0.5f + joyX() * PLAYER_SPEED * dt, 0.0f, 15.0f);
      bx = px; by = 12;
      if (firePressed && !charging) { charging = true; chargeStart = now; }
      if (charging) {
        power = min(1.0f, (now - chargeStart) / (float)CHARGE_MS);
        if (!fireHeld || now - chargeStart > CHARGE_MS + 300) {  // tir au relâchement (ou tout seul si on tient trop)
          charging = false;
          shots++;
          bvx = 0;
          bvy = -(SHOT_SPEED_MIN + power * (SHOT_SPEED_MAX - SHOT_SPEED_MIN));
          phase = SHOT;
          message = power > 0.8f ? "TIR DU DRAGON !" : nullptr;
          play(SND_KICK);
          dirty = true;
        }
      }
      break;

    case SHOT: {
      bvx += pulses * CURVE_PER_PULSE + joyX() * 8 * dt;  // effet : on courbe le ballon
      float ny = by + bvy * dt, nx = constrain(bx + bvx * dt, 0.0f, 15.99f);
      // Arrêté seulement si le ballon touche vraiment un des 2 pixels du défenseur ou du gardien
      auto touches = [&](float who) { int c = (int)nx, w = (int)who; return c == w || c == w + 1; };
      if (by > 7.5f && ny <= 7.5f && touches(dx)) { endShot(now, false, "CONTRÉ !"); break; }
      if (by > 1.5f && ny <= 1.5f && touches(kx)) { endShot(now, false, "ARRÊT !"); break; }
      bx = nx; by = ny;
      if (by <= 0.5f) {
        if (bx >= GOAL_LEFT && bx < GOAL_RIGHT + 1) endShot(now, true, "BUUUUT !");
        else endShot(now, false, "À côté...");
      }
      break;
    }

    case RESULT:
      if (now >= phaseUntil) {
        power = 0;
        if (goals >= GOALS_TO_WIN || shots - goals > SHOTS_PER_ROUND - GOALS_TO_WIN) {  // manche décidée
          phase = END;
          phaseUntil = now + 2500;
          message = goals >= GOALS_TO_WIN ? "Victoire !" : "Perdu...";
          play(goals >= GOALS_TO_WIN ? SND_WIN : SND_LOSE, true);
        } else {
          phase = AIM;
          message = nullptr;
        }
        dirty = true;
      }
      break;

    case END:
      if (now >= phaseUntil) {
        bool won = goals >= GOALS_TO_WIN;
        if (story) return won ? GAME_WON : GAME_LOST;
        if (!won) {
          if (score > progress.footballBest) { progress.footballBest = score; progressSave(); }
          return GAME_QUIT;
        }
        level++;  // niveau suivant : gardien et défenseur plus rapides
        newRound();
      }
      break;
  }

  firePressed = false;
  render(now);
  if (dirty) { drawOled(); dirty = false; }
  return GAME_RUNNING;
}
