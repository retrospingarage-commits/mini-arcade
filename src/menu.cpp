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

constexpr uint32_t SLEEP_MS = 180000;  // veille après 3 minutes sans toucher à rien dans le menu
uint32_t lastInput = 0;
bool     sleeping = false;

// Activités déjà programmées
bool isReady(Activity a) { return a < ACT_COUNT; }

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
  static const Picture ICONS[ACT_COUNT] = {PIC_CHAT, PIC_NONE, PIC_BRIQUES, PIC_NOTE, PIC_STADE, PIC_FLIPPER,
                                           PIC_NONE, PIC_BOCAUX};
  if (!isUnlocked(a))             drawPicture(PIC_CADENAS);
  else if (ICONS[a] != PIC_NONE)  drawPicture(ICONS[a]);
  else for (int i = 0; i < 12; i++) leds[random16(NUM_LEDS)] = CHSV(random8(), 200, 200);  // étincelles
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
  lastInput = millis();
  if (sleeping) { oled.setPowerSave(0); sleeping = false; }
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
  bool b2 = btn2.pressed();  // le bouton du joystick ne sert qu'à réveiller la borne
  bool input = d || dir || ok || b2;
  if (input) lastInput = now;
  if (sleeping) {  // en veille : n'importe quelle action rallume, sans rien choisir
    if (input) { sleeping = false; oled.setPowerSave(0); dirty = true; }
    return -1;
  }
  if (now - lastInput > SLEEP_MS) {  // veille : tout s'éteint
    sleeping = true;
    FastLED.clear(); FastLED.show();
    setRgb(CRGB::Black);
    oled.setPowerSave(1);
    return -1;
  }
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
