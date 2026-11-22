// Moteur de l'Aventure : affiche une scène (texte sur l'OLED, dessin et effet animé sur la matrice, LED, jingle),
// laisse choisir avec l'encodeur ou le joystick, donne les récompenses et sauvegarde la progression.
// Sur les chemins d'errance, la LED « chauffe » (bleu → orange) quand on se rapproche du but.
// Les scènes elles-mêmes sont dans include/scenes.h.
#include "adventure.h"
#include "scenes.h"
#include "activities.h"
#include "breakout.h"

namespace {

constexpr int      LINE_CHARS = 21;   // caractères par ligne sur l'OLED (police 6x10)
constexpr int      TEXT_LINES = 5;    // lignes de texte au-dessus de la barre de choix
constexpr uint8_t  SOON       = 0;    // scène « bientôt » : la cible n'est pas encore écrite
constexpr uint8_t  FIRST_SCENE = 47;  // 1re scène d'une nouvelle partie (on y demande le prénom)
constexpr uint8_t  REFLEX_STARS = 3;  // étoiles à attraper au mini-jeu de réflexe
const uint16_t     REFLEX_WINDOW[REFLEX_STARS] = {900, 700, 550};  // temps pour appuyer (ms), de plus en plus court

const Note SND_TICK[]  = {{1200, 8}, {0, 0}};
const Note SND_GO[]    = {{1760, 60}, {0, 0}};
const Note SND_STAR[]  = {{1319, 60}, {1760, 120}, {0, 0}};
const Note SND_MISS[]  = {{200, 300}, {0, 0}};

const Scene SCENE_SOON = {SOON, "Cette partie de l'histoire arrive bientôt ! Mochi s'impatiente.",
                          {{"Revenir", 0, ALWAYS}}, PIC_CADENAS, FX_NONE, 0x202020, -1, J_BIENTOT, 0, 0, 0,
                          GAME_NONE, 0, 0};
const Choice CONTINUE = {"Continuer", 2, ALWAYS};  // secours si aucun choix n'est visible

const Scene*  scene = nullptr;
uint8_t       previous = 1;     // scène d'où l'on vient (pour revenir de la scène « bientôt »)
const Choice* visible[5];
uint8_t       nVisible = 0, selected = 0;
char          lines[TEXT_LINES][64];
int           nLines = 0;
bool          hasTitle = false;  // la 1re ligne est un titre (fins de l'histoire)
int32_t       encBase = 0;
int8_t        joyWas = 0;
bool          dirty = true;
uint32_t      rewardUntil = 0;
const char*   popupTitle = nullptr;  // annonce par-dessus le texte (activité débloquée, score…)
char          popupText[24];
uint32_t      popupUntil = 0;

// Score de la partie et saisie des initiales (3 lettres, comme sur les bornes d'arcade)
constexpr uint16_t END_POINTS = 300, LEGEND_POINTS = 1000, CHARM_POINTS = 100, TROPHY_POINTS = 150;
constexpr int      SPEED_BONUS = 400, POINTS_PER_STEP = 10, IMPASSE_PENALTY = 20;
uint16_t finalScore = 0;
uint8_t  finalEnding = 0;
char     initials[4];
bool     askingName = false, nameGiven = false;  // saisie du prénom au début d'une partie (sinon : initiales pour le score)
char     textBuf[160];        // texte de la scène avec le prénom à la place de « ~ »
uint8_t  letterPos = 0;

// Mini-jeu de réflexe (scène 23) : attendre que la matrice devienne verte, puis appuyer
enum Phase : uint8_t { SCENE, REFLEX_WAIT, REFLEX_GO, REFLEX_END, EXTERNAL_GAME, NAME_ENTRY };  // EXTERNAL_GAME : un jeu complet
Phase    phase = SCENE;
uint8_t  stars = 0;
uint32_t phaseUntil = 0;
bool     reflexWon = false;

const Scene* findScene(uint8_t id) {
  for (const Scene& s : SCENES)
    if (s.id == id) return &s;
  return nullptr;
}

bool isVisible(Cond c) {
  switch (c) {
    case IF_ALL_CHARMS:      return (progress.charms & ALL_CHARMS) == ALL_CHARMS;
    case IF_TWO_CHARMS:      return countBits(progress.charms & ALL_CHARMS) >= 2;
    case UNLESS_TWO_CHARMS:  return countBits(progress.charms & ALL_CHARMS) < 2;
    case UNLESS_PINCEAU:     return !(progress.charms & CHARM_PINCEAU);
    case IF_LEGEND:          return (progress.charms & ALL_CHARMS) == ALL_CHARMS
                                    && (progress.trophies & ALL_TROPHIES) == ALL_TROPHIES;
    case UNLESS_BOL:         return !(progress.charms & CHARM_BOL);
    case UNLESS_FEUILLE:     return !(progress.charms & CHARM_FEUILLE);
    case UNLESS_NOTE:        return !(progress.charms & CHARM_NOTE);
    case UNLESS_BALLON:      return !(progress.trophies & TROPHY_BALLON);
    case UNLESS_BILLE:       return !(progress.trophies & TROPHY_BILLE);
    case UNLESS_VOLANT:      return !(progress.trophies & TROPHY_VOLANT);
    case UNLESS_MARKET_DONE: return !((progress.charms & CHARM_BOL) && (progress.trophies & TROPHY_BILLE));
    default:                 return true;
  }
}

// Donne une récompense pas encore obtenue ; vrai si quelque chose de nouveau a été gagné
bool giveRewards(const Scene* s) {
  bool fresh = (s->charm & ~progress.charms) || (s->trophy & ~progress.trophies) || (s->ending & ~progress.endings);
  progress.charms   |= s->charm;
  progress.trophies |= s->trophy;
  progress.endings  |= s->ending;
  return fresh;
}

void showPopup(const char* title, const char* text) {
  popupTitle = title;
  strncpy(popupText, text, sizeof(popupText) - 1);
  popupText[sizeof(popupText) - 1] = 0;
  popupUntil = millis() + 4000;
}

// Recopie un texte de scène en remplaçant « ~ » par le prénom (« LOU » devient « Lou ») ou par « toi »
void personalize(const char* src, char* dst, size_t size) {
  char name[4] = "toi";
  if (progress.player[0]) {
    int n = 0;
    for (int i = 0; i < 3 && progress.player[i] && progress.player[i] != ' '; i++)
      name[n++] = i ? tolower(progress.player[i]) : progress.player[i];
    name[n] = 0;
    if (!n) strcpy(name, "toi");
  }
  size_t o = 0;
  for (; *src && o + 4 < size; src++) {
    if (*src == '~') { strcpy(dst + o, name); o += strlen(name); }
    else dst[o++] = *src;
  }
  dst[o] = 0;
}

void enterScene(uint8_t id) {
  const Scene* s = findScene(id);
  if (!s) {
    if (scene && scene != &SCENE_SOON) previous = scene->id;
    s = &SCENE_SOON;
  }
  scene = s;
  phase = SCENE;
  bool before[ACT_COUNT];
  for (int i = 0; i < ACT_COUNT; i++) before[i] = isUnlocked((Activity)i);
  bool reward = giveRewards(s);
  for (int i = 0; i < ACT_COUNT; i++)
    if (!before[i] && isUnlocked((Activity)i)) {  // on l'annonce : sinon on ne le saurait pas
      showPopup("Nouveau dans le menu :", activityName((Activity)i));
    }
  if (s->id != SOON && (s->id < 47 || s->id > 49)) {  // compteurs de la partie, pour le score (sauf l'introduction)
    progress.runSteps++;
    if (!strncmp(s->text, "IMPASSE", 7)) progress.runImpasses++;
  }
  if (s->ending) {  // une fin : on calcule le score de la partie
    int score = (s->ending & END_LEGENDAIRE ? LEGEND_POINTS : END_POINTS)
              + CHARM_POINTS * countBits(progress.charms) + TROPHY_POINTS * countBits(progress.trophies)
              + max(0, SPEED_BONUS - POINTS_PER_STEP * progress.runSteps) - IMPASSE_PENALTY * progress.runImpasses;
    finalScore = max(score, 0);
    finalEnding = s->ending;
    int rank = scoreRank(finalScore);
    if (rank >= 0 && progress.player[0]) {  // il entre au classement, sous le prénom donné au début
      scoreAdd(progress.player, finalScore, finalEnding);
      char buf[24];
      snprintf(buf, sizeof(buf), "%s : %u points", progress.player, finalScore);
      showPopup(rank == 0 ? "Meilleur score !" : "Bienvenue au classement", buf);
    } else if (rank >= 0) {  // pas de prénom (partie commencée avec une ancienne version) : on le demande
      lastInitials(initials);
      letterPos = 0;
      askingName = false;
      phase = NAME_ENTRY;
    } else {
      char buf[24];
      snprintf(buf, sizeof(buf), "%u points", finalScore);
      showPopup("Score de la partie :", buf);
    }
  }
  if (s->id != SOON) progress.scene = s->id;
  progressSave();

  nVisible = 0;
  for (const Choice& c : s->choices)
    if (c.label && isVisible(c.cond) && nVisible < 4) visible[nVisible++] = &c;
  if (!nVisible) visible[nVisible++] = &CONTINUE;
  selected = 0;

  personalize(s->text, textBuf, sizeof(textBuf));
  const char* bar = strchr(textBuf, '|');  // « Titre|texte » : titre centré dans la bande jaune
  hasTitle = bar != nullptr;
  if (hasTitle) {
    int n = min((int)(bar - textBuf), 63);
    memcpy(lines[0], textBuf, n);
    lines[0][n] = 0;
    nLines = 1 + wrapText(bar + 1, lines + 1, TEXT_LINES - 1);
  } else nLines = wrapText(textBuf, lines, TEXT_LINES);
  if (s->id == FIRST_SCENE && !nameGiven && !progress.player[0]) {  // 1re partie : on demande d'abord le prénom
    lastInitials(initials);  // (pour en changer : Réglages, Tout effacer ; les scores restent)
    letterPos = 0;
    askingName = true;
    phase = NAME_ENTRY;
  }
  play(s->jingle, true);
  rewardUntil = reward ? millis() + 2500 : 0;
  dirty = true;
}

void startGame(uint32_t now) {
  if (scene->game == GAME_FOOTBALL || scene->game == GAME_PINBALL || scene->game == GAME_BREAKOUT || scene->game == GAME_BOSS || scene->game == GAME_RACE || scene->game == GAME_MAZE || scene->game == GAME_RAMEN) {
    phase = EXTERNAL_GAME;  // version courte du jeu complet, qui se débloque ensuite dans le menu
    if (scene->game == GAME_FOOTBALL) footballBegin(true);
    else if (scene->game == GAME_PINBALL) pinballBegin(true);
    else if (scene->game == GAME_BOSS) bossBegin(progress.charms);
    else if (scene->game == GAME_RACE) raceBegin(true);
    else if (scene->game == GAME_MAZE) mazeBegin(true);
    else if (scene->game == GAME_RAMEN) ramenBegin();
    else breakoutBeginStory();
    return;
  }
  phase = REFLEX_WAIT;
  stars = 0;
  phaseUntil = now + random(1500, 4000);
  dirty = true;
}

void updateReflex(uint32_t now, bool pressed) {
  switch (phase) {
    case REFLEX_WAIT:
      if (pressed) { reflexWon = false; phase = REFLEX_END; phaseUntil = now + 1200; play(SND_MISS, true); dirty = true; }
      else if (now >= phaseUntil) { phase = REFLEX_GO; phaseUntil = now + REFLEX_WINDOW[stars]; play(SND_GO, true); dirty = true; }
      break;
    case REFLEX_GO:
      if (pressed) {
        stars++;
        play(SND_STAR, true);
        if (stars >= REFLEX_STARS) { reflexWon = true; phase = REFLEX_END; phaseUntil = now + 800; }
        else { phase = REFLEX_WAIT; phaseUntil = now + random(1200, 3500); }
        dirty = true;
      } else if (now >= phaseUntil) {
        reflexWon = false; phase = REFLEX_END; phaseUntil = now + 1200; play(SND_MISS, true); dirty = true;
      }
      break;
    case REFLEX_END:
      if (now >= phaseUntil) enterScene(reflexWon ? scene->winTarget : scene->loseTarget);
      break;
    default: break;
  }
}

void moveSelection(int d) {
  if (nVisible < 2) return;
  selected = ((selected + d) % nVisible + nVisible) % nVisible;
  play(SND_TICK);
  dirty = true;
}

void drawCentered(const char* s, int baseline) {
  oled.setCursor((128 - oled.getUTF8Width(s)) / 2, baseline);
  oled.print(s);
}

void drawOled() {
  oled.clearBuffer();
  oled.setFont(u8g2_font_6x10_tf);
  if (phase == NAME_ENTRY) {  // saisie du prénom (début de partie) ou des initiales (score)
    char buf[24];
    if (askingName) snprintf(buf, sizeof(buf), "Ton prénom ?");
    else snprintf(buf, sizeof(buf), "Score : %u !", finalScore);
    drawCentered(buf, 11);  // bande jaune
    drawCentered(askingName ? "3 lettres, bouton = OK" : "Tes initiales :", 29);
    oled.setFont(u8g2_font_10x20_tf);
    for (int i = 0; i < 3; i++) {
      char c[2] = {initials[i], 0};
      oled.setCursor(36 + i * 20, 56);
      oled.print(c);
      if (i == letterPos) oled.drawHLine(34 + i * 20, 59, 14);  // lettre en cours
    }
    oled.sendBuffer();
    return;
  }
  if (phase != SCENE) {  // écran du mini-jeu de réflexe
    char buf[24];
    snprintf(buf, sizeof(buf), "Étoile %d / %d", stars < REFLEX_STARS ? stars + 1 : REFLEX_STARS, REFLEX_STARS);
    drawCentered(buf, 14);
    if (phase == REFLEX_WAIT)    drawCentered("Attends le vert...", 38);
    else if (phase == REFLEX_GO) drawCentered("MAINTENANT !", 38);
    else                         drawCentered(reflexWon ? "Bravo !" : "Raté... encore !", 38);
    oled.sendBuffer();
    return;
  }
  for (int i = 0; i < nLines; i++) {
    if (i == 0 && hasTitle) { drawCentered(lines[0], 11); continue; }
    oled.setCursor(0, i == 0 ? 11 : 15 + i * 9);  // 1re ligne dans la bande jaune, les suivantes dans le bleu
    oled.print(lines[i]);
  }
  if (popupTitle && millis() < popupUntil) {  // annonce par-dessus le texte (activité débloquée, score…)
    oled.setDrawColor(0);
    oled.drawBox(4, 18, 120, 33);
    oled.setDrawColor(1);
    oled.drawFrame(4, 18, 120, 33);
    drawCentered(popupTitle, 31);
    drawCentered(popupText, 45);
  }
  // Barre de choix en bas, en négatif
  oled.drawBox(0, 54, 128, 10);
  oled.setDrawColor(0);
  drawCentered(visible[selected]->label, 62);
  if (nVisible > 1) {
    oled.setCursor(1, 62);   oled.print("<");
    oled.setCursor(122, 62); oled.print(">");
  }
  oled.setDrawColor(1);
  oled.sendBuffer();
}

// --- Effets animés de la matrice --------------------------------------------
void drawEffect(Effect fx, uint32_t now) {
  switch (fx) {
    case FX_DARK:  // ruelle sombre
      fadeToBlackBy(leds, NUM_LEDS, 200);
      break;
    case FX_FOG:   // brume qui ondule
      for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++)
          nblend(leds[XY(x, y)], CRGB(70, 70, 80), inoise8(x * 40, y * 40, now / 4));
      break;
    case FX_WAVES:  // vagues en bas de l'image
      for (int y = 9; y < 16; y++)
        for (int x = 0; x < 16; x++) {
          uint8_t v = sin8(x * 24 + y * 40 + now / 6);
          if (y > 10 || v > 200) setPixel(x, y, CHSV(150, 220, 60 + v / 2));
        }
      break;
    case FX_RAIN:  // larmes qui tombent
      for (int i = 0; i < 8; i++) setPixel((i * 5 + 3) % 16, (now / 80 + i * 3) % 16, CRGB(0, 80, 255));
      break;
    case FX_DRIP: {  // tout devient blanc, de haut en bas
      int rows = (now / 120) % 18;
      for (int y = 0; y < rows && y < 16; y++)
        for (int x = 0; x < 16; x++) setPixel(x, y, CRGB(200, 200, 200));
      break;
    }
    case FX_BOUNCE:  // une balle qui rebondit
      setPixel((now / 90) % 16, 14 - quadwave8(now / 4) * 12 / 255, CRGB::White);
      break;
    case FX_CONFETTI:
      for (int i = 0; i < 18; i++) leds[random16(NUM_LEDS)] = CHSV(random8(), 220, 255);
      break;
    case FX_RAINBOW:
      for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++) setPixel(x, y, CHSV(x * 10 + y * 6 + now / 8, 255, 255));
      break;
    case FX_STARS:
    case FX_PORTAL:
      for (int i = 0; i < NUM_LEDS; i++) leds[i] = CRGB(0, 0, 12);
      for (int i = 0; i < 12; i++) setPixel((i * 7 + 3) % 16, (i * 11 + 5) % 16, CHSV(160, 60, sin8(now / 4 + i * 40)));
      if (fx == FX_PORTAL)  // anneau d'étoiles qui tourne
        for (int a = 0; a < 16; a++) {
          uint8_t angle = a * 16 + now / 6;
          setPixel(8 + ((int)cos8(angle) - 128) * 6 / 128, 8 + ((int)sin8(angle) - 128) * 6 / 128,
                   CHSV(a * 16 + now / 4, 200, 255));
        }
      break;
    case FX_FIREWORKS:  // trois bouquets qui éclatent
      for (int k = 0; k < 3; k++) {
        uint32_t t = now + k * 233;
        uint16_t seed = t / 700 * 7 + k * 13;
        int cx = seed * 37 % 12 + 2, cy = seed * 53 % 10 + 2, r = (t % 700) / 120;
        static const int8_t DIRS[8][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
        for (auto& d : DIRS) setPixel(cx + d[0] * r, cy + d[1] * r, CHSV(seed * 40, 200, 255 - r * 35));
      }
      break;
    default: break;
  }
}

