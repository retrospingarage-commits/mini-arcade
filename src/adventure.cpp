// Moteur de l'Aventure : affiche une scène (texte sur l'OLED, dessin sur la matrice, LED, jingle),
// laisse choisir avec l'encodeur ou le joystick, donne les récompenses et sauvegarde la progression.
// Les scènes elles-mêmes sont dans include/scenes.h.
#include "adventure.h"
#include "scenes.h"

namespace {

constexpr int LINE_CHARS = 21;  // caractères par ligne sur l'OLED (police 6x10)
constexpr int TEXT_LINES = 5;   // lignes de texte au-dessus de la barre de choix
constexpr uint8_t SOON   = 0;   // scène « bientôt » : la cible n'est pas encore écrite

const Note SND_TICK[] = {{1200, 8}, {0, 0}};

const Scene SCENE_SOON = {SOON, "Cette partie de l'histoire arrive bientôt ! Mochi s'impatiente.",
                          {{"Revenir", 0, ALWAYS}}, PIC_CADENAS, 0x202020, J_BIENTOT, 0, 0, 0};
const Choice CONTINUE = {"Continuer", 2, ALWAYS};  // secours si aucun choix n'est visible

const Scene*  scene = nullptr;
uint8_t       previous = 1;     // scène d'où l'on vient (pour revenir de la scène « bientôt »)
const Choice* visible[4];
uint8_t       nVisible = 0, selected = 0;
char          lines[TEXT_LINES][64];
int           nLines = 0;
int32_t       encBase = 0;
int8_t        joyWas = 0;
bool          dirty = true;
uint32_t      rewardUntil = 0;

const Scene* findScene(uint8_t id) {
  for (const Scene& s : SCENES)
    if (s.id == id) return &s;
  return nullptr;
}

bool isVisible(Cond c) {
  switch (c) {
    case IF_ALL_CHARMS:      return (progress.charms & ALL_CHARMS) == ALL_CHARMS;
    case UNLESS_BOL:         return !(progress.charms & CHARM_BOL);
    case UNLESS_FEUILLE:     return !(progress.charms & CHARM_FEUILLE);
    case UNLESS_NOTE:        return !(progress.charms & CHARM_NOTE);
    case UNLESS_BALLON:      return !(progress.trophies & TROPHY_BALLON);
    case UNLESS_BILLE:       return !(progress.trophies & TROPHY_BILLE);
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

void enterScene(uint8_t id) {
  const Scene* s = findScene(id);
  if (!s) {
    previous = scene ? scene->id : 1;
    s = &SCENE_SOON;
  }
  scene = s;
  bool reward = giveRewards(s);
  if (s->id != SOON) progress.scene = s->id;
  progressSave();

  nVisible = 0;
  for (const Choice& c : s->choices)
    if (c.label && isVisible(c.cond)) visible[nVisible++] = &c;
  if (!nVisible) visible[nVisible++] = &CONTINUE;
  selected = 0;

  nLines = wrapText(s->text, lines, TEXT_LINES);
  play(s->jingle, true);
  rewardUntil = reward ? millis() + 2500 : 0;
  dirty = true;
}

void moveSelection(int d) {
  if (nVisible < 2) return;
  selected = ((selected + d) % nVisible + nVisible) % nVisible;
  play(SND_TICK);
  dirty = true;
}

void drawOled() {
  oled.clearBuffer();
  oled.setFont(u8g2_font_6x10_tf);
  for (int i = 0; i < nLines; i++) {
    oled.setCursor(0, 9 + i * 10);
    oled.print(lines[i]);
  }
  // Barre de choix en bas, en négatif
  oled.drawBox(0, 53, 128, 11);
  oled.setDrawColor(0);
  const char* label = visible[selected]->label;
  oled.setCursor((128 - oled.getUTF8Width(label)) / 2, 62);
  oled.print(label);
  if (nVisible > 1) {
    oled.setCursor(1, 62);   oled.print("<");
    oled.setCursor(122, 62); oled.print(">");
  }
  oled.setDrawColor(1);
  oled.sendBuffer();
}

void renderMatrix(uint32_t now) {
  FastLED.clear();
  drawPicture(scene->picture);
  // Barre d'état : les 3 charmes en haut à droite (éteints tant qu'ils ne sont pas gagnés)
  const uint8_t charms[3] = {CHARM_BOL, CHARM_FEUILLE, CHARM_NOTE};
  const CRGB    colors[3] = {CRGB(255, 90, 0), CRGB(0, 200, 0), CRGB(255, 200, 0)};
  for (int i = 0; i < 3; i++) setPixel(13 + i, 0, progress.charms & charms[i] ? colors[i] : CRGB(20, 20, 20));
  if (now < rewardUntil) {  // pluie d'étincelles quand on gagne quelque chose
    for (int i = 0; i < 6; i++) leds[random16(NUM_LEDS)] = CHSV(random8(), 180, 255);
    setRgb(CHSV(now / 4, 255, 255));
  } else {
    setRgb(CRGB(scene->led));
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
      if (*p == ' ') cut = p;                 // couper avant l'espace
      else if (*p == '-') cut = next;         // couper après le tiret
      count++;
      p = next;
    }
    const char* end;
    if (!*p || *p == ' ') end = p;            // fin du texte, ou la ligne tombe pile sur un espace
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
  enterScene(progress.scene ? progress.scene : 1);
}

void adventureGoto(uint8_t sceneId) {
  enterScene(sceneId);
}

bool adventureLoop(uint32_t now) {
  static uint32_t lastFrame = 0;

  int32_t d = (encoderCount() - encBase) / 4;  // un cran de l'encodeur = un choix
  if (d) {
    encBase += d * 4;
    moveSelection(d);
  }
  float x = joyX();
  int8_t dir = x > 0.6f ? 1 : x < -0.6f ? -1 : 0;
  if (dir && dir != joyWas) moveSelection(dir);
  joyWas = dir;

  bool ok = btn1.pressed();
  ok |= btnEnc.pressed();
  if (btn2.pressed()) return false;  // bouton du joystick : retour au menu (la progression est gardée)
  if (ok) {  // le jingle de la scène suivante sert de son de validation
    uint8_t target = scene == &SCENE_SOON ? previous : visible[selected]->target;
    enterScene(target);
  }

  if (now - lastFrame >= 33) {  // ~30 images/s suffisent pour un dessin fixe
    lastFrame = now;
    renderMatrix(now);
  }
  if (dirty) { drawOled(); dirty = false; }
  return true;
}
