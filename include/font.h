#pragma once
// Police 3x5 pour la matrice : chaque caractère = 5 lignes de 3 bits (bit 2 = colonne de gauche)
#include <stdint.h>

const uint8_t FONT_LETTERS[26][5] = {
  {7, 5, 7, 5, 5},  // A
  {6, 5, 6, 5, 6},  // B
  {7, 4, 4, 4, 7},  // C
  {6, 5, 5, 5, 6},  // D
  {7, 4, 6, 4, 7},  // E
  {7, 4, 6, 4, 4},  // F
  {7, 4, 5, 5, 7},  // G
  {5, 5, 7, 5, 5},  // H
  {7, 2, 2, 2, 7},  // I
  {1, 1, 1, 5, 7},  // J
  {5, 5, 6, 5, 5},  // K
  {4, 4, 4, 4, 7},  // L
  {5, 7, 7, 5, 5},  // M
  {6, 5, 5, 5, 5},  // N
  {7, 5, 5, 5, 7},  // O
  {7, 5, 7, 4, 4},  // P
  {7, 5, 5, 7, 1},  // Q
  {6, 5, 6, 5, 5},  // R
  {7, 4, 7, 1, 7},  // S
  {7, 2, 2, 2, 2},  // T
  {5, 5, 5, 5, 7},  // U
  {5, 5, 5, 5, 2},  // V
  {5, 5, 7, 7, 5},  // W
  {5, 5, 2, 5, 5},  // X
  {5, 5, 2, 2, 2},  // Y
  {7, 1, 2, 4, 7},  // Z
};

const uint8_t FONT_DIGITS[10][5] = {
  {7, 5, 5, 5, 7}, {2, 6, 2, 2, 7}, {7, 1, 7, 4, 7}, {7, 1, 3, 1, 7}, {5, 5, 7, 1, 1},
  {7, 4, 7, 1, 7}, {7, 4, 7, 5, 7}, {7, 1, 1, 2, 2}, {7, 5, 7, 5, 7}, {7, 5, 7, 1, 7},
};

const uint8_t FONT_DASH[5]  = {0, 0, 7, 0, 0};
const uint8_t FONT_BANG[5]  = {2, 2, 2, 0, 2};
const uint8_t FONT_SPACE[5] = {0, 0, 0, 0, 0};

inline const uint8_t* glyph(char c) {
  if (c >= 'A' && c <= 'Z') return FONT_LETTERS[c - 'A'];
  if (c >= '0' && c <= '9') return FONT_DIGITS[c - '0'];
  if (c == '-') return FONT_DASH;
  if (c == '!') return FONT_BANG;
  return FONT_SPACE;
}
