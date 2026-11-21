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

## Le jeu : casse-briques façon Arkanoid

- **Encodeur** (avec inertie) ou **joystick** (gauche/droite) : déplace la raquette
- **Bouton 1**, **bouton de l'encodeur** ou **joystick vers le haut** : lancer la balle / valider
- **Bouton du joystick** : pause
- L'OLED affiche le niveau, le score et les vies (3 au départ)
- Le buzzer fait les sons du jeu, la LED RVB prend la couleur du niveau (rouge quand on perd une vie)
- Le joystick est détecté au démarrage : sans lui, le jeu marche avec l'encodeur seul. Ne pas toucher le joystick pendant
  l'allumage, c'est à ce moment que son centre est mesuré

6 niveaux de difficulté croissante (balle plus rapide, raquette plus petite) :

| # | Niveau | Originalité |
|---|---|---|
| 1 | VANESSA | Un grand cœur, balle lente pour se mettre en jambes |
| 2 | GUILLAUME | Une forteresse de briques solides (2 coups) |
| 3 | LISA-LOUISE | Les lignes de briques glissent à gauche et à droite |
| 4 | ANTOINE | Des briques de métal incassables |
| 5 | LUCIE | Dans le noir : seule la balle éclaire les briques |
| 6 | EMILIE-ROSE | Une rose qui descend vers la raquette |

Les niveaux se modifient dans `include/levels.h`.

Réglages en direct dans le moniteur série (115200 bauds) :

- `g <gain>` : vitesse ajoutée par cran de l'encodeur
- `f <frottement>` : freinage (entre 0,80 et 0,999)
- `m <vitesse max>` : vitesse maximale de la raquette
- `n <niveau>` : aller directement à un niveau (1 à 6)
- `s` : couper ou remettre le son

## Commandes (PlatformIO)

- `pio run` : compiler
- `pio run -t upload` : téléverser
- `pio device monitor` : moniteur série
