#!/bin/sh
# Banc d'essai sur PC (sans carte) : le jeu tourne avec de faux matériels (dossier stub/).
#   ./build.sh simD && ./simD            taux de réussite de chaque jeu à chaque difficulté (robots « joueur correct »)
#   ./simD Football                      un seul jeu
#   ./build.sh simC && ./simC            le Cercle de rythme : % de réussite de robots plus ou moins précis, par morceau
#   ./build.sh sim8 && ./sim8            toutes les scènes tiennent-elles à l'écran ? (5 lignes de 21 caractères)
#   ./build.sh simM && ./simM            la Mood light : les commandes changent-elles bien la lumière ?
#   ./build.sh simL && ./simL            le Cercle de rythme : texte qui chevauche ou sort de l'écran (tous les écrans)
#   (simD et simM affichent aussi le contrôle de mise en page de leurs jeux)
cd "$(dirname "$0")" || exit 1
SRC=$(ls ../../src/*.cpp | grep -v 'atelier.cpp')   # le Wi-Fi n'existe pas sur PC : atelier_stub.cpp le remplace
g++ -std=gnu++17 -O1 -w -Istub -I../../include $SRC atelier_stub.cpp "$1.cpp" -o "$1"
