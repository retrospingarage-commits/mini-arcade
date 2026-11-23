// Course manga : vue de dessus, la route défile vers le bas. On part dernière sur 9 voitures.
// Encodeur : aller à gauche ou à droite pour doubler. Bouton (ou bouton de l'encodeur) maintenu : turbo.
// Chaque voiture doublée remplit la jauge de turbo, rouler juste derrière une voiture aussi (aspiration).
// Les premières voitures sont lentes, les dernières plus rapides que nous : il faut garder du turbo pour elles.
// Un choc ralentit beaucoup et vide un peu la jauge. Après quelques tours : classement.
// La route a des virages : elle ondule, et la voiture est poussée vers l'extérieur (il faut tourner l'encodeur
// pour rester dessus ; sur l'herbe on ralentit). Des taches d'huile font déraper (on perd la direction un instant).
// La difficulté règle l'amplitude des virages et le nombre de taches.
// Bouton du joystick : quitter.
#include "hw.h"
#include "save.h"
#include "activities.h"

namespace race {

// --- Réglages ---------------------------------------------------------------
constexpr int      RIVALS         = 8;       // voitures à doubler
constexpr int      LAPS           = 3;       // tours (2 dans l'histoire)
constexpr int      STORY_LAPS     = 2;
constexpr float    LAP_LENGTH     = 300.0f;  // longueur d'un tour (cases)
constexpr float    BASE_SPEED     = 12.0f;   // cases/s sans turbo
constexpr float    TURBO_SPEED    = 18.0f;   // cases/s avec turbo
constexpr float    ACCEL          = 6.0f;    // reprise après un choc (cases/s²)
constexpr float    STEER_STEP     = 0.5f;    // cases par impulsion de l'encodeur
constexpr float    TURBO_PER_PASS = 40.0f;   // jauge gagnée par voiture doublée (sur 100)
constexpr float    TURBO_DRAFT    = 15.0f;   // jauge gagnée par seconde dans l'aspiration d'une voiture
constexpr float    TURBO_BURN     = 35.0f;   // jauge consommée par seconde de turbo
constexpr float    TURBO_TRICKLE  = 1.5f;    // jauge gagnée par seconde en roulant (sans turbo)
constexpr float    CRASH_SPEED    = 4.0f;    // vitesse juste après un choc
constexpr float    CRASH_TURBO    = 25.0f;   // jauge perdue à chaque choc
// Vitesse (cases/s) et avance au départ (cases) de chaque rivale, de la plus facile à la plus dure
const float RIVAL_SPEED[RIVALS] = {6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.5f, 12.8f, 13.8f};
const float RIVAL_START[RIVALS] = {6, 14, 22, 30, 38, 46, 54, 62};
constexpr float    STORY_SLOWER   = 1.0f;    // l'histoire : rivales un peu moins rapides (cases/s en moins)
// Virages : décalage maximal de la route (cases) et longueur d'onde (cases), selon la difficulté
inline float curveAmp()   { return byLevel(2.0f, 3.2f, 4.4f); }
inline float curveLen()   { return byLevel(80.0f, 60.0f, 46.0f); }
constexpr float    CURVE_GRIP     = 0.8f;    // part de la dérive due au virage que la voiture subit
constexpr float    GRASS_SPEED    = 0.55f;   // vitesse sur l'herbe (part de la vitesse normale)
// Taches d'huile : nombre pour une course de 3 tours, selon la difficulté
inline int   oilCount()   { return (int)byLevel(5, 9, 13); }
constexpr int      OIL_MAX        = 16;
constexpr uint32_t SKID_MS        = 1100;    // temps sans direction après une tache d'huile
constexpr float    SKID_SIDE      = 3.5f;    // dérive sur le côté pendant le dérapage (cases/s)
constexpr float    SKID_SPEED     = 0.45f;   // vitesse après une tache (part de la vitesse normale)
// ---------------------------------------------------------------------------
constexpr int ROAD_L = 2, ROAD_R = 13;  // bords de la route (colonnes)
constexpr int CAR_Y  = 12;              // ligne du haut de notre voiture (elle fait 3 lignes)

const Note SND_PASS[]  = {{988, 40}, {1319, 60}, {0, 0}};
const Note SND_CRASH[] = {{120, 200}, {0, 0}};
const Note SND_TURBO[] = {{400, 30}, {600, 30}, {0, 0}};
const Note SND_GO[]    = {{1568, 300}, {0, 0}};
const Note SND_BEEP[]  = {{784, 150}, {0, 0}};
const Note SND_WIN[]   = {{523, 100}, {659, 100}, {784, 100}, {1047, 300}, {784, 100}, {1047, 500}, {0, 0}};
const Note SND_END[]   = {{523, 150}, {440, 150}, {392, 400}, {0, 0}};

struct Rival { float dist, x, targetX, speed; uint8_t hue; bool ahead; };
Rival rivals[RIVALS];
struct Oil { float dist, x; bool hit; };  // x en cases de route
Oil  oils[OIL_MAX];
int  nOils;
float skidDir;
uint32_t skidUntil, msgUntil;
const char* msg;

enum State : uint8_t { COUNTDOWN, RACE, FINISH };
State    state;
bool     story;
int      laps, place, crashes;
float    dist, speed, carX, turbo;
bool     turboOn, dirty;
uint32_t startAt, finishAt, raceStart, raceTime, hurtUntil, nextLaneChange;
int      lastLap, shownPlace, shownTurbo;
int32_t  encBase;

// Décalage de la route (cases) à la distance d : plusieurs ondulations, tout droit au départ
float curve(float d) {
  float ramp = constrain(d / 40.0f, 0.0f, 1.0f), k = 6.2832f / curveLen();
  return ramp * curveAmp() * (sinf(d * k) + 0.5f * sinf(d * k * 2.3f + 1.0f)) / 1.5f;
}
float curveSlope(float d) { return curve(d + 0.5f) - curve(d - 0.5f); }  // cases de décalage par case parcourue
int roadShift(int row) { return (int)roundf(curve(dist + (CAR_Y - row)) - curve(dist)); }  // décalage de la route sur cette ligne

int rivalsAhead() { int n = 0; for (Rival& r : rivals) n += r.dist > dist; return n; }

void drawOled() {
  oled.clearBuffer();
  oled.setFont(u8g2_font_6x10_tf);
  char buf[32];
  int lap = min((int)(dist / LAP_LENGTH) + 1, laps);
  snprintf(buf, sizeof(buf), story ? "Course des tanukis" : "Course  tour %d/%d", lap, laps);
  if (story) snprintf(buf, sizeof(buf), "Tanukis  tour %d/%d", lap, laps);
  oled.setCursor((128 - oled.getUTF8Width(buf)) / 2, 11);  // bande jaune
  oled.print(buf);
  oled.setFont(u8g2_font_10x20_tf);
  snprintf(buf, sizeof(buf), "%d%s / %d", place, place == 1 ? "re" : "e", RIVALS + 1);
  oled.setCursor((128 - oled.getUTF8Width(buf)) / 2, 36);
  oled.print(buf);
  oled.setFont(u8g2_font_6x10_tf);
  oled.setCursor(0, 52);
  oled.print("Turbo");
  oled.drawFrame(36, 45, 90, 8);
  oled.drawBox(38, 47, 86 * (int)turbo / 100, 4);
  oled.setCursor(0, 63);
  if (state == FINISH) {
    snprintf(buf, sizeof(buf), "Arrivée : %lu,%lu s", (unsigned long)(raceTime / 1000), (unsigned long)(raceTime / 100 % 10));
    oled.print(buf);
  } else if (msg && millis() < msgUntil) {
    oled.print(msg);
  } else if (curveSlope(dist + 6) > 0.06f) {
    oled.print("Virage à droite >>");
  } else if (curveSlope(dist + 6) < -0.06f) {
    oled.print("<< Virage à gauche");
  } else if (crashes) {
    snprintf(buf, sizeof(buf), "Chocs : %d", crashes);
    oled.print(buf);
  } else oled.print("Encodeur : doubler");
  oled.sendBuffer();
}

void drawCar(int x, int y, CRGB body, CRGB top) {
  for (int dy = 0; dy < 3; dy++)
    for (int dx = 0; dx < 2; dx++) setPixel(x + dx, y + dy, dy == 1 ? top : body);
}

void render(uint32_t now) {
  int scroll = (int)dist;
  // Herbe, bordures rouge et blanc, route et pointillés qui défilent (la route ondule : décalage par ligne)
  for (int y = 0; y < 16; y++) {
    int w = y - scroll;  // position « dans le monde » de cette ligne
    int sh = roadShift(y);
    bool stripe = ((w % 4) + 4) % 4 < 2;
    for (int x = 0; x < 16; x++) {
      int rx = x - sh;  // colonne dans le repère de la route
      CRGB c;
      if (rx < ROAD_L - 1 || rx > ROAD_R + 1) c = stripe ? CRGB(0, 40, 0) : CRGB(0, 28, 0);
      else if (rx == ROAD_L - 1 || rx == ROAD_R + 1) c = stripe ? CRGB(120, 0, 0) : CRGB(110, 110, 110);
      else c = CRGB(12, 12, 14);
      setPixel(x, y, c);
    }
    if ((((w % 6) + 6) % 6) < 3) setPixel(7 + sh, y, CRGB(70, 70, 70)), setPixel(8 + sh, y, CRGB(70, 70, 70));
    // Ligne d'arrivée en damier à chaque tour
    float worldY = dist + (CAR_Y - y);
    int lapLine = (int)((worldY + 0.5f) / LAP_LENGTH);
    if (lapLine >= 1 && lapLine <= laps && fabsf(worldY - lapLine * LAP_LENGTH) < 0.5f)
      for (int x = ROAD_L; x <= ROAD_R; x++) setPixel(x + sh, y, (x + y) % 2 ? CRGB::White : CRGB::Black);
  }
  // Taches d'huile (violet sombre avec un reflet qui brille)
  for (int i = 0; i < nOils; i++) {
    Oil& o = oils[i];
    int y = CAR_Y - (int)roundf(o.dist - dist);
    if (y < -1 || y > 15) continue;
    int x = (int)roundf(o.x) + roadShift(y);
    bool shine = !o.hit && (now / 180 + i) % 3 == 0;
    CRGB base = o.hit ? CRGB(14, 4, 20) : CRGB(40, 0, 70);
    for (int dy = 0; dy < 2; dy++) for (int dx = 0; dx < 2; dx++) setPixel(x + dx, y + dy, base);
    setPixel(x, y, shine ? CRGB(160, 90, 255) : base);
  }
  // Rivales
  for (Rival& r : rivals) {
    int y = CAR_Y - (int)roundf(r.dist - dist);
    if (y > -3 && y < 16) drawCar((int)roundf(r.x) + roadShift(y), y, CHSV(r.hue, 230, 200), CHSV(r.hue, 120, 255));
  }
  // Notre voiture (clignote après un choc), flammes du turbo
  if (now >= hurtUntil || (now / 90) % 2) drawCar((int)roundf(carX), CAR_Y, CRGB(255, 40, 140), CRGB(255, 200, 220));
  if (turboOn) {
    for (int dx = 0; dx < 2; dx++) setPixel((int)roundf(carX) + dx, CAR_Y + 3, CHSV(random8(10, 40), 255, 255));
    for (int i = 0; i < 4; i++) setPixel(random8(ROAD_L, ROAD_R + 1), random8(16), CRGB(80, 80, 80));  // lignes de vitesse
  }
  // Départ : feux (rouge, rouge, rouge, vert)
  if (state == COUNTDOWN) {
    int n = (now - startAt) / 1000;
    CRGB light = n >= 3 ? CRGB(0, 255, 0) : CRGB(255, 0, 0);
    for (int i = 0; i <= min(n, 2); i++)
      for (int dx = 0; dx < 2; dx++) setPixel(4 + i * 3 + dx, 2, light), setPixel(4 + i * 3 + dx, 3, light);
  }
  if (state == FINISH) {
    char buf[4];
    snprintf(buf, sizeof(buf), "%d", place);
    fadeToBlackBy(leds, NUM_LEDS, 150);
    if (place == 1) for (int i = 0; i < 15; i++) leds[random16(NUM_LEDS)] = CHSV(random8(), 200, 255);
    drawText(buf, (16 - textWidth(buf)) / 2, 5, place == 1 ? CRGB(255, 200, 0) : CRGB(120, 160, 255));
  }
  FastLED.show();
  // LED : rouge après un choc, arc-en-ciel au turbo, sinon la jauge (bleu → cyan quand elle est pleine)
  if (now < hurtUntil) setRgb(CRGB(255, 0, 0));
  else if (now < skidUntil) setRgb(CRGB(160, 0, 255));  // violet : dérapage
  else if (turboOn) setRgb(CHSV(now / 2, 255, 255));
  else setRgb(blend(CRGB(0, 0, 60), CRGB(0, 255, 255), (uint8_t)(turbo * 2.55f)));
}

}  // namespace race

