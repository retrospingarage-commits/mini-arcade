// À propos : le guide pour reprogrammer la borne avec une IA.
// La borne allume son Wi-Fi (comme l'Atelier) : on scanne le QR code avec le téléphone, une page s'ouvre toute seule
// avec le guide et un bouton « Copier ». Tout est dans la borne : il ne faut ni internet ni GitHub.
// N'importe quel bouton : retour au menu (le Wi-Fi s'éteint).
#include "hw.h"
#include "activities.h"

void aboutBegin() { atelierGuideBegin(); }

GameResult aboutLoop(uint32_t now) { return atelierLoop(now); }
