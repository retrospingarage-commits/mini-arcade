// Mood light : ambiances lumineuses sur la matrice et la LED RVB.
// Comme un petit menu de 3 lignes : Ambiance, Luminosité, Veille.
// Joystick haut/bas : choisir la ligne. Encodeur ou joystick gauche/droite : changer la valeur de la ligne.
// Bouton (ou bouton de l'encodeur) : ligne suivante ; sur « Veille » : tout s'éteint (n'importe quel bouton rallume).
// Bouton du joystick : retour au menu.
#include "hw.h"
#include "save.h"
#include "activities.h"

namespace {

const char* const NAMES[] = {"Sakura", "Nuit étoilée", "Océan", "Feu de camp", "Aurore", "Arc-en-ciel", "Lucioles"};
constexpr int     COUNT = sizeof(NAMES) / sizeof(NAMES[0]);
// luminosité réglable sur l'échelle progressive de hw.h (1 à 40, la limite de sécurité)
constexpr uint32_t OLED_OFF_MS = 8000;  // l'OLED s'éteint après 8 s sans toucher à rien

struct Particle { float x, y, vx, vy; uint8_t hue; };
Particle parts[10];
uint8_t  heat[16][16];        // feu de camp
uint8_t  mood, level;
uint8_t  line;                 // 0 ambiance, 1 luminosité, 2 veille
int8_t   joyXWas, joyYWas;
bool     sleeping, oledOn;
uint32_t lastInput;
int32_t  encBase;
bool     dirty;

void resetParticles() {
  for (Particle& p : parts) p = {(float)random8(16), (float)random8(16), random(-10, 11) / 20.0f, random(5, 20) / 20.0f, random8()};
}

void drawOled() {
  oled.clearBuffer();
  oled.setFont(u8g2_font_6x10_tf);
  oled.setCursor(0, 11);  // bande jaune
  oled.print("Mood light");
  for (int i = 0; i < 3; i++) {   // la ligne choisie est en vidéo inverse
    int base = 26 + i * 13;
    if (i == line) { oled.drawBox(0, base - 9, 128, 12); oled.setDrawColor(0); }
    oled.setCursor(2, base);
    if (i == 0) { oled.print("< "); oled.print(NAMES[mood]); oled.print(" >"); }
    else if (i == 1) {
      oled.print("Lumière");
      oled.drawFrame(50, base - 7, 76, 7);
      oled.drawBox(52, base - 5, (brightnessStep(level) + 1) * 72 / BRIGHTNESS_STEP_COUNT, 3);
    }
    else oled.print("Éteindre (veille)");
    oled.setDrawColor(1);
  }
  oled.setCursor(0, 63);
  oled.print(line == 2 ? "Bouton : éteindre" : "Tourne : changer");
  oled.sendBuffer();
}

void render(uint32_t now) {
  switch (mood) {
    case 0:  // Sakura : pétales roses qui tombent en zigzag sur un fond violet
      for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) setPixel(x, y, CRGB(15 + y, 0, 20 + y));
      for (Particle& p : parts) {
        p.y += p.vy * 0.4f; p.x += sinf(now / 600.0f + p.hue) * 0.08f;
        if (p.y > 16) { p.y = -1; p.x = random8(16); }
        setPixel((int)p.x, (int)p.y, CRGB(255, 120, 170));
      }
      break;
    case 1:  // Nuit étoilée : étoiles qui scintillent et une lune
      for (int i = 0; i < NUM_LEDS; i++) leds[i] = CRGB(0, 0, 10);
      for (int i = 0; i < 14; i++) setPixel((i * 7 + 2) % 16, (i * 5 + 1) % 13, CHSV(40, 40, sin8(now / 6 + i * 50) / 2));
      for (int dy = 0; dy < 3; dy++) for (int dx = 0; dx < 3; dx++) if (dx + dy != 4) setPixel(11 + dx, 2 + dy, CRGB(255, 220, 120));
      break;
    case 2:  // Océan
      for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++) setPixel(x, y, CHSV(140 + inoise8(x * 30, y * 30, now / 6) / 8, 255, 80 + inoise8(x * 20, now / 4, y * 20) / 2));
      break;
    case 3:  // Feu de camp : chaleur qui monte et se refroidit
      for (int x = 0; x < 16; x++) {
        for (int y = 0; y < 16; y++) heat[y][x] = qsub8(heat[y][x], random8(0, 25));
        for (int y = 0; y < 15; y++) heat[y][x] = (heat[y + 1][x] * 2 + heat[min(y + 2, 15)][x]) / 3;
        if (random8() < 160) heat[15][x] = qadd8(heat[15][x], random8(120, 255));
        for (int y = 0; y < 16; y++) setPixel(x, y, HeatColor(heat[y][x]));
      }
      break;
    case 4:  // Aurore : voiles verts et violets qui ondulent
      for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++) {
          uint8_t n = inoise8(x * 25, y * 40 + now / 3, now / 10);
          setPixel(x, y, y < 11 ? blend(CRGB(0, 255, 120), CRGB(140, 0, 255), n).nscale8(n) : CRGB(0, 0, 8));
        }
      break;
    case 5:  // Arc-en-ciel
      for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) setPixel(x, y, CHSV(x * 8 + y * 8 + now / 30, 255, 200));
      break;
    default:  // Lucioles : points jaunes qui se promènent et clignotent
      for (int i = 0; i < NUM_LEDS; i++) leds[i] = CRGB(0, 12, 0);
      for (Particle& p : parts) {
        p.x += p.vx * 0.1f; p.y += (p.vy - 0.5f) * 0.1f;
        if (p.x < 0 || p.x > 15) p.vx = -p.vx;
        if (p.y < 0 || p.y > 15) p.vy = 1 - p.vy;
        p.x = constrain(p.x, 0.0f, 15.0f); p.y = constrain(p.y, 0.0f, 15.0f);
        setPixel((int)p.x, (int)p.y, CRGB(255, 220, 0).nscale8(sin8(now / 4 + p.hue)));
      }
      break;
  }
}

