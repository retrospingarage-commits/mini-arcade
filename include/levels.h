#pragma once
// Les 6 niveaux du casse-briques, du plus facile au plus difficile.
// Grille de 8 x 7 briques de 2x1 pixels : '.' vide, '1' à '3' = nombre de coups, '#' incassable
#include <stdint.h>

constexpr uint8_t BRICK_COLS = 8;
constexpr uint8_t BRICK_ROWS = 7;
constexpr uint8_t METAL = 9;  // valeur d'une brique incassable dans la grille

enum Special : uint8_t {
  SP_NONE,
  SP_MOVING,   // les lignes de briques glissent de gauche à droite
  SP_DARK,     // les briques ne sont visibles qu'à la lumière de la balle
  SP_FALLING,  // le mur descend petit à petit vers la raquette
};

struct Level {
  const char* name;
  const char* rows[BRICK_ROWS];
  uint8_t     hue[3];  // teinte (FastLED) des briques à 1, 2 et 3 coups
  float       speed;   // vitesse de départ de la balle (pixels/s)
  uint8_t     paddle;  // largeur de la raquette (pixels)
  Special     special;
};

const Level LEVELS[] = {
  // VANESSA : un grand cœur, balle lente et grande raquette pour se mettre en jambes
  {"VANESSA",
   {"........",
    ".11..11.",
    "11111111",
    "11111111",
    ".111111.",
    "..1111..",
    "...11..."},
   {232, 0, 0}, 6.0f, 5, SP_NONE},

  // GUILLAUME : une forteresse de briques solides qu'il faut toucher deux fois
  {"GUILLAUME",
   {"22222222",
    "21111112",
    "21222212",
    "21211212",
    "21222212",
    "21111112",
    "22222222"},
   {160, 192, 0}, 7.0f, 5, SP_NONE},

  // LISA-LOUISE : les lignes glissent, une sur deux vers la droite, l'autre vers la gauche
  {"LISA-LOUISE",
   {"111..111",
    ".1.1.1.1",
    "22..22..",
    "1.1.1.1.",
    "2.2.2.2.",
    "11..11..",
    "..2..2.."},
   {64, 96, 0}, 8.0f, 4, SP_MOVING},

  // ANTOINE : des briques de métal incassables font rebondir la balle n'importe où
  {"ANTOINE",
   {"1#1111#1",
    "22222222",
    "#2.11.2#",
    "11111111",
    "..#11#..",
    "........",
    "........"},
   {32, 0, 0}, 9.0f, 4, SP_NONE},

  // LUCIE : dans le noir, seule la balle (une luciole) éclaire les briques autour d'elle
  {"LUCIE",
   {"11111111",
    "12222221",
    "12.33.21",
    "12.33.21",
    "12222221",
    "11111111",
    "........"},
   {192, 160, 128}, 10.0f, 4, SP_DARK},

  // EMILIE-ROSE : une rose qui descend vers la raquette, petite raquette et balle rapide
  {"EMILIE-ROSE",
   {"..3333..",
    ".322223.",
    "32233223",
    ".322223.",
    "..3333..",
    "...11...",
    "..1111.."},
   {96, 224, 250}, 11.0f, 3, SP_FALLING},
};

constexpr uint8_t NUM_LEVELS = sizeof(LEVELS) / sizeof(LEVELS[0]);
