#pragma once
// Les morceaux du Cercle de rythme.
// Notation : « NOTE:LONGUEUR » séparés par des espaces (do4 = C4, dièse = #, ex. A#4 ; la longueur est en doubles croches :
// 2 = une croche, 4 = une noire, 8 = une blanche) ; « R:4 » = un silence.
// Après une note : « ! » = note dorée (il faut appuyer sur le bouton), « ~ » = note tournante (il faut tourner l'encodeur).
// « | » sépare les mesures (16 doubles croches chacune) : il ne sert qu'à relire. tools/check_songs.py vérifie les mesures.
#include <stdint.h>

struct Song { const char* name; uint16_t bpm; const char* notes; };

const Song SONGS[] = {
  // Composition originale : un petit air sautillant
  {"Mochi Mochi", 144,
   "C5:2 E5:2 C5:2 G5:2 E5:2 C5:2 G5:4 | A5:2 G5:2 E5:2 G5:2 A5:2 C6:2 A5:4 | F5:2 A5:2 F5:2 C6:2 A5:2 F5:2 C6:4 | B5:2 G5:2 D5:2 G5:2 B5:4 G5:4! | "
   "E5:2 G5:2 E5:2 C5:2 E5:2 G5:2 C6:4 | D5:2 F5:2 D5:2 A5:2 F5:2 D5:2 A5:4 | E5:2 G5:2 C6:2 B5:2 A5:2 G5:2 E5:2 C5:2 | D5:4 G5:4 C5:8~ | "
   "C5:2 R:2 E5:2 R:2 G5:2 R:2 C6:4 | B5:2 R:2 G5:2 R:2 D5:2 R:2 G5:4 | A5:2 G5:2 F5:2 E5:2 D5:2 E5:2 F5:2 G5:2 | C6:4 G5:4 E5:4 C5:4 | "
   "C5:2 E5:2 C5:2 G5:2 E5:2 C5:2 G5:4 | A5:2 G5:2 E5:2 G5:2 A5:2 C6:2 A5:4 | F5:2 A5:2 F5:2 C6:2 A5:2 F5:2 C6:4 | B5:2 G5:2 D5:2 G5:2 B5:4 G5:4! | "
   "C5:2 E5:2 G5:2 C6:2 C6:8~"},
  // La marche royale du château (jingle de l'histoire, développé)
  {"Marche royale", 108,
   "G4:4 G4:2 C5:6 G4:2 C5:2 | E5:4 E5:2 D5:6 C5:2 D5:2 | E5:4 E5:2 G5:6 E5:2 G5:2 | F5:4 E5:2 D5:6 C5:4! | "
   "A4:4 A4:2 E5:6 A4:2 E5:2 | G5:4 G5:2 F5:6 E5:2 F5:2 | G5:4 G5:2 C6:6 G5:2 E5:2 | D5:4 G5:4 B5:8~ | "
   "C5:4 C5:2 G5:6 C5:2 G5:2 | G5:4 G5:2 E5:6 D5:2 E5:2 | F5:4 F5:2 A5:6 F5:2 A5:2 | G5:4 E5:4 C5:8! | "
   "C5:2 E5:2 G5:2 C6:2 G5:2 E5:2 C5:2 G4:2 | C5:16~"},
  // La fête de la fin heureuse (jingle de l'histoire, développé)
  {"Fête des Couleurs", 132,
   "C5:2 E5:2 G5:2 E5:2 G5:4 C6:4 | B5:2 G5:2 E5:2 G5:2 B5:4 G5:4 | A5:2 F5:2 C5:2 F5:2 A5:4 F5:4 | G5:2 E5:2 D5:2 E5:2 C5:8! | "
   "E5:4 G5:4 C6:4 G5:4 | D5:4 G5:4 B5:4 G5:4 | C5:4 F5:4 A5:4 F5:4 | E5:2 G5:2 C6:2 B5:2 C6:8~ | "
   "A5:2 G5:2 F5:2 E5:2 F5:2 E5:2 D5:2 C5:2 | D5:2 E5:2 F5:2 G5:2 A5:2 G5:2 F5:2 E5:2 | E5:2 G5:2 C6:2 G5:2 E5:2 G5:2 C6:2 G5:2 | B5:4 G5:4 D5:4 G5:4 | "
   "C5:2 E5:2 G5:2 E5:2 G5:4 C6:4 | B5:2 G5:2 E5:2 G5:2 B5:4 G5:4 | A5:2 F5:2 C5:2 F5:2 A5:4 F5:4 | G5:2 E5:2 D5:2 B4:2 C5:8! | "
   "C5:4 E5:4 G5:4 C6:4 | C6:16~"},
  // L'hymne de la finale au stade (jingle de l'histoire, développé)
  {"Hymne du stade", 120,
   "G4:4 C5:4 E5:4 G5:4 | E5:4 G5:4 E5:4 C5:4 | F5:4 A5:4 F5:4 D5:4 | E5:4 D5:4 C5:8! | "
   "C5:4 E5:4 G5:4 C6:4 | B5:4 G5:4 E5:4 G5:4 | A5:4 F5:4 D5:4 F5:4 | G5:4 E5:4 C5:8~ | "
   "E5:2 F5:2 G5:2 A5:2 G5:4 E5:4 | F5:2 G5:2 A5:2 B5:2 A5:4 F5:4 | G5:2 A5:2 B5:2 C6:2 B5:2 A5:2 G5:2 E5:2 | C6:4 G5:4 C6:8~ | "
   "G4:4 C5:4 E5:4 G5:4 | E5:4 G5:4 E5:4 C5:4 | F5:4 A5:4 F5:4 D5:4 | E5:4 D5:4 C5:8! | "
   "C5:4 G4:4 C5:8~"},
  // Composition originale : la nuit sur le village gris
  {"Nuit à Pixelville", 90,
   "A4:4 C5:4 E5:8 | D5:4 C5:4 B4:8 | C5:4 B4:4 A4:8! | E5:4 D5:4 C5:4 B4:4 | "
   "A4:4 C5:4 E5:4 A5:4 | G5:4 E5:4 D5:4 C5:4 | B4:4 D5:4 F5:4 E5:4 | E5:4 D5:4 E5:8~ | "
   "A5:4 G5:4 E5:4 C5:4 | D5:4 F5:4 A5:4 G5:4 | E5:4 D5:4 C5:4 B4:4 | A4:4 C5:4 A4:8~"},
  // Composition originale : le combat contre le roi
  {"Le Roi Gluant", 156,
   "D5:2 F5:2 A5:2 F5:2 D5:2 F5:2 A5:4 | C5:2 E5:2 G5:2 E5:2 C5:2 E5:2 G5:4 | A#4:2 D5:2 F5:2 D5:2 A#4:2 D5:2 F5:4 | A4:2 C#5:2 E5:2 C#5:2 A4:2 C#5:2 E5:4! | "
   "D5:4 F5:4 A5:4 C6:4 | A#5:4 G5:4 E5:4 C#5:4 | D5:2 E5:2 F5:2 G5:2 A5:2 A#5:2 A5:2 G5:2 | A5:12~ R:4 | "
   "D5:2 F5:2 A5:2 F5:2 D5:2 F5:2 A5:4 | C5:2 E5:2 G5:2 E5:2 C5:2 E5:2 G5:4 | A#4:2 D5:2 F5:2 D5:2 A#4:2 D5:2 F5:4 | A4:2 C#5:2 E5:2 C#5:2 A4:2 C#5:2 E5:4! | "
   "D5:4 F5:4 A5:4 C6:4 | A#5:4 G5:4 E5:4 C#5:4 | D5:2 E5:2 F5:2 G5:2 A5:2 A#5:2 A5:2 G5:2 | D5:4 F5:4 A5:8~"},
};
constexpr int NUM_SONGS = sizeof(SONGS) / sizeof(SONGS[0]);
