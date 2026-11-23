#!/usr/bin/env python3
"""Range GUIDE-IA.md dans include/guide_text.h, pour que la borne puisse le proposer à copier (page « À propos »).

- `python tools/embed_guide.py`          : écrit le fichier si le guide a changé ;
- `python tools/embed_guide.py --check`  : ne fait rien, échoue si le fichier n'est plus à jour.
PlatformIO lance ce script avant chaque compilation (voir platformio.ini), donc le guide de la borne est toujours
celui du dépôt.
"""
import os
import sys

DELIM = "GUIDEEND"
HEADER = ("// Fichier généré par tools/embed_guide.py à partir de GUIDE-IA.md : ne pas le modifier à la main.\n"
          "#pragma once\n"
          "#include <pgmspace.h>\n\n"
          "const char GUIDE_TEXT[] PROGMEM = R\"%s(" % DELIM)


def build(root):
    with open(os.path.join(root, "GUIDE-IA.md"), encoding="utf-8") as f:
        text = f.read().replace("\r\n", "\n")
    if ")%s\"" % DELIM in text:
        raise SystemExit("GUIDE-IA.md contient la suite de caractères réservée )%s\"" % DELIM)
    return HEADER + text + ")%s\";\n" % DELIM


def main(root, check=False):
    out = os.path.join(root, "include", "guide_text.h")
    want = build(root)
    have = open(out, encoding="utf-8").read() if os.path.exists(out) else None
    if have == want:
        return 0
    if check:
        print("include/guide_text.h n'est plus à jour : lance « python tools/embed_guide.py »")
        return 1
    with open(out, "w", encoding="utf-8", newline="\n") as f:
        f.write(want)
    return 0


try:  # lancé par PlatformIO (SCons) : « Import » existe alors
    Import("env")  # noqa: F821
    main(env["PROJECT_DIR"])  # noqa: F821
except NameError:
    if __name__ == "__main__":
        here = os.path.dirname(os.path.abspath(__file__))
        sys.exit(main(os.path.dirname(here), "--check" in sys.argv))
