// Labyrinthe des sous-sols du château : 9 salles (3 x 3), vues du dessus, dans le noir.
// Joystick : se déplacer (le garder poussé pour avancer). Passer une porte au bord de la matrice = salle voisine.
// Une petite lumière éclaire autour de la joueuse ; Mochi la suit. La LED RVB chauffe près de la planque secrète.
// Une clé ouvre la porte fermée de la salle du trésor ; le trésor est derrière un faux mur (un peu plus clair).
// Les mochis gluants font des allers-retours : les toucher renvoie à l'entrée de la salle.
// Dans l'histoire, les salles sont toujours les mêmes ; dans le menu, un nouveau château à chaque partie.
// Bouton du joystick : quitter.
#include "hw.h"
#include "save.h"
#include "activities.h"

namespace maze {

// --- Réglages ---------------------------------------------------------------
constexpr uint32_t STORY_SEED   = 2026;   // château de l'histoire (changer le nombre = autres salles)
constexpr int      STORY_SLIMES = 4;      // mochis gluants (histoire, puis menu)
constexpr int      MENU_SLIMES  = 6;
constexpr uint32_t STEP_MS      = 130;    // temps entre deux pas, joystick poussé
constexpr float    SLIME_SPEED  = 2.5f;   // cases/s
constexpr uint32_t SLIME_ON_MS  = 2600;   // un mochi gluant est dangereux pendant ce temps...
constexpr uint32_t SLIME_OFF_MS = 1500;   // ... puis il fond en flaque (on peut passer) pendant celui-ci
constexpr float    LIGHT_EASY   = 3.5f;   // rayon de la lumière (cases) ; plus petit en moyen et difficile
const char* const  ROOM_NAMES[9] = {"L'escalier", "Cave aux bocaux", "Salle des échos", "Cellier gluant",
                                    "Galerie des toiles", "Le puits", "Réserve du roi", "Couloir venteux",
                                    "Crypte oubliée"};
// ---------------------------------------------------------------------------

enum Tile : uint8_t { WALL, FLOOR, FAKE, LOCK };
constexpr int R = 3, N = 9;            // salles : 3 x 3
constexpr int MID = 7;                 // les portes sont au milieu des bords
const int8_t DX[4] = {0, 1, 0, -1}, DY[4] = {-1, 0, 1, 0};  // haut, droite, bas, gauche

uint8_t  grid[N][16][16];
uint16_t seen[N][16];                  // cases déjà éclairées (restent un peu visibles)
bool     door[N][4];                   // portes de chaque salle
int      roomDist[N];                  // distance (en salles) jusqu'à la salle du trésor
int      keyDist[N];                   //   et jusqu'à celle de la clé (la LED guide d'abord vers la clé)
int      treasureRoom, keyRoom, lockRoom, lockDir;
int8_t   keyX, keyY, treasureX, treasureY;

struct Slime { int room; int8_t path[10][2]; int len; float pos; float dir; uint32_t phase; };
Slime    slimes[MENU_SLIMES + 4];
int      nSlimes;

bool     story, hasKey, lockOpen, found, dirty;
int      room, px, py, mx, my, entryX, entryY;  // joueuse, Mochi, point d'entrée de la salle
uint16_t visited;
uint32_t nextStep, startAt, endAt, hurtUntil, hintUntil, elapsed, keyFlashUntil;
const char* hint;
uint32_t rng;

uint32_t rnd(uint32_t n) { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return rng % n; }
int roomAt(int rx, int ry) { return rx < 0 || ry < 0 || rx >= R || ry >= R ? -1 : ry * R + rx; }
int neighbor(int r, int d) { return roomAt(r % R + DX[d], r / R + DY[d]); }

// Case de la porte d, côté intérieur, et sur le bord
void doorEdge(int d, int& x, int& y) {
  x = d == 1 ? 15 : d == 3 ? 0 : MID;
  y = d == 2 ? 15 : d == 0 ? 0 : MID;
}

void carveRoom(int r) {
  // Labyrinthe parfait sur une grille de 7 x 7 cases (pixels impairs), par exploration au hasard
  memset(grid[r], WALL, sizeof(grid[r]));
  bool done[7][7] = {};
  int8_t stack[49][2];
  int top = 0;
  stack[top][0] = 3; stack[top][1] = 3; top++;
  done[3][3] = true;
  grid[r][7][7] = FLOOR;
  while (top) {
    int cx = stack[top - 1][0], cy = stack[top - 1][1];
    int dirs[4], n = 0;
    for (int d = 0; d < 4; d++) {
      int nx = cx + DX[d], ny = cy + DY[d];
      if (nx >= 0 && ny >= 0 && nx < 7 && ny < 7 && !done[ny][nx]) dirs[n++] = d;
    }
    if (!n) { top--; continue; }
    int d = dirs[rnd(n)];
    int nx = cx + DX[d], ny = cy + DY[d];
    done[ny][nx] = true;
    grid[r][1 + cy * 2 + DY[d]][1 + cx * 2 + DX[d]] = FLOOR;
    grid[r][1 + ny * 2][1 + nx * 2] = FLOOR;
    stack[top][0] = nx; stack[top][1] = ny; top++;
  }
  // Quelques murs en moins pour qu'il y ait plusieurs chemins
  for (int k = 0; k < 4; k++) {
    int x = 2 + rnd(6) * 2, y = 1 + rnd(7) * 2;
    if (k & 1) { int t = x; x = y; y = t; }
    if (x > 0 && x < 14 && y > 0 && y < 14) grid[r][y][x] = FLOOR;
  }
  // Portes
  for (int d = 0; d < 4; d++)
    if (door[r][d]) {
      int x, y;
      doorEdge(d, x, y);
      grid[r][y][x] = FLOOR;
      if (d == 1) grid[r][MID][14] = FLOOR;
      if (d == 2) grid[r][14][MID] = FLOOR;
    }
}

int openings(int r, int x, int y) {
  int n = 0;
  for (int d = 0; d < 4; d++) {
    int nx = x + DX[d], ny = y + DY[d];
    if (nx >= 0 && ny >= 0 && nx < 16 && ny < 16 && grid[r][ny][nx] != WALL) n++;
  }
  return n;
}

// Cul-de-sac le plus loin d'un point (distance à vol d'oiseau), hors des cases de porte
void farDeadEnd(int r, int fromX, int fromY, int8_t& ox, int8_t& oy) {
  int best = -1;
  for (int cy = 0; cy < 7; cy++)
    for (int cx = 0; cx < 7; cx++) {
      int x = 1 + cx * 2, y = 1 + cy * 2;
      if (openings(r, x, y) != 1 || x == MID || y == MID) continue;
      int d = abs(x - fromX) + abs(y - fromY) + (int)rnd(3);
      if (d > best) { best = d; ox = x; oy = y; }
    }
  if (best < 0) { ox = 1; oy = 1; }
}

void build(uint32_t seed) {
  rng = seed ? seed : 1;
  memset(door, 0, sizeof(door));
  // Plan des salles : un arbre au hasard depuis l'escalier (salle 0), donc toutes reliées
  bool inTree[N] = {true};
  int stack[N], top = 0;
  stack[top++] = 0;
  while (top) {
    int r = stack[top - 1], dirs[4], n = 0;
    for (int d = 0; d < 4; d++) { int nb = neighbor(r, d); if (nb >= 0 && !inTree[nb]) dirs[n++] = d; }
    if (!n) { top--; continue; }
    int d = dirs[rnd(n)], nb = neighbor(r, d);
    door[r][d] = door[nb][(d + 2) % 4] = true;
    inTree[nb] = true;
    stack[top++] = nb;
  }
  // Salle du trésor : la plus loin de l'escalier
  int dist0[N], queue[N], parent[N], qh = 0, qt = 0;
  for (int i = 0; i < N; i++) dist0[i] = -1;
  dist0[0] = 0; queue[qt++] = 0; parent[0] = -1;
  while (qh < qt) {
    int r = queue[qh++];
    for (int d = 0; d < 4; d++)
      if (door[r][d]) { int nb = neighbor(r, d); if (dist0[nb] < 0) { dist0[nb] = dist0[r] + 1; parent[nb] = r; queue[qt++] = nb; } }
  }
  treasureRoom = 1;
  for (int i = 1; i < N; i++) if (dist0[i] > dist0[treasureRoom]) treasureRoom = i;
  lockRoom = parent[treasureRoom];
  for (int d = 0; d < 4; d++) if (neighbor(lockRoom, d) == treasureRoom) lockDir = d;
  // Une porte en plus (une boucle), loin de la salle du trésor
  for (int tries = 0; tries < 20; tries++) {
    int r = rnd(N), d = rnd(4), nb = neighbor(r, d);
    if (nb < 0 || door[r][d] || r == treasureRoom || nb == treasureRoom) continue;
    door[r][d] = door[nb][(d + 2) % 4] = true;
    break;
  }
  // Clé : la salle la plus loin, à part celle du trésor
  keyRoom = 0;
  for (int i = 1; i < N; i++) if (i != treasureRoom && dist0[i] >= dist0[keyRoom]) keyRoom = i;
  // Distance de chaque salle jusqu'au trésor et jusqu'à la clé (pour la LED qui chauffe)
  for (int pass = 0; pass < 2; pass++) {
    int* dist = pass ? keyDist : roomDist;
    int from = pass ? keyRoom : treasureRoom;
    for (int i = 0; i < N; i++) dist[i] = -1;
    qh = qt = 0; dist[from] = 0; queue[qt++] = from;
    while (qh < qt) {
      int r = queue[qh++];
      for (int d = 0; d < 4; d++)
        if (door[r][d]) { int nb = neighbor(r, d); if (dist[nb] < 0) { dist[nb] = dist[r] + 1; queue[qt++] = nb; } }
    }
  }
  for (int r = 0; r < N; r++) carveRoom(r);
  // Porte fermée à clé (des deux côtés)
  int x, y;
  doorEdge(lockDir, x, y);
  grid[lockRoom][y][x] = LOCK;
  doorEdge((lockDir + 2) % 4, x, y);
  grid[treasureRoom][y][x] = LOCK;
  // Clé au fond d'un cul-de-sac, trésor derrière un faux mur
  farDeadEnd(keyRoom, MID, MID, keyX, keyY);
  farDeadEnd(treasureRoom, x, y, treasureX, treasureY);
  for (int d = 0; d < 4; d++) {
    int nx = treasureX + DX[d], ny = treasureY + DY[d];
    if (grid[treasureRoom][ny][nx] == FLOOR) grid[treasureRoom][ny][nx] = FAKE;
  }
  // Mochis gluants : chacun fait des allers-retours sur un bout de couloir
  nSlimes = (story ? STORY_SLIMES : MENU_SLIMES) + (int)byLevel(0, 2, 4);  // plus de mochis gluants selon la difficulté
  for (int i = 0; i < nSlimes; i++) {
    Slime& s = slimes[i];
    s.room = 1 + rnd(N - 1);
    int sx = 1 + rnd(7) * 2, sy = 1 + rnd(7) * 2;
    s.len = 0;
    for (int k = 0; k < 8; k++) {
      s.path[s.len][0] = sx; s.path[s.len][1] = sy; s.len++;
      int dirs[4], n = 0;
      for (int d = 0; d < 4; d++) {
        int nx = sx + DX[d], ny = sy + DY[d];
        bool used = false;
        for (int j = 0; j < s.len; j++) used |= s.path[j][0] == nx && s.path[j][1] == ny;
        if (nx > 1 && ny > 1 && nx < 13 && ny < 13 && grid[s.room][ny][nx] == FLOOR && !used) dirs[n++] = d;
      }
      if (!n) break;
      int d = dirs[rnd(n)];
      sx += DX[d]; sy += DY[d];
    }
    s.pos = 0; s.dir = 1; s.phase = rnd(SLIME_ON_MS + SLIME_OFF_MS);
  }
}

float LIGHT = LIGHT_EASY;

bool slimeMelted(const Slime& s, uint32_t now) { return (now + s.phase) % (SLIME_ON_MS + SLIME_OFF_MS) >= SLIME_ON_MS; }

bool isSeen(int r, int x, int y) { return seen[r][y] & (1 << x); }

void enterRoom(int r, int x, int y) {
  room = r; px = mx = entryX = x; py = my = entryY = y;
  visited |= 1 << r;
  dirty = true;
}

void say(const char* h, uint32_t now) { hint = h; hintUntil = now + 2500; dirty = true; }

// Un pas dans la direction d (0 haut, 1 droite, 2 bas, 3 gauche)
void tryMove(int d, uint32_t now) {
  int nx = px + DX[d], ny = py + DY[d];
  if (nx < 0 || ny < 0 || nx > 15 || ny > 15) {  // on sort par une porte : salle voisine
    int nb = neighbor(room, d);
    if (nb < 0) return;
    int ex = nx < 0 ? 15 : nx > 15 ? 0 : nx, ey = ny < 0 ? 15 : ny > 15 ? 0 : ny;
    enterRoom(nb, ex, ey);
    beep(330, 30);
    return;
  }
  uint8_t t = grid[room][ny][nx];
  if (t == WALL) return;
  if (t == LOCK) {
    if (!hasKey) { say("Clé requise", now); beep(150, 80); return; }
    int x, y;  // la clé ouvre la porte des deux côtés
    doorEdge(lockDir, x, y);
    grid[lockRoom][y][x] = FLOOR;
    doorEdge((lockDir + 2) % 4, x, y);
    grid[treasureRoom][y][x] = FLOOR;
    lockOpen = true;
    say("Ça s'ouvre !", now);
    beep(880, 120);
  }
  mx = px; my = py;  // Mochi suit
  px = nx; py = ny;
  if (t == FAKE) say("Un faux mur !", now);
  if (room == keyRoom && !hasKey && px == keyX && py == keyY) {
    hasKey = true;
    keyFlashUntil = now + 600;
    say("Une clé !", now);
    static const Note KEY[] = {{1319, 80}, {1568, 80}, {2093, 200}, {0, 0}};
    play(KEY, true);
  }
  if (room == treasureRoom && px == treasureX && py == treasureY && !found) {
    found = true;
    elapsed = now - startAt;
    endAt = now + 3500;
    static const Note WIN[] = {{523, 100}, {659, 100}, {784, 100}, {1047, 150}, {1319, 150}, {1568, 400}, {0, 0}};
    play(WIN, true);
    say(story ? "Le Pinceau !" : "Le trésor !", now);
  }
}

// Chaleur : 0 (loin, bleu) à 4 (tout près, orange). D'abord vers la clé, puis vers le trésor.
int heat() {
  if (!hasKey) {
    if (room == keyRoom) return abs(px - keyX) + abs(py - keyY) < 5 ? 4 : 3;
    return max(0, 3 - keyDist[room]);
  }
  if (room == treasureRoom) return abs(px - treasureX) + abs(py - treasureY) < 5 ? 4 : 3;
  return max(0, 3 - roomDist[room]);
}

void drawOled(uint32_t now) {
  oled.clearBuffer();
  oled.setFont(u8g2_font_6x10_tf);
  oled.setCursor(0, 11);  // bande jaune
  oled.print(found ? (story ? "Pinceau Arc-en-ciel" : "Trésor trouvé !") : ROOM_NAMES[room]);
  // Plan des salles visitées
  for (int r = 0; r < N; r++) {
    int x = 88 + (r % R) * 13, y = 20 + (r / R) * 13;
    if (r == room) oled.drawBox(x, y, 11, 11);
    else if (visited & (1 << r)) oled.drawFrame(x, y, 11, 11);
    else oled.drawPixel(x + 5, y + 5);
  }
  // Objectif en cours, en 3 étapes
  const char* goal1 = !hasKey ? "1. Trouve la" : !lockOpen ? "2. Ouvre la" : "3. Cherche un";
  const char* goal2 = !hasKey ? "clé jaune" : !lockOpen ? "porte orange" : "faux mur";
  oled.setCursor(0, 28); oled.print(goal1);
  oled.setCursor(0, 39); oled.print(goal2);
  if (!story) {
    char buf[16];
    uint32_t t = (found ? elapsed : now - startAt) / 1000;
    snprintf(buf, sizeof(buf), "%lu:%02lu", (unsigned long)(t / 60), (unsigned long)(t % 60));
    oled.setCursor(0, 52);
    oled.print(buf);
  }
  oled.setCursor(0, 63);  // sous le plan
  if (hint && now < hintUntil) { oled.print("Mochi : "); oled.print(hint); }
  else if (heat() >= 4) oled.print(hasKey ? "Mochi : ça sent bon !" : "Mochi : la clé brille");
  else oled.print("Stick : avancer");
  oled.sendBuffer();
}

void render(uint32_t now) {
  FastLED.clear();
  for (int y = 0; y < 16; y++)
    for (int x = 0; x < 16; x++) {
      float d = sqrtf((x - px) * (x - px) + (y - py) * (y - py));
      uint8_t t = grid[room][y][x];
      if (d <= LIGHT) {
        seen[room][y] |= 1 << x;
        uint8_t light = 255 - (uint8_t)(d / LIGHT * 170);
        CRGB c = t == WALL ? CRGB(25, 40, 140) : t == FAKE ? CRGB(45, 65, 170) : t == LOCK ? CRGB(160, 90, 0) : CRGB(6, 5, 10);
        setPixel(x, y, c.nscale8(light));
      } else if (isSeen(room, x, y) && t != FLOOR) {
        setPixel(x, y, t == LOCK ? CRGB(30, 15, 0) : CRGB(4, 6, 20));  // souvenir des murs déjà vus
      }
    }
  // La porte fermée luit faiblement même dans le noir, pour qu'on la remarque
  for (int y = 0; y < 16; y++)
    for (int x = 0; x < 16; x++)
      if (grid[room][y][x] == LOCK) setPixel(x, y, CRGB(120 + sin8(now / 4) / 4, 50, 0));
  // Clé, trésor (scintillent dans la lumière), mochis gluants
  auto lit = [&](int x, int y, float r) { return (x - px) * (x - px) + (y - py) * (y - py) <= r * r; };
  if (room == keyRoom && !hasKey && lit(keyX, keyY, LIGHT + 1)) setPixel(keyX, keyY, CHSV(40, 255, 150 + sin8(now / 3) / 3));
  if (room == treasureRoom && !found && lit(treasureX, treasureY, LIGHT + 1)) setPixel(treasureX, treasureY, CHSV(now / 4, 255, 255));
  for (int i = 0; i < nSlimes; i++) {
    Slime& s = slimes[i];
    if (s.room != room || s.len < 2) continue;
    int k = (int)(s.pos + 0.5f);
    int sx = s.path[k][0], sy = s.path[k][1];
    if (lit(sx, sy, LIGHT + 1)) setPixel(sx, sy, slimeMelted(s, now) ? CRGB(10, 50, 10) : CRGB(140, 255, 140));
  }
  // Mochi et la joueuse
  if (mx != px || my != py) setPixel(mx, my, CRGB(90, 90, 90));
  if (now >= hurtUntil || (now / 100) % 2) setPixel(px, py, CRGB(255, 60, 140));
  if (found) for (int i = 0; i < 20; i++) leds[random16(NUM_LEDS)] = CHSV(random8(), 200, 255);
  if (now < keyFlashUntil) for (int i = 0; i < 12; i++) leds[random16(NUM_LEDS)] = CRGB(255, 200, 0);  // clé trouvée
  FastLED.show();
  // LED : chaud-froid vers la planque
  int h = heat();
  uint8_t beat = 140 + sin8(now / (12 - h * 2)) / 3;
  if (found) setRgb(CHSV(now / 3, 255, 255));
  else if (now < hurtUntil) setRgb(CRGB(0, 255, 0));
  else setRgb(blend(CRGB(0, 40, 255), CRGB(255, 90, 0), h * 63).nscale8(beat));
}

}  // namespace maze

