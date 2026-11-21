# mini-arcade

Mini borne d'arcade sur ESP32 avec une matrice LED 16x16.

## Matériel

- ESP32 DevKit 30 broches (WROOM-32D) sur la carte « Shield capteurs v2.1 »
- Matrice WS2812B 16x16 (alimentée en 5 V directement, pas par la carte)
- Encodeur rotatif KY-040 (HW-040)
- Bouton HW-483 et joystick 2 axes (avec bouton)
- Buzzer passif HW-508 et LED RVB HW-478
- Écran OLED SSD1306 0,96" (128x64, I2C)

Le brochage complet est dans `include/config.h`, l'alimentation est décrite dans `CLAUDE.md`
(un seul chargeur 5 V / 2,4 A pour tout, la matrice limitée à 1,5 A par le programme).

## Le jeu

L'histoire **« Les Couleurs du Roi Gluant »** est au cœur de la borne : 43 scènes, des chemins où l'on peut se
perdre (la LED RVB « chauffe » quand on se rapproche du but), 3 charmes, 2 trophées, 4 fins. En la jouant, on
débloque les activités du menu :

| Activité | Se débloque avec |
|---|---|
| Mood light (7 ambiances) | le Bol Magique |
| Casse-briques (6 niveaux) | la Feuille Magique (défi du tanuki, un mini casse-briques) |
| Jukebox des jingles | la Note Magique |
| Football manga | le Ballon d'Or (finale au stade) |
| Flipper manga | la Bille Étoile (record du vieux flipper) |
| Atelier pixel art (dessin depuis le téléphone par Wi-Fi) et Galerie | une fin heureuse |
| Scores et Réglages | toujours disponibles |
| Mode secret (couleurs animées, niveaux bonus) | la fin légendaire |

Dans le menu, le **bouton du joystick** puis l'**encodeur** règlent la luminosité de la matrice (gardée en mémoire).
Chaque fin de l'Aventure donne un **score** ; les 5 meilleurs, avec leurs initiales, sont gardés pour toujours (ligne
**Scores** du menu). La dernière ligne, **Réglages**, règle la luminosité et la veille, et permet de rejouer l'histoire
depuis le début ou de tout effacer (les scores restent).
Les commandes de chaque activité sont dans `GUIDE-IA.md`.

Le code est sous licence MIT (`LICENSE`). `GUIDE-IA.md` explique le matériel et le programme pour qu'une IA
puisse aider à reprogrammer la borne.

Commandes du moniteur série (115200 bauds) : `u` tout débloquer, `r` effacer la progression,
`a <scène>` aller à une scène, `l <2-40>` luminosité, `n <1-6>` casse-briques à un niveau, `s` son, `d`/`g`/`f`/`m` ressenti de la raquette (déplacement par cran, élan, frottement, vitesse max).

## Commandes (PlatformIO)

- `pio run` : compiler
- `pio run -t upload` : téléverser
- `pio device monitor` : moniteur série
