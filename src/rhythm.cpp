// Le concert de la grenouille DJ (Aventure, scène 7) : un jeu de rythme façon « Simon ».
// La grenouille joue une suite de notes : chaque commande a sa couleur et son son.
//   rouge = le bouton, bleu = l'encodeur (un cran dans n'importe quel sens), vert = le joystick (le pousser).
// Il faut la rejouer dans le même ordre. 3 manches, de plus en plus longues ; on a quelques vies (erreurs permises).
// Bouton du joystick (hors jeu) : abandonner.
#include "hw.h"
#include "save.h"
#include "activities.h"

namespace rhythm {

// --- Réglages ---------------------------------------------------------------
constexpr int      ROUNDS        = 3;
constexpr int      MAX_LEN       = 12;
inline int      baseLen()  { return (int)byLevel(3, 4, 5); }        // longueur de la 1re manche (+1 par manche)
inline int      lives()    { return (int)byLevel(4, 3, 2); }        // erreurs permises avant de perdre
inline uint32_t noteMs()   { return (uint32_t)byLevel(480, 380, 300); }  // durée d'une note jouée par la grenouille
inline uint32_t gapMs()    { return (uint32_t)byLevel(180, 130, 100); }  // silence entre deux notes
constexpr uint32_t ANSWER_MS  = 4500;   // temps pour répondre à chaque note
constexpr uint32_t LOCKOUT_MS = 140;    // anti-rebond entre deux commandes
constexpr int      ENC_NOTCH  = 4;      // impulsions de l'encodeur pour un cran
// ---------------------------------------------------------------------------

enum Sym : uint8_t { S_BTN, S_ENC, S_JOY };
const CRGB    COLORS[3] = {CRGB(255, 30, 30), CRGB(30, 80, 255), CRGB(0, 220, 60)};
const uint16_t FREQ[3]  = {523, 659, 784};
const Note SND_WRONG[]  = {{150, 150}, {120, 300}, {0, 0}};
const Note SND_ROUND[]  = {{523, 80}, {659, 80}, {784, 80}, {1047, 250}, {0, 0}};
const Note SND_WIN[]    = {{523, 100}, {659, 100}, {784, 100}, {1047, 120}, {784, 100}, {1047, 120}, {1319, 500}, {0, 0}};
const Note SND_LOSE[]   = {{392, 200}, {349, 200}, {294, 600}, {0, 0}};

enum State : uint8_t { INTRO, LISTEN, ANSWER, WRONG, ROUND_OK, WIN, LOSE };
State    state;
Sym      seq[MAX_LEN];
int      round_, len, pos, shown, heartsLeft;
int      lit;                       // commande allumée en ce moment (-1 = aucune)
uint32_t stateAt, nextNote, lastInput, answerBy, litUntil;
int32_t  encBase, encAcc;
int8_t   joyWas;
bool     dirty;
const char* message;

void newSequence() {
  len = min(baseLen() + round_, MAX_LEN);
  for (int i = 0; i < len; i++) seq[i] = (Sym)random(3);
  pos = 0; shown = 0;
}

void startListening(uint32_t now) {
  state = LISTEN; stateAt = now; nextNote = now + 900; shown = 0; lit = -1;
  message = "Écoute la grenouille";
  dirty = true;
}

void light(int s, uint32_t now, uint32_t ms) {
  static Note tone[2];  // la mélodie doit rester en mémoire pendant qu'elle joue
  tone[0] = {FREQ[s], (uint16_t)ms};
  tone[1] = {0, 0};
  lit = s; litUntil = now + ms;
  play(tone, true);
}

void drawOled() {
  oled.clearBuffer();
  oled.setFont(u8g2_font_6x10_tf);
  char buf[28];
  snprintf(buf, sizeof(buf), "Concert, manche %d/%d", round_ + 1, ROUNDS);
  oled.setCursor(0, 11);  // bande jaune
  oled.print(buf);
  oled.setCursor(0, 25); oled.print("Rouge : le bouton");
  oled.setCursor(0, 35); oled.print("Bleu  : l'encodeur");
  oled.setCursor(0, 45); oled.print("Vert  : le joystick");
  oled.setCursor(0, 62);
  if (message) oled.print(message);
  oled.sendBuffer();
}

void render(uint32_t now) {
  FastLED.clear();
  // 3 bandes verticales : une par commande (la lumière s'allume quand elle est jouée ou pressée)
  static const int X0[3] = {0, 5, 11}, W[3] = {5, 6, 5};
  for (int s = 0; s < 3; s++) {
    bool on = lit == s && now < litUntil;
    CRGB c = on ? COLORS[s] : CRGB(COLORS[s]).nscale8(on ? 255 : 28);
    for (int y = 3; y < 15; y++)
      for (int x = X0[s]; x < X0[s] + W[s]; x++) setPixel(x, y, c);
    // petit symbole au centre de chaque bande
    CRGB g = on ? CRGB::White : CRGB(90, 90, 90);
    int cx = X0[s] + W[s] / 2;
    if (s == S_BTN) { setPixel(cx, 8, g); setPixel(cx, 9, g); setPixel(cx - 1, 8, g); setPixel(cx - 1, 9, g); }  // bouton rond
    if (s == S_ENC) { setPixel(cx, 7, g); setPixel(cx + 1, 8, g); setPixel(cx, 9, g); setPixel(cx - 1, 8, g); }  // molette
    if (s == S_JOY) { setPixel(cx, 7, g); setPixel(cx, 8, g); setPixel(cx, 9, g); setPixel(cx - 1, 8, g); setPixel(cx + 1, 8, g); }  // manche
  }
  // Rangée du haut : progression dans la suite (points) ; rangée du bas : vies
  for (int i = 0; i < len && state != INTRO; i++) {
    bool done = state == ANSWER ? i < pos : state == LISTEN ? i < shown : state >= ROUND_OK;
    setPixel(i, 0, done ? CRGB(255, 200, 0) : CRGB(40, 30, 0));
  }
  for (int i = 0; i < heartsLeft; i++) setPixel(1 + i * 2, 15, CRGB(255, 40, 80));
  if (state == WIN) for (int i = 0; i < 25; i++) leds[random16(NUM_LEDS)] = CHSV(random8(), 200, 255);
  if (state == LOSE) fadeToBlackBy(leds, NUM_LEDS, 160);
  FastLED.show();
  if (state == WRONG || state == LOSE) setRgb(CRGB(255, 0, 0));
  else if (lit >= 0 && now < litUntil) setRgb(COLORS[lit]);
  else if (state == WIN) setRgb(CHSV(now / 3, 255, 255));
  else setRgb(CRGB(255, 160, 0).nscale8(60));
}

}  // namespace rhythm