using namespace maze;

void mazeBegin(bool storyMode) {
  story = storyMode;
  LIGHT = LIGHT_EASY * byLevel(1.0f, 0.85f, 0.72f);
  build(story ? STORY_SEED : (esp_random() | 1));
  memset(seen, 0, sizeof(seen));
  visited = 0;
  hasKey = lockOpen = found = false;
  keyFlashUntil = 0;
  hint = nullptr;
  enterRoom(0, MID, MID);
  uint32_t now = millis();
  startAt = now; nextStep = now; hurtUntil = 0; hintUntil = 0;
  say("Cherche la clé", now);
}

GameResult mazeLoop(uint32_t now) {
  static uint32_t lastFrame = 0;
  btn1.pressed();
  btnEnc.pressed();
  if (btn2.pressed()) return story ? GAME_LOST : GAME_QUIT;
  if (now - lastFrame < 20) return GAME_RUNNING;
  float dt = min((now - lastFrame) / 1000.0f, 0.05f);
  lastFrame = now;

  if (found) {
    render(now);
    if (dirty) { drawOled(now); dirty = false; }
    if (now < endAt) return GAME_RUNNING;
    if (!story && (!progress.mazeBest || elapsed / 100 < progress.mazeBest)) {
      progress.mazeBest = elapsed / 100;
      progressSave();
    }
    return story ? GAME_WON : GAME_QUIT;
  }

  // Joystick : un pas à la fois, plus vite en le gardant poussé
  float jx = joyX(), jy = joyY();
  int d = -1;
  if (fabsf(jx) > 0.5f || fabsf(jy) > 0.5f) d = fabsf(jx) > fabsf(jy) ? (jx > 0 ? 1 : 3) : (jy > 0 ? 2 : 0);
  if (d < 0) nextStep = now;
  else if (now >= nextStep) { tryMove(d, now); nextStep = now + STEP_MS; }

  // Mochis gluants
  for (int i = 0; i < nSlimes; i++) {
    Slime& s = slimes[i];
    if (s.len < 2) continue;
    if (slimeMelted(s, now)) continue;  // en flaque : il ne bouge pas et ne colle pas
    s.pos += s.dir * SLIME_SPEED * byLevel(1.0f, 1.2f, 1.4f) * dt;
    if (s.pos >= s.len - 1) { s.pos = s.len - 1; s.dir = -1; }
    if (s.pos <= 0) { s.pos = 0; s.dir = 1; }
    int k = (int)(s.pos + 0.5f);
    if (s.room == room && s.path[k][0] == px && s.path[k][1] == py && now >= hurtUntil) {
      static const Note GLOOP[] = {{300, 60}, {150, 200}, {0, 0}};
      play(GLOOP, true);
      hurtUntil = now + 1000;
      px = mx = entryX; py = my = entryY;  // retour à l'entrée de la salle
      say("Beurk, collé !", now);
    }
  }
  if (hint && now >= hintUntil) { hint = nullptr; dirty = true; }
  static int lastHeat = -1;
  if (heat() != lastHeat) { lastHeat = heat(); dirty = true; }
  static uint32_t lastSecond = 0;
  if (!story && now / 1000 != lastSecond) { lastSecond = now / 1000; dirty = true; }
  render(now);
  if (dirty) { drawOled(now); dirty = false; }
  return GAME_RUNNING;
}
