// Menu sur l'OLED : l'encodeur (ou le joystick) choisit une activité, un bouton valide.
// Les activités pas encore gagnées dans l'Aventure sont verrouillées (cadenas).
#include "hw.h"
#include "menu.h"
#include "pictures.h"
#include "save.h"

namespace {

constexpr int VISIBLE_ITEMS = 5;  // lignes de menu visibles sous le titre

const Note SND_TICK[]   = {{1200, 8}, {0, 0}};
const Note SND_LOCKED[] = {{150, 150}, {0, 0}};
const Note SND_OK[]     = {{880, 60}, {1320, 90}, {0, 0}};

uint8_t     cursor = 0;
int32_t     encBase = 0;
int8_t      joyWas = 0;
bool        dirty = true;
const char* message = nullptr;  // message affiché un instant (activité verrouillée ou pas encore prête)
uint32_t    messageUntil = 0;

// Activités déjà programmées (les autres arrivent dans les prochaines étapes)
bool isReady(Activity a) { return a == ACT_ADVENTURE || a == ACT_BREAKOUT; }

void drawLock(int x, int baseline) {  // petit cadenas de 7x8 pixels
  oled.drawBox(x, baseline - 5, 7, 5);
  oled.drawFrame(x + 1, baseline - 8, 5, 4);
}

void drawOled(uint32_t now) {
  oled.clearBuffer();
  oled.setFont(u8g2_font_6x10_tf);
  oled.setCursor(0, 9);
  oled.print("MENU");
  oled.setCursor(62, 9);
  oled.printf("Charmes %d/3", countBits(progress.charms & ALL_CHARMS));
  oled.drawHLine(0, 11, 128);

  int top = constrain((int)cursor - VISIBLE_ITEMS / 2, 0, ACT_COUNT - VISIBLE_ITEMS);
  for (int i = 0; i < VISIBLE_ITEMS; i++) {
    Activity a = (Activity)(top + i);
    int baseline = 21 + i * 10;
    if (a == cursor) {  // ligne choisie en négatif
      oled.drawBox(0, baseline - 8, 128, 10);
      oled.setDrawColor(0);
    }
    oled.setCursor(4, baseline);
    oled.print(activityName(a));
    if (!isUnlocked(a)) drawLock(116, baseline);
    oled.setDrawColor(1);
  }

  if (message && now < messageUntil) {  // bulle par-dessus le menu
    oled.setDrawColor(0);
    oled.drawBox(6, 22, 116, 24);
    oled.setDrawColor(1);
    oled.drawFrame(6, 22, 116, 24);
    oled.setCursor((128 - oled.getUTF8Width(message)) / 2, 37);
    oled.print(message);
  }
  oled.sendBuffer();
}

void renderMatrix(uint32_t now) {
  Activity a = (Activity)cursor;
  FastLED.clear();
  if (!isUnlocked(a))           drawPicture(PIC_CADENAS);
  else if (a == ACT_ADVENTURE)  drawPicture(PIC_CHAT);
  else if (a == ACT_BREAKOUT)   drawPicture(PIC_BRIQUES);
  else for (int i = 0; i < 12; i++) leds[random16(NUM_LEDS)] = CHSV(random8(), 200, 200);  // pas encore de dessin
  FastLED.show();
  setRgb(isUnlocked(a) ? CRGB(CHSV(cursor * 32, 255, 255)) : CRGB(40, 40, 40));
}

void move(int d) {
  cursor = ((cursor + d) % ACT_COUNT + ACT_COUNT) % ACT_COUNT;
  play(SND_TICK);
  dirty = true;
}

}  // namespace

void menuBegin() {
  encBase = encoderCount();
  message = nullptr;
  dirty = true;
}

int menuLoop(uint32_t now) {
  static uint32_t lastFrame = 0;

  int32_t d = (encoderCount() - encBase) / 4;
  if (d) {
    encBase += d * 4;
    move(d);
  }
  float y = joyY();  // le joystick monte et descend dans la liste
  int8_t dir = y > 0.6f ? 1 : y < -0.6f ? -1 : 0;
  if (dir && dir != joyWas) move(dir);
  joyWas = dir;

  bool ok = btn1.pressed();
  ok |= btnEnc.pressed();
  btn2.pressed();  // le bouton du joystick ne sert pas dans le menu
  if (ok) {
    Activity a = (Activity)cursor;
    if (!isUnlocked(a)) {
      message = "Joue à l'Aventure !";
      play(SND_LOCKED);
    } else if (!isReady(a)) {
      message = "Bientôt !";
      play(SND_LOCKED);
    } else {
      play(SND_OK);
      return a;
    }
    messageUntil = now + 1500;
    dirty = true;
  }
  if (message && now >= messageUntil) { message = nullptr; dirty = true; }

  if (now - lastFrame >= 50) {
    lastFrame = now;
    renderMatrix(now);
  }
  if (dirty) { drawOled(now); dirty = false; }
  return -1;
}
