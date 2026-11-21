#pragma once
// Les scènes de l'Aventure « Les Couleurs du Roi Gluant » (section 6 du cahier des charges).
// Une scène = un texte (100 caractères max ; « Titre|texte » affiche un titre centré dans la bande jaune, 5 lignes de 21 caractères à l'écran), 1 à 5 choix (20 caractères
// max, 4 visibles au plus à la fois), un dessin, un effet animé, une couleur de LED ou une « chaleur »,
// un jingle, une récompense éventuelle et un mini-jeu éventuel.
// Une cible qui n'existe pas encore mène à une scène « bientôt » qui ramène en arrière.
#include "hw.h"
#include "pictures.h"
#include "save.h"

// Condition pour qu'un choix soit visible
enum Cond : uint8_t {
  ALWAYS,
  IF_ALL_CHARMS,       // « si 3 charmes »
  IF_TWO_CHARMS,       // au moins 2 charmes (pour briser les sceaux du château)
  UNLESS_TWO_CHARMS,
  IF_LEGEND,           // 3 charmes et 3 trophées (portail de la fin légendaire)
  UNLESS_BOL,          // disparaît une fois le charme ou le trophée obtenu
  UNLESS_FEUILLE,
  UNLESS_NOTE,
  UNLESS_BALLON,
  UNLESS_BILLE,
  UNLESS_VOLANT,
  UNLESS_PINCEAU,
  UNLESS_MARKET_DONE,  // le marché disparaît de la place quand le Bol et la Bille Étoile sont obtenus
};

// Effet animé dessiné par-dessus le dessin (ou seul, avec PIC_NONE)
enum Effect : uint8_t { FX_NONE, FX_DARK, FX_FOG, FX_WAVES, FX_RAIN, FX_DRIP, FX_BOUNCE, FX_CONFETTI, FX_RAINBOW,
                        FX_STARS, FX_PORTAL, FX_FIREWORKS };

// Mini-jeu lancé par un choix dont la cible est GAME
enum MiniGame : uint8_t { GAME_NONE, GAME_REFLEX, GAME_FOOTBALL, GAME_PINBALL, GAME_BREAKOUT, GAME_BOSS, GAME_RACE, GAME_MAZE };
constexpr uint8_t GAME = 250;

struct Choice {
  const char* label;
  uint8_t     target;
  Cond        cond;
};

struct Scene {
  uint8_t     id;
  const char* text;
  Choice      choices[5];  // label nullptr = pas de choix
  Picture     picture;
  Effect      effect;
  uint32_t    led;         // couleur de la LED RVB (0xRRGGBB), utilisée si heat < 0
  int8_t      heat;        // 0 (froid, bleu) à 4 (brûlant, orange) : on se rapproche du but ; -1 = couleur led
  const Note* jingle;
  uint8_t     charm, trophy, ending;  // récompense en arrivant (0 = aucune)
  MiniGame    game;
  uint8_t     winTarget, loseTarget;  // scènes après le mini-jeu
};

