# Prompt à copier dans ChatGPT

---

Tu es designer pour la découpe et la gravure laser. Je fabrique une petite borne d'arcade murale pour une jeune fan de manga. Elle est faite de **4 planches carrées en bois** (contreplaqué de 10 mm d'épaisseur), suspendues les unes sous les autres par des ficelles, et accrochée au mur comme une décoration. Je te demande le design des 4 planches.

## Ce que j'attends de toi

Pour **chaque planche**, deux fichiers à l'échelle 1:1 (240 × 240 mm) :

1. **Un fichier vectoriel SVG pour la découpe** : seulement des contours fermés, trait de 0,01 mm, couleur rouge pur (#FF0000), sans remplissage. Il contient le contour extérieur, tous les trous et toutes les ouvertures. Unités en millimètres, `viewBox="0 0 240 240"`, `width="240mm" height="240mm"`.
2. **Une image PNG pour la gravure** : en niveaux de gris (idéalement en noir et blanc pur, sans dégradés fins), 240 × 240 mm à 300 ppp (2835 × 2835 pixels), fond blanc = non gravé, noir = gravé. Elle doit se superposer exactement au SVG.

Donne aussi un **aperçu** des 4 planches assemblées verticalement, comme elles seront au mur.

## Règles communes aux 4 planches

- Carré de **240 × 240 mm**, **coins arrondis** (rayon 15 mm).
- **Trous pour les ficelles** : diamètre **5 mm**, centre à **10 mm du bord du haut ou du bas et à 10 mm du bord du côté**, dans les angles :
  - planche 1 (tout en haut) : 2 trous **en bas** seulement (bas gauche, bas droit) ;
  - planches 2 et 3 : 4 trous, **en haut et en bas** ;
  - planche 4 (tout en bas) : 2 trous **en haut** seulement.
- La gravure ne doit **pas** toucher les trous ni les ouvertures : garde une marge d'au moins 4 mm autour de chaque trou et de chaque ouverture.
- Adapté à la gravure laser sur bois : traits d'au moins 0,3 mm, pas de texte plus petit que 4 mm de haut, aplats noirs francs plutôt que des dégradés, éviter les grandes surfaces entièrement noires (elles brûlent et noircissent le bois).
- Le style et l'histoire sont décrits plus bas : les 4 planches doivent former **une seule fresque manga** qui se lit de haut en bas.

## Chaque planche

### Planche 1 (tout en haut) : décoration et accroche murale
- Un **trou d'accroche** au centre de la planche, pour la vis du mur : un trou en forme de **trou de serrure** (cercle de 10 mm de diamètre, surmonté d'une fente de 5 mm de large et 10 mm de haut), centré horizontalement et verticalement.
- Aucun composant : c'est la planche **titre**. Grave le titre **« Les Couleurs du Roi Gluant »** en lettrage manga, et l'illustration principale : le château de mochi blanc et gluant au-dessus de Pixelville toute grise, le Roi Gluant qui garde les couleurs dans des bocaux, et un arc-en-ciel qui s'échappe.

### Planche 2 : l'écran à LED
- Derrière cette planche est fixée une **matrice LED 16 × 16 de 160 × 160 mm** (256 LED, une tous les 10 mm).
- Découpe une **fenêtre carrée de 162 × 162 mm, centrée**, coins arrondis (rayon 3 mm), pour voir toute la matrice.
- Grave un **cadre manga** autour de la fenêtre, comme un écran de borne d'arcade : lignes de vitesse, étoiles et pixels, avec Mochi (le chat gris) qui regarde l'écran depuis un coin.