// Couleur de la LED : chaleur (bleu → orange) sur les chemins, couleur de la scène sinon
CRGB ledColor(uint32_t now) {
  if (scene->heat < 0) return CRGB(scene->led);
  CRGB c = blend(CRGB(0, 60, 255), CRGB(255, 50, 0), scene->heat * 63);
  uint8_t pulse = 120 + sin8(now / (12 - scene->heat * 2)) / 2;  // bat plus vite quand c'est chaud
  return c.nscale8(pulse);
}

void renderMatrix(uint32_t now) {
  FastLED.clear();
  if (phase == SCENE) {
    drawPicture(scene->picture);
    drawEffect(scene->effect, now);
    // Barre d'état : les 3 charmes en haut à droite (éteints tant qu'ils ne sont pas gagnés)
    const uint8_t charms[3] = {CHARM_BOL, CHARM_FEUILLE, CHARM_NOTE};
    const CRGB    colors[3] = {CRGB(255, 90, 0), CRGB(0, 200, 0), CRGB(255, 200, 0)};
    for (int i = 0; i < 3; i++) setPixel(13 + i, 0, progress.charms & charms[i] ? colors[i] : CRGB(20, 20, 20));
  } else if (phase == NAME_ENTRY) {
    drawEffect(FX_FIREWORKS, now);
  } else if (phase == REFLEX_GO) {
    fill_solid(leds, NUM_LEDS, CRGB(0, 160, 0));
  } else {
    drawEffect(FX_STARS, now);
    for (int i = 0; i < stars; i++) setPixel(6 + i * 2, 15, CRGB(255, 200, 0));  // étoiles déjà attrapées
  }
  if (now < rewardUntil) {  // pluie d'étincelles quand on gagne quelque chose
    for (int i = 0; i < 6; i++) leds[random16(NUM_LEDS)] = CHSV(random8(), 180, 255);
    setRgb(CHSV(now / 4, 255, 255));
  } else if (phase == REFLEX_GO) {
    setRgb(CRGB::Green);
  } else if (phase == NAME_ENTRY) {
    setRgb(CHSV(now / 4, 255, 255));
  } else {
    setRgb(ledColor(now));
  }
  FastLED.show();
}

}  // namespace