// --- Jingles ----------------------------------------------------------------
const Note J_BAILLEMENT[] = {{523, 150}, {494, 150}, {440, 150}, {392, 400}, {0, 0}};
const Note J_CALME[]      = {{392, 200}, {523, 200}, {659, 200}, {523, 400}, {0, 0}};
const Note J_PAS[]        = {{330, 80}, {0, 40}, {392, 80}, {0, 40}, {330, 80}, {0, 0}};
const Note J_MYSTERE[]    = {{220, 200}, {233, 200}, {220, 200}, {208, 400}, {0, 0}};
const Note J_RONDS[]      = {{523, 80}, {392, 80}, {523, 80}, {392, 80}, {523, 80}, {0, 0}};
const Note J_HIBOU[]      = {{392, 250}, {0, 100}, {330, 400}, {0, 0}};
const Note J_SOUFFLE[]    = {{300, 80}, {280, 80}, {260, 80}, {240, 300}, {0, 0}};
const Note J_FANFARE[]    = {{523, 100}, {659, 100}, {784, 100}, {1047, 150}, {0, 50}, {1047, 300}, {0, 0}};
const Note J_TAMBOUR[]    = {{150, 60}, {0, 60}, {150, 60}, {0, 60}, {200, 120}, {0, 0}};
const Note J_BASSE[]      = {{82, 150}, {0, 50}, {82, 150}, {0, 50}, {110, 250}, {0, 0}};
const Note J_MELODIE[]    = {{659, 120}, {784, 120}, {880, 120}, {784, 120}, {659, 300}, {0, 0}};
const Note J_VENT[]       = {{200, 300}, {250, 300}, {200, 400}, {0, 0}};
const Note J_ROULEMENT[]  = {{180, 40}, {0, 20}, {180, 40}, {0, 20}, {180, 40}, {0, 20}, {240, 200}, {0, 0}};
const Note J_MARCHE[]     = {{392, 200}, {392, 100}, {523, 300}, {392, 200}, {523, 400}, {0, 0}};
const Note J_TINTEMENT[]  = {{1568, 80}, {2093, 120}, {0, 0}};
const Note J_TROMBONE[]   = {{311, 250}, {294, 250}, {277, 250}, {262, 600}, {0, 0}};
const Note J_BOING[]      = {{200, 60}, {400, 60}, {800, 80}, {0, 0}};
const Note J_POUF[]       = {{800, 40}, {400, 60}, {200, 200}, {0, 0}};
const Note J_PLOUF[]      = {{600, 40}, {300, 80}, {150, 200}, {0, 0}};
const Note J_RONFLEMENT[] = {{110, 400}, {0, 200}, {165, 300}, {0, 0}};
const Note J_FETE[]       = {{523, 100}, {659, 100}, {784, 100}, {659, 100}, {784, 100}, {1047, 300}, {0, 0}};
const Note J_GOURMAND[]   = {{784, 120}, {659, 120}, {784, 120}, {1047, 300}, {0, 0}};
const Note J_HARPE[]      = {{523, 70}, {659, 70}, {784, 70}, {1047, 70}, {1319, 70}, {1568, 300}, {0, 0}};
const Note J_SPECIAL[]    = {{1047, 80}, {1319, 80}, {1568, 80}, {2093, 200}, {0, 60}, {2093, 200}, {0, 0}};
const Note J_ORGUE[]      = {{262, 300}, {330, 300}, {392, 300}, {523, 500}, {0, 0}};
const Note J_LEGENDE[]    = {{523, 150}, {659, 150}, {784, 150}, {1047, 300}, {988, 150}, {1047, 150}, {1175, 150},
                             {1319, 500}, {0, 0}};
const Note J_SIFFLET[]    = {{2000, 150}, {0, 80}, {2000, 400}, {0, 0}};
const Note J_HYMNE[]      = {{392, 150}, {523, 150}, {659, 150}, {784, 300}, {659, 150}, {784, 400}, {0, 0}};
const Note J_TILT[]       = {{300, 80}, {0, 40}, {300, 80}, {0, 0}};
const Note J_DING[]       = {{1319, 80}, {0, 40}, {1319, 80}, {1760, 200}, {0, 0}};
const Note J_BIENTOT[]    = {{659, 100}, {784, 100}, {0, 0}};