### Planche 3 : la carte électronique
- Derrière est fixée la carte électronique (sur sa face avant se trouve le logo **RétroSpin Garage**).
- Découpe un **trou rond de 70 mm de diamètre au centre exact** de la planche, pour voir le logo.
- Grave autour du trou un **décor de « hublot » manga** (cercle d'énergie, rayons, petits engrenages et circuits stylisés), et le texte **« RétroSpin Garage »** en arc au-dessus ou en dessous du hublot.
- Prévois **4 trous de fixation de 3,2 mm** (vis M3) aux coins d'un rectangle de 60 × 80 mm centré : je les ajusterai à la vraie carte.

### Planche 4 (tout en bas) : les commandes
Elle porte toutes les commandes. Place-les comme une manette, confortable pour deux mains, avec l'écran en haut au centre :
- **Écran OLED 1,3"** (module d'environ 35 × 34 mm, zone d'affichage d'environ 29 × 15 mm) : fenêtre rectangulaire de **31 × 16,5 mm**, centrée horizontalement, à environ 55 mm du haut, avec **4 petits trous de vis de 2,5 mm** aux coins du module (écartement exact à confirmer : mets-les à titre provisoire, à 30 × 28 mm).
- **Joystick** (à gauche) : trou rond de **28 mm** de diamètre, pour laisser bouger le manche.
- **Bouton rotatif (encodeur)** (à droite) : trou rond de **7,5 mm** pour l'axe fileté.
- **Gros bouton poussoir** (à droite, sous l'encodeur) : trou rond de **13 mm**.
- **LED RVB** (en haut, à côté de l'écran) : trou rond de **5 mm**.
- **Buzzer** : une petite grille de 7 trous de 2 mm (en forme de fleur) pour laisser passer le son.
- Grave autour de chaque commande un petit symbole manga clair : flèches autour du joystick, « tourner » autour de l'encodeur, une étoile sur le bouton, une bulle « !! » près de la LED.
- Grave dans le bas un petit bandeau avec Mochi qui dit **« À toi de jouer ! »**.

## Fixation des composants
Tous les composants sont **simplement vissés** sur le bois, par l'arrière. Marque donc, sur les planches 2, 3 et 4, des **trous de vis provisoires de 3,2 mm** (M3) à chaque endroit où un composant doit être fixé, repérés par une **petite croix gravée de 3 mm** à côté du trou, pour que je puisse les déplacer facilement. Ne grave rien sur la face avant à moins de 4 mm de ces trous.

## L'univers à illustrer (style et personnages)

- **Style** : manga kawaii, lignes nettes et rondes, grands yeux brillants, effets typiques (lignes de vitesse, étoiles, onomatopées « BOING ! », « SLURP ! », « BUUUT ! »), avec un clin d'œil au pixel art rétro (petits carrés). C'est pour une enfant : joyeux et mignon, jamais effrayant.
- **L'histoire** : le Roi Gluant, un roi en mochi rose et tout collant, avec une couronne, a volé les couleurs de **Pixelville**, qui est devenue toute grise. L'héroïne et son chat **Mochi** (gris, rond, petites oreilles pointues, l'air un peu paresseux) partent les rendre.
- **Personnages à placer sur les planches** : un fantôme gourmand avec un bol de ramen fumant, un tanuki joueur avec une feuille sur la tête, une grenouille DJ avec un casque et des platines, un capitaine lapin footballeur, un vieux flipper, des karts de course de tanukis, des mochis géants tout mous, un hibou sur un panneau, le Mochi-Mage (un mochi sorcier étoilé).
- **Objets magiques** : le Bol Magique, la Feuille Magique, la Note Magique, le Ballon d'Or, la Bille Étoile, le Volant d'Or, le Pinceau Arc-en-ciel.
- **Lieux** : la place du village, la forêt aux champignons qui brillent, le lac brumeux avec un pont, la route qui monte vers le château blanc et gluant, les sous-sols du château.
- Les couleurs réelles viendront des LED : sur le bois, tout est en gravure monochrome.

## À me rendre

1. Les 4 fichiers SVG de découpe (`planche-1-decoupe.svg` … `planche-4-decoupe.svg`).
2. Les 4 images PNG de gravure (`planche-1-gravure.png` … `planche-4-gravure.png`).
3. L'aperçu des 4 planches assemblées.
4. Un petit tableau récapitulatif des trous et ouvertures de chaque planche (forme, diamètre ou dimensions, position du centre en mm depuis le coin en haut à gauche), pour que je puisse vérifier avant de découper.

---
