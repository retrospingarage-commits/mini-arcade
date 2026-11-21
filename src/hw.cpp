// Matériel partagé par toutes les activités : matrice, OLED, boutons, encodeur, joystick, buzzer, LED RVB
#include "hw.h"
#include "font.h"

CRGB leds[NUM_LEDS];
U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

// --- Réglages ---------------------------------------------------------------
constexpr uint32_t SCROLL_MS     = 45;     // vitesse du texte défilant (ms par pixel)
constexpr uint8_t  RGB_LED_LEVEL = 64;     // luminosité max de la LED RVB (0 à 255)
constexpr float    JOY_DEADZONE  = 0.15f;  // zone morte autour du centre du joystick (0 à 1)
constexpr bool     JOY_INVERT_X  = false;  // à passer à true si la droite et la gauche sont inversées
constexpr bool     JOY_INVERT_Y  = false;  // à passer à true si le haut et le bas sont inversés
// ---------------------------------------------------------------------------

// Encodeur en quadrature, lu par interruption (4 impulsions par cran sur un KY-040)
static volatile int32_t encCount = 0;
static volatile uint8_t encState = 0;
void IRAM_ATTR onEnc() {
  static const int8_t table[16] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};
  encState = ((encState << 2) | (digitalRead(PIN_ENC_A) << 1) | digitalRead(PIN_ENC_B)) & 0x0F;
  encCount += table[encState];
}

uint16_t XY(uint8_t x, uint8_t y) {
  if (MATRIX_SERPENTINE && (y & 1)) x = MATRIX_W - 1 - x;
  return y * MATRIX_W + x;
}

void setPixel(int x, int y, CRGB c) {
  if (x < 0 || x >= MATRIX_W || y < 0 || y >= MATRIX_H) return;
  leds[XY(x, y)] = c;
}


int32_t encoderCount() {
  noInterrupts(); int32_t c = encCount; interrupts();
  return c;
}

bool Button::pressed() {  // front descendant avec anti-rebond logiciel
  bool v = digitalRead(pin);
  bool p = (last && !v && millis() - t > 20);
  if (v != last) { last = v; t = millis(); }
  return p;
}
Button btn1{PIN_BTN1}, btn2{PIN_BTN2}, btnEnc{PIN_ENC_SW};

// --- Texte sur la matrice (police 3x5) ---------------------------------------------------------
int textWidth(const char* s) { return strlen(s) * 4 - 1; }
uint32_t scrollDuration(const char* s) { return (MATRIX_W + textWidth(s)) * SCROLL_MS; }

void drawText(const char* s, int x, int y, CRGB col) {
  for (; *s; s++, x += 4) {
    const uint8_t* g = glyph(*s);
    for (int r = 0; r < 5; r++)
      for (int c = 0; c < 3; c++)
        if (g[r] & (4 >> c)) setPixel(x + c, y + r, col);
  }
}

void drawScroll(const char* s, uint32_t t, int y, CRGB col) {
  drawText(s, MATRIX_W - (int)(t / SCROLL_MS), y, col);
}

const Note* melody = nullptr;
bool        melodyPriority = false;
uint32_t    noteEnd = 0;
bool        soundOn = true;

// Une petite mélodie (priority) n'est jamais coupée par un bruitage de rebond
void play(const Note* m, bool priority) {
  if (!soundOn || (melody && melodyPriority && !priority)) return;
  melody = m;
  melodyPriority = priority;
  noteEnd = 0;
}

static uint16_t curFreq = 0;
uint16_t currentFreq() { return curFreq; }

void beep(uint16_t freq, uint16_t ms) {
  static Note single[2];
  single[0] = {freq, ms};
  single[1] = {0, 0};
  play(single);
}

void updateSound(uint32_t now) {
  if (!melody || now < noteEnd) return;
  if (!melody->ms) { ledcWriteTone(PIN_BUZZER, 0); curFreq = 0; melody = nullptr; return; }
  ledcWriteTone(PIN_BUZZER, melody->freq);
  curFreq = melody->freq;
  noteEnd = now + melody->ms;
  melody++;
}

// --- LED RVB ----------------------------------------------------------------
void setRgb(CRGB c) {
  ledcWrite(PIN_RGB_R, scale8(c.r, RGB_LED_LEVEL));
  ledcWrite(PIN_RGB_G, scale8(c.g, RGB_LED_LEVEL));
  ledcWrite(PIN_RGB_B, scale8(c.b, RGB_LED_LEVEL));
}

// --- Joystick ---------------------------------------------------------------
int  joyCenterX = 2048, joyCenterY = 2048;
bool joyOk = false;  // faux si le joystick n'est pas branché (lecture incohérente au démarrage)

void calibrateJoystick() {  // à faire sans toucher le joystick
  long sx = 0, sy = 0;
  for (int i = 0; i < 16; i++) { sx += analogRead(PIN_JOY_X); sy += analogRead(PIN_JOY_Y); delay(2); }
  joyCenterX = sx / 16;
  joyCenterY = sy / 16;
  joyOk = joyCenterX > 1000 && joyCenterX < 3100 && joyCenterY > 1000 && joyCenterY < 3100;
}

// Position d'un axe entre -1 et 1, 0 dans la zone morte
float joyAxis(uint8_t pin, int center, bool invert) {
  if (!joyOk) return 0;
  int raw = analogRead(pin) - center;
  float v = raw / (float)(raw < 0 ? center : 4095 - center);
  if (fabsf(v) < JOY_DEADZONE) return 0;
  v = constrain((fabsf(v) - JOY_DEADZONE) / (1 - JOY_DEADZONE), 0.0f, 1.0f) * (v < 0 ? -1 : 1);
  return invert ? -v : v;
}

// Vrai une seule fois quand on pousse le joystick vers le haut
bool joyUpPressed() {
  static bool wasUp = false;
  bool up = joyY() < -0.6f;
  bool p = up && !wasUp;
  wasUp = up;
  return p;
}

float joyX() { return joyAxis(PIN_JOY_X, joyCenterX, JOY_INVERT_X); }
bool  joyActive() { return fabsf(joyX()) > 0.5f || fabsf(joyY()) > 0.5f || btn2.held(); }
float joyY() { return joyAxis(PIN_JOY_Y, joyCenterY, JOY_INVERT_Y); }

void hwBegin() {
  Serial.begin(115200);
  FastLED.addLeds<WS2812B, PIN_LED_DATA, GRB>(leds, NUM_LEDS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, MAX_MILLIAMPS);
  FastLED.setBrightness(BRIGHTNESS);

  pinMode(PIN_ENC_A, INPUT_PULLUP);
  pinMode(PIN_ENC_B, INPUT_PULLUP);
  pinMode(PIN_ENC_SW, INPUT_PULLUP);
  pinMode(PIN_BTN1, INPUT_PULLUP);
  pinMode(PIN_BTN2, INPUT_PULLUP);
  attachInterrupt(PIN_ENC_A, onEnc, CHANGE);
  attachInterrupt(PIN_ENC_B, onEnc, CHANGE);

  ledcAttach(PIN_BUZZER, 2000, 8);
  ledcAttach(PIN_RGB_R, 5000, 8);
  ledcAttach(PIN_RGB_G, 5000, 8);
  ledcAttach(PIN_RGB_B, 5000, 8);
  calibrateJoystick();

  oled.setBusClock(400000);  // I2C rapide : l'OLED bloque moins longtemps le jeu
  oled.begin();
  oled.enableUTF8Print();    // accents sur l'OLED
  oled.setFontMode(1);       // texte transparent (lisible sur les barres en négatif)
}
