# Réglages du laser (diode 10 W, LaserGRBL)

Ce sont des **points de départ**. Chaque bois et chaque machine réagit différemment : fais toujours un test sur une chute du même bois avant de graver une vraie planche.

## Avant de commencer
- **Mise au point** : règle la distance entre la lentille et le bois avec la cale de ta machine. C'est le réglage le plus important : un laser mal réglé grave flou et coupe mal.
- **Souffleur d'air (air assist)** : indispensable pour couper. Il évite les flammes et les bords noircis.
- **Aspiration** ou fenêtre ouverte, lunettes de protection adaptées à ta longueur d'onde (445 nm en général), extincteur ou seau d'eau à portée.
- **Ne jamais laisser le laser sans surveillance.**
- Garde le plan de travail propre et fixe le bois (aimants ou ruban adhésif).

## Dans LaserGRBL
- Mode laser GRBL activé (`$32=1`) et puissance maximale `$30=1000`. Dans LaserGRBL : « Grbl » → « Grbl Configuration ».
- **Gravure** (image PNG) : ouvrir le fichier, choix « **Line To Line** », qualité de **10 lignes/mm** (254 ppp : suffisant pour un laser à diode, inutile d'aller plus fin), « **Dithering** » ou seuil (« Threshold ») pour du noir et blanc franc.
- **Découpe** (SVG) : ouvrir le fichier SVG, puis « Vector » / « Cutting ».
- Commence par l'image, puis le SVG : le bois ne doit **pas bouger** entre les deux, sinon la gravure et les trous ne coïncident plus. Utilise le même point d'origine (« Set zero ») pour les deux.

## Gravure sur contreplaqué (points de départ)
| Essai | Vitesse | Puissance | Résultat attendu |
|---|---|---|---|
| 1 | 4000 mm/min | 40 % | gravure claire |
| 2 | 3000 mm/min | 50 % | gravure moyenne (souvent le bon réglage) |
| 3 | 2000 mm/min | 60 % | gravure foncée, plus de fumée |

Fais une petite grille de ces essais sur une chute, garde le meilleur. Évite les très grandes surfaces noires : elles chauffent, noircissent le bois autour et peuvent le déformer.

## Découpe : le point difficile
- **Un laser à diode de 10 W ne coupe pas 10 mm de contreplaqué** de façon fiable : il faudrait beaucoup de passes (une vingtaine), le bois brûlerait sur les bords et le risque d'incendie est réel.
- Estimation de départ, puissance 100 %, avec souffleur d'air :

| Épaisseur | Vitesse | Passes |
|---|---|---|
| 3 mm | 250 à 350 mm/min | 2 à 3 |
| 4 mm | 150 à 250 mm/min | 3 à 5 |
| 5 mm | 100 à 150 mm/min | 5 à 8 |

- **Solution conseillée** : graver au laser, puis **découper le contour et les gros trous avec une scie sauteuse ou à chantourner**, avec un gabarit, ou utiliser du contreplaqué de **3 à 4 mm** que le laser sait couper. Un contreplaqué mince est aussi plus pratique pour visser l'encodeur et le bouton (leur filetage fait environ 7 mm).
- Deux planches de 4 mm collées font 8 mm si tu veux de la rigidité.
- Les trous de 5 mm pour les ficelles et les petits trous de vis se percent très bien à la perceuse.

## Méthode conseillée pour le 10 mm : tracer au laser, finir à la scie
Le laser ne coupe pas le bois, il **trace une rainure** le long des contours et des trous (le SVG), puis on découpe à la scie en suivant la rainure.
- **Réglage du tracé** : vitesse 300 mm/min, puissance 100 %, **2 passes** (puis une 3e si la rainure est trop légère). Vise 1 à 2 mm de profondeur.
- **Ne cherche pas à aller très profond** : après 3 à 4 mm, la mise au point se perd, le fond de la rainure noircit, il y a beaucoup de fumée et le risque de flammes augmente. Garde le souffleur d'air allumé, et arrête-toi si le bois s'enflamme ou noircit trop.
- **Ordre** : d'abord la gravure de l'image, puis le tracé du SVG, sans bouger le bois entre les deux.
- **À la scie** : scie sauteuse ou scie à chantourner avec une lame fine à bois (dents fines pour moins d'éclats), en sciant **un tout petit peu à l'extérieur** de la rainure (côté déchet). Ponce ensuite jusqu'à la ligne.
- **Gros trou rond de 70 mm** : perce un trou de départ de 10 mm, passe la lame de la scie sauteuse, ou utilise une scie cloche de 70 mm.
- **Fenêtres carrées** (matrice de 162 mm, OLED) : perce un trou à chaque coin, puis scie d'un coin à l'autre.
- **Petits trous** (ficelles 5 mm, vis 3,2 mm) : à la perceuse, au centre du petit cercle tracé.
- **Coins arrondis de 15 mm** : scie en suivant la courbe tracée, ou arrondis à la ponceuse.
- Pense à **poser le bois sur une planche de sacrifice** pour que la scie et la perceuse ne déchirent pas le dessous.

## Après la coupe
- Ponce légèrement les bords noircis, essuie avec un chiffon sec.
- Un coup de vernis ou de cire d'abeille met la gravure en valeur.