// --- Scènes (1 à 29 : le cahier des charges ; 30 et plus : chemins d'errance) -----------------------
const Scene SCENES[] = {
  {1, "Pixelville est devenue toute grise ! Ton chat Mochi soupire : Debout, ~ !",
   {{"Sortir du lit", 2, ALWAYS}, {"Se rendormir", 25, ALWAYS}},
   PIC_CHAT, FX_NONE, 0x505050, -1, J_BAILLEMENT, 0, 0, 0, GAME_NONE, 0, 0},
  {2, "Place du village, sans couleur. Où aller ?",
   {{"Marché", 30, UNLESS_MARKET_DONE}, {"Forêt", 32, UNLESS_FEUILLE}, {"Lac", 35, UNLESS_NOTE}, {"Route du château", 38, ALWAYS}, {"Raccourci château", 9, IF_ALL_CHARMS}},
   PIC_PLACE, FX_NONE, 0x808080, -1, J_CALME, 0, 0, 0, GAME_NONE, 0, 0},
  {3, "Un fantôme affamé flotte : J'ai faim ! Un ramen, vite !",
   {{"Cuisiner un ramen", 4, UNLESS_BOL}, {"Tout le piment", 15, ALWAYS}, {"Retour à la rue", 30, ALWAYS}},
   PIC_FANTOME, FX_NONE, 0x60A0FF, -1, J_SOUFFLE, 0, 0, 0, GAME_NONE, 0, 0},
  {4, "Slurp ! Le fantôme est ravi et t'offre le Bol Magique. Charme obtenu !",
   {{"Retour au village", 2, ALWAYS}},
   PIC_BOL, FX_NONE, 0xFF5A00, -1, J_FANFARE, CHARM_BOL, 0, 0, GAME_NONE, 0, 0},
  {5, "Un tanuki te défie : casse toutes ses briques-feuilles ! Si tu gagnes, cadeau !",
   {{"Relever le défi", GAME, ALWAYS}, {"Refuser poliment", 33, ALWAYS}},
   PIC_TANUKI, FX_NONE, 0x00C000, -1, J_TAMBOUR, 0, 0, 0, GAME_BREAKOUT, 6, 5},
  {6, "Bravo ! Le tanuki, beau joueur, t'offre la Feuille Magique. Charme obtenu !",
   {{"Retour au village", 2, ALWAYS}},
   PIC_FEUILLE, FX_NONE, 0x00FF00, -1, J_FANFARE, CHARM_FEUILLE, 0, 0, GAME_NONE, 0, 0},
  {7, "Une grenouille DJ cherche une voix pour son concert. Tu chantes ?",
   {{"Chanter", 8, ALWAYS}, {"La pousser à l'eau", 16, ALWAYS}, {"Passer son chemin", 35, ALWAYS}},
   PIC_GRENOUILLE, FX_NONE, 0x8C00FF, -1, J_BASSE, 0, 0, 0, GAME_NONE, 0, 0},
  {8, "Tu chantes faux, mais tout le monde adore ! Note Magique obtenue !",
   {{"Retour au village", 2, ALWAYS}},
   PIC_NOTE, FX_NONE, 0xFFC800, -1, J_MELODIE, CHARM_NOTE, 0, 0, GAME_NONE, 0, 0},
  {9, "Le château de mochi se dresse, tout gluant. Sa porte est fermée par 3 sceaux de couleur.",
   {{"Briser les sceaux", 10, IF_TWO_CHARMS}, {"Toucher les sceaux", 45, UNLESS_TWO_CHARMS}, {"Aux sous-sols", 53, UNLESS_PINCEAU}, {"Retour au village", 2, ALWAYS}},
   PIC_CHATEAU, FX_NONE, 0xFFFFFF, -1, J_VENT, 0, 0, 0, GAME_NONE, 0, 0},
  {10, "Un garde mochi bloque la porte : Fais-moi rire ou rebrousse chemin !",
   {{"Raconter une blague", 11, ALWAYS}, {"Foncer dans la porte", 17, ALWAYS}},
   PIC_GARDE, FX_NONE, 0xFFFFFF, -1, J_ROULEMENT, 0, 0, 0, GAME_NONE, 0, 0},
  {11, "Le Roi Gluant trône près d'un gâteau géant. C'est lui, le voleur de couleurs ! En garde !",
   {{"Combattre !", GAME, ALWAYS}, {"Fuir", 9, ALWAYS}},
   PIC_ROI, FX_NONE, 0xFF3C8C, -1, J_MARCHE, 0, 0, 0, GAME_BOSS, 46, 14},
  {12, "Cuisine secrète ! Les bocaux de couleurs volées brillent sur l'étagère.",
   {{"Ouvrir les bocaux", 20, ALWAYS}, {"Retour au roi", 46, ALWAYS}},
   PIC_BOCAUX, FX_NONE, 0xFF60FF, -1, J_TINTEMENT, 0, 0, 0, GAME_NONE, 0, 0},
  {13, "Le roi sanglote : Personne ne vient jamais à mon anniversaire ! Snif.",
   {{"Inviter le village", 18, ALWAYS}, {"Hausser les épaules", 46, ALWAYS}},
   PIC_ROI, FX_RAIN, 0x0028FF, -1, J_TROMBONE, 0, 0, 0, GAME_NONE, 0, 0},
  {14, "IMPASSE. Le roi t'a engluée dans le mochi ! Tout devient blanc... Réveille-toi !",
   {{"Se réveiller", 1, ALWAYS}},
   PIC_NONE, FX_DRIP, 0xFFFFFF, -1, J_BOING, 0, 0, 0, GAME_NONE, 0, 0},
  {15, "IMPASSE. Le fantôme crache des flammes ! Tu t'évanouis... c'était un rêve ?",
   {{"Se réveiller", 1, ALWAYS}},
   PIC_FLAMMES, FX_NONE, 0xFF0000, -1, J_POUF, 0, 0, 0, GAME_NONE, 0, 0},
  {16, "IMPASSE. Plouf ! La grenouille te lance dans le lac. Glou glou... Réveille-toi !",
   {{"Se réveiller", 1, ALWAYS}},
   PIC_NONE, FX_WAVES, 0x0028FF, -1, J_PLOUF, 0, 0, 0, GAME_NONE, 0, 0},
  {17, "IMPASSE. Boing ! Tu rebondis sur le ventre du garde jusqu'à ton lit.",
   {{"Se réveiller", 1, ALWAYS}},
   PIC_GARDE, FX_BOUNCE, 0xFFFFFF, -1, J_BOING, 0, 0, 0, GAME_NONE, 0, 0},
  {18, "La Fête des Couleurs|Tout le village danse à l'anniversaire du roi, qui rend les couleurs !",
   {{"Rejouer", 47, ALWAYS}},
   PIC_NONE, FX_CONFETTI, 0xFFFFFF, -1, J_FETE, 0, 0, END_FETE, GAME_NONE, 0, 0},
  {19, "La Cheffe pâtissière|Divin ! Le roi rend les couleurs et t'engage : Cheffe pâtissière royale !",
   {{"Rejouer", 47, ALWAYS}},
   PIC_GATEAU, FX_NONE, 0xFF3C8C, -1, J_GOURMAND, 0, 0, END_PATISSIERE, GAME_NONE, 0, 0},
  {20, "La Pluie arc-en-ciel|Les bocaux s'ouvrent : les couleurs filent sur tout Pixelville !",
   {{"Rejouer", 47, ALWAYS}},
   PIC_NONE, FX_RAINBOW, 0xFFFFFF, -1, J_HARPE, 0, 0, END_ARC_EN_CIEL, GAME_NONE, 0, 0},
  {21, "WIZARD MODE ! Tes trésors brillent. Un portail d'étoiles s'ouvre. Mochi : Prépare-toi, ~ !",
   {{"Entrer", 22, ALWAYS}, {"Pas prête", 46, ALWAYS}},
   PIC_NONE, FX_PORTAL, 0x8C00FF, -1, J_SPECIAL, 0, 0, 0, GAME_NONE, 0, 0},
  {22, "Le gardien des étoiles demande : Qu'est-ce qui rend un gâteau meilleur ?",
   {{"Le partage", 23, ALWAYS}, {"Plus de sucre", 21, ALWAYS}},
   PIC_NONE, FX_STARS, 0x8C00FF, -1, J_ORGUE, 0, 0, 0, GAME_NONE, 0, 0},
  {23, "Le Mochi-Mage lance 3 étoiles. Appuie sur le bouton quand la matrice devient verte !",
   {{"Je suis prête !", GAME, ALWAYS}},
   PIC_NONE, FX_STARS, 0x8C00FF, -1, J_SPECIAL, 0, 0, 0, GAME_REFLEX, 24, 21},
  {24, "La Légende étoilée|Le Mochi-Mage devient ton ami et invente des couleurs jamais vues !",
   {{"Rejouer", 47, ALWAYS}},
   PIC_NONE, FX_FIREWORKS, 0xFFFFFF, -1, J_LEGENDE, 0, 0, END_LEGENDAIRE, GAME_NONE, 0, 0},
  {25, "IMPASSE. Tu dors 100 ans. Mochi a mangé ton goûter. Quel réveil !",
   {{"Se réveiller", 1, ALWAYS}},
   PIC_DODO, FX_NONE, 0x000830, -1, J_RONFLEMENT, 0, 0, 0, GAME_NONE, 0, 0},
  {26, "Le capitaine lapin des Mochi FC cherche une buteuse pour la finale. Tu joues ?",
   {{"Jouer le match", GAME, ALWAYS}, {"Retour à la route", 38, ALWAYS}},
   PIC_STADE, FX_NONE, 0x00C000, -1, J_SIFFLET, 0, 0, 0, GAME_FOOTBALL, 27, 26},
  {27, "BUT ! Les Mochi FC gagnent. Ballon d'Or obtenu : le football est débloqué !",
   {{"Retour à la route", 38, ALWAYS}},
   PIC_BALLON, FX_NONE, 0xFFB400, -1, J_HYMNE, 0, TROPHY_BALLON, 0, GAME_NONE, 0, 0},
  {28, "Un vieux flipper gris clignote : Bats mon record, je te rends mes couleurs !",
   {{"Jouer", GAME, ALWAYS}, {"Retour à la rue", 30, ALWAYS}},
   PIC_FLIPPER, FX_NONE, 0xFF3C8C, -1, J_TILT, 0, 0, 0, GAME_PINBALL, 29, 28},
  {29, "Le flipper s'illumine ! Bille Étoile obtenue : le flipper est débloqué !",
   {{"Retour à la rue", 30, ALWAYS}},
   PIC_FLIPPER, FX_CONFETTI, 0xFFFFFF, -1, J_DING, 0, TROPHY_BILLE, 0, GAME_NONE, 0, 0},
  {30, "Rue des étals, toute grise. Ça sent le bouillon... et plus loin, ça fait ding-ding !",
   {{"Suivre l'odeur", 3, UNLESS_BOL}, {"Suivre les ding-ding", 28, UNLESS_BILLE}, {"La ruelle sombre", 31, ALWAYS}, {"Retour au village", 2, ALWAYS}},
   PIC_ETALS, FX_NONE, 0x000000, 2, J_PAS, 0, 0, 0, GAME_NONE, 0, 0},
  {31, "Mochi : Raccourci, ~ ! Trois chats et deux poubelles plus tard... revoilà la rue des étals.",
   {{"Hum... d'accord", 30, ALWAYS}},
   PIC_ETALS, FX_DARK, 0x000000, 0, J_RONDS, 0, 0, 0, GAME_NONE, 0, 0},
  {32, "L'orée de la forêt. Deux sentiers : des champignons qui brillent, ou un ruisseau qui chante.",
   {{"Les champignons", 33, ALWAYS}, {"Le ruisseau", 34, ALWAYS}, {"Retour au village", 2, ALWAYS}},
   PIC_CHAMPIGNONS, FX_NONE, 0x000000, 2, J_PAS, 0, 0, 0, GAME_NONE, 0, 0},
  {33, "Un carrefour aux panneaux effacés. Mochi : À gauche, j'en suis sûr ! Un hibou bâille là-haut.",
   {{"À gauche", 41, ALWAYS}, {"À droite", 5, ALWAYS}, {"Demander au hibou", 42, ALWAYS}},
   PIC_PANNEAUX, FX_NONE, 0x000000, 3, J_MYSTERE, 0, 0, 0, GAME_NONE, 0, 0},
  {34, "Le ruisseau chante... puis se perd dans les ronces. Mochi : Euh, ~, on traverse ?",
   {{"Traverser", 33, ALWAYS}, {"Remonter", 32, ALWAYS}},
   PIC_NONE, FX_WAVES, 0x000000, 1, J_MYSTERE, 0, 0, 0, GAME_NONE, 0, 0},
  {35, "Le lac est gris et brumeux. Au loin, une basse fait boum, boum, boum...",
   {{"Prendre le pont", 36, ALWAYS}, {"Faire le tour", 43, ALWAYS}, {"Retour au village", 2, ALWAYS}},
   PIC_PONT, FX_FOG, 0x000000, 2, J_BASSE, 0, 0, 0, GAME_NONE, 0, 0},
  {36, "Le pont grince. Au milieu, la brume est si épaisse qu'on n'y voit plus rien !",
   {{"Suivre la basse", 7, ALWAYS}, {"Tout droit", 44, ALWAYS}, {"Faire demi-tour", 35, ALWAYS}},
   PIC_PONT, FX_FOG, 0x000000, 3, J_MYSTERE, 0, 0, 0, GAME_NONE, 0, 0},
  {38, "La route monte vers un château blanc et gluant. Des panneaux tout gris : Stade, Circuit.",
   {{"Vers le château", 39, ALWAYS}, {"Vers le stade", 26, UNLESS_BALLON}, {"Vers le circuit", 50, UNLESS_VOLANT}, {"Retour au village", 2, ALWAYS}},
   PIC_ROUTE, FX_NONE, 0x000000, 2, J_PAS, 0, 0, 0, GAME_NONE, 0, 0},
  {39, "Des mochis géants bloquent la route. Mochi : Saute dessus, ~, ils sont tout mous !",
   {{"Sauter dessus", 40, ALWAYS}, {"Les contourner", 9, ALWAYS}, {"Faire demi-tour", 38, ALWAYS}},
   PIC_MOCHIS, FX_NONE, 0x000000, 3, J_MYSTERE, 0, 0, 0, GAME_NONE, 0, 0},
  {40, "BOING ! Les mochis te renvoient en bas de la route. Mochi : Bon... TRÈS mous, alors.",
   {{"Remonter", 38, ALWAYS}},
   PIC_MOCHIS, FX_BOUNCE, 0x000000, 0, J_BOING, 0, 0, 0, GAME_NONE, 0, 0},
  {41, "Tiens, revoilà les champignons... Mochi siffle, l'air de rien. Tu as tourné en rond !",
   {{"Oups !", 33, ALWAYS}},
   PIC_CHAMPIGNONS, FX_NONE, 0x000000, 1, J_RONDS, 0, 0, 0, GAME_NONE, 0, 0},
  {42, "Le hibou : Hou ! Le tanuki joue là où le soleil se couche... à droite ! Hou hou !",
   {{"Merci, hibou !", 33, ALWAYS}},
   PIC_HIBOU, FX_NONE, 0x000000, 3, J_HIBOU, 0, 0, 0, GAME_NONE, 0, 0},
  {43, "Le tour du lac est très long. Mochi traîne la patte : On aurait dû prendre le pont...",
   {{"Retourner au pont", 36, ALWAYS}, {"Continuer", 7, ALWAYS}},
   PIC_NONE, FX_WAVES, 0x000000, 1, J_PAS, 0, 0, 0, GAME_NONE, 0, 0},
  {44, "Tu marches, tu marches... et te revoilà au début du pont. Mochi : C'est la brume qui triche !",
   {{"Réessayer", 36, ALWAYS}},
   PIC_NONE, FX_FOG, 0x000000, 1, J_RONDS, 0, 0, 0, GAME_NONE, 0, 0},
  {47, "La quête|~, le Roi Gluant a volé les couleurs du village et les a cachées au château !",
   {{"Et alors ?", 48, ALWAYS}},
   PIC_CHATEAU, FX_NONE, 0x505050, -1, J_MYSTERE, 0, 0, 0, GAME_NONE, 0, 0},
  {48, "Sa porte a 3 sceaux. Gagne au moins 2 charmes magiques au village pour les briser...",
   {{"Et ensuite ?", 49, ALWAYS}},
   PIC_PLACE, FX_NONE, 0x808080, -1, J_TINTEMENT, 0, 0, 0, GAME_NONE, 0, 0},
  {49, "... puis bats le roi pour rendre les couleurs. Chaque charme te donnera un pouvoir !",
   {{"C'est parti !", 1, ALWAYS}},
   PIC_ROI, FX_NONE, 0xFF3C8C, -1, J_FANFARE, 0, 0, 0, GAME_NONE, 0, 0},
  {50, "Les tanukis font la course en karts ! Mochi : Ils te prêtent un bolide, ~. Tu cours ?",
   {{"Courir !", GAME, ALWAYS}, {"Retour à la route", 38, ALWAYS}},
   PIC_KART, FX_NONE, 0xFF3C8C, -1, J_ROULEMENT, 0, 0, 0, GAME_RACE, 51, 52},
  {51, "1re place ! Les tanukis t'offrent le Volant d'Or : la course est débloquée !",
   {{"Retour à la route", 38, ALWAYS}},
   PIC_VOLANT, FX_CONFETTI, 0xFFC800, -1, J_HYMNE, 0, TROPHY_VOLANT, 0, GAME_NONE, 0, 0},
  {52, "Les tanukis rient : Pas mal ! Mais seule la 1re place gagne le Volant d'Or.",
   {{"Revanche !", 50, ALWAYS}, {"Retour à la route", 38, ALWAYS}},
   PIC_KART, FX_NONE, 0x0028FF, -1, J_TROMBONE, 0, 0, 0, GAME_NONE, 0, 0},
  {53, "Un escalier humide descend sous le château. Mochi : Il fait tout noir, ~... Je te suis !",
   {{"Descendre", GAME, ALWAYS}, {"Remonter", 9, ALWAYS}},
   PIC_CHATEAU, FX_DARK, 0x000830, -1, J_MYSTERE, 0, 0, 0, GAME_MAZE, 54, 9},
  {54, "Le Pinceau Arc-en-ciel ! Mochi : Avec lui, le roi va voir de toutes les couleurs !",
   {{"Remonter", 9, ALWAYS}},
   PIC_PINCEAU, FX_CONFETTI, 0xFFFFFF, -1, J_SPECIAL, CHARM_PINCEAU, 0, 0, GAME_NONE, 0, 0},
  {45, "Les sceaux restent gris. Mochi : ~, il nous faut au moins 2 charmes pour les briser !",
   {{"Retour au village", 2, ALWAYS}},
   PIC_CHATEAU, FX_NONE, 0x505050, -1, J_TROMBONE, 0, 0, 0, GAME_NONE, 0, 0},
  {46, "Vaincu, le roi fond en larmes de mochi : Snif... Je voulais juste des couleurs à moi !",
   {{"Le consoler", 13, ALWAYS}, {"Goûter son gâteau", 19, ALWAYS}, {"Fouiller le château", 12, ALWAYS}, {"Portail d'étoiles", 21, IF_LEGEND}},
   PIC_ROI, FX_RAIN, 0xFF3C8C, -1, J_TROMBONE, 0, 0, 0, GAME_NONE, 0, 0},
};

constexpr uint8_t NUM_SCENES = sizeof(SCENES) / sizeof(SCENES[0]);