using namespace race;

void raceBegin(bool storyMode) {
  story = storyMode;
  laps = story ? STORY_LAPS : LAPS;
  dist = 0; speed = 0; carX = 7; turbo = 0; turboOn = false; crashes = 0;
  for (int i = 0; i < RIVALS; i++) {
    float x = ROAD_L + (i * 5 % 11);
    rivals[i] = {RIVAL_START[i], x, x, RIVAL_SPEED[i] - (story ? STORY_SLOWER : 0) + byLevel(0.5f, 1.3f, 2.1f), (uint8_t)(i * 32 + 60), true};
  }
  // Taches d'huile réparties sur la course (pas dans les 60 premières cases)
  nOils = min(oilCount() * laps / LAPS, OIL_MAX);
  for (int i = 0; i < nOils; i++) {
    float span = laps * LAP_LENGTH - 90 - 60;
    oils[i] = {60 + span * (i + 0.5f) / nOils + random(-10, 11), (float)random(ROAD_L, ROAD_R - 1), false};
  }
  skidUntil = 0; msg = nullptr; msgUntil = 0; skidDir = 1;
  place = RIVALS + 1; shownPlace = -1; shownTurbo = -1;
  state = COUNTDOWN;
  startAt = millis(); hurtUntil = 0; nextLaneChange = startAt + 3000;
  encBase = encoderCount();
  lastLap = 0;
  dirty = true;
}

