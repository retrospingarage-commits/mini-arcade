# Mini borne d'arcade (ESP32 + matrice LED 16x16)

Projet d'apprentissage de Claude Code. Matériel : carte « Shield capteurs v2.1 » (Les Jardins du Treille)
+ ESP32 DevKit 30 broches (WROOM-32D), matrice WS2812B 16x16, encodeur KY-040 (HW-040), bouton HW-483, joystick 2 axes,
buzzer passif HW-508, LED RVB HW-478, OLED SSD1306 0,96" (128x64).

## Commandes
- `pio run` : compiler
- `pio run -t upload` : téléverser
- `pio device monitor` : moniteur série (115200)

## Brochage (voir `include/config.h`, seule source de vérité)
| Fonction | Connecteur de la carte | GPIO |
|---|---|---|
| Matrice WS2812B (données) | RELAIS | D16 (+ GND commun ; 5 V par l'alimentation directe, pas par la carte) |
| LED RVB R / V / B | RELAIS | D17 / D27 / D33 (+ GND) |
| Encodeur A / B / bouton | LORA | D18 / D19 / D23 (+ 3V3, GND) |
| Buzzer passif | LORA | D25 (+ GND) |
| Bouton 1 (HW-483) | PLUVIO 1 | D15 |
| Bouton 2 (bouton du joystick) | PLUVIO 2 | D36 (VP, entrée seule, 10K externe obligatoire) |
| Joystick X / Y | HUM SOL 1 / HUM SOL 2 | D32 / D34 (via les 1K ; joystick alimenté en 3V3, pas en 5V) |
| OLED I2C | T H AIR 2 | SDA D21 / SCL D22, 0x3C |
| Libres | EXT (D13, D26), D14 (LORA) | |

Le fichier KiCad de la carte nomme mal 4 pastilles de l'empreinte ESP32, mais les pistes gravées arrivent sur les bonnes broches.

## Règles
- Matrice : toujours limiter le courant (`MAX_MILLIAMPS`) ; pistes de la carte de 0,5 mm, ne pas faire passer le courant de la matrice par elle.
- Ne pas utiliser D0, D2, D12 (démarrage) ni D34/D35/D39 pour des sorties.
- Les réglages de ressenti de l'encodeur (gain, frottement, vitesse max) et du jeu sont en haut de `src/main.cpp`.
- Les 6 niveaux du casse-briques sont dans `include/levels.h`, la police 3x5 de la matrice dans `include/font.h`.
- Code en français pour les commentaires, noms de variables en anglais.
