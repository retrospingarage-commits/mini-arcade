// Meilleurs scores de l'Aventure : les 5 premiers, gardés pour toute la vie de la borne.
// Tourner l'encodeur (ou le joystick gauche/droite) : page « Avancement » (ce qu'on a trouvé dans l'histoire).
// N'importe quel bouton : retour au menu.
#include "hw.h"
#include "save.h"
#include "pictures.h"
#include "activities.h"

namespace {

ScoreEntry table[MAX_SCORES];
int        count = 0;
bool       dirty = true;
bool       progressPage = false;  // 2e page : avancement de l'histoire
int32_t    encBase = 0;
int8_t     joyWas = 0;

void drawProgress() {
  char buf[24];
  snprintf(buf, sizeof(buf), "Avancement : %d %%", progressPercent());
  oled.setCursor(0, 11);  // bande jaune
  oled.print(buf);
  snprintf(buf, sizeof(buf), "Charmes      %d / 3", countBits(progress.charms & ALL_CHARMS));
  oled.setCursor(0, 26); oled.print(buf);
  snprintf(buf, sizeof(buf), "Pinceau      %s", progress.charms & CHARM_PINCEAU ? "oui" : "non");
  oled.setCursor(0, 36); oled.print(buf);
  snprintf(buf, sizeof(buf), "Trophées     %d / 3", countBits(progress.trophies & ALL_TROPHIES));
  oled.setCursor(0, 46); oled.print(buf);
  snprintf(buf, sizeof(buf), "Fins         %d / 4", countBits(progress.endings & (HAPPY_ENDS | END_LEGENDAIRE)));
  oled.setCursor(0, 56); oled.print(buf);
}

const char* endingMark(uint8_t ending) {  // fin atteinte, en une lettre
  return ending & END_LEGENDAIRE ? "*" : "";
}

void drawOled() {
  oled.clearBuffer();
  oled.setFont(u8g2_font_6x10_tf);
  if (progressPage) { drawProgress(); oled.sendBuffer(); return; }
  oled.setCursor(0, 11);  // bande jaune
  oled.print("Meilleurs scores  >");
  if (!count) {
    oled.setCursor(0, 34); oled.print("Pas encore de score :");
    oled.setCursor(0, 46); oled.print("finis l'Aventure !");
  }
  for (int i = 0; i < count; i++) {  // 5 lignes de 9 pixels dans la partie bleue
    oled.setCursor(0, 24 + i * 9);
    static const char LEVEL[] = "FMD";  // facile, moyen, difficile
    oled.printf("%d. %-3s %5u %c %s", i + 1, table[i].name, table[i].score, LEVEL[(table[i].ending >> SCORE_LEVEL_SHIFT) % 3],
                endingMark(table[i].ending));
  }
  oled.sendBuffer();
}

}  // namespace

void scoresBegin() {
  count = scoresLoad(table);
  progressPage = false;
  encBase = encoderCount();
  dirty = true;
}

GameResult scoresLoop(uint32_t now) {
  static uint32_t lastFrame = 0;
  bool any = btn1.pressed();
  any |= btnEnc.pressed();
  any |= btn2.pressed();
  if (any) return GAME_QUIT;
  int32_t d = (encoderCount() - encBase) / 4;
  float x = joyX();
  int8_t dir = x > 0.6f ? 1 : x < -0.6f ? -1 : 0;
  if (d || (dir && dir != joyWas)) { encBase = encoderCount(); progressPage = !progressPage; dirty = true; beep(1200, 10); }
  joyWas = dir;
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
