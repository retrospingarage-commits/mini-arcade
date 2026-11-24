// Cercle de rythme : un jeu de rythme où des lumières partent du centre vers un anneau de 8 cibles.
// Il faut agir quand la lumière arrive sur l'anneau, en rythme avec la mélodie :
//   - lumière colorée vers une cible : pousser le joystick dans cette direction (8 directions, comme les 8 cibles) ;
//   - cercle doré qui grandit depuis le centre : appuyer sur le bouton quand il touche l'anneau ;
//   - toupie bleue au centre : tourner l'encodeur pendant qu'elle tourne.
// La mélodie joue toute seule (le buzzer ne fait qu'une note à la fois) : les touches se voient sur la matrice et à la
// LED. Les morceaux sont dans include/songs.h ; le tracé (quelles notes, quelle direction) en est déduit
// automatiquement : la direction suit la hauteur de la note, la difficulté retire des notes et réduit les marges.
// Menu : encodeur ou joystick haut/bas pour choisir, bouton pour jouer, bouton du joystick pour quitter.
#include "hw.h"
#include "save.h"
#include "songs.h"
#include "activities.h"

namespace circle {

// --- Réglages (selon la difficulté : facile, moyen, difficile) ---------------
inline int      perfectMs()  { return (int)byLevel(95, 60, 42); }       // marge d'un « Parfait » (±ms)
inline int      goodMs()     { return (int)byLevel(190, 115, 82); }    // marge d'un « Bien » (±ms)
inline uint32_t approachMs() { return (uint32_t)byLevel(2200, 1650, 1200); }  // temps de trajet centre → anneau
inline float    gapBeats()   { return byLevel(1.1f, 0.65f, 0.3f); }     // écart minimal entre deux notes jouées (en temps)
inline float    spinK()      { return byLevel(1.0f, 1.4f, 1.9f); }      // effort demandé à la toupie
constexpr int      MIN_GAP_MS   = 160;   // jamais deux notes plus proches que ça
constexpr float    STICK_LEVEL  = 0.55f; // le joystick poussé plus que ça compte
constexpr uint32_t COUNT_MS     = 1500;  // décompte avant le morceau
constexpr int      PTS_PERFECT  = 100, PTS_GOOD = 60;  // points d'une note (la toupie vaut le double)
// Rangs, en % des points possibles
constexpr int      RANK_S = 95, RANK_A = 85, RANK_B = 70, RANK_C = 50;
// ---------------------------------------------------------------------------

constexpr float PI_F = 3.14159265f;
constexpr int   MAX_EV = 220, MAX_CH = 170;

// Les 8 cibles : haut, haut-droite, droite… (sens des aiguilles d'une montre), à 6 cases du centre (8, 8)
const float TX[8] = {8, 12.24f, 14, 12.24f, 8, 3.76f, 2, 3.76f};
const float TY[8] = {2, 3.76f, 8, 12.24f, 14, 12.24f, 8, 3.76f};
const char* const DIR_NAMES[8] = {"haut", "haut-droite", "droite", "bas-droite", "bas", "bas-gauche", "gauche", "haut-gauche"};

enum NoteType : uint8_t { TAP, GOLD, SPIN };
enum NoteState : uint8_t { PENDING, PERFECT, GOOD, MISS, ACTIVE };
struct Ev { uint32_t t; uint16_t dur; uint16_t freq; char mark; int8_t midi; };   // une note de la mélodie
struct GNote { uint32_t t; uint16_t dur; NoteType type; uint8_t dir; NoteState state; int16_t got, need; };  // une note à jouer
Ev    ev[MAX_EV];
GNote ch[MAX_CH];
int   nEv, nCh, evNext;

enum Mode : uint8_t { SELECT, HELP, COUNT, PLAY, RESULT };
constexpr int HELP_PAGES = 3;
uint8_t helpPage;
bool    helpSeen;   // vrai = on saute l'explication (utilisé par le banc d'essai) ; sur la borne elle s'affiche à chaque morceau
Mode     mode;
int      sel;                                   // morceau choisi
uint32_t clock0, startAt, endAt, lastOled, flashUntil[8], goldUntil, spinUntil, judgeAt, modeAt;
uint32_t flashColor[8];
uint32_t burstAt[8];   // début de la gerbe d'étincelles sur chaque cible (0 = aucune)
int      score, maxScore, combo, bestCombo, nPerfect, nGood, nMiss, lastJudge, pct;  // lastJudge : 0 parfait, 1 bien, 2 raté
bool     dirty, newRecord;
int32_t  encBase;
int8_t   lastSector;

const Note SND_RANK_HI[] = {{523, 100}, {659, 100}, {784, 100}, {1047, 120}, {784, 100}, {1047, 100}, {1319, 400}, {0, 0}};
const Note SND_RANK_MID[] = {{523, 120}, {659, 120}, {784, 300}, {0, 0}};
const Note SND_RANK_LOW[] = {{392, 200}, {349, 200}, {330, 400}, {0, 0}};
const Note SND_START[]  = {{1568, 80}, {0, 0}};

// --- Lecture d'un morceau ---------------------------------------------------
int semitone(char letter) {
  switch (letter) { case 'C': return 0; case 'D': return 2; case 'E': return 4; case 'F': return 5;
                    case 'G': return 7; case 'A': return 9; default: return 11; }
}

void parseSong(const Song& s) {
  nEv = 0;
  const char* p = s.notes;
  float sixteenth = 15000.0f / s.bpm, pos = 0;   // en ms ; pos = début de la note en doubles croches
  int t16 = 0;
  while (*p && nEv < MAX_EV) {
    while (*p == ' ' || *p == '|') p++;
    if (!*p) break;
    bool rest = *p == 'R';
    int midi = 0;
    if (rest) p++;
    else {
      midi = 12 * 1 + semitone(*p++);
      if (*p == '#') { midi++; p++; } else if (*p == 'b') { midi--; p++; }
      midi += 12 * (*p++ - '0');
    }
    if (*p == ':') p++;
    int len = 0;
    while (*p >= '0' && *p <= '9') len = len * 10 + (*p++ - '0');
    char mark = 0;
    if (*p == '!' || *p == '~') mark = *p++;
    pos = t16 * sixteenth;
    if (!rest)
      ev[nEv++] = {(uint32_t)pos, (uint16_t)(len * sixteenth), (uint16_t)(440.0f * powf(2.0f, (midi - 69) / 12.0f)), mark, (int8_t)midi};
    t16 += len;
  }
}

// Choisit les notes à jouer : toutes les notes dorées et tournantes, et les autres si elles sont assez espacées.
// La direction suit la hauteur : la note la plus grave est en bas, puis on tourne dans le sens des aiguilles d'une montre.
void buildChart(const Song& s) {
  nCh = 0; maxScore = 0;
  int lo = 127, hi = 0;
  for (int i = 0; i < nEv; i++) { lo = min(lo, (int)ev[i].midi); hi = max(hi, (int)ev[i].midi); }
  float gap = max((float)MIN_GAP_MS, 60000.0f / s.bpm * gapBeats());
  bool have = false;
  uint32_t lastT = 0, lastDirT = 0;
  int lastDir = -1;
  for (int i = 0; i < nEv && nCh < MAX_CH; i++) {
    const Ev& e = ev[i];
    if (!e.mark && have && e.t - lastT < gap) continue;
    GNote g = {e.t, e.dur, e.mark == '!' ? GOLD : e.mark == '~' ? SPIN : TAP, 0, PENDING, 0, 0};
    if (g.type == TAP) {
      float u = (e.midi - lo) / (float)max(1, hi - lo);
      int dir = (4 + (int)roundf(u * 7)) % 8;
      if (dir == lastDir && e.t - lastDirT < 420) dir = (dir + ((nCh & 1) ? 1 : 7)) & 7;  // jamais deux fois la même direction de suite, vite
      g.dir = dir; lastDir = dir; lastDirT = e.t;
    }
    if (g.type == SPIN) g.need = constrain((int)(e.dur / 110.0f * spinK()), 5, 30);
    maxScore += (g.type == SPIN ? 2 : 1) * PTS_PERFECT;
    ch[nCh++] = g;
    lastT = e.t; have = true;
  }
}

char rankOf(int p) { return p >= RANK_S ? 'S' : p >= RANK_A ? 'A' : p >= RANK_B ? 'B' : p >= RANK_C ? 'C' : 'D'; }
uint8_t& bestOf(int song) { return progress.circleBest[song][progress.difficulty % 3]; }

// --- Jeu --------------------------------------------------------------------
void startSong(uint32_t now) {
  parseSong(SONGS[sel]);
  buildChart(SONGS[sel]);
  mode = COUNT; modeAt = now;
  clock0 = now + max(COUNT_MS, approachMs() + 500);
  evNext = 0; score = combo = bestCombo = nPerfect = nGood = nMiss = 0; lastJudge = 0; judgeAt = 0;
  for (int d = 0; d < 8; d++) flashUntil[d] = burstAt[d] = 0;
  goldUntil = spinUntil = 0;
  lastSector = -1; lastOled = 0;
  encBase = encoderCount();
  uint32_t last = 0;
  for (int i = 0; i < nEv; i++) last = max(last, ev[i].t + ev[i].dur);
  endAt = clock0 + last + 900;
  play(SND_START);
  dirty = true;
}

void registerJudge(int j, int points, uint32_t now) {   // j : 0 parfait, 1 bien, 2 raté
  lastJudge = j; judgeAt = now; dirty = true;
  if (j == 2) { nMiss++; combo = 0; return; }
  (j == 0 ? nPerfect : nGood)++;
  score += points;
  combo++; bestCombo = max(bestCombo, combo);
}

void hitNote(GNote& g, int dt, uint32_t now) {
  bool perfect = abs(dt) <= perfectMs();
  g.state = perfect ? PERFECT : GOOD;
  registerJudge(perfect ? 0 : 1, perfect ? PTS_PERFECT : PTS_GOOD, now);
}

inline int stickTolerance() { return (int)byLevel(1, 0, 0); }   // facile : une direction voisine compte aussi
inline bool dirMatches(int a, int b) { int d = abs(a - b); d = min(d, 8 - d); return d <= stickTolerance(); }

GNote* findPending(NoteType type, int dir, int32_t s) {
  GNote* best = nullptr;
  int bestDt = goodMs() + 1;
  for (int i = 0; i < nCh; i++) {
    GNote& g = ch[i];
    if (g.state != PENDING || g.type != type || (type == TAP && !dirMatches(g.dir, dir))) continue;
    int dt = abs((int32_t)g.t - s);
    if (dt <= goodMs() && dt < bestDt) { best = &g; bestDt = dt; }
  }
  return best;
}

void finishSong(uint32_t now) {
  pct = maxScore ? 100 * score / maxScore : 0;
  uint8_t& b = bestOf(sel);
  newRecord = pct > b;
  if (newRecord) { b = (uint8_t)pct; progressSave(); }
  mode = RESULT; modeAt = now;
  char r = rankOf(pct);
  play(r == 'S' || r == 'A' ? SND_RANK_HI : r == 'B' || r == 'C' ? SND_RANK_MID : SND_RANK_LOW, true);
  dirty = true;
}

void updatePlay(uint32_t now, bool gold) {
  int32_t s = (int32_t)(now - clock0);
  // La mélodie
  while (evNext < nEv && s >= (int32_t)ev[evNext].t) {
    beep(ev[evNext].freq, max(50, (int)(ev[evNext].dur * 0.92f)));
    evNext++;
  }
  // Commandes
  float jx = joyX(), jy = joyY();
  int8_t sector = -1;
  if (hypotf(jx, jy) > STICK_LEVEL) sector = (int8_t)(((int)floorf(atan2f(jx, -jy) / (PI_F / 4) + 0.5f) + 8) % 8);
  if (sector >= 0 && (sector != lastSector || progress.difficulty == 0))   // facile : joystick tenu dans la direction = ça compte
    if (GNote* g = findPending(TAP, sector, s)) { hitNote(*g, (int32_t)g->t - s, now); flashUntil[g->dir] = now + 260; burstAt[g->dir] = now; flashColor[g->dir] = g->state == PERFECT ? 0xFFFFFF : 0x30FF30; }
  lastSector = sector;
  if (gold)
    if (GNote* g = findPending(GOLD, 0, s)) { hitNote(*g, (int32_t)g->t - s, now); goldUntil = now + 220; }
  int32_t pulses = abs(encoderCount() - encBase);
  encBase = encoderCount();
  // Les notes : ratées si trop tard, toupies en cours
  for (int i = 0; i < nCh; i++) {
    GNote& g = ch[i];
    if (g.type == SPIN) {
      if (g.state == PENDING && s >= (int32_t)g.t - 250) g.state = ACTIVE;
      if (g.state == ACTIVE) {
        g.got += pulses;
        if (s >= (int32_t)(g.t + g.dur)) {
          float r = g.got / (float)g.need;
          if (r >= 1.0f) { g.state = PERFECT; registerJudge(0, 2 * PTS_PERFECT, now); spinUntil = now + 300; }
          else if (r >= 0.5f) { g.state = GOOD; registerJudge(1, 2 * PTS_GOOD, now); spinUntil = now + 300; }
          else { g.state = MISS; registerJudge(2, 0, now); }
        }
      }
    } else if (g.state == PENDING && s > (int32_t)g.t + goodMs()) {
      g.state = MISS;
      if (g.type == TAP) { flashUntil[g.dir] = now + 260; burstAt[g.dir] = 0; flashColor[g.dir] = 0xFF0000; }
      registerJudge(2, 0, now);
    }
  }
  if (now >= endAt) finishSong(now);
}

// --- Affichage --------------------------------------------------------------
void drawBlock(float cx, float cy, CRGB c) {   // petit carré de 2 x 2 pixels centré en (cx, cy)
  int x = (int)roundf(cx - 1), y = (int)roundf(cy - 1);
  setPixel(x, y, c); setPixel(x + 1, y, c); setPixel(x, y + 1, c); setPixel(x + 1, y + 1, c);
}

void drawRing(float radius, CRGB c) {
  int n = max(8, (int)(radius * 6));
  for (int i = 0; i < n; i++) {
    float a = 2 * PI_F * i / n;
    setPixel((int)floorf(8 + radius * sinf(a)), (int)floorf(8 - radius * cosf(a)), c);
  }
}

void renderField(uint32_t now, int32_t s, bool showNotes) {
  FastLED.clear();
  drawRing(6.0f, CRGB(5, 5, 12));
  for (int d = 0; d < 8; d++) {
    bool flash = now < flashUntil[d];
    CRGB c = flash ? CRGB(flashColor[d]) : CRGB(CHSV(d * 32, 255, 70));
    drawBlock(TX[d], TY[d], c);
  }
  for (int d = 0; d < 8; d++) {   // gerbe d'étincelles autour de la cible touchée
    if (!burstAt[d] || now - burstAt[d] > 260) continue;
    float age = (now - burstAt[d]) / 260.0f;
    CRGB c = CRGB(flashColor[d]).nscale8((uint8_t)(255 * (1.0f - age)));
    for (int k = 0; k < 8; k++) {
      float a = k * PI_F / 4, r = 1.5f + 2.5f * age;
      setPixel((int)floorf(TX[d] + 0.5f + r * cosf(a)), (int)floorf(TY[d] + 0.5f + r * sinf(a)), c);
    }
  }
  drawBlock(8, 8, CRGB(25, 25, 35));
  if (now < goldUntil) drawRing(6.0f, CRGB(255, 190, 0));
  if (!showNotes) return;
  uint32_t ap = approachMs();
  for (int i = 0; i < nCh; i++) {
    const GNote& g = ch[i];
    if (g.state == PERFECT || g.state == GOOD || g.state == MISS) continue;
    if (g.type == SPIN) {   // la toupie : visible un peu avant, puis tant que la note dure
      if (s < (int32_t)g.t - 450 || s > (int32_t)(g.t + g.dur)) continue;
      float prog = g.state == ACTIVE ? constrain(g.got / (float)g.need, 0.0f, 1.0f) : 0.0f;
      float ph = now / 70.0f;
      for (int r = 1; r <= 4; r++)
        for (int k = 0; k < 2; k++) {
          float a = ph + k * PI_F;
          setPixel((int)floorf(8 + r * cosf(a)), (int)floorf(8 + r * sinf(a)), CHSV(140, 255, g.state == ACTIVE ? 255 : 90));
        }
      int lit = (int)(prog * 20);
      for (int j = 0; j < lit; j++) {
        float a = 2 * PI_F * j / 20;
        setPixel((int)floorf(8 + 5 * sinf(a)), (int)floorf(8 - 5 * cosf(a)), CRGB(0, 200, 255));
      }
      continue;
    }
    float p = 1.0f - ((int32_t)g.t - s) / (float)ap;
    if (p < 0 || p > 1.18f) continue;
    if (g.type == GOLD) { drawRing(6.0f * p, CRGB(255, 190, 0).nscale8(p > 1 ? 120 : 255)); continue; }
    float x = 8 + (TX[g.dir] - 8) * p, y = 8 + (TY[g.dir] - 8) * p;
    for (int tr = 1; tr <= 2; tr++) {   // traînée
      float q = max(0.0f, p - 0.09f * tr);
      setPixel((int)floorf(8 + (TX[g.dir] - 8) * q), (int)floorf(8 + (TY[g.dir] - 8) * q), CHSV(g.dir * 32, 255, 120 / tr));
    }
    drawBlock(x, y, CHSV(g.dir * 32, 140, 255));
  }
}

void renderResult(uint32_t now) {
  FastLED.clear();
  char r[2] = {rankOf(pct), 0};
  char p[8];
  snprintf(p, sizeof(p), "%d", pct);
  CRGB c = r[0] == 'S' ? CRGB(255, 200, 0) : r[0] == 'A' ? CRGB(255, 80, 160) : r[0] == 'B' ? CRGB(80, 160, 255) : CRGB(150, 150, 150);
  drawText(r, (16 - textWidth(r)) / 2, 3, c);
  drawText(p, (16 - textWidth(p)) / 2, 10, CRGB(200, 200, 200));
  if (r[0] == 'S' || r[0] == 'A') for (int i = 0; i < 18; i++) leds[random16(NUM_LEDS)] = CHSV(random8(), 200, 255);
  for (int d = 0; d < 8; d++) drawBlock(TX[d], TY[d], CRGB(CHSV(d * 32 + now / 8, 255, 90)));
}

void renderSelect(uint32_t now) {
  renderField(now, 0, false);
  for (int i = 0; i < NUM_SONGS; i++) setPixel(3 + i * 2, 15, i == sel ? CRGB(255, 255, 255) : CRGB(40, 40, 40));  // un point par morceau
  // une lumière de démonstration qui tourne autour de l'anneau
  int d = (now / 350) % 8;
  drawBlock(TX[d], TY[d], CHSV(d * 32, 160, 255));
}

// Explication : une lumière de démonstration qui part du centre, avec ce qu'on voit quand c'est réussi ou raté
void renderHelp(uint32_t now) {
  constexpr uint32_t CYCLE = 2600, TRAVEL = 1700;
  static uint32_t lastCycle = 0xFFFF;
  uint32_t cyc = (now - modeAt) / CYCLE, t = (now - modeAt) % CYCLE;
  int d = (cyc * 3) % 8;
  if (cyc != lastCycle) { lastCycle = cyc; for (int i = 0; i < 8; i++) burstAt[i] = flashUntil[i] = 0; }
  bool arrived = t >= TRAVEL;
  int outcome = helpPage == 1 ? (int)(cyc % 3) : 0;   // page 2 : Parfait, Bien, puis Raté tour à tour
  if (arrived && !flashUntil[d]) {
    flashUntil[d] = now + 400;
    if (outcome == 2) { flashColor[d] = 0xFF0000; burstAt[d] = 0; }
    else { flashColor[d] = outcome == 0 ? 0xFFFFFF : 0x30FF30; burstAt[d] = now; }
  }
  renderField(now, 0, false);
  float p = min(1.0f, t / (float)TRAVEL);
  if (helpPage == 2) {   // cercle doré qui grandit : on appuie sur le bouton quand il touche l'anneau
    if (!arrived) drawRing(6.0f * p, CRGB(255, 190, 0));
    else if (t < TRAVEL + 350) drawRing(6.0f, CRGB(255, 190, 0));
  } else if (!arrived) {
    float x = 8 + (TX[d] - 8) * p, y = 8 + (TY[d] - 8) * p;
    for (int tr = 1; tr <= 2; tr++) {
      float q = max(0.0f, p - 0.09f * tr);
      setPixel((int)floorf(8 + (TX[d] - 8) * q), (int)floorf(8 + (TY[d] - 8) * q), CHSV(d * 32, 255, 120 / tr));
    }
    drawBlock(x, y, CHSV(d * 32, 140, 255));
  }
}

void drawOled(uint32_t now) {
  oled.clearBuffer();
  oled.setFont(u8g2_font_6x10_tf);
  char buf[32];
  if (mode == SELECT) {
    snprintf(buf, sizeof(buf), "Rythme (%s)", difficultyName());
    oled.setCursor(0, 11);  // bande jaune
    oled.print(buf);
    int top = constrain(sel - 2, 0, NUM_SONGS - 4);
    for (int i = 0; i < 4; i++) {
      int idx = top + i, baseline = 26 + i * 12;
      if (idx == sel) { oled.drawBox(0, baseline - 9, 128, 11); oled.setDrawColor(0); }
      oled.setCursor(4, baseline);
      oled.print(SONGS[idx].name);
      uint8_t b = bestOf(idx);
      char rk[2] = {b ? rankOf(b) : '-', 0};
      oled.setCursor(118, baseline);
      oled.print(rk);
      oled.setDrawColor(1);
    }
  } else if (mode == HELP) {
    static const char* const T[HELP_PAGES] = {"Comment jouer", "Réussi ou raté ?", "Cercle et toupie"};
    static const char* const L[HELP_PAGES][4] = {
      {"Une lumière part du", "centre vers le bord.", "Quand elle arrive sur", "la cible : joystick !"},
      {"Blanc : Parfait !", "Vert : Bien", "Rouge : raté (trop tôt", "ou trop tard)"},
      {"Cercle doré : appuie", "sur le bouton à temps", "Toupie bleue : tourne", "l'encodeur vite !"}};
    oled.setCursor(0, 11);  // bande jaune
    oled.print(T[helpPage]);
    for (int i = 0; i < 4; i++) { oled.setCursor(0, 25 + i * 11); oled.print(L[helpPage][i]); }
    snprintf(buf, sizeof(buf), "%d/%d", helpPage + 1, HELP_PAGES);
    oled.setCursor(110, 62); oled.print(buf);
    oled.setCursor(0, 62); oled.print(helpPage + 1 < HELP_PAGES ? "Bouton : suite" : "Bouton : jouer !");
  } else if (mode == RESULT) {
    oled.setCursor(0, 11);
    oled.print(SONGS[sel].name);
    snprintf(buf, sizeof(buf), "Score %d", score);
    oled.setCursor(0, 26); oled.print(buf);
    snprintf(buf, sizeof(buf), "P %d   B %d   R %d", nPerfect, nGood, nMiss);
    oled.setCursor(0, 37); oled.print(buf);
    oled.setFont(u8g2_font_10x20_tf);
    snprintf(buf, sizeof(buf), "%c  %d %%", rankOf(pct), pct);
    oled.setCursor(0, 58); oled.print(buf);
    oled.setFont(u8g2_font_6x10_tf);
    if (newRecord) { oled.setCursor(84, 58); oled.print("Record !"); }
  } else {
    oled.setCursor(0, 11);
    oled.print(SONGS[sel].name);
    oled.setFont(u8g2_font_logisoso24_tn);
    oled.setCursor(0, 42); oled.print(score);
    oled.setFont(u8g2_font_6x10_tf);
    if (combo > 1) { snprintf(buf, sizeof(buf), "x%d", combo); oled.setCursor(128 - oled.getUTF8Width(buf), 30); oled.print(buf); }
    oled.setCursor(0, 62);
    if (mode == COUNT) oled.print("Prépare-toi !");
    else if (now - judgeAt < 700) oled.print(lastJudge == 0 ? "Parfait !" : lastJudge == 1 ? "Bien" : "Raté...");
    else oled.print("Joystick : quitter");
  }
  oled.sendBuffer();
}

void begin() {
  mode = SELECT; sel = constrain(sel, 0, NUM_SONGS - 1);
  encBase = encoderCount();
  lastSector = -1;
  dirty = true;
}

GameResult loop(uint32_t now) {
  static uint32_t lastFrame = 0;
  static int8_t jyWas = 0;
  bool fire = btn1.pressed();
  fire |= btnEnc.pressed();
  bool quit = btn2.pressed();

  switch (mode) {
    case SELECT: {
      if (quit) return GAME_QUIT;
      int32_t d = (encoderCount() - encBase) / 4;
      float jy = joyY();
      int8_t dir = jy > 0.6f ? 1 : jy < -0.6f ? -1 : 0;
      int step = (int)d + (dir && dir != jyWas ? dir : 0);
      jyWas = dir;
      if (d) encBase += d * 4;
      if (step) { sel = ((sel + step) % NUM_SONGS + NUM_SONGS) % NUM_SONGS; dirty = true; beep(1200, 10); }
      if (fire) {
        if (helpSeen) startSong(now);
        else { mode = HELP; helpPage = 0; modeAt = now; for (int i = 0; i < 8; i++) burstAt[i] = flashUntil[i] = 0; dirty = true; }
      }
      break;
    }
    case HELP:
      if (quit) { mode = SELECT; dirty = true; break; }
      if (fire) {
        if (++helpPage >= HELP_PAGES) startSong(now);
        else { modeAt = now; for (int i = 0; i < 8; i++) burstAt[i] = flashUntil[i] = 0; dirty = true; }
      }
      break;
    case COUNT:
      if (quit) { mode = SELECT; dirty = true; break; }
      if ((int32_t)(now - clock0) >= -300) { mode = PLAY; dirty = true; }   // les premières notes arrivent
      break;
    case PLAY:
      if (quit) { mode = SELECT; dirty = true; ledcWriteTone(PIN_BUZZER, 0); break; }
      updatePlay(now, fire);
      break;
    case RESULT:
      if (fire || quit) { mode = SELECT; dirty = true; }
      break;
  }

  // Images : ~45 par seconde (la matrice bloque un instant à chaque image)
  if (now - lastFrame >= 22) {
    lastFrame = now;
    int32_t s = (int32_t)(now - clock0);
    if (mode == SELECT) renderSelect(now);
    else if (mode == HELP) renderHelp(now);
    else if (mode == RESULT) renderResult(now);
    else {
      renderField(now, s, true);
      if (mode == COUNT) {   // décompte : 3, 2, 1
        int n = 3 - (int)((now - modeAt) * 3 / COUNT_MS);
        if (n >= 1 && n <= 3) { char c[2] = {(char)('0' + n), 0}; drawText(c, 6, 5, CRGB(255, 255, 255)); }
      }
    }
    FastLED.show();
    // LED : couleur du dernier jugement, arc-en-ciel en grand combo
    if (mode == PLAY && now - judgeAt < 260) setRgb(lastJudge == 0 ? CRGB(255, 255, 255) : lastJudge == 1 ? CRGB(0, 255, 160) : CRGB(255, 0, 0));
    else if (mode == PLAY && combo >= 10) setRgb(CHSV(now / 4, 255, 255));
    else setRgb(CHSV(now / 40, 200, 90));
  }

  // L'OLED bloque ~25 ms : pendant le morceau, on ne le met à jour que loin d'une note à jouer
  bool nearNote = false;
  if (mode == PLAY) {
    int32_t s = (int32_t)(now - clock0);
    for (int i = 0; i < nCh && !nearNote; i++)
      if ((ch[i].state == PENDING || ch[i].state == ACTIVE) && abs((int32_t)ch[i].t - s) < goodMs() + 60) nearNote = true;
  }
  bool canDraw = mode != PLAY || (now - lastOled > 400 && (!nearNote || now - lastOled > 1500));
  if (dirty && canDraw) { drawOled(now); dirty = false; lastOled = now; }
  return GAME_RUNNING;
}

}  // namespace circle

// Les noms de ce jeu (PENDING, PLAY…) existent aussi dans les bibliothèques de la carte : tout reste dans l'espace de noms
void circleBegin() { circle::begin(); }
GameResult circleLoop(uint32_t now) { return circle::loop(now); }
