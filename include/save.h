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
  uint16_t runSteps;         // partie d'Aventure en cours : scènes traversées depuis la scène 1
  uint8_t  runImpasses;      //   et impasses rencontrées (servent au score)
  uint8_t  sleepMode;        // veille du menu : 0 = 3 min, 1 = 10 min, 2 = 30 min, 3 = jamais
};
extern Progress progress;

void progressLoad();
void progressSave();
void progressReset();      // efface tout (nouvelle partie)
void progressUnlockAll();  // pour tester : tous les charmes, trophées et fins

// Activités du menu, dans l'ordre d'affichage
enum Activity : uint8_t {
  ACT_ADVENTURE, ACT_MOOD, ACT_BREAKOUT, ACT_JUKEBOX, ACT_FOOTBALL, ACT_PINBALL, ACT_ATELIER, ACT_GALLERY,
  ACT_SCORES,    // meilleurs scores de l'Aventure
  ACT_SETTINGS,  // Réglages : géré par le menu lui-même (luminosité, veille, recommencer, tout effacer)
  ACT_COUNT
};
bool        isUnlocked(Activity a);

// Meilleurs scores de l'Aventure : gardés à part, pour toute la vie de la borne (même « Tout effacer » les garde)
constexpr int MAX_SCORES = 5;
struct ScoreEntry { char name[4]; uint16_t score; uint8_t ending; };  // ending = la fin atteinte (END_…)
int  scoresLoad(ScoreEntry out[MAX_SCORES]);  // renvoie le nombre de scores enregistrés
int  scoreRank(uint16_t score);               // place (0 = premier) que prendrait ce score, -1 s'il n'entre pas
int  scoreAdd(const char* name, uint16_t score, uint8_t ending);  // renvoie sa place
void lastInitials(char out[4]);               // les dernières initiales tapées (pour gagner du temps)
const char* activityName(Activity a);
int         countBits(uint8_t v);
