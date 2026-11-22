// Menu sur l'OLED : l'encodeur (ou le joystick) choisit une activité, un bouton valide.
// Les activités pas encore gagnées dans l'Aventure sont verrouillées (cadenas).
#include "hw.h"
#include "menu.h"
#include "pictures.h"
#include "save.h"

namespace {

constexpr int VISIBLE_ITEMS = 4;  // lignes de menu visibles dans la partie bleue de l'OLED

const Note SND_TICK[]   = {{1200, 8}, {0, 0}};
const Note SND_LOCKED[] = {{150, 150}, {0, 0}};
const Note SND_OK[]     = {{880, 60}, {1320, 90}, {0, 0}};

uint8_t     cursor = 0;
int32_t     encBase = 0;
int8_t      joyWas = 0;
bool        dirty = true;
const char* message = nullptr;  // message affiché un instant (activité verrouillée ou pas encore prête)
bool        adjusting = false;  // réglage de la luminosité (bouton du joystick dans le menu)

// Réglages : la dernière ligne du menu
const char* const SETTINGS[] = {"Luminosité", "Veille", "Difficulté", "Rejouer l'histoire", "Tout effacer", "Retour"};
constexpr int     SETTINGS_COUNT = 6;
const char* const SLEEP_NAMES[] = {"3 min", "10 min", "30 min", "jamais"};
const uint32_t    SLEEP_DELAYS[] = {180000, 600000, 1800000, 0};  // 0 = jamais de veille
bool        inSettings = false;
uint8_t     setCursor = 0;
int8_t      confirming = -1;    // réglage qui attend un 2e appui pour confirmer (-1 = aucun)
uint32_t    messageUntil = 0;

uint32_t lastInput = 0;  // la veille du menu se règle dans Réglages (3, 10, 30 min ou jamais)
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
  oled.setCursor(0, 11);  // titre dans la bande jaune (16 lignes du haut)
  if (inSettings) {
    oled.print("Réglages");
    int top = constrain((int)setCursor - VISIBLE_ITEMS / 2, 0, SETTINGS_COUNT - VISIBLE_ITEMS);
    for (int i = 0; i < VISIBLE_ITEMS; i++) {
      int item = top + i, baseline = 26 + i * 12;
      if (item == setCursor) { oled.drawBox(0, baseline - 9, 128, 11); oled.setDrawColor(0); }
      oled.setCursor(4, baseline);
      oled.print(SETTINGS[item]);
      if (item == 1) { oled.print(" : "); oled.print(SLEEP_NAMES[progress.sleepMode % 4]); }
      if (item == 2) { oled.print(" : "); oled.print(difficultyName()); }
      oled.setDrawColor(1);
    }
  } else {
  oled.print("MENU");
  char pct[20];  // avancement de l'histoire, à droite de la bande jaune
  snprintf(pct, sizeof(pct), "Avancement %d%%", progressPercent());
  oled.setCursor(128 - oled.getUTF8Width(pct), 11);
  oled.print(pct);

  int top = constrain((int)cursor - VISIBLE_ITEMS / 2, 0, ACT_COUNT - VISIBLE_ITEMS);
  for (int i = 0; i < VISIBLE_ITEMS; i++) {
    Activity a = (Activity)(top + i);
    int baseline = 26 + i * 12;
    if (a == cursor) {  // ligne choisie en négatif
      oled.drawBox(0, baseline - 9, 128, 11);
      oled.setDrawColor(0);
    }
    oled.setCursor(4, baseline);
    oled.print(activityName(a));
    if (!isUnlocked(a)) drawLock(116, baseline);
    oled.setDrawColor(1);
  }
  }

