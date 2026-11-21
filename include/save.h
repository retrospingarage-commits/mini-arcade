#pragma once
// Progression de l'histoire, gardée dans la mémoire permanente (NVS) de l'ESP32
#include <stdint.h>

// Charmes (quêtes secondaires), trophées (défis) et fins, sous forme de bits
enum : uint8_t { CHARM_BOL = 1, CHARM_FEUILLE = 2, CHARM_NOTE = 4, ALL_CHARMS = 7 };
enum : uint8_t { TROPHY_BALLON = 1, TROPHY_BILLE = 2 };
enum : uint8_t { END_FETE = 1, END_PATISSIERE = 2, END_ARC_EN_CIEL = 4, END_LEGENDAIRE = 8, HAPPY_ENDS = 7 };

struct Progress {
  uint8_t scene;     // scène en cours de l'Aventure (0 = jamais commencée)
  uint8_t charms;
  uint8_t trophies;
  uint8_t endings;
  uint16_t pinballBest;  // meilleurs scores
  uint16_t footballBest;
  uint8_t  mood, moodLevel;  // dernière ambiance et luminosité de la Mood light
  uint8_t  brightness;       // luminosité de la matrice pour toute la borne
};
extern Progress progress;

void progressLoad();
void progressSave();
void progressReset();      // efface tout (nouvelle partie)
void progressUnlockAll();  // pour tester : tous les charmes, trophées et fins

// Activités du menu, dans l'ordre d'affichage
enum Activity : uint8_t {
  ACT_ADVENTURE, ACT_MOOD, ACT_BREAKOUT, ACT_JUKEBOX, ACT_FOOTBALL, ACT_PINBALL, ACT_ATELIER, ACT_GALLERY,
  ACT_SETTINGS,  // Réglages : géré par le menu lui-même (luminosité, recommencer, tout effacer)
  ACT_COUNT
};
bool        isUnlocked(Activity a);
const char* activityName(Activity a);
int         countBits(uint8_t v);
