// Jukebox : réécouter les jingles de l'histoire, avec un égaliseur sur la matrice.
// Encodeur ou joystick (haut/bas) : choisir. Bouton ou bouton de l'encodeur : jouer. Bouton du joystick : menu.
#include "hw.h"
#include "scenes.h"
#include "activities.h"

namespace {

struct Track { const char* name; const Note* notes; };
const Track TRACKS[] = {
  {"Bâillement", J_BAILLEMENT}, {"Place du village", J_CALME}, {"Fanfare du charme", J_FANFARE},
  {"Tambour du tanuki", J_TAMBOUR}, {"Basse de la grenouille", J_BASSE}, {"Chanson de la note", J_MELODIE},
  {"Hibou", J_HIBOU}, {"Brume mystérieuse", J_MYSTERE}, {"Marche royale", J_MARCHE},
  {"Trombone triste", J_TROMBONE}, {"La Fête", J_FETE}, {"La Pâtissière", J_GOURMAND},
  {"L'Arc-en-ciel", J_HARPE}, {"Wizard Mode", J_SPECIAL}, {"Orgue des étoiles", J_ORGUE},
  {"Hymne du stade", J_HYMNE}, {"La Légende", J_LEGENDE},
};
constexpr int COUNT = sizeof(TRACKS) / sizeof(TRACKS[0]);
constexpr int VISIBLE = 5;

int      cursor = 0;
uint8_t  bars[16];
int32_t  encBase;
int8_t   joyWas;
bool     dirty;

void drawOled() {
  oled.clearBuffer();
  oled.setFont(u8g2_font_6x10_tf);
  oled.setCursor(0, 9);
  oled.print("Jukebox");
  oled.drawHLine(0, 11, 128);
  int top = constrain(cursor - VISIBLE / 2, 0, COUNT - VISIBLE);
  for (int i = 0; i < VISIBLE; i++) {
    int baseline = 21 + i * 10;
    if (top + i == cursor) { oled.drawBox(0, baseline - 8, 128, 10); oled.setDrawColor(0); }
    oled.setCursor(4, baseline);
    oled.print(TRACKS[top + i].name);
    oled.setDrawColor(1);
  }
  oled.sendBuffer();
}

void move(int d) {
  cursor = ((cursor + d) % COUNT + COUNT) % COUNT;
  dirty = true;
}

}  // namespace

void jukeboxBegin() {
  encBase = encoderCount();
  memset(bars, 0, sizeof(bars));
  dirty = true;
}

GameResult jukeboxLoop(uint32_t now) {
  static uint32_t lastFrame = 0;
  if (btn2.pressed()) return GAME_QUIT;
  bool playIt = btn1.pressed();
  playIt |= btnEnc.pressed();
  if (playIt) play(TRACKS[cursor].notes, true);
  int32_t d = (encoderCount() - encBase) / 4;
  if (d) { encBase += d * 4; move(d); }
  float y = joyY();
  int8_t dir = y > 0.6f ? 1 : y < -0.6f ? -1 : 0;
  if (dir && dir != joyWas) move(dir);
  joyWas = dir;

  if (now - lastFrame >= 30) {  // égaliseur : la note jouée fait monter la barre de sa hauteur
    lastFrame = now;
    uint16_t f = currentFreq();
    if (f) {
      int col = constrain((int)((log2f(f) - 6.3f) * 3.2f), 0, 15);  // ~80 Hz à gauche, ~2 kHz à droite
      bars[col] = 16;
      if (col > 0) bars[col - 1] = max<uint8_t>(bars[col - 1], 10);
      if (col < 15) bars[col + 1] = max<uint8_t>(bars[col + 1], 10);
    }
    FastLED.clear();
    for (int x = 0; x < 16; x++) {
      for (int h = 0; h < bars[x]; h++) setPixel(x, 15 - h, CHSV(x * 16 - h * 6, 255, 200));
      if (bars[x]) bars[x]--;
    }
    FastLED.show();
    setRgb(f ? CRGB(CHSV(f / 8, 255, 255)) : CRGB(20, 0, 30));
  }
  if (dirty) { drawOled(); dirty = false; }
  return GAME_RUNNING;
}
