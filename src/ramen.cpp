// Le ramen du fantôme (Aventure, scène 3) : le fantôme gourmand fait tomber des ingrédients, on les attrape
// dans le bol avec l'encodeur (ou le joystick gauche/droite). Il faut réunir sa recette (sur l'OLED).
// Les intrus (chaussette, arête, caillou) font « Beurk ! » et retirent un ingrédient du bol.
// Le piment rouge : le fantôme crache des flammes (perdu). Bouton du joystick : abandonner.
#include "hw.h"
#include "save.h"
#include "activities.h"

namespace ramen {

// --- Réglages ---------------------------------------------------------------
constexpr int      BOWL_W       = 4;       // largeur du bol (pixels)
constexpr float    BOWL_STEP    = 0.5f;    // pixels par impulsion de l'encodeur
constexpr float    JOY_SPEED    = 12.0f;   // pixels/s au joystick
constexpr float    FALL_SPEED   = 4.0f;    // vitesse de chute au début (pixels/s)
constexpr float    FALL_GAIN    = 0.25f;   // vitesse en plus à chaque ingrédient attrapé
constexpr uint32_t SPAWN_MS     = 950;     // temps entre deux chutes au début
constexpr int      CHANCE_GOOD  = 55;      // % d'ingrédients de la recette ; le reste : intrus et piments
constexpr int      CHANCE_CHILI = 15;      // % de piments
// ---------------------------------------------------------------------------

enum Kind : uint8_t { NOODLES, EGG, SEAWEED, SOCK, BONE, ROCK, CHILI, KINDS };
constexpr int RECIPE_ITEMS = 3;
const int     NEEDED[RECIPE_ITEMS] = {3, 2, 2};  // nouilles, oeufs, algues
const char* const NAMES[RECIPE_ITEMS] = {"Nouilles jaunes", "Oeufs blancs", "Algues vertes"};
const char* const YUCK[] = {"Beurk, chaussette !", "Beurk, une arête !", "Aïe, un caillou !"};

const Note SND_CATCH[] = {{988, 40}, {1319, 60}, {0, 0}};
const Note SND_YUCK[]  = {{220, 80}, {180, 200}, {0, 0}};
const Note SND_FIRE[]  = {{150, 100}, {300, 80}, {120, 400}, {0, 0}};
const Note SND_SLURP[] = {{392, 80}, {523, 80}, {659, 80}, {784, 80}, {1047, 300}, {0, 0}};

struct Item { float x, y; Kind kind; bool on; };
Item     items[6];
int      have[RECIPE_ITEMS];
float    bowlX, ghostX, ghostDir, fallSpeed;
uint32_t nextSpawn, endAt, msgUntil;
enum State : uint8_t { PLAY, WIN, BURNT };
State    state;
const char* message;
int32_t  encBase;
bool     dirty;

// Couleurs des 2 x 2 pixels de chaque objet (haut gauche, haut droite, bas gauche, bas droite)
void itemColors(Kind k, CRGB c[4]) {
  switch (k) {
    case NOODLES: c[0] = c[3] = CRGB(255, 200, 0); c[1] = c[2] = CRGB(255, 240, 140); break;
    case EGG:     c[0] = c[1] = c[2] = CRGB(255, 255, 255); c[3] = CRGB(255, 140, 0); break;
    case SEAWEED: c[0] = c[3] = CRGB(0, 90, 0); c[1] = c[2] = CRGB(0, 170, 40); break;
    case SOCK:    c[0] = c[2] = c[3] = CRGB(140, 0, 255); c[1] = CRGB(0, 0, 0); break;
    case BONE:    c[0] = c[3] = CRGB(170, 170, 170); c[1] = c[2] = CRGB(60, 60, 60); break;
    case ROCK:    c[0] = c[1] = c[2] = c[3] = CRGB(90, 70, 50); break;
    default:      c[0] = CRGB(0, 160, 0); c[1] = c[2] = c[3] = CRGB(255, 0, 0); break;  // piment
  }
}

bool recipeDone() { for (int i = 0; i < RECIPE_ITEMS; i++) if (have[i] < NEEDED[i]) return false; return true; }

void say(const char* m, uint32_t now) { message = m; msgUntil = now + 1500; dirty = true; }

void spawn() {
  for (Item& it : items)
    if (!it.on) {
      int r = random(100);
      Kind k;
      if (r < CHANCE_GOOD) {  // de préférence un ingrédient qui manque encore
        int missing[RECIPE_ITEMS], n = 0;
        for (int i = 0; i < RECIPE_ITEMS; i++) if (have[i] < NEEDED[i]) missing[n++] = i;
        k = (Kind)(n ? missing[random(n)] : random(RECIPE_ITEMS));
      } else if (r < CHANCE_GOOD + CHANCE_CHILI * byLevel(1.0f, 1.4f, 2.0f)) k = CHILI;
      else k = (Kind)(SOCK + random(3));
      it = {constrain(ghostX, 0.0f, 14.0f), 2, k, true};
      return;
    }
}

void catchItem(Item& it, uint32_t now) {
  it.on = false;
  if (it.kind < RECIPE_ITEMS) {
    if (have[it.kind] < NEEDED[it.kind]) { have[it.kind]++; fallSpeed += FALL_GAIN; }
    play(SND_CATCH);
    dirty = true;
    if (recipeDone()) { state = WIN; endAt = now + 2500; play(SND_SLURP, true); say("Slurp ! Délicieux !", now); }
  } else if (it.kind == CHILI) {
    state = BURNT; endAt = now + 2000; play(SND_FIRE, true); say("PIMENT ! Du feu !", now);
  } else {  // intrus : il gâche un ingrédient du bol
    int full[RECIPE_ITEMS], n = 0;
    for (int i = 0; i < RECIPE_ITEMS; i++) if (have[i]) full[n++] = i;
    if (n) have[full[random(n)]]--;
    play(SND_YUCK, true);
    say(YUCK[it.kind - SOCK], now);
  }
}

void drawOled(uint32_t now) {
  oled.clearBuffer();
  oled.setFont(u8g2_font_6x10_tf);
  const char* title = "Le ramen du fantôme";
  oled.setCursor((128 - oled.getUTF8Width(title)) / 2, 11);  // bande jaune
  oled.print(title);
  for (int i = 0; i < RECIPE_ITEMS; i++) {
    char buf[28];
    snprintf(buf, sizeof(buf), "%-15s %d/%d", NAMES[i], have[i], NEEDED[i]);
    oled.setCursor(0, 25 + i * 11);
    oled.print(buf);
  }
  oled.setCursor(0, 62);
  oled.print(message && now < msgUntil ? message : "Piment rouge : NON !");
  oled.sendBuffer();
}

void render(uint32_t now) {
  FastLED.clear();
  // Le fantôme en haut, qui se balance
  int g = (int)roundf(ghostX);
  CRGB ghost = state == BURNT ? CRGB(255, 60, 0) : state == WIN ? CRGB(CHSV(now / 3, 150, 255)) : CRGB(200, 220, 255);
  for (int dx = 0; dx < 3; dx++) setPixel(g - 1 + dx, 0, ghost);
  setPixel(g - 1, 1, ghost); setPixel(g + 1, 1, ghost);
  setPixel(g, 1, CRGB::Black);  // bouche
  // Ingrédients qui tombent
  for (Item& it : items) {
    if (!it.on) continue;
    CRGB c[4];
    itemColors(it.kind, c);
    int x = (int)it.x, y = (int)it.y;
    setPixel(x, y, c[0]); setPixel(x + 1, y, c[1]); setPixel(x, y + 1, c[2]); setPixel(x + 1, y + 1, c[3]);
  }
  // Le bol (bouillon qui monte avec la recette)
  int b = (int)roundf(bowlX);
  CRGB bowl(230, 230, 230);
  setPixel(b - 1, 14, bowl); setPixel(b + BOWL_W, 14, bowl);
  for (int dx = -1; dx <= BOWL_W; dx++) setPixel(b + dx, 15, bowl);
  int total = 0, need = 0;
  for (int i = 0; i < RECIPE_ITEMS; i++) { total += have[i]; need += NEEDED[i]; }
  for (int dx = 0; dx < BOWL_W * total / need; dx++) setPixel(b + dx, 14, CRGB(200, 120, 20));
  // Flammes ou étincelles de victoire
  if (state == BURNT)
    for (int y = 2; y < 16; y++)
      for (int x = 0; x < 16; x++)
        if (random8() < 120 - y * 6) setPixel(x, y, CHSV(random8(0, 40), 255, 255));
  if (state == WIN) for (int i = 0; i < 20; i++) leds[random16(NUM_LEDS)] = CHSV(random8(), 180, 255);
  FastLED.show();
  if (state == BURNT) setRgb(CHSV(random8(0, 30), 255, 255));
  else if (state == WIN) setRgb(CHSV(now / 3, 255, 255));
  else setRgb(CRGB(255, 90, 0).nscale8(60 + sin8(now / 4) / 4));  // le bouillon chauffe
}

}  // namespace ramen