using namespace rhythm;

void rhythmBegin() {
  round_ = 0; heartsLeft = lives();
  newSequence();
  state = INTRO; stateAt = millis(); lit = -1; lastInput = 0;
  message = "La grenouille joue !";
  encBase = encoderCount(); encAcc = 0; joyWas = 0;
  dirty = true;
}

GameResult rhythmLoop(uint32_t now) {
  static uint32_t lastFrame = 0;
  btn1.pressed();
  btnEnc.pressed();
  bool quit = btn2.pressed();
  if (quit) return GAME_QUIT;
  if (now - lastFrame < 16) return GAME_RUNNING;
  lastFrame = now;

  // Lecture des commandes : -1 = rien, sinon le symbole joué
  int input = -1;
  int32_t pulses = encoderCount() - encBase;
  encBase += pulses;
  encAcc += pulses;
  float jx = joyX(), jy = joyY();
  int8_t joyNow = fabsf(jx) > 0.6f || fabsf(jy) > 0.6f ? 1 : 0;
  static bool btnWasHeld = false;
  bool held = btn1.held();
  bool btnEdge = held && !btnWasHeld;
  btnWasHeld = held;
  if (now - lastInput >= LOCKOUT_MS) {
    if (btnEdge) input = S_BTN;
    else if (abs(encAcc) >= ENC_NOTCH) input = S_ENC;
    else if (joyNow && !joyWas) input = S_JOY;
  }
  if (abs(encAcc) >= ENC_NOTCH && input != S_ENC) encAcc = 0;  // un cran jeté pendant l'écoute ne compte pas
  joyWas = joyNow;

  switch (state) {
    case INTRO:  // le temps de lire la légende des couleurs
      if (now - stateAt > 2200) startListening(now);
      break;
    case LISTEN:
      if (shown < len && now >= nextNote) {
        light(seq[shown], now, noteMs());
        shown++;
        nextNote = now + noteMs() + gapMs();
        dirty = true;
      } else if (shown >= len && now >= nextNote + 200) {
        state = ANSWER; stateAt = now; pos = 0; answerBy = now + ANSWER_MS; encAcc = 0;
        message = "À toi !";
        dirty = true;
      }
      break;
    case ANSWER: {
      bool wrong = false;
      if (input >= 0) {
        lastInput = now; encAcc = 0;
        if (input == seq[pos]) {
          light(input, now, 220);
          pos++; answerBy = now + ANSWER_MS;
          if (pos >= len) {
            round_++;
            if (round_ >= ROUNDS) { state = WIN; stateAt = now; message = "Bravo, quel rythme !"; play(SND_WIN, true); }
            else { state = ROUND_OK; stateAt = now; message = "Bien joué ! Encore !"; play(SND_ROUND, true); }
          }
          dirty = true;
        } else wrong = true;
      } else if (now >= answerBy) wrong = true;
      if (wrong) {
        heartsLeft--;
        play(SND_WRONG, true);
        if (heartsLeft <= 0) { state = LOSE; stateAt = now; message = "Raté... plus de vies"; play(SND_LOSE, true); }
        else { state = WRONG; stateAt = now; message = "Raté ! Réécoute"; }
        dirty = true;
      }
      break;
    }
    case WRONG:
      if (now - stateAt > 1100) startListening(now);
      break;
    case ROUND_OK:
      if (now - stateAt > 1400) { newSequence(); startListening(now); }
      break;
    case WIN:
      if (now - stateAt > 2800) return GAME_WON;
      break;
    case LOSE:
      if (now - stateAt > 2200) return GAME_LOST;
      break;
  }
  render(now);
  if (dirty) { drawOled(); dirty = false; }
  return GAME_RUNNING;
}
