#pragma once
// Dessins pixel art 16x16 de la matrice : une lettre par pixel, couleurs dans PALETTE ci-dessous.
// Pour ajouter un dessin : 16 lignes de 16 lettres, puis l'ajouter à la liste Picture.
#include "hw.h"

// Lettre -> couleur
inline CRGB paletteColor(char c) {
  switch (c) {
    case 'w': return CRGB(255, 255, 255);  // blanc
    case 'G': return CRGB(150, 150, 150);  // gris clair
    case 'g': return CRGB(80, 80, 80);     // gris
    case 'k': return CRGB(30, 30, 30);     // gris très foncé
    case 'r': return CRGB(255, 0, 0);      // rouge
    case 'o': return CRGB(255, 90, 0);     // orange
    case 'y': return CRGB(255, 200, 0);    // jaune
    case 'v': return CRGB(0, 200, 0);      // vert
    case 'V': return CRGB(0, 70, 0);       // vert foncé
    case 'b': return CRGB(0, 40, 255);     // bleu
    case 'c': return CRGB(80, 180, 255);   // bleu ciel
    case 'p': return CRGB(255, 60, 140);   // rose
    case 'm': return CRGB(110, 45, 10);    // marron
    case 'u': return CRGB(140, 0, 255);    // violet
    default:  return CRGB::Black;          // . = éteint
  }
}

enum Picture : uint8_t { PIC_CHAT, PIC_DODO, PIC_PLACE, PIC_FANTOME, PIC_BOL, PIC_TANUKI, PIC_FEUILLE, PIC_FLAMMES, PIC_CADENAS, PIC_BRIQUES, PIC_COUNT };

const char* const PICTURES[PIC_COUNT][16] = {
  {  // PIC_CHAT
    "................",
    "................",
    "...G........G...",
    "...GG......GG...",
    "...GGG....GGG...",
    "..GGGGGGGGGGGG..",
    "..GGGGGGGGGGGG..",
    "..GvGGGGGGGGvG..",
    "..GGGGGGGGGGGG..",
    "..GGGGGppGGGGG..",
    "..wGGGGGGGGGGw..",
    "...GGGGGGGGGG...",
    "....GGGGGGGG....",
    "................",
    "mmmmmmmmmmmmmmmm",
    "mmmmmmmmmmmmmmmm",
  },
  {  // PIC_DODO
    "...........ccc..",
    "............c...",
    "...G.......cccG.",
    "...GG......GG...",
    "...GGG....GGG...",
    "..GGGGGGGGGGGG..",
    "..GGGGGGGGGGGG..",
    "..GkGGGGGGGGkG..",
    "..GGGGGGGGGGGG..",
    "..GGGGGppGGGGG..",
    "..wGGGGGGGGGGw..",
    "...GGGGGGGGGG...",
    "....GGGGGGGG....",
    "................",
    "bbbbbbbbbbbbbbbb",
    "bbbbbbbbbbbbbbbb",
  },
  {  // PIC_PLACE
    "................",
    "................",
    "...k........k...",
    "..kkk......kkk..",
    ".kkkkk....kkkkk.",
    "..ggg......ggg..",
    "..gkg......gkg..",
    "..ggg..GG..ggg..",
    "..gkg..GG..gkg..",
    "..ggg.GGGG.ggg..",
    "......GGGG......",
    "GGGGGGGGGGGGGGGG",
    "gggggggggggggggg",
    "GgGgGgGgGgGgGgGg",
    "gggggggggggggggg",
    "GGGGGGGGGGGGGGGG",
  },
  {  // PIC_FANTOME
    "................",
    ".....wwwww......",
    "....wwwwwww.....",
    "...wwwwwwwww....",
    "...wwbwwwbww....",
    "...wwwwwwwww....",
    "...wwwwkwwww....",
    "...wwwwwwwww....",
    "...wwwwwwwww....",
    "...ww.www.ww....",
    "...w...w...w....",
    "................",
    "mmmmmmmmmmmmmmmm",
    "m..o...y...r...m",
    "m..............m",
    "m..............m",
  },
  {  // PIC_BOL
    "........y.......",
    "...y....y....y..",
    "....y.......y...",
    "................",
    "..y..w.w.w...y..",
    ".....w.w.w......",
    "................",
    ".oooooooooooooo.",
    ".oyyyyyyyyyyyyo.",
    "..oooooooooooo..",
    "...oooooooooo...",
    "....oooooooo....",
    ".....oooooo.....",
    "......mmmm......",
    "................",
    "................",
  },
  {  // PIC_TANUKI
    "VVV..........VVV",
    "VVVV........VVVV",
    "VV..m......m..VV",
    "V..mmm....mmm..V",
    "...mmmmmmmmmm...",
    "..mmmmmmmmmmmm..",
    "..mkkwmmmmwkkm..",
    "..mkkkmmmmkkkm..",
    "..mmmmmwwmmmmm..",
    "...mmmwkkwmmm...",
    "....mmwwwwmm....",
    ".....mmmmmm.....",
    "................",
    "vvvvvvvvvvvvvvvv",
    "VvVvVvVvVvVvVvVv",
    "VVVVVVVVVVVVVVVV",
  },
  {  // PIC_FEUILLE
    "...........y....",
    "..y.......vvv...",
    "........vvvvv...",
    ".......vvvvvvv..",
    "......vvvVvvvv..",
    ".....vvvVvvvvv..",
    "....vvvVvvvvvv..",
    "...vvvVvvvvvv...",
    "...vvVvvvvvv....",
    "..vvVvvvvvv.....",
    "..vVvvvvvv...y..",
    "..Vvvvvv........",
    ".V.vvv..........",
    "V...............",
    "....y...........",
    "................",
  },
  {  // PIC_FLAMMES
    "................",
    ".......r........",
    "......rr....r...",
    "..r...rro...rr..",
    "..rr.rroo..rro..",
    ".rro.rooy.rroo..",
    ".roo.royy.royy..",
    ".rooyrooyyrooy..",
    "rroyyrryyrroyyr.",
    "rooyyroyyroyyyrr",
    "royyyoyyyoyyyyor",
    "oyyyyyyyyyyyyyyo",
    "yyyywwyyyywwyyyy",
    "yywwwwwyywwwwwyy",
    "wwwwwwwwwwwwwwww",
    "wwwwwwwwwwwwwwww",
  },
  {  // PIC_CADENAS
    "................",
    "................",
    ".....GGGGGG.....",
    "....GG....GG....",
    "....G......G....",
    "....G......G....",
    "....G......G....",
    "..yyyyyyyyyyyy..",
    "..yyyyyyyyyyyy..",
    "..yyyyykkyyyyy..",
    "..yyyyykkyyyyy..",
    "..yyyyyykyyyyy..",
    "..yyyyyykyyyyy..",
    "..yyyyyyyyyyyy..",
    "................",
    "................",
  },
  {  // PIC_BRIQUES
    "................",
    "rrrrrrrrrrrrrrrr",
    "oooooooooooooooo",
    "yyyyyyyyyyyyyyyy",
    "vvvvvvvvvvvvvvvv",
    "cccccccccccccccc",
    "uuuuuuuuuuuuuuuu",
    "................",
    "................",
    "................",
    ".........w......",
    "................",
    "................",
    "................",
    "................",
    ".....GGGGGG.....",
  },
};

inline void drawPicture(Picture p) {
  for (int y = 0; y < 16; y++)
    for (int x = 0; x < 16; x++) setPixel(x, y, paletteColor(PICTURES[p][y][x]));
}
