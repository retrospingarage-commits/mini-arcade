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
| Matrice WS2812B 16×16 (256 LED couleur) | écran principal | D4 (données) | TEMP SOL 1 (TEMP SOL 2 et 3 partagent ce D4) |
| Écran OLED SSD1306 0,96" 128×64, bicolore (16 lignes du haut jaunes, le reste bleu), I2C (0x3C, ou 0x3D détecté) ; un SH1106 1,3" marche aussi avec `OLED_SH1106 1` dans `include/config.h` | textes, menus, scores | SDA D21, SCL D22 (tirages 4K7 soudés) | T H AIR 2 |
| LED RVB HW-478 (une seule LED, cathode commune, résistances 220 Ω / 100 Ω / 100 Ω) | ambiance | R D17, V D27, B D33 | RELAIS |
| Encodeur rotatif HW-040 (KY-040) : rotation sans fin + bouton | commande principale | A D18, B D19, bouton D23 | LORA |
| Bouton poussoir HW-483 | « bouton 1 » | D15 | PLUVIO 1 |
| Joystick 2 axes analogiques + bouton | commande secondaire | X D32, Y D34, bouton D26 | HUM SOL 1, HUM SOL 2, EXT |
| Buzzer passif HW-508 | sons et mélodies, une note à la fois | D13 | EXT (signal), TEMP SOL 2 (GND et 3V3) |

Broches encore libres : D25 et D14 (connecteur LORA), D16 (connecteur RELAIS), D36/VP (connecteur PLUVIO 2, entrée
seule, avec une 10K de tirage soudée). D14 sait détecter un toucher (pastille tactile), D25 peut sortir un signal
audio analogique.

La source de vérité du brochage est `include/config.h`. La matrice de cette borne a sa première ligne qui part de
la droite : `MATRIX_FLIP_X` remet l'image à l'endroit (à changer si une autre matrice affiche tout en miroir).

## 2. Règles de sécurité (à respecter absolument)

1. **Courant de la matrice** : toujours garder `FastLED.setMaxPowerInVoltsAndMilliamps(5, MAX_MILLIAMPS)`
   avec `MAX_MILLIAMPS` à 1500 au maximum, et la luminosité à 40/255 au maximum (`BRIGHTNESS` ; la joueuse règle
   la sienne entre 2 et 40 dans le menu, `matrixBrightness`). Toute la borne est alimentée
   par un seul chargeur USB de 2,4 A : au-delà, il chauffe ou coupe.
