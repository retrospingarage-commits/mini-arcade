# Prompt à copier dans ChatGPT

---

Tu es illustrateur et designer pour la gravure laser sur bois. Je fabrique une petite borne d'arcade murale pour une jeune fan de manga. Elle est faite de **4 planches carrées en contreplaqué de 24 × 24 cm**, décorées sur **une seule face** et suspendues les unes sous les autres par des ficelles, le tout accroché au mur comme un tableau. Je poserai moi-même les composants électroniques par-dessus : **ne dessine aucun emplacement de composant**. Je veux surtout un **décor manga magnifique** qui forme une seule histoire de haut en bas.

## Ce que j'attends de toi, pour chaque planche (4 planches)

1. **Une image PNG pour la gravure**, 240 × 240 mm à 300 ppp (2835 × 2835 pixels), **noir et blanc pur** (noir = gravé, blanc = bois nu).
2. **Un fichier SVG de tracé de coupe**, à l'échelle 1:1 (`viewBox="0 0 240 240"`, `width="240mm" height="240mm"`), traits rouges (#FF0000) de 0,01 mm, sans remplissage. Il ne contient **que** :
   - le contour extérieur : carré de 240 × 240 mm à **coins arrondis (rayon 15 mm)** ;
   - **uniquement sur la planche 3** : un cercle de **70 mm de diamètre, centré exactement**. Ce trou laisse voir le logo « RétroSpin Garage » de la carte électronique placée derrière.
3. Un **aperçu des 4 planches assemblées** verticalement, comme elles seront au mur (avec un petit écart entre elles, pour les ficelles).

L'image et le SVG doivent se superposer exactement. Je grave l'image, je trace le SVG avec le laser (rainure peu profonde), puis je découpe à la scie en suivant la rainure.

## Repères de perçage (à graver dans l'image PNG)

Je perce ces trous à la perceuse : marque chaque emplacement par un **petit cercle gravé avec un point au centre** (trait de 0,4 mm), du diamètre indiqué :
- **Trous de ficelle** : cercle de **5 mm**, centre à **10 mm du bord du haut ou du bas et à 10 mm du bord du côté**, dans les angles :
  - planche 1 (tout en haut) : seulement les 2 angles **du bas** ;
  - planches 2 et 3 : les 4 angles (**haut et bas**) ;
  - planche 4 (tout en bas) : seulement les 2 angles **du haut**.
- **Trou d'accroche** : sur la planche 1 seulement, un cercle de **10 mm** au centre exact, pour la vis du mur.
- Garde **au moins 6 mm sans rien graver** autour de chaque repère de perçage, du trou de 70 mm et de la ligne de coupe extérieure (je scie un peu à l'extérieur de la rainure).

## Contraintes de gravure laser (diode, bois)

- Noir et blanc pur, sans gris. Pour les ombres et les textures, utilise des **trames manga** (motifs de points, hachures, lignes parallèles) plutôt que des dégradés.
- Traits de **0,4 mm minimum**, texte d'au moins 5 mm de haut.
- Évite les grandes surfaces entièrement noires : elles brûlent et noircissent le bois. Préfère des aplats **hachurés ou en trame**.
- Garde un petit **cadre décoratif** à environ 8 mm du bord, qui donne un bel aspect fini.

## Le style

**Manga kawaii**, lignes nettes et rondes, grands yeux brillants, effets typiques (lignes de vitesse, étoiles, bulles, onomatopées comme « BOING ! », « SLURP ! », « BUUUT ! »), avec un clin d'œil au **pixel art rétro** (petits carrés, motifs de pixels, couleurs remplacées par des trames). C'est pour une enfant : joyeux et mignon, jamais effrayant. Utilise des zones de décor plus simples (rayons, étoiles, motifs) au **centre des planches 2 et 4**, car j'y fixerai des composants. Les détails importants vont autour, vers les bords.

## L'histoire à raconter, de haut en bas

Le **Roi Gluant**, un roi en mochi rose et collant avec une couronne, a volé les couleurs de **Pixelville**, qui est devenue toute grise. Une jeune héroïne et son chat **Mochi** (gris, rond, petites oreilles pointues, l'air paresseux) partent les rendre. Les **couleurs réelles viendront des LED** : sur le bois, tout est en noir et blanc.

- **Planche 1 (en haut), le titre.** Le titre **« Les Couleurs du Roi Gluant »** en grand lettrage manga. Le château de mochi blanc et gluant au sommet d'une colline, le Roi Gluant à son balcon qui garde les couleurs dans des bocaux, et un arc-en-ciel qui s'en échappe.
- **Planche 2, le village.** Pixelville toute grise, la place, et les habitants : le **fantôme gourmand** avec son bol de ramen, le **tanuki** avec une feuille sur la tête, la **grenouille DJ** avec casque et platines, un **hibou** sur un panneau, la forêt aux champignons et le lac brumeux avec un pont. Mochi et l'héroïne au milieu.
- **Planche 3, le hublot.** Autour du trou central de 70 mm, un grand décor en forme de **hublot ou de portail magique** : cercle d'énergie, rayons, engrenages et circuits stylisés, étoiles. Le texte **« RétroSpin Garage »** en arc au-dessus ou en dessous. Des objets magiques tout autour : Bol, Feuille, Note, Ballon d'Or, Bille Étoile, Volant d'Or, Pinceau Arc-en-ciel.
- **Planche 4 (en bas), les jeux et la victoire.** Un décor d'arcade : le **capitaine lapin footballeur** et son ballon, un **vieux flipper**, des **karts de tanukis** en pleine course, les **sous-sols du château** avec un mochi gluant, le **Mochi-Mage** étoilé. Le bas se termine par la victoire : l'arc-en-ciel, les couleurs qui reviennent, et un petit bandeau avec Mochi qui dit **« À toi de jouer ! »**.

Si possible, fais **se rejoindre** un motif d'une planche à l'autre (par exemple l'arc-en-ciel ou une banderole qui descend), en tenant compte de l'écart des ficelles.

## À me rendre

1. Les 4 images PNG de gravure (`planche-1-gravure.png` … `planche-4-gravure.png`).
2. Les 4 fichiers SVG de tracé de coupe (`planche-1-coupe.svg` … `planche-4-coupe.svg`).
3. L'aperçu des 4 planches assemblées.
4. Une courte liste des repères de perçage de chaque planche (diamètre, position du centre en mm depuis le coin en haut à gauche), pour que je vérifie avant de percer.

---
