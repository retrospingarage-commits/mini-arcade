# Mini borne d'arcade (ESP32 + matrice LED 16x16)

Projet d'apprentissage de Claude Code. Matériel : carte « Shield capteurs v2.1 » (Les Jardins du Treille)
+ ESP32 DevKit 30 broches (WROOM-32D), matrice WS2812B 16x16, encodeur KY-040, 2 boutons, OLED SSD1306 0,96" (128x64).

## Commandes
- `pio run` : compiler
- `pio run -t upload` : téléverser
- `pio device monitor` : moniteur série (115200)

## Brochage (voir `include/config.h`, seule source de vérité)
| Fonction | Connecteur de la carte | GPIO |
|---|---|---|
| Matrice WS2812B (données) | RELAIS | D16 (+ GND commun ; 5 V par l'alimentation directe, pas par la carte) |
| Encodeur A / B / bouton | LORA | D18 / D19 / D23 (+ 3V3, GND) |
| Bouton 1 | PLUVIO 1 | D15 |
| Bouton 2 | PLUVIO 2 | D36 (VP, entrée seule, 10K externe obligatoire) |
| OLED I2C | T H AIR 2 | SDA D21 / SCL D22, 0x3C |
| Libres | EXT (D13, D26), D25, D14 (LORA), D17, D27, D33 (RELAIS) | |

## Règles
- Matrice : toujours limiter le courant (`MAX_MILLIAMPS`) ; pistes de la carte de 0,5 mm, ne pas faire passer le courant de la matrice par elle.
- Ne pas utiliser D0, D2, D12 (démarrage) ni D34/D35/D39 pour des sorties.
- Les réglages de ressenti de l'encodeur (gain, frottement, vitesse max) sont en haut de `src/main.cpp`.
- Code en français pour les commentaires, noms de variables en anglais.
