// Progression de l'histoire et déblocage des activités (voir la section 4 bis du cahier des charges)
#include <Preferences.h>
#include <Arduino.h>
#include "save.h"

Progress progress = {0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0};
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
  progress.brightness   = prefs.getUChar("bright", 5);
  progress.runSteps     = prefs.getUShort("rsteps", 0);
  progress.runImpasses  = prefs.getUChar("rimp", 0);
  progress.sleepMode    = prefs.getUChar("sleep", 0);
  progress.raceBestPlace = prefs.getUChar("rplace", 0);
  progress.raceBestTime  = prefs.getUShort("rtime", 0);
  progress.mazeBest      = prefs.getUShort("mbest", 0);
  progress.difficulty    = min((int)prefs.getUChar("diff", 0), 2);
  String name = prefs.getString("player", "");
  strncpy(progress.player, name.c_str(), 3);
  progress.player[3] = 0;
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
  prefs.putUChar("bright", progress.brightness);
  prefs.putUShort("rsteps", progress.runSteps);
  prefs.putUChar("rimp", progress.runImpasses);
  prefs.putUChar("sleep", progress.sleepMode);
  prefs.putUChar("rplace", progress.raceBestPlace);
  prefs.putUShort("rtime", progress.raceBestTime);
  prefs.putUShort("mbest", progress.mazeBest);
  prefs.putUChar("diff", progress.difficulty);
  prefs.putString("player", progress.player);
  prefs.end();
}

void progressReset() {
  uint8_t bright = progress.brightness, sleep = progress.sleepMode;  // des réglages, pas une progression : gardés
  uint8_t diff = progress.difficulty;
  progress = {0, 0, 0, 0, 0, 0, 0, 0, bright, 0, 0, sleep};
  progress.difficulty = diff;
  progressSave();
}

void progressUnlockAll() {
  progress.charms = ALL_CHARMS | CHARM_PINCEAU;
  progress.trophies = ALL_TROPHIES;
  progress.endings = HAPPY_ENDS | END_LEGENDAIRE;
  progressSave();
}

bool isUnlocked(Activity a) {
  switch (a) {
    case ACT_ADVENTURE:
    case ACT_SCORES:
    case ACT_ABOUT:
    case ACT_SETTINGS:  return true;
    case ACT_MOOD:      return progress.charms & CHARM_BOL;
    case ACT_BREAKOUT:  return progress.charms & CHARM_FEUILLE;
    case ACT_JUKEBOX:   return progress.charms & CHARM_NOTE;
    case ACT_FOOTBALL:  return progress.trophies & TROPHY_BALLON;
    case ACT_PINBALL:   return progress.trophies & TROPHY_BILLE;
    case ACT_RACE:      return progress.trophies & TROPHY_VOLANT;
    case ACT_MAZE:      return progress.charms & CHARM_PINCEAU;
    case ACT_ATELIER:
    case ACT_GALLERY:   return progress.endings & HAPPY_ENDS;
    default:            return false;
  }
}

const char* activityName(Activity a) {
  static const char* const NAMES[ACT_COUNT] = {
    "Aventure", "Mood light", "Casse-briques", "Jukebox", "Football", "Flipper", "Course", "Labyrinthe", "Atelier", "Galerie", "Scores", "À propos", "Réglages"};
  return a < ACT_COUNT ? NAMES[a] : "";
}

const char* difficultyName() {
  static const char* const NAMES[] = {"facile", "moyen", "difficile"};
  return NAMES[progress.difficulty % 3];
}

int progressPercent() {
  int done = countBits(progress.charms & (ALL_CHARMS | CHARM_PINCEAU)) + countBits(progress.trophies & ALL_TROPHIES)
           + countBits(progress.endings & (HAPPY_ENDS | END_LEGENDAIRE));
  return done * 100 / 11;
}

int countBits(uint8_t v) {
  int n = 0;
  for (; v; v >>= 1) n += v & 1;
  return n;
}

// --- Meilleurs scores (espace « scores », jamais effacé par progressReset) ---
static Preferences scorePrefs;

int scoresLoad(ScoreEntry out[MAX_SCORES]) {
  scorePrefs.begin("scores", true);
  int n = scorePrefs.getUChar("n", 0);
  if (n > MAX_SCORES || scorePrefs.getBytes("tab", out, sizeof(ScoreEntry) * MAX_SCORES) != sizeof(ScoreEntry) * MAX_SCORES) n = 0;
  scorePrefs.end();
  return n;
}

int scoreRank(uint16_t score) {
  ScoreEntry tab[MAX_SCORES];
  int n = scoresLoad(tab);
  for (int i = 0; i < n; i++)
    if (score > tab[i].score) return i;
  return n < MAX_SCORES ? n : -1;
}

int scoreAdd(const char* name, uint16_t score, uint8_t ending) {
  ScoreEntry tab[MAX_SCORES];
  int n = scoresLoad(tab);
  int rank = scoreRank(score);
  if (rank < 0) return -1;
  for (int i = min(n, MAX_SCORES - 1); i > rank; i--) tab[i] = tab[i - 1];  // on décale les scores plus bas
  ScoreEntry e = {};
  strncpy(e.name, name, 3);
  e.score = score;
  e.ending = ending;
  tab[rank] = e;
  scorePrefs.begin("scores", false);
  scorePrefs.putBytes("tab", tab, sizeof(tab));
  scorePrefs.putUChar("n", min(n + 1, MAX_SCORES));
  scorePrefs.putString("last", name);
  scorePrefs.end();
  return rank;
}

void lastInitials(char out[4]) {
  scorePrefs.begin("scores", true);
  String s = scorePrefs.getString("last", "AAA");
  scorePrefs.end();
  strncpy(out, s.c_str(), 3);
  out[3] = 0;
}
