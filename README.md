# mini-arcade

Mini borne d'arcade sur ESP32 avec une matrice LED 16x16.

## Matériel

- ESP32 DevKit 30 broches (WROOM-32D) sur la carte « Shield capteurs v2.1 »
- Matrice WS2812B 16x16 (alimentée en 5 V directement, pas par la carte)
- Encodeur rotatif KY-040
- 2 boutons
- Écran OLED SSD1306 0,96" (128x64, I2C)

Le brochage complet est dans `include/config.h`.

## Le jeu : casse-briques façon Arkanoid

- **Encodeur** : déplace la raquette (avec inertie)
- **Bouton 1** ou **bouton de l'encodeur** : lancer la balle / valider
- **Bouton 2** : pause
- L'OLED affiche le niveau, le score et les vies (3 au départ)

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

## Commandes (PlatformIO)

- `pio run` : compiler
- `pio run -t upload` : téléverser
- `pio device monitor` : moniteur série