GameResult raceLoop(uint32_t now) {
  static uint32_t lastFrame = 0;
  static int lastCount = -1;
  btn1.pressed();
  btnEnc.pressed();
  bool turboHeld = btn1.held() || btnEnc.held();
  if (btn2.pressed()) return GAME_QUIT;
  if (now - lastFrame < 16) return GAME_RUNNING;
  float dt = min((now - lastFrame) / 1000.0f, 0.05f);
  lastFrame = now;
  int32_t pulses = encoderCount() - encBase;
  encBase += pulses;

  if (state == COUNTDOWN) {
    int n = (now - startAt) / 1000;
    if (n != lastCount) { lastCount = n; play(n >= 3 ? SND_GO : SND_BEEP, true); }
    carX = constrain(carX + pulses * STEER_STEP * ENC_X_SIGN, (float)ROAD_L, ROAD_R - 1.0f);
    if (n >= 3) { state = RACE; raceStart = now; lastCount = -1; }
    render(now);
    if (dirty) { drawOled(); dirty = false; }
    return GAME_RUNNING;
  }

  if (state == FINISH) {
    render(now);
    if (dirty) { drawOled(); dirty = false; }
    if (now < finishAt) return GAME_RUNNING;
    if (story) return place == 1 ? GAME_WON : GAME_LOST;
    if (place < progress.raceBestPlace || !progress.raceBestPlace ||
        (place == progress.raceBestPlace && raceTime / 100 < progress.raceBestTime)) {
      progress.raceBestPlace = place;
      progress.raceBestTime = raceTime / 100;
      progressSave();
    }
    return GAME_QUIT;
  }

  // Notre voiture : direction à l'encodeur (perdue pendant un dérapage), turbo au bouton
  bool skidding = now < skidUntil;
  if (skidding) carX += skidDir * SKID_SIDE * dt;                      // la tache d'huile nous envoie sur le côté
  else carX += pulses * STEER_STEP * ENC_X_SIGN;
  carX -= curveSlope(dist) * speed * CURVE_GRIP * dt;                  // le virage nous pousse vers l'extérieur
  carX = constrain(carX, 0.0f, 14.0f);
  bool onGrass = carX < ROAD_L - 0.5f || carX > ROAD_R - 1.0f + 0.5f;
  bool wasTurbo = turboOn;
  turboOn = turboHeld && turbo > 0 && now >= hurtUntil && !skidding;
  if (turboOn && !wasTurbo) play(SND_TURBO);
  if (turboOn) turbo = max(0.0f, turbo - TURBO_BURN * dt);
  else turbo = min(100.0f, turbo + TURBO_TRICKLE * byLevel(1.0f, 0.6f, 0.3f) * dt);
  float target = (turboOn ? TURBO_SPEED : BASE_SPEED) * (onGrass ? GRASS_SPEED : 1.0f) * (skidding ? SKID_SPEED : 1.0f);
  speed = speed < target ? min(target, speed + ACCEL * (turboOn ? 3 : 1) * dt) : max(target, speed - ACCEL * dt);
  dist += speed * dt;

  // Taches d'huile : si on passe dessus, on dérape
  for (int i = 0; i < nOils; i++) {
    Oil& o = oils[i];
    float rel = o.dist - dist;
    if (!o.hit && rel < 1.0f && rel > -2.0f && fabsf(o.x - carX) < 2.0f) {
      o.hit = true;
      skidUntil = now + SKID_MS;
      skidDir = o.x + 1 > carX + 1 ? -1.0f : 1.0f;  // on part du côté opposé au centre de la tache
      turbo = max(0.0f, turbo - 20.0f);
      static const Note SND_SKID[] = {{500, 40}, {350, 60}, {250, 80}, {180, 120}, {0, 0}};
      play(SND_SKID, true);
      msg = "Dérapage !"; msgUntil = now + 1200;
      dirty = true;
    }
  }
  // Rivales : elles avancent et changent de file de temps en temps
  if (now >= nextLaneChange) {
    Rival& r = rivals[random(RIVALS)];
    r.targetX = random(ROAD_L, ROAD_R);
    nextLaneChange = now + random(600, 1500);
  }
  for (Rival& r : rivals) {
    r.dist += r.speed * dt;
    r.x += constrain(r.targetX - r.x, -3.0f * dt, 3.0f * dt);
    float rel = r.dist - dist;  // > 0 : devant nous
    bool sideBySide = fabsf(r.x - carX) < 2.0f;
    // Choc : on lui rentre dedans (elle est juste devant, sur notre file)
    if (sideBySide && rel > 0 && rel < 3.0f && now >= hurtUntil) {
      crashes++;
      speed = CRASH_SPEED;
      turbo = max(0.0f, turbo - CRASH_TURBO);
      hurtUntil = now + 900;
      r.dist = dist + 3.0f;  // on rebondit derrière elle
      play(SND_CRASH, true);
      dirty = true;
    }
    // Aspiration : juste derrière une voiture, sur sa file
    if (sideBySide && rel >= 3.0f && rel < 7.0f) turbo = min(100.0f, turbo + TURBO_DRAFT * dt);
    // Dépassement
    bool ahead = rel > 0;
    if (r.ahead && !ahead) { turbo = min(100.0f, turbo + TURBO_PER_PASS * byLevel(1.0f, 0.8f, 0.6f)); play(SND_PASS); }
    r.ahead = ahead;
  }
  place = rivalsAhead() + 1;
  static int lastCurve = 0;
  int curveNow = curveSlope(dist + 6) > 0.06f ? 1 : curveSlope(dist + 6) < -0.06f ? -1 : 0;
  if (curveNow != lastCurve) { lastCurve = curveNow; dirty = true; }
  if (msg && now >= msgUntil) { msg = nullptr; dirty = true; }
  if (place != shownPlace || (int)turbo / 5 != shownTurbo) { shownPlace = place; shownTurbo = (int)turbo / 5; dirty = true; }
  int lap = (int)(dist / LAP_LENGTH);
  if (lap != lastLap) { lastLap = lap; dirty = true; if (lap < laps) play(SND_BEEP); }

  // Arrivée : les rivales déjà arrivées sont devant
  if (dist >= laps * LAP_LENGTH) {
    state = FINISH;
    raceTime = now - raceStart;
    finishAt = now + 4000;
    turboOn = false;
    play(place == 1 ? SND_WIN : SND_END, true);
    dirty = true;
  }
  render(now);
  if (dirty) { drawOled(); dirty = false; }
  return GAME_RUNNING;
}
