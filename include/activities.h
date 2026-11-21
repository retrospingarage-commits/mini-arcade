#pragma once
// Toutes les activités du menu. Chacune a une fonction xxxBegin() et une fonction xxxLoop(now) appelée à chaque
// tour de loop() ; la boucle renvoie GAME_RUNNING tant que l'activité continue.
#include <stdint.h>

enum GameResult : uint8_t { GAME_RUNNING, GAME_WON, GAME_LOST, GAME_QUIT };

// Football manga : encodeur ou joystick pour se déplacer, bouton à maintenir pour charger le tir,
// encodeur pendant le tir pour le courber. story = version courte de l'histoire (3 buts en 5 tirs).
void       footballBegin(bool story);
GameResult footballLoop(uint32_t now);

// Flipper manga : bouton = flipper gauche, joystick (n'importe quelle action) = flipper droit,
// tourner l'encodeur = secouer la table (plus c'est rapide, plus c'est fort). story = battre le record du vieux flipper.
void       pinballBegin(bool story);
GameResult pinballLoop(uint32_t now);

void       moodBegin();      // Mood light : ambiances lumineuses
GameResult moodLoop(uint32_t now);
void       jukeboxBegin();   // Jukebox : réécouter les jingles de l'histoire
GameResult jukeboxLoop(uint32_t now);
void       atelierBegin();   // Atelier pixel art : dessiner depuis le téléphone par le Wi-Fi de la borne
GameResult atelierLoop(uint32_t now);
void       galleryBegin();   // Galerie : diaporama des dessins enregistrés
GameResult galleryLoop(uint32_t now);
