# Mini borne d'arcade (ESP32 + matrice LED 16x16)

Projet d'apprentissage de Claude Code. Matériel : carte « Shield capteurs v2.1 » (Les Jardins du Treille)
+ ESP32 DevKit 30 broches (WROOM-32D), matrice WS2812B 16x16, encodeur KY-040 (HW-040), bouton HW-483, joystick 2 axes,
buzzer passif HW-508, LED RVB HW-478, OLED SH1106 1,3" (128x64, monochrome ; l'ancien SSD1306 0,96" bicolore marche avec OLED_SH1106 0).

## Commandes
- `pio run` : compiler
- `pio run -t upload` : téléverser
- `pio device monitor` : moniteur série (115200)

## Brochage (voir `include/config.h`, seule source de vérité)
| Fonction | Connecteur de la carte | GPIO |
|---|---|---|
| Matrice WS2812B (données) | TEMP SOL 1 | D4 + GND (5 V par l'alimentation directe, pas par la carte) |
| LED RVB R / V / B | RELAIS | D17 / D27 / D33 (+ GND) |
| Encodeur A / B / bouton | LORA | D18 / D19 / D23 (+ 3V3, GND) |
| Buzzer passif | EXT (signal S) + TEMP SOL 2 (GND, 3V3) | D13 ; fil D4 de TEMP SOL 2 laissé vide |
| Bouton 1 (HW-483) | PLUVIO 1 | D15 |
| Bouton 2 (bouton du joystick) | EXT | D26 (pull-up interne) ; PLUVIO 2 (VP) n'est plus utilisé |
| Joystick X / Y | HUM SOL 1 / HUM SOL 2 | D32 / D34 (via les 1K ; joystick alimenté en 3V3, pas en 5V) |
| OLED I2C | T H AIR 2 | SDA D21 / SCL D22, 0x3C |
| Libres | D25 et D14 (LORA), D16 (RELAIS), VP (PLUVIO 2) | |

TEMP SOL 1, 2 et 3 partagent la même broche D4 : un seul appareil peut y envoyer un signal (la matrice).

## Alimentation
Un seul chargeur USB 5 V / 2,4 A, câble coupé (rouge = +5V, noir = GND) et dédoublé :
- gros fils vers la matrice (+5V, GND), sans passer par la carte ;
- petits fils vers le connecteur BATTERIE (broches 5V et GND) : la diode D1 (1N5822) amène le 5V sur le VIN de
  l'ESP32 et empêche le chargeur et le port USB du PC de s'alimenter l'un l'autre (on peut flasher chargeur branché).
- La broche 5V du connecteur RELAIS reste libre.

Le fichier KiCad de la carte nomme mal 4 pastilles de l'empreinte ESP32, mais les pistes gravées arrivent sur les bonnes broches.

## Règles
- Matrice : toujours limiter le courant (`MAX_MILLIAMPS`) ; pistes de la carte de 0,5 mm, ne pas faire passer le courant de la matrice par elle.
- Ne pas utiliser D0, D2, D12 (démarrage) ni D34/D35/D39 pour des sorties.
- Organisation du programme et façon d'ajouter une scène, un dessin ou une activité : voir `GUIDE-IA.md`,
  à tenir à jour à chaque évolution (il est livré avec la borne pour que sa propriétaire le donne à son IA).
- Les réglages sont des constantes en haut de chaque fichier ; le ressenti de la raquette (gain, frottement,
  vitesse max) est en haut de `src/breakout.cpp`.
- Le Wi-Fi (Atelier) n'est allumé que dans l'Atelier ; son mot de passe est aléatoire, gardé en NVS, jamais dans le code.
- Le cahier des charges (scénario, déblocages, commandes des jeux) est le document Claude Docs
  « Mini-arcade – Cahier des charges + Aventure manga ».
- Code en français pour les commentaires, noms de variables en anglais.

## État du projet (23 novembre 2026) : à lire en reprenant le travail
- **Branche de travail** `ccr-118c22dc-37f6y7`, **PR n° 2 ouverte vers `main`, pas encore fusionnée** (c'est à sa propriétaire de la fusionner). Dernier .bin testé par CI : compilation n° 57 (artifact `mini-arcade-firmware`, fichier `mini-arcade-complet.bin` à flasher en 0x0).
- Matériel : OLED **SH1106 1,3"** (`OLED_SH1106 1` dans `include/config.h`).
- Le jeu : histoire « Les Couleurs du Roi Gluant » (55 scènes, `include/scenes.h`) et mini-jeux : casse-briques, football, flipper, course (virages, taches d'huile), labyrinthe (noir, clé, mochis gluants), ramen, rythme (grenouille), « 1, 2, 3, soleil » (garde), combat du roi, étoiles. Le **Cercle de rythme** (`src/circle.cpp`, morceaux dans `include/songs.h`, 3 mélodies de l'histoire développées + 3 compositions) a remplacé le Jukebox ; banc d'essai : `tools/sim/build.sh simC && tools/sim/simC`. Prénom demandé à la 1re partie, avancement en %, scores gardés pour toujours.
- **Difficulté** (Réglages) : facile = exploration (presque tout le monde réussit), moyen = on échoue parfois, difficile = environ 1 réussite sur 6. Réglée avec des robots : `tools/sim/build.sh simD && tools/sim/simD`. Les temps limites du labyrinthe (4 min 30 / 2 min 50) sont **estimés, à vérifier avec de vraies parties** ; les réglages sont à ajuster d'après les retours.
- « À propos » : le Wi-Fi de la borne sert une page avec le guide à copier (`tools/embed_guide.py` range `GUIDE-IA.md` dans `include/guide_text.h`). Après toute modification de `GUIDE-IA.md`, lancer `python tools/embed_guide.py` et commiter le fichier. Page web facultative : `docs/index.html` (GitHub Pages, à activer sur `main` / `docs` après la fusion).
- Les textes de l'OLED tiennent sur **21 caractères** (6 px chacun) ; `tools/sim/sim8` vérifie les scènes.
- `fabrication/` : réglages du laser (diode 10 W, LaserGRBL, bois de 10 mm : tracer puis scier) et prompts pour faire dessiner 4 planches de 24 × 24 cm. **Ce projet de décor a été mis de côté** (les images générées par ChatGPT et Gemini étaient trop chargées pour la gravure : il faut un style d'estampe, contours épais, grandes zones blanches).
- Le cahier des charges (Claude Docs) et `GUIDE-IA.md` sont à jour avec tout ce qui précède.