using namespace ramen;

void ramenBegin() {
  for (Item& it : items) it.on = false;
  for (int& h : have) h = 0;
  bowlX = 6; ghostX = 8; ghostDir = 1; fallSpeed = FALL_SPEED * byLevel(1.0f, 1.25f, 1.6f);
  state = PLAY; message = nullptr;
  nextSpawn = millis() + 1200;
  encBase = encoderCount();
  dirty = true;
}

GameResult ramenLoop(uint32_t now) {
  static uint32_t lastFrame = 0;
  btn1.pressed();
  btnEnc.pressed();
  if (btn2.pressed()) return GAME_QUIT;
  if (now - lastFrame < 16) return GAME_RUNNING;
  float dt = min((now - lastFrame) / 1000.0f, 0.05f);
  lastFrame = now;
  int32_t pulses = encoderCount() - encBase;
  encBase += pulses;

  if (state == PLAY) {
    bowlX = constrain(bowlX + pulses * BOWL_STEP * ENC_X_SIGN + joyX() * JOY_SPEED * dt, 1.0f, 15.0f - BOWL_W);
    ghostX += ghostDir * 3.0f * dt;
    if (ghostX < 1) { ghostX = 1; ghostDir = 1; }
    if (ghostX > 14) { ghostX = 14; ghostDir = -1; }
    if (random8() < 3) ghostDir = -ghostDir;  // il change d'avis de temps en temps
    if (now >= nextSpawn) {
      spawn();
      int done = 0;
      for (int h : have) done += h;
      nextSpawn = now + SPAWN_MS - done * 40 + random(0, 300);
    }
    for (Item& it : items) {
      if (!it.on) continue;
      it.y += fallSpeed * dt;
      int b = (int)roundf(bowlX);
      if (it.y >= 12.5f && it.y < 14.0f && it.x + 1 >= b && it.x <= b + BOWL_W - 1) { catchItem(it, now); if (state != PLAY) break; }
      else if (it.y >= 16) it.on = false;
    }
  } else if (now >= endAt) {
    return state == WIN ? GAME_WON : GAME_LOST;
  }
  if (message && now >= msgUntil) { message = nullptr; dirty = true; }
  render(now);
  if (dirty) { drawOled(now); dirty = false; }
  return GAME_RUNNING;
}
