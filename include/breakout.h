#pragma once
// Casse-briques (activité débloquée par la Feuille Magique)
#include <stdint.h>

extern float gain, friction, vMax;  // ressenti de la raquette, réglable par le moniteur série

void breakoutBegin(uint8_t level = 0);  // nouvelle partie au niveau donné (0 à 5)
void breakoutBeginStory();              // défi du tanuki dans l'Aventure (un niveau, 3 vies)
bool breakoutWon();                     // après la fin de la partie : vrai si gagnée
bool breakoutLoop(uint32_t now);        // faux quand la joueuse revient au menu
