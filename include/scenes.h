#pragma once
// Les scènes de l'Aventure « Les Couleurs du Roi Gluant » (section 6 du cahier des charges).
// Une scène = un texte (100 caractères max), 1 à 4 choix (20 caractères max), un dessin, une couleur de LED,
// un jingle, et éventuellement une récompense donnée en arrivant dans la scène.
// Une cible de choix qui n'existe pas encore mène à une scène « bientôt » qui ramène en arrière.
#include "hw.h"
#include "pictures.h"
#include "save.h"

// Condition pour qu'un choix soit visible
enum Cond : uint8_t {
  ALWAYS,
  IF_ALL_CHARMS,   // « si 3 charmes »
  UNLESS_BOL,      // disparaît une fois le charme ou le trophée obtenu
  UNLESS_FEUILLE,
  UNLESS_NOTE,
  UNLESS_BALLON,
  UNLESS_BILLE,
  UNLESS_MARKET_DONE,  // le marché (fantôme et flipper) disparaît quand ses deux récompenses sont obtenues
};

struct Choice {
  const char* label;
  uint8_t     target;
  Cond        cond;
};

struct Scene {
  uint8_t     id;
  const char* text;
  Choice      choices[4];  // label nullptr = pas de choix
  Picture     picture;
  uint32_t    led;         // couleur de la LED RVB (0xRRGGBB)
  const Note* jingle;
  uint8_t     charm, trophy, ending;  // récompense en arrivant (0 = aucune)
};

// --- Jingles ----------------------------------------------------------------
const Note J_BAILLEMENT[] = {{523, 150}, {494, 150}, {440, 150}, {392, 400}, {0, 0}};
const Note J_CALME[]      = {{392, 200}, {523, 200}, {659, 200}, {523, 400}, {0, 0}};
const Note J_SOUFFLE[]    = {{300, 80}, {280, 80}, {260, 80}, {240, 300}, {0, 0}};
const Note J_FANFARE[]    = {{523, 100}, {659, 100}, {784, 100}, {1047, 150}, {0, 50}, {1047, 300}, {0, 0}};
const Note J_TAMBOUR[]    = {{150, 60}, {0, 60}, {150, 60}, {0, 60}, {200, 120}, {0, 0}};
const Note J_POUF[]       = {{800, 40}, {400, 60}, {200, 200}, {0, 0}};
const Note J_RONFLEMENT[] = {{110, 400}, {0, 200}, {165, 300}, {0, 0}};
const Note J_BIENTOT[]    = {{659, 100}, {784, 100}, {0, 0}};

// --- Scènes -----------------------------------------------------------------
const Scene SCENES[] = {
  {1, "Pixelville est devenue toute grise ! Ton chat Mochi soupire : Debout, héroïne !",
   {{"Sortir du lit", 2, ALWAYS}, {"Se rendormir", 25, ALWAYS}},
   PIC_CHAT, 0x505050, J_BAILLEMENT, 0, 0, 0},

  {2, "Place du village, sans couleur. Où aller ?",
   {{"Marché", 3, UNLESS_MARKET_DONE}, {"Forêt", 5, UNLESS_FEUILLE}, {"Lac", 7, UNLESS_NOTE},
    {"Route du château", 9, ALWAYS}},
   PIC_PLACE, 0x808080, J_CALME, 0, 0, 0},

  {3, "Un fantôme affamé flotte : J'ai faim ! Un ramen, vite !",
   {{"Cuisiner un ramen", 4, UNLESS_BOL}, {"Tout le piment", 15, ALWAYS}, {"Salle de flipper", 28, UNLESS_BILLE},
    {"Retour au village", 2, ALWAYS}},
   PIC_FANTOME, 0x60A0FF, J_SOUFFLE, 0, 0, 0},

  {4, "Slurp ! Le fantôme est ravi et t'offre le Bol Magique. Charme obtenu !",
   {{"Retour au village", 2, ALWAYS}},
   PIC_BOL, 0xFF5A00, J_FANFARE, CHARM_BOL, 0, 0},

  {5, "Un tanuki te défie à pierre-feuille-ciseaux. Si tu gagnes, cadeau !",
   {{"Relever le défi", 6, ALWAYS}, {"Refuser poliment", 2, ALWAYS}},
   PIC_TANUKI, 0x00C000, J_TAMBOUR, 0, 0, 0},

  {6, "Tu gagnes avec les ciseaux, car le tanuki les avait oubliés ! Feuille Magique obtenue !",
   {{"Retour au village", 2, ALWAYS}},
   PIC_FEUILLE, 0x00FF00, J_FANFARE, CHARM_FEUILLE, 0, 0},

  {15, "IMPASSE. Le fantôme crache des flammes ! Tu t'évanouis... c'était un rêve ?",
   {{"Se réveiller", 1, ALWAYS}},
   PIC_FLAMMES, 0xFF0000, J_POUF, 0, 0, 0},

  {25, "IMPASSE. Tu dors 100 ans. Mochi a mangé ton goûter. Quel réveil !",
   {{"Se réveiller", 1, ALWAYS}},
   PIC_DODO, 0x000830, J_RONFLEMENT, 0, 0, 0},
};

constexpr uint8_t NUM_SCENES = sizeof(SCENES) / sizeof(SCENES[0]);
