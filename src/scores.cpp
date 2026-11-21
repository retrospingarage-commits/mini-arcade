// Meilleurs scores de l'Aventure : les 5 premiers, gardés pour toute la vie de la borne.
// N'importe quel bouton : retour au menu.
#include "hw.h"
#include "save.h"
#include "pictures.h"
#include "activities.h"

namespace {

ScoreEntry table[MAX_SCORES];
int        count = 0;
bool       dirty = true;

const char* endingMark(uint8_t ending) {  // fin atteinte, en une lettre
  return ending & END_LEGENDAIRE ? "*" : "";
}

void drawOled() {
  oled.clearBuffer();
  oled.setFont(u8g2_font_6x10_tf);
  oled.setCursor(0, 11);  // bande jaune
  oled.print("Meilleurs scores");
  if (!count) {
    oled.setCursor(0, 34); oled.print("Pas encore de score :");
    oled.setCursor(0, 46); oled.print("finis l'Aventure !");
  }
  for (int i = 0; i < count; i++) {  // 5 lignes de 9 pixels dans la partie bleue
    oled.setCursor(0, 24 + i * 9);
    oled.printf("%d. %-3s %5u %s", i + 1, table[i].name, table[i].score, endingMark(table[i].ending));
  }
  oled.sendBuffer();
}

}  // namespace

void scoresBegin() {
  count = scoresLoad(table);
  dirty = true;
}

GameResult scoresLoop(uint32_t now) {
  static uint32_t lastFrame = 0;
  bool any = btn1.pressed();
  any |= btnEnc.pressed();
  any |= btn2.pressed();
  if (any) return GAME_QUIT;
  if (now - lastFrame >= 50) {
    lastFrame = now;
    FastLED.clear();
    drawPicture(PIC_BALLON);
    for (int i = 0; i < 3; i++) leds[random16(NUM_LEDS)] = CHSV(random8(), 150, 255);
    FastLED.show();
    setRgb(CHSV(40, 255, 200));
  }
  if (dirty) { drawOled(); dirty = false; }
  return GAME_RUNNING;
}