// Coupe le texte en lignes d'au plus 21 caractères (un accent compte pour un),
// aux espaces ou juste après un tiret ; renvoie le nombre de lignes
int wrapText(const char* s, char out[][64], int maxLines) {
  int n = 0;
  while (*s && n < maxLines) {
    while (*s == ' ') s++;
    const char* p = s;
    const char* cut = nullptr;
    int count = 0;
    while (*p && count < LINE_CHARS) {
      const char* next = p + 1;
      while ((*next & 0xC0) == 0x80) next++;  // octets suivants d'un caractère accentué
      bool beforePunct = p[1] && strchr("!?:;", p[1]);  // « obtenue ! » : le ! reste avec son mot
      if (*p == ' ' && !beforePunct) cut = p;  // couper avant l'espace
      else if (*p == '-') cut = next;         // couper après le tiret
      count++;
      p = next;
    }
    const char* end;
    if (!*p || (*p == ' ' && !(p[1] && strchr("!?:;", p[1])))) end = p;  // fin du texte, ou pile sur un espace
    else end = cut && cut > s ? cut : p;      // sinon on revient à la dernière coupure possible
    int len = end - s;
    if (len > 63) len = 63;
    memcpy(out[n], s, len);
    out[n][len] = 0;
    n++;
    s = end;
  }
  return n;
}

