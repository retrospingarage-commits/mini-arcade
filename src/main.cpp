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
#include "activities.h"

static int  current = -1;  // activité en cours (Activity), -1 = menu
static bool inMenu = true;

static void openMenu() {
  inMenu = true;
  FastLED.setBrightness(BRIGHTNESS);
  menuBegin();
}

static void start(int a) {
  inMenu = false;
  current = a;
  switch (a) {
    case ACT_ADVENTURE: adventureBegin(); break;
    case ACT_MOOD:      moodBegin(); break;
    case ACT_BREAKOUT:  breakoutBegin(); break;
    case ACT_JUKEBOX:   jukeboxBegin(); break;
    case ACT_FOOTBALL:  footballBegin(false); break;
    case ACT_PINBALL:   pinballBegin(false); break;
    case ACT_ATELIER:   atelierBegin(); break;
    case ACT_GALLERY:   galleryBegin(); break;
    default:            openMenu(); break;
  }
}

// Fait tourner l'activité en cours ; faux quand elle est terminée
static bool runCurrent(uint32_t now) {
  switch (current) {
    case ACT_ADVENTURE: return adventureLoop(now);
    case ACT_BREAKOUT:  return breakoutLoop(now);
    case ACT_MOOD:      return moodLoop(now) == GAME_RUNNING;
    case ACT_JUKEBOX:   return jukeboxLoop(now) == GAME_RUNNING;
    case ACT_FOOTBALL:  return footballLoop(now) == GAME_RUNNING;
    case ACT_PINBALL:   return pinballLoop(now) == GAME_RUNNING;
    case ACT_ATELIER:   return atelierLoop(now) == GAME_RUNNING;
    case ACT_GALLERY:   return galleryLoop(now) == GAME_RUNNING;
    default:            return false;
  }
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
      case 'n': if (v >= 1 && v <= 6) { start(ACT_BREAKOUT); breakoutBegin((uint8_t)v - 1); } break;
      case 'a': if (v >= 1) { start(ACT_ADVENTURE); adventureGoto((uint8_t)v); } break;
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
  if (!oledAddress) {  // pas d'OLED : on le dit sur la matrice pendant 3 secondes
    for (uint32_t t0 = millis(), t; (t = millis() - t0) < 3000;) {
      FastLED.clear();
      drawScroll("OLED ?", t % scrollDuration("OLED ?"), 5, CRGB::Red);
      FastLED.show();
      delay(20);
    }
  }
  progressLoad();
  openMenu();
  Serial.printf("Pret (joystick %s). Commandes : g f m n a s u r (voir src/main.cpp)\n", joyOk ? "detecte" : "absent");
}

void loop() {
  uint32_t now = millis();
  handleSerial();
  updateSound(now);
  if (inMenu) {
    int a = menuLoop(now);
    if (a >= 0) start(a);
  } else if (!runCurrent(now)) {
    openMenu();
  }
}
