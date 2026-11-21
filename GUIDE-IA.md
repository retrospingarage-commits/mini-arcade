# Guide de la mini-arcade pour une IA

Ce document est fait pour être donné tel quel à un assistant IA (Claude, ChatGPT, Gemini…) afin de
reprogrammer la mini-arcade. Il décrit le matériel, les règles à ne jamais enfreindre, l'organisation
du programme et la façon d'y ajouter des choses. Le code est sous licence MIT (voir `LICENSE`) : tu peux le
modifier, le partager et t'en servir comme tu veux.

> Pour l'IA : lis tout ce guide avant de proposer une modification. Respecte la section « Règles de
> sécurité » même si on te demande autre chose, et explique pourquoi si une demande les contredit.

## 1. Le matériel

Une carte ESP32 (DevKit 30 broches, puce WROOM-32D) posée sur une carte d'extension « Shield capteurs v2.1 ».

| Élément | Rôle | Broche (GPIO) | Connecteur de la carte |
|---|---|---|---|
| Matrice WS2812B 16×16 (256 LED couleur) | écran principal | D16 (données) | RELAIS |
| Écran OLED SSD1306 128×64, monochrome, I2C (adresse 0x3C) | textes, menus, scores | SDA D21, SCL D22 | T H AIR 2 |
| LED RVB HW-478 (une seule LED, cathode commune, résistances 220 Ω / 100 Ω / 100 Ω) | ambiance | R D17, V D27, B D33 | RELAIS |
| Encodeur rotatif HW-040 (KY-040) : rotation sans fin + bouton | commande principale | A D18, B D19, bouton D23 | LORA |
| Bouton poussoir HW-483 | « bouton 1 » | D15 | PLUVIO 1 |
| Joystick 2 axes analogiques + bouton | commande secondaire | X D32, Y D34, bouton D36 (VP) | HUM SOL 1, HUM SOL 2, PLUVIO 2 |
| Buzzer passif HW-508 | sons et mélodies, une note à la fois | D25 | LORA |

Broches encore libres : D13 et D26 (connecteur EXT), D14 (connecteur LORA). D13 et D14 savent détecter un
toucher (pastille tactile), D26 peut sortir un signal audio analogique.

La source de vérité du brochage est `include/config.h`.

## 2. Règles de sécurité (à respecter absolument)

1. **Courant de la matrice** : toujours garder `FastLED.setMaxPowerInVoltsAndMilliamps(5, MAX_MILLIAMPS)`
   avec `MAX_MILLIAMPS` à 1500 au maximum, et la luminosité générale à 40/255. Toute la borne est alimentée
   par un seul chargeur USB de 2,4 A : au-delà, il chauffe ou coupe.
2. **Broches interdites en sortie** : D0, D2 et D12 (elles décident du démarrage de l'ESP32), et D34, D35,
   D36, D39 (entrées seulement).
3. **D36 (VP)** n'a pas de résistance de tirage interne : le bouton du joystick fonctionne grâce à une
   résistance de 10 kΩ soudée sur la carte. Ne pas demander `INPUT_PULLUP` sur cette broche.
4. **Joystick** : il est alimenté en 3,3 V, jamais en 5 V.
5. **Alimentation** : ne jamais brancher en même temps le chargeur et un ordinateur sur la borne.
6. **Wi-Fi** (quand il sera ajouté) : réseau propre à la borne, protégé par un mot de passe, sans accès à
   internet. Aucun mot de passe ni identifiant écrit en clair dans le code publié.

## 3. Compiler et installer le programme

Le projet utilise PlatformIO (`platformio.ini`), avec des versions de bibliothèques fixées.

- Sur un ordinateur avec PlatformIO : `pio run` compile, `pio run -t upload` installe sur la borne branchée en
  USB, `pio device monitor` ouvre le moniteur série (115200 bauds).
- Sans rien installer : à chaque envoi sur GitHub, l'action « Compilation ESP32 » produit le fichier
  `mini-arcade-complet.bin` (onglet Actions, section Artifacts). On l'installe depuis Chrome ou Edge avec un
  outil de flashage web (par exemple esptool.spacehuhn.com), à l'adresse **0x0**.

Commandes utiles dans le moniteur série : `u` débloque tout (pour tester), `r` efface la progression,
`a <numéro>` saute à une scène de l'Aventure, `n <1-6>` lance le casse-briques à un niveau, `s` coupe le son.

## 4. Organisation du programme

| Fichier | Contenu |
|---|---|
| `src/main.cpp` | démarrage, choix de l'activité en cours, commandes du moniteur série |
| `src/hw.cpp`, `include/hw.h` | tout le matériel partagé : matrice, OLED, boutons, encodeur, joystick, buzzer, LED RVB |
| `src/save.cpp`, `include/save.h` | progression sauvegardée en mémoire permanente (NVS), règles de déblocage du menu |
| `src/menu.cpp` | le menu sur l'OLED, avec les activités verrouillées |
| `src/adventure.cpp` | le moteur de l'histoire (affichage, choix, récompenses, effets animés, LED « chaud-froid », mini-jeu de réflexe) |
| `include/scenes.h` | les scènes de l'histoire : textes, choix, dessins, couleurs, jingles |
| `include/pictures.h` | les dessins pixel art 16×16 |
| `src/breakout.cpp`, `include/levels.h` | le casse-briques et ses 6 niveaux |
| `include/font.h` | la petite police 3×5 pour écrire sur la matrice |

Principe : `loop()` appelle à chaque tour la fonction de l'activité en cours (`menuLoop`, `adventureLoop`,
`breakoutLoop`…). Chaque activité lit elle-même les boutons, dessine la matrice et l'OLED, et renvoie `false`
quand on revient au menu. Rien ne doit bloquer : pas de `delay()` dans les boucles, on compare `millis()`.