void adventureBegin() {
  encBase = encoderCount();
  scene = nullptr;
  if (!progress.scene) { progress.runSteps = 0; progress.runImpasses = 0; }  // nouvelle partie
  enterScene(progress.scene ? progress.scene : FIRST_SCENE);  // nouvelle partie : la quête est d'abord expliquée
}

void adventureGoto(uint8_t sceneId) {
  enterScene(sceneId);
}

bool adventureLoop(uint32_t now) {
  static uint32_t lastFrame = 0;

  if (phase == EXTERNAL_GAME) {  // le jeu lit lui-même les boutons et dessine tout
    GameResult r;
    if (scene->game == GAME_FOOTBALL)     r = footballLoop(now);
    else if (scene->game == GAME_PINBALL) r = pinballLoop(now);
    else if (scene->game == GAME_BOSS)    r = bossLoop(now);
    else if (scene->game == GAME_RACE)    { r = raceLoop(now); if (r == GAME_QUIT) r = GAME_LOST; }
    else if (scene->game == GAME_MAZE)    r = mazeLoop(now);
    else if (scene->game == GAME_RAMEN)   r = ramenLoop(now);
    else r = breakoutLoop(now) ? GAME_RUNNING : breakoutWon() ? GAME_WON : GAME_LOST;
    if (r != GAME_RUNNING) {
      encBase = encoderCount();
      enterScene(r == GAME_WON ? scene->winTarget : r == GAME_QUIT ? scene->id : scene->loseTarget);  // abandon : on reste là
    }
    return true;
  }
  bool ok = btn1.pressed();
  ok |= btnEnc.pressed();
  bool back = btn2.pressed();
  if (phase == NAME_ENTRY) {
    int32_t d = (encoderCount() - encBase) / 4;
    if (d) { encBase += d * 4; }
    float y = joyY();
    int8_t dir = y > 0.6f ? 1 : y < -0.6f ? -1 : 0;
    int step = d + (dir && dir != joyWas ? dir : 0);
    joyWas = dir;
    if (step) {  // A à Z, puis un espace
      static const char LETTERS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
      const char* at = strchr(LETTERS, initials[letterPos]);
      int i = at ? at - LETTERS : 0;
      initials[letterPos] = LETTERS[((i + step) % 27 + 27) % 27];
      play(SND_TICK);
      dirty = true;
    }
    if (ok && ++letterPos >= 3 && askingName) {  // prénom validé : l'histoire commence
      memcpy(progress.player, initials, 4);
      progressSave();
      askingName = false;
      phase = SCENE;
      nameGiven = true;  // on réaffiche la scène avec le prénom, sans le redemander
      enterScene(FIRST_SCENE);
      nameGiven = false;
    } else if (ok && letterPos >= 3) {  // 3e lettre validée : on enregistre le score
      memcpy(progress.player, initials, 4);
      int rank = scoreAdd(initials, finalScore, finalEnding);
      char buf[24];
      snprintf(buf, sizeof(buf), "%s : %u points", initials, finalScore);
      showPopup(rank == 0 ? "Meilleur score !" : "Bienvenue au classement", buf);
      phase = SCENE;
      play(J_FANFARE, true);
    }
    if (ok) dirty = true;
  } else if (phase != SCENE) {
    updateReflex(now, ok);
  } else {
    int32_t d = (encoderCount() - encBase) / 4;  // un cran de l'encodeur = un choix
    if (d) {
      encBase += d * 4;
      moveSelection(d);
    }
    float x = joyX();
    int8_t dir = x > 0.6f ? 1 : x < -0.6f ? -1 : 0;
    if (dir && dir != joyWas) moveSelection(dir);
    joyWas = dir;

    if (back) return false;  // bouton du joystick : retour au menu (la progression est gardée)
    if (ok) {  // le jingle de la scène suivante sert de son de validation
      if (scene == &SCENE_SOON) enterScene(previous);
      else if (visible[selected]->target == GAME) startGame(now);
      else {
        if (scene->ending) { progress.runSteps = 0; progress.runImpasses = 0; }  // « Rejouer » : nouvelle partie
        enterScene(visible[selected]->target);
      }
    }
  }
  if (phase != SCENE && phase != NAME_ENTRY) encBase = encoderCount();  // l'encodeur ne compte pas pendant le mini-jeu

  if (now - lastFrame >= 33) {  // ~30 images/s
    lastFrame = now;
    renderMatrix(now);
  }
  if (popupTitle && now >= popupUntil) { popupTitle = nullptr; dirty = true; }  // fin de l'annonce
  if (dirty) { drawOled(); dirty = false; }
  return true;
}
