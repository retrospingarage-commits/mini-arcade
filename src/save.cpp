// Progression de l'histoire et déblocage des activités (voir la section 4 bis du cahier des charges)
#include <Preferences.h>
#include "save.h"

Progress progress = {0, 0, 0, 0, 0, 0, 0, 0};
static Preferences prefs;

void progressLoad() {
  prefs.begin("arcade", true);
  progress.scene    = prefs.getUChar("scene", 0);
  progress.charms   = prefs.getUChar("charms", 0);
  progress.trophies = prefs.getUChar("trophies", 0);
  progress.endings  = prefs.getUChar("endings", 0);
  progress.pinballBest  = prefs.getUShort("pbest", 0);
  progress.footballBest = prefs.getUShort("fbest", 0);
  progress.mood         = prefs.getUChar("mood", 0);
  progress.moodLevel    = prefs.getUChar("moodlvl", 20);
  prefs.end();
}

void progressSave() {
  prefs.begin("arcade", false);
  prefs.putUChar("scene", progress.scene);
  prefs.putUChar("charms", progress.charms);
  prefs.putUChar("trophies", progress.trophies);
  prefs.putUChar("endings", progress.endings);
  prefs.putUShort("pbest", progress.pinballBest);
  prefs.putUShort("fbest", progress.footballBest);
  prefs.putUChar("mood", progress.mood);
  prefs.putUChar("moodlvl", progress.moodLevel);
  prefs.end();
}

void progressReset() {
  progress = {0, 0, 0, 0, 0, 0, 0, 0};
  progressSave();
}

void progressUnlockAll() {
  progress.charms = ALL_CHARMS;
  progress.trophies = TROPHY_BALLON | TROPHY_BILLE;
  progress.endings = HAPPY_ENDS | END_LEGENDAIRE;
  progressSave();
}

bool isUnlocked(Activity a) {
  switch (a) {
    case ACT_ADVENTURE: return true;
    case ACT_MOOD:      return progress.charms & CHARM_BOL;
    case ACT_BREAKOUT:  return progress.charms & CHARM_FEUILLE;
    case ACT_JUKEBOX:   return progress.charms & CHARM_NOTE;
    case ACT_FOOTBALL:  return progress.trophies & TROPHY_BALLON;
    case ACT_PINBALL:   return progress.trophies & TROPHY_BILLE;
    case ACT_ATELIER:
    case ACT_GALLERY:   return progress.endings & HAPPY_ENDS;
    default:            return false;
  }
}

const char* activityName(Activity a) {
  static const char* const NAMES[ACT_COUNT] = {
    "Aventure", "Mood light", "Casse-briques", "Jukebox", "Football", "Flipper", "Atelier", "Galerie"};
  return a < ACT_COUNT ? NAMES[a] : "";
}

int countBits(uint8_t v) {
  int n = 0;
  for (; v; v >>= 1) n += v & 1;
  return n;
}
