#!/usr/bin/env python3
"""Vérifie include/songs.h : chaque mesure fait 16 doubles croches, les notes sont connues, la durée est raisonnable."""
import re, sys, os
root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
src = open(os.path.join(root, "include", "songs.h"), encoding="utf-8").read()
songs = re.findall(r'\{"([^"]+)",\s*(\d+),\s*((?:"[^"]*"\s*)+)\}', src)
bad = 0
for name, bpm, body in songs:
    text = "".join(re.findall(r'"([^"]*)"', body))
    bars = [b.strip() for b in text.split("|") if b.strip()]
    total = 0
    for i, bar in enumerate(bars, 1):
        n = 0
        for tok in bar.split():
            m = re.fullmatch(r'(?:([A-G][#b]?\d)|R):(\d+)([!~]?)', tok)
            if not m:
                print(f"  {name} mesure {i} : jeton inconnu « {tok} »"); bad += 1; continue
            n += int(m.group(2))
        total += n
        if n != 16:
            print(f"  {name} mesure {i} : {n} doubles croches au lieu de 16 ({bar})"); bad += 1
    secs = total * 15.0 / int(bpm)
    print(f"{name:20s} {len(bars):2d} mesures, {bpm} battements/min, {secs:4.1f} s")
sys.exit(1 if bad else 0)
