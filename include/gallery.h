#pragma once
// Dessins 16x16 de l'Atelier et de la Galerie : un numéro de couleur par pixel, enregistrés en mémoire permanente
#include "hw.h"

constexpr int PALETTE_SIZE  = 16;  // couleurs normales
constexpr int MAGIC_COLORS  = 4;   // couleurs inédites animées, débloquées par la fin légendaire
constexpr int MAX_DRAWINGS  = 20;  // au-delà, le plus ancien dessin est remplacé

extern uint8_t canvas[256];        // dessin en cours dans l'Atelier

int         availableColors();                                 // 16, ou 20 avec le mode secret
CRGB        drawingColor(uint8_t idx, int x, int y, uint32_t now);
const char* paletteCss(uint8_t idx);                           // couleur pour la page web
void        drawDrawing(const uint8_t* d, uint32_t now);       // affiche un dessin sur la matrice
int         galleryCount();
bool        galleryLoad(int i, uint8_t* out);                  // i = 0 pour le plus ancien
int         gallerySave(const uint8_t* in);                    // renvoie le nombre de dessins
