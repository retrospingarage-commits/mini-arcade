// Mini-arcade : l'histoire « Les Couleurs du Roi Gluant » est au cœur du jeu.
// Elle débloque les activités du menu (voir la section 4 bis du cahier des charges et include/save.h).
// Commandes du moniteur série (115200) :
//   g <gain>  f <frottement>  m <vitesse max>  : ressenti de la raquette du casse-briques
//   n <niveau 1-6> : casse-briques à ce niveau    a <scène> : Aventure à cette scène
//   s : son oui/non    u : tout débloquer (test)    r : effacer la progression
#include "hw.h"
#include "save.h"
#include "menu.h"
#include "adventure.h"
#include "breakout.h"

enum App { APP_MENU, APP_ADVENTURE, APP_BREAKOUT };
static App app = APP_MENU;

static void openMenu() {
  app = APP_MENU;
  menuBegin();
}

static void handleSerial() {
  static String line;
  while (Serial.available()) {
    char c = Serial.read();
    if (c != '\n') { line += c; continue; }
    float v = line.substring(2).toFloat();
    switch (line[0]) {
      case 'g': gain = v; break;
      case 'f': friction = constrain(v, 0.80f, 0.999f); break;
      case 'm': vMax = v; break;
      case 'n': if (v >= 1 && v <= 6) { app = APP_BREAKOUT; breakoutBegin((uint8_t)v - 1); } break;
      case 'a': if (v >= 1) { app = APP_ADVENTURE; adventureBegin(); adventureGoto((uint8_t)v); } break;
      case 's': soundOn = !soundOn; if (!soundOn) ledcWriteTone(PIN_BUZZER, 0); break;
      case 'u': progressUnlockAll(); openMenu(); break;
      case 'r': progressReset(); openMenu(); break;
    }
    Serial.printf("gain=%.2f friction=%.3f vmax=%.0f son=%s joystick=%s charmes=%d trophees=%d fins=%d scene=%d\n",
                  gain, friction, vMax, soundOn ? "oui" : "non", joyOk ? "oui" : "non",
                  progress.charms, progress.trophies, progress.endings, progress.scene);
    line = "";
  }
}

void setup() {
  hwBegin();
  progressLoad();
  openMenu();
  Serial.printf("Pret (joystick %s). Commandes : g f m n a s u r (voir src/main.cpp)\n", joyOk ? "detecte" : "absent");
}

void loop() {
  uint32_t now = millis();
  handleSerial();
  updateSound(now);

  switch (app) {
    case APP_MENU: {
      int a = menuLoop(now);
      if (a == ACT_ADVENTURE)     { app = APP_ADVENTURE; adventureBegin(); }
      else if (a == ACT_BREAKOUT) { app = APP_BREAKOUT; breakoutBegin(); }
      break;
    }
    case APP_ADVENTURE: if (!adventureLoop(now)) openMenu(); break;
    case APP_BREAKOUT:  if (!breakoutLoop(now)) openMenu(); break;
  }
}