  if (adjusting) {  // bulle du réglage de la luminosité
    oled.setDrawColor(0);
    oled.drawBox(6, 18, 116, 36);
    oled.setDrawColor(1);
    oled.drawFrame(6, 18, 116, 36);
    oled.setCursor(30, 31);
    oled.print("Luminosité");
    oled.drawFrame(14, 38, 100, 9);
    oled.drawBox(16, 40, (brightnessStep(matrixBrightness) + 1) * 96 / BRIGHTNESS_STEP_COUNT, 5);
  } else if (confirming >= 0) {  // une remise à zéro attend sa confirmation
    oled.setDrawColor(0);
    oled.drawBox(6, 18, 116, 36);
    oled.setDrawColor(1);
    oled.drawFrame(6, 18, 116, 36);
    oled.setCursor(12, 32); oled.print("Appuie encore pour");
    oled.setCursor(12, 45); oled.print("confirmer");
  } else if (message && now < messageUntil) {  // bulle par-dessus le menu
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
                                           PIC_KART, PIC_PINCEAU, PIC_NONE, PIC_BOCAUX, PIC_BALLON, PIC_CHAT, PIC_NONE};
  if (!isUnlocked(a))             drawPicture(PIC_CADENAS);
  else if (ICONS[a] != PIC_NONE)  drawPicture(ICONS[a]);
  else for (int i = 0; i < 12; i++) leds[random16(NUM_LEDS)] = CHSV(random8(), 200, 200);  // étincelles
  if (adjusting) {  // barre du niveau en bas de la matrice (visible même sans OLED)
    int n = brightnessStep(matrixBrightness) + 1;  // un pixel par cran
    for (int x = 0; x < 16; x++) setPixel(x, 15, x < n ? CRGB(255, 200, 0) : CRGB(30, 30, 30));
  }
  FastLED.show();
  setRgb(isUnlocked(a) ? CRGB(CHSV(cursor * 32, 255, 255)) : CRGB(40, 40, 40));
}

void move(int d) {
  if (inSettings) {
    setCursor = ((setCursor + d) % SETTINGS_COUNT + SETTINGS_COUNT) % SETTINGS_COUNT;
    confirming = -1;  // bouger annule une confirmation en attente
  } else {
    cursor = ((cursor + d) % ACT_COUNT + ACT_COUNT) % ACT_COUNT;
  }
  play(SND_TICK);
  dirty = true;
}

void chooseSetting(uint32_t now) {
  switch (setCursor) {
    case 0: adjusting = true; play(SND_TICK); break;
    case 1:  // veille : 3 min, 10 min, 30 min, jamais
      progress.sleepMode = (progress.sleepMode + 1) % 4;
      progressSave();
      play(SND_TICK);
      break;
    case 2:  // difficulté : facile, moyen, difficile
      progress.difficulty = (progress.difficulty + 1) % 3;
      progressSave();
      play(SND_TICK);
      break;
    case 3:
    case 4:
      if (confirming != setCursor) { confirming = setCursor; play(SND_LOCKED); break; }
      if (setCursor == 3) { progress.scene = 0; progressSave(); message = "Histoire au début !"; }
      else { progressReset(); cursor = 0; message = "Tout est effacé !"; }
      messageUntil = now + 2000;
      confirming = -1;
      inSettings = false;
      play(SND_OK);
      break;
    default: inSettings = false; play(SND_TICK); break;
  }
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
    if (adjusting) { setMatrixBrightness(brightnessShift(matrixBrightness, d)); dirty = true; }
    else move(d);
  }
  float y = joyY();  // le joystick monte et descend dans la liste
  int8_t dir = y > 0.6f ? 1 : y < -0.6f ? -1 : 0;
  if (dir && dir != joyWas && !adjusting) move(dir);
  joyWas = dir;

  bool ok = btn1.pressed();
  ok |= btnEnc.pressed();
  bool b2 = btn2.pressed();  // le bouton du joystick : réglage de la luminosité
  bool input = d || dir || ok || b2;
  if (input) lastInput = now;
  if (sleeping) {  // en veille : n'importe quelle action rallume, sans rien choisir
    if (input) { sleeping = false; oled.setPowerSave(0); dirty = true; }
    return -1;
  }
  uint32_t sleepDelay = SLEEP_DELAYS[progress.sleepMode % 4];
  if (sleepDelay && now - lastInput > sleepDelay) {  // veille : tout s'éteint
    sleeping = true;
    FastLED.clear(); FastLED.show();
    setRgb(CRGB::Black);
    oled.setPowerSave(1);
    return -1;
  }
  if (adjusting) {  // n'importe quel bouton valide et enregistre la luminosité
    if (ok || b2) {
      adjusting = false;
      progress.brightness = matrixBrightness;
      progressSave();
      play(SND_OK);
      dirty = true;
    }
  } else if (inSettings) {  // bouton du joystick : sortir des réglages
    if (b2) { inSettings = false; confirming = -1; dirty = true; }
    else if (ok) chooseSetting(now);
  } else if (b2) {  // bouton du joystick : réglage de la luminosité
    adjusting = true;
    play(SND_TICK);
    dirty = true;
  } else if (ok) {
    Activity a = (Activity)cursor;
    if (a == ACT_SETTINGS) {
      inSettings = true; setCursor = 0; confirming = -1;
      play(SND_TICK);
      dirty = true;
    } else if (!isUnlocked(a)) {
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