Outils disponibles dans `hw.h` :
- `btn1.pressed()`, `btn2.pressed()`, `btnEnc.pressed()` : vrai une seule fois par appui.
- `encoderCount()` : compteur de l'encodeur (4 impulsions par cran ; la différence entre deux lectures donne
  le sens et la vitesse de rotation).
- `joyX()`, `joyY()` : position du joystick entre -1 et 1 (Y négatif vers le haut), 0 s'il n'est pas branché.
- `setPixel(x, y, couleur)`, `drawText`, `drawScroll`, `drawPicture` : dessin sur la matrice, puis `FastLED.show()`.
- `oled` : l'écran OLED (bibliothèque U8g2, police `u8g2_font_6x10_tf` avec accents, 21 caractères par ligne).
- `play(melodie, prioritaire)` : joue une liste de notes `{fréquence en Hz, durée en ms}` terminée par `{0, 0}`.
- `setRgb(couleur)` : couleur de la LED RVB.

## 5. Ajouter quelque chose

### Une scène de l'histoire
Dans `include/scenes.h`, ajouter une ligne au tableau `SCENES`, dans cet ordre :
1. numéro de la scène ;
2. texte : 100 caractères maximum, et il doit tenir en 5 lignes de 21 caractères à l'écran ;
3. jusqu'à 5 choix `{"libellé", scène cible, condition}` (libellé de 20 caractères maximum ; 4 visibles au plus
   à la fois ; la condition `ALWAYS`, `IF_ALL_CHARMS`, `UNLESS_BOL`… cache le choix selon la progression) ;
4. dessin (`PIC_…`, ou `PIC_NONE`) et effet animé (`FX_FOG` brume, `FX_WAVES` vagues, `FX_RAINBOW`… ou `FX_NONE`) ;
5. couleur de la LED (`0xRRGGBB`) et « chaleur » de -1 à 4 : sur un chemin, 0 = loin du but (LED bleue),
   4 = tout près (LED orange qui bat vite) ; -1 = utiliser la couleur ;
6. jingle (`J_…`) ;
7. récompense donnée en arrivant : charme, trophée, fin (0 si aucune) ;
8. mini-jeu (`GAME_NONE`, `GAME_REFLEX`…) et scènes d'arrivée en cas de victoire et de défaite. Un choix dont la
   cible est `GAME` lance le mini-jeu.

Un choix qui mène à une scène pas encore écrite affiche automatiquement « bientôt ». Après une modification,
vérifier que chaque scène reste accessible et qu'on peut toujours revenir au village ou au lit.

### Un dessin
Dans `include/pictures.h`, 16 lignes de 16 lettres (une lettre par pixel ; les couleurs sont dans
`paletteColor`), puis ajouter son nom à la liste `Picture`.

### Une activité dans le menu
1. Ajouter son nom dans `Activity` et `activityName` (`include/save.h`, `src/save.cpp`), et sa règle de
   déblocage dans `isUnlocked`.
2. Écrire ses fonctions `xxxBegin()` et `bool xxxLoop(uint32_t now)` dans un nouveau fichier `src/xxx.cpp`.
3. La déclarer prête dans `isReady` (`src/menu.cpp`) et la brancher dans `loop()` (`src/main.cpp`).

## 6. Conventions du code

- Commentaires en français, noms de variables et de fonctions en anglais.
- Les réglages (vitesses, couleurs, durées) sont des constantes en haut de chaque fichier.
- Tester sur la borne après chaque petite modification, et garder une version qui marche.
