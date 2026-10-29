# mini-arcade

Mini borne d'arcade sur ESP32 avec une matrice LED 16x16.

## Matériel

- ESP32 DevKit 30 broches (WROOM-32D) sur la carte « Shield capteurs v2.1 »
- Matrice WS2812B 16x16 (alimentée en 5 V directement, pas par la carte)
- Encodeur rotatif KY-040
- 2 boutons
- Écran OLED SSD1306 0,96" (128x64, I2C)

Le brochage complet est dans `include/config.h`.

## Test actuel

Un point glisse sur la matrice avec de l'inertie :

- **Encodeur** : lance le point, qui ralentit tout seul et rebondit sur les bords
- **Bouton de l'encodeur** : recentre le point
- **Bouton 1** : +1 au score (affiché sur l'OLED)
- **Bouton 2** : remet le score à 0

Réglages en direct dans le moniteur série (115200 bauds) :

- `g <gain>` : vitesse ajoutée par cran de l'encodeur
- `f <frottement>` : freinage (entre 0,80 et 0,999)
- `m <vitesse max>` : vitesse maximale

## Commandes (PlatformIO)

- `pio run` : compiler
- `pio run -t upload` : téléverser
- `pio device monitor` : moniteur série
