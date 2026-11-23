// Le Wi-Fi n'existe pas sur PC : l'Atelier est remplacé par une version vide dans le simulateur
#include "activities.h"
void atelierBegin() {}
void atelierGuideBegin() {}
GameResult atelierLoop(uint32_t) { return GAME_QUIT; }
