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

L'histoire **« Les Couleurs du Roi Gluant »** est au cœur de la borne : en la jouant, on débloque les
activités du menu (casse-briques, football, flipper, mood light, jukebox, atelier pixel art…).

- **Menu** : l'encodeur ou le joystick (haut/bas) choisit, le bouton ou le bouton de l'encodeur valide.
  Les activités pas encore gagnées sont verrouillées.
- **Aventure** : le texte s'affiche sur l'OLED, le décor sur la matrice. L'encodeur ou le joystick
  (gauche/droite) choisit la réponse, le bouton valide, le bouton du joystick revient au menu.
- **Casse-briques** : 6 niveaux (VANESSA, GUILLAUME, LISA-LOUISE, ANTOINE, LUCIE, EMILIE-ROSE), raquette à
  l'encodeur ou au joystick, pause avec le bouton du joystick.

Le code est sous licence MIT (`LICENSE`). `GUIDE-IA.md` explique le matériel et le programme pour qu'une IA
puisse aider à reprogrammer la borne.

Commandes du moniteur série (115200 bauds) : `u` tout débloquer, `r` effacer la progression,
`a <scène>` aller à une scène, `n <1-6>` casse-briques à un niveau, `s` son, `g`/`f`/`m` ressenti de la raquette.

## Commandes (PlatformIO)

- `pio run` : compiler
- `pio run -t upload` : téléverser
- `pio device monitor` : moniteur série
