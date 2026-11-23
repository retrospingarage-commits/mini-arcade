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

// Combat final de l'Aventure contre le Roi Gluant ; charms = charmes obtenus (chacun donne un pouvoir).
void       bossBegin(uint8_t charms);
GameResult bossLoop(uint32_t now);

// Course manga : encodeur = gauche/droite pour doubler, bouton maintenu = turbo (se remplit en doublant).
// story = version de l'histoire (2 tours, rivales un peu plus lentes, il faut finir 1re).
void       raceBegin(bool story);
GameResult raceLoop(uint32_t now);

// Labyrinthe des sous-sols : joystick pour se déplacer de salle en salle dans le noir, trouver la clé puis
// le trésor caché derrière un faux mur. story = château de l'histoire (toujours le même, Pinceau Arc-en-ciel).
void       mazeBegin(bool story);
GameResult mazeLoop(uint32_t now);

// Le ramen du fantôme (Aventure) : attraper les ingrédients de la recette dans le bol, éviter le piment.
void       ramenBegin();
GameResult ramenLoop(uint32_t now);

// Concert de la grenouille DJ (Aventure) : jeu de rythme, rejouer la suite de commandes (bouton, encodeur, joystick).
void       rhythmBegin();
GameResult rhythmLoop(uint32_t now);

// « 1, 2, 3, soleil ! » (Aventure, le garde du château) : avancer au joystick, se figer quand le garde se retourne.
void       sunBegin();
GameResult sunLoop(uint32_t now);

void       moodBegin();      // Mood light : ambiances lumineuses
GameResult moodLoop(uint32_t now);
void       jukeboxBegin();   // Jukebox : réécouter les jingles de l'histoire
GameResult jukeboxLoop(uint32_t now);
void       atelierBegin();   // Atelier pixel art : dessiner depuis le téléphone par le Wi-Fi de la borne
GameResult atelierLoop(uint32_t now);
void       galleryBegin();   // Galerie : diaporama des dessins enregistrés
GameResult galleryLoop(uint32_t now);
void       aboutBegin();     // À propos : le Wi-Fi de la borne ouvre une page avec le guide pour IA (GUIDE-IA.md) à copier
void       atelierGuideBegin();  // (utilisé par aboutBegin) Wi-Fi allumé en mode « guide »
GameResult aboutLoop(uint32_t now);
void       scoresBegin();    // Meilleurs scores de l'Aventure
GameResult scoresLoop(uint32_t now);
