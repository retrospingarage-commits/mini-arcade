#pragma once
// Moteur de l'Aventure : une scène à la fois, choix à l'encodeur ou au joystick, progression sauvegardée
#include <stdint.h>

void adventureBegin();                  // reprend à la scène sauvegardée (ou à la scène 1)
void adventureGoto(uint8_t sceneId);    // pour tester : saute à une scène
bool adventureLoop(uint32_t now);       // faux quand la joueuse revient au menu
int  wrapText(const char* s, char lines[][64], int maxLines);  // coupe un texte en lignes de 21 caractères
