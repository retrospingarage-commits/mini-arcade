#pragma once
// Menu sur l'OLED : l'encodeur (ou le joystick) choisit, un bouton valide.
// Les activités pas encore gagnées dans l'histoire sont verrouillées.
#include <stdint.h>

void menuBegin();
int  menuLoop(uint32_t now);  // renvoie l'activité choisie (Activity), ou -1 tant que rien n'est choisi