2. **Broches interdites en sortie** : D0, D2 et D12 (elles décident du démarrage de l'ESP32), et D34, D35,
   D36, D39 (entrées seulement).
3. **D36 (VP)** n'a pas de résistance de tirage interne : s'il sert un jour, il faut la résistance de 10 kΩ soudée
   près de PLUVIO 2. Ne pas demander `INPUT_PULLUP` sur cette broche.
4. **Joystick** : il est alimenté en 3,3 V, jamais en 5 V.
5. **Alimentation** : ne jamais brancher en même temps le chargeur et un ordinateur sur la borne.
6. **Wi-Fi** : réseau propre à la borne (« Mini-arcade »), allumé seulement dans l'Atelier, sans accès à
   internet. Son mot de passe est tiré au hasard au premier lancement et gardé dans la borne (NVS) ; il
   s'affiche sur l'OLED. Ne jamais écrire de mot de passe ou d'identifiant en clair dans le code.

## 3. Compiler et installer le programme

Le projet utilise PlatformIO (`platformio.ini`), avec des versions de bibliothèques fixées.

- Sur un ordinateur avec PlatformIO : `pio run` compile, `pio run -t upload` installe sur la borne branchée en
  USB, `pio device monitor` ouvre le moniteur série (115200 bauds).
- Sans rien installer : à chaque envoi sur GitHub, l'action « Compilation ESP32 » produit le fichier
  `mini-arcade-complet.bin` (onglet Actions, section Artifacts). On l'installe depuis Chrome ou Edge avec un
  outil de flashage web (par exemple esptool.spacehuhn.com), à l'adresse **0x0**.

- Sans câble : ouvrir l'Atelier sur la borne, connecter l'ordinateur au Wi-Fi « Mini-arcade » avec le mot de passe
  affiché, puis `ARCADE_PASS=<mot de passe> pio run -e ota -t upload`.

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
| `include/activities.h` | la liste des activités du menu et la façon de les lancer |
| `src/breakout.cpp`, `include/levels.h` | le casse-briques et ses 6 niveaux (+ 1 niveau bonus en mode secret, + le niveau TANUKI joué dans l'histoire, scène 5) |
| `src/football.cpp` | le football manga (aussi en version courte dans l'histoire, scène 26) |
| `src/boss.cpp` | le combat final contre le Roi Gluant (scène 11) ; ses réglages (vie du roi, vitesse des bulles…) sont en haut du fichier |
| `src/pinball.cpp` | le flipper manga (aussi en version courte dans l'histoire, scène 28) |
| `src/mood.cpp` | la Mood light (7 ambiances, luminosité, veille) |
| `src/jukebox.cpp` | le Jukebox des jingles, avec égaliseur |
| `src/atelier.cpp` | l'Atelier pixel art : Wi-Fi de la borne, page de dessin pour téléphone, QR code, mise à jour sans câble |
| `src/gallery.cpp`, `include/gallery.h` | la palette des dessins, leur enregistrement (20 maximum, rangés du plus ancien au plus récent ; un dessin tout noir n'est pas enregistré), l'effacement et la Galerie |
| `include/font.h` | la petite police 3×5 pour écrire sur la matrice |

Principe : `loop()` appelle à chaque tour la fonction de l'activité en cours (`menuLoop`, `adventureLoop`,
`breakoutLoop`…). Chaque activité lit elle-même les boutons, dessine la matrice et l'OLED, et renvoie `false`
quand on revient au menu. Rien ne doit bloquer : pas de `delay()` dans les boucles, on compare `millis()`.

Outils disponibles dans `hw.h` :
- `btn1.pressed()`, `btn2.pressed()`, `btnEnc.pressed()` : vrai une seule fois par appui.
- `encoderCount()` : compteur de l'encodeur (4 impulsions par cran ; la différence entre deux lectures donne
  le sens et la vitesse de rotation). Pour un déplacement gauche/droite, multiplier par `ENC_X_SIGN`.
- `joyX()`, `joyY()` : position du joystick entre -1 et 1 (Y négatif vers le haut), 0 s'il n'est pas branché.
- `setPixel(x, y, couleur)`, `drawText`, `drawScroll`, `drawPicture` : dessin sur la matrice, puis `FastLED.show()`.
- `oled` : l'écran OLED (bibliothèque U8g2, police `u8g2_font_6x10_tf` avec accents, 21 caractères par ligne).
  Il est bicolore : garder les titres dans la bande jaune (lignes 0 à 15) et ne pas faire chevaucher un texte
  sur la frontière avec la partie bleue.
- `play(melodie, prioritaire)` : joue une liste de notes `{fréquence en Hz, durée en ms}` terminée par `{0, 0}`.
- `setRgb(couleur)` : couleur de la LED RVB.

## 5. Commandes de chaque activité

| Activité | Commandes |
|---|---|
| Menu | encodeur ou joystick haut/bas : choisir ; bouton ou bouton de l'encodeur : valider ; bouton du joystick puis encodeur : luminosité de la matrice (gardée en mémoire). Veille après 3 min |
| Scores | les 5 meilleurs scores de l'Aventure (initiales, points, * = fin légendaire) ; n'importe quel bouton : menu |
| Réglages (dernière ligne du menu) | Luminosité (échelle progressive à 16 crans, de 1 à 40) ; Veille du menu (3, 10, 30 min ou jamais) ; Rejouer l'histoire (garde les déblocages) ; Tout effacer (garde la Galerie, les scores et les réglages) — ces deux-là demandent un 2e appui pour confirmer |
| Aventure | encodeur ou joystick gauche/droite : choisir la réponse ; bouton : valider ; bouton du joystick : menu |
| Combat contre le Roi Gluant (Aventure) | encodeur ou joystick : se déplacer ; bouton (ou bouton de l'encodeur) : tirer, on peut le garder enfoncé. Pouvoirs des charmes : Bol = bouclier qui revient, Feuille = tir triple, Note = bulles ralenties. Bouton du joystick : abandonner |
| Casse-briques | encodeur ou joystick : raquette ; bouton ou joystick vers le haut : lancer ; bouton du joystick : pause |
| Football | encodeur ou joystick : se déplacer ; maintenir le bouton : charger le tir, et pendant la charge tourner l'encodeur ou pousser le joystick : viser en biais (ligne de points) ; relâcher : tirer ; tourner pendant le tir : le courber. Le gardien « lit » une partie des tirs (de plus en plus avec les niveaux) : réglages en haut de `src/football.cpp` |
| Flipper | bouton : flipper gauche ; joystick (toute action) : flipper droit ; tourner l'encodeur : secouer (vite = fort, trop = TILT) ; bouton de l'encodeur : pause, deux fois : menu |
| Mood light | encodeur : ambiance ; bouton de l'encodeur : réglage de la luminosité ; bouton : veille ; bouton du joystick : menu |
| Jukebox | encodeur : choisir ; bouton : jouer ; bouton du joystick : menu |
| Atelier | on dessine sur le téléphone ; bouton du joystick : fermer (le Wi-Fi s'éteint) |
| Galerie | encodeur : durée de chaque dessin ; bouton : suivant ; bouton de l'encodeur : effacer le dessin affiché (un 2e appui confirme, tourner ou le bouton annule) ; bouton du joystick : menu |

**Score de l'Aventure** (calculé à chaque fin, dans `src/adventure.cpp`) : fin heureuse 300 (légendaire 1 000), +100 par
charme et +150 par trophée possédés, bonus de rapidité 400 − 10 par scène traversée, −20 par impasse. S'il entre dans
les 5 meilleurs, on tape 3 initiales (encodeur : lettre, bouton : suivante). Le classement est rangé à part (espace NVS
« scores ») : rien ne l'efface, ni « Tout effacer », ni une mise à jour du programme.

**Mode secret** (après la fin légendaire) : 4 couleurs animées dans l'Atelier, niveau MOCHI-MAGE au casse-briques,
gardien plus rapide et points doublés au football, bumpers étoilés à points doublés au flipper.

## 6. Ajouter quelque chose

### Une scène de l'histoire
Dans `include/scenes.h`, ajouter une ligne au tableau `SCENES`, dans cet ordre :
1. numéro de la scène ;
2. texte : 100 caractères maximum, et il doit tenir en 5 lignes de 21 caractères à l'écran ; « Titre|texte » affiche
   un titre centré dans la bande jaune (21 caractères maximum, utilisé pour les fins) puis le texte sur 4 lignes ;
3. jusqu'à 5 choix `{"libellé", scène cible, condition}` (libellé de 20 caractères maximum ; 4 visibles au plus
   à la fois ; la condition `ALWAYS`, `IF_ALL_CHARMS`, `IF_TWO_CHARMS`, `IF_LEGEND`, `UNLESS_BOL`… cache le choix selon la progression) ;
4. dessin (`PIC_…`, ou `PIC_NONE`) et effet animé (`FX_FOG` brume, `FX_WAVES` vagues, `FX_RAINBOW`… ou `FX_NONE`) ;
5. couleur de la LED (`0xRRGGBB`) et « chaleur » de -1 à 4 : sur un chemin, 0 = loin du but (LED bleue),
   4 = tout près (LED orange qui bat vite) ; -1 = utiliser la couleur ;
6. jingle (`J_…`) ;
7. récompense donnée en arrivant : charme, trophée, fin (0 si aucune) ;
8. mini-jeu (`GAME_NONE`, `GAME_REFLEX`, `GAME_BREAKOUT`, `GAME_FOOTBALL`, `GAME_PINBALL`, `GAME_BOSS`) et scènes d'arrivée en cas de victoire et de défaite. Un choix dont la
   cible est `GAME` lance le mini-jeu.

Quand une récompense débloque une activité, l'OLED l'annonce (« Nouveau dans le menu : … »).
Un choix qui mène à une scène pas encore écrite affiche automatiquement « bientôt ». Après une modification,
vérifier que chaque scène reste accessible et qu'on peut toujours revenir au village ou au lit.

### Un dessin
Dans `include/pictures.h`, 16 lignes de 16 lettres (une lettre par pixel ; les couleurs sont dans
`paletteColor`), puis ajouter son nom à la liste `Picture`.

### Une activité dans le menu
1. Ajouter son nom dans `Activity` et `activityName` (`include/save.h`, `src/save.cpp`), et sa règle de
   déblocage dans `isUnlocked`.
2. Déclarer `xxxBegin()` et `GameResult xxxLoop(uint32_t now)` dans `include/activities.h`, et les écrire dans un
   nouveau fichier `src/xxx.cpp` (la boucle renvoie `GAME_RUNNING` tant que l'activité continue).
3. Les brancher dans `start()` et `runCurrent()` (`src/main.cpp`), et choisir son icône dans `ICONS` (`src/menu.cpp`).

## 7. Conventions du code

- Commentaires en français, noms de variables et de fonctions en anglais.
- Les réglages (vitesses, couleurs, durées) sont des constantes en haut de chaque fichier.
- Tester sur la borne après chaque petite modification, et garder une version qui marche.