CRGB averageColor() {
  uint32_t r = 0, g = 0, b = 0;
  for (int i = 0; i < NUM_LEDS; i++) { r += leds[i].r; g += leds[i].g; b += leds[i].b; }
  return CRGB(min<uint32_t>(r / 64, 255), min<uint32_t>(g / 64, 255), min<uint32_t>(b / 64, 255));
}

void wake(uint32_t now) {
  lastInput = now;
  if (!oledOn) { oled.setPowerSave(0); oledOn = true; dirty = true; }
}

}  // namespace

void moodBegin() {
  mood = progress.mood % COUNT;
  level = brightnessShift(progress.moodLevel ? progress.moodLevel : 19, 0);
  line = 0; joyXWas = joyYWas = 0;
  sleeping = false;
  oledOn = true;
  lastInput = millis();
  encBase = encoderCount();
  resetParticles();
  memset(heat, 0, sizeof(heat));
  dirty = true;
}

GameResult moodLoop(uint32_t now) {
  static uint32_t lastFrame = 0;
  bool b1 = btn1.pressed(), be = btnEnc.pressed(), b2 = btn2.pressed();
  int32_t d = (encoderCount() - encBase) / 4;
  if (d) encBase += d * 4;

  if (sleeping) {  // en veille : n'importe quel bouton ou cran rallume
    if (b1 || be || b2 || d) { sleeping = false; wake(now); }
    return GAME_RUNNING;
  }
  if (b2) {  // retour au menu : on garde l'ambiance et la luminosité, la matrice revient à la normale
    progress.mood = mood;
    progress.moodLevel = level;
    progressSave();
    FastLED.setBrightness(matrixBrightness);
    if (!oledOn) oled.setPowerSave(0);
    return GAME_QUIT;
  }
  // Joystick : haut/bas = ligne, gauche/droite = valeur (un cran par poussée)
  float jx = joyX(), jy = joyY();
  int8_t jxd = jx > 0.6f ? 1 : jx < -0.6f ? -1 : 0, jyd = jy > 0.6f ? 1 : jy < -0.6f ? -1 : 0;
  bool newJoy = (jxd && jxd != joyXWas) || (jyd && jyd != joyYWas);
  if (!oledOn && (b1 || be || d || newJoy)) {  // écran éteint : le premier geste le rallume seulement
    joyXWas = jxd; joyYWas = jyd;
    wake(now);
    return GAME_RUNNING;
  }
  if (jyd && jyd != joyYWas) { line = (line + 3 + jyd) % 3; wake(now); dirty = true; }
  if (jxd && jxd != joyXWas && line < 2) d += jxd;
  joyXWas = jxd; joyYWas = jyd;
  if ((b1 || be) && line < 2) { line++; wake(now); dirty = true; }
  else if ((b1 || be) && line == 2) {  // veille
    line = 0;
    sleeping = true;
    FastLED.clear(); FastLED.show();
    setRgb(CRGB::Black);
    oled.setPowerSave(1); oledOn = false;
    return GAME_RUNNING;
  }
  if (d && line < 2) {
    if (line == 1) level = brightnessShift(level, d);
    else { mood = ((mood + d) % COUNT + COUNT) % COUNT; resetParticles(); }
    wake(now);
    dirty = true;
  }
  if (oledOn && now - lastInput > OLED_OFF_MS) { oled.setPowerSave(1); oledOn = false; }

  if (now - lastFrame >= 33) {
    lastFrame = now;
    FastLED.setBrightness(level);
    render(now);
    FastLED.show();
    setRgb(averageColor().nscale8(level * 255 / BRIGHTNESS));
  }
  if (dirty && oledOn) { drawOled(); dirty = false; }
  return GAME_RUNNING;
}
