// Galerie : palette des dessins, enregistrement en mémoire permanente et diaporama sur la matrice.
// Encodeur : durée d'affichage de chaque dessin. Bouton : dessin suivant. Bouton du joystick : retour au menu.
#include <Preferences.h>
#include "gallery.h"
#include "save.h"
#include "activities.h"

uint8_t canvas[256];

namespace {

struct Color { uint8_t r, g, b; const char* css; };
const Color PALETTE[PALETTE_SIZE] = {
  {0, 0, 0, "#000"},        {255, 255, 255, "#fff"},  {90, 90, 90, "#777"},     {255, 0, 0, "#f00"},
  {255, 90, 0, "#f70"},     {255, 200, 0, "#fc0"},    {120, 255, 60, "#8f4"},   {0, 160, 0, "#0a0"},
  {0, 200, 200, "#0cc"},    {0, 50, 255, "#04f"},     {140, 0, 255, "#90f"},    {255, 60, 140, "#f4a"},
  {110, 45, 10, "#841"},    {255, 170, 120, "#fb8"},  {0, 0, 90, "#006"},       {255, 150, 200, "#fae"},
};
const char* const MAGIC_CSS[MAGIC_COLORS] = {
  "linear-gradient(45deg,red,orange,yellow,lime,cyan,blue,violet)",   // arc-en-ciel qui défile
  "radial-gradient(#fff,#9cf 40%,#000)",                              // paillettes
  "radial-gradient(#ff0,#f40 50%,#600)",                              // braise
  "linear-gradient(#0f8,#80f)",                                       // aurore
};

Preferences prefs;
uint8_t  slide[256];
int      current = 0, count = 0;
uint8_t  delaySec = 6;
uint32_t nextAt = 0;
int32_t  encBase = 0;
bool     dirty = true;

void drawOled() {
  oled.clearBuffer();
  oled.setFont(u8g2_font_6x10_tf);
  oled.setCursor(0, 10);
  oled.print("Galerie");
  if (!count) {
    oled.setCursor(0, 34); oled.print("Aucun dessin pour");
    oled.setCursor(0, 46); oled.print("l'instant : va dans");
    oled.setCursor(0, 58); oled.print("l'Atelier !");
  } else {
    oled.setCursor(0, 30); oled.printf("Dessin %d / %d", current + 1, count);
    oled.setCursor(0, 46); oled.printf("Change toutes les %d s", delaySec);
    oled.setCursor(0, 62); oled.print("Bouton : suivant");
  }
  oled.sendBuffer();
}

void show(int i, uint32_t now) {
  current = (i % count + count) % count;
  galleryLoad(current, slide);
  nextAt = now + delaySec * 1000UL;
  dirty = true;
}

}  // namespace

int availableColors() { return PALETTE_SIZE + (progress.endings & END_LEGENDAIRE ? MAGIC_COLORS : 0); }

CRGB drawingColor(uint8_t idx, int x, int y, uint32_t now) {
  if (idx < PALETTE_SIZE) return CRGB(PALETTE[idx].r, PALETTE[idx].g, PALETTE[idx].b);
  switch (idx - PALETTE_SIZE) {
    case 0:  return CHSV(now / 10 + x * 12 + y * 6, 255, 255);                                   // arc-en-ciel
    case 1:  return random8() < 40 ? CRGB::White : CRGB(40, 80, 160);                            // paillettes
    case 2:  return HeatColor(160 + (inoise8(x * 60, y * 60, now / 3) >> 2));                    // braise
    case 3:  return blend(CRGB(0, 255, 120), CRGB(140, 0, 255), sin8(now / 8 + y * 16));         // aurore
    default: return CRGB::Black;
  }
}

const char* paletteCss(uint8_t idx) {
  if (idx < PALETTE_SIZE) return PALETTE[idx].css;
  return idx < PALETTE_SIZE + MAGIC_COLORS ? MAGIC_CSS[idx - PALETTE_SIZE] : "#000";
}

void drawDrawing(const uint8_t* d, uint32_t now) {
  for (int y = 0; y < 16; y++)
    for (int x = 0; x < 16; x++) setPixel(x, y, drawingColor(d[y * 16 + x], x, y, now));
}

int galleryCount() {
  prefs.begin("gallery", true);
  int n = prefs.getUChar("n", 0);
  prefs.end();
  return n;
}

bool galleryLoad(int i, uint8_t* out) {
  prefs.begin("gallery", true);
  int n = prefs.getUChar("n", 0), next = prefs.getUChar("next", 0);
  bool ok = false;
  if (i >= 0 && i < n) {
    int slot = n < MAX_DRAWINGS ? i : (next + i) % MAX_DRAWINGS;  // le plus ancien d'abord
    char key[8];
    snprintf(key, sizeof(key), "d%d", slot);
    ok = prefs.getBytes(key, out, 256) == 256;
  }
  prefs.end();
  return ok;
}

int gallerySave(const uint8_t* in) {
  prefs.begin("gallery", false);
  int n = prefs.getUChar("n", 0), next = prefs.getUChar("next", 0);
  char key[8];
  snprintf(key, sizeof(key), "d%d", next);
  prefs.putBytes(key, in, 256);
  prefs.putUChar("next", (next + 1) % MAX_DRAWINGS);
  if (n < MAX_DRAWINGS) prefs.putUChar("n", ++n);
  prefs.end();
  return n;
}

void galleryBegin() {
  count = galleryCount();
  prefs.begin("gallery", true);
  delaySec = prefs.getUChar("delay", 6);
  prefs.end();
  encBase = encoderCount();
  if (count) show(count - 1, millis());  // commence par le dernier dessin
  dirty = true;
}

GameResult galleryLoop(uint32_t now) {
  static uint32_t lastFrame = 0;
  if (btn2.pressed()) {
    prefs.begin("gallery", false);
    prefs.putUChar("delay", delaySec);
    prefs.end();
    return GAME_QUIT;
  }
  bool next = btn1.pressed();
  next |= btnEnc.pressed();
  int32_t d = (encoderCount() - encBase) / 4;
  if (d) { encBase += d * 4; delaySec = constrain(delaySec + d, 2, 30); dirty = true; }
  if (count && (next || now >= nextAt)) show(current + 1, now);

  if (now - lastFrame >= 40) {
    lastFrame = now;
    FastLED.clear();
    if (count) drawDrawing(slide, now);
    else for (int i = 0; i < 4; i++) leds[random16(NUM_LEDS)] = CHSV(random8(), 200, 120);
    FastLED.show();
    setRgb(count ? drawingColor(slide[8 * 16 + 8], 8, 8, now) : CRGB(30, 30, 30));
  }
  if (dirty) { drawOled(); dirty = false; }
  return GAME_RUNNING;
}
