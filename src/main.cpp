// Test 1 : un point glisse sur la matrice avec de l'inertie.
// Réglages en direct via le moniteur série :  g <gain>   f <frottement>   m <vitesse max>
#include <Arduino.h>
#include <FastLED.h>
#include <U8g2lib.h>
#include "config.h"

CRGB leds[NUM_LEDS];
U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);

// --- Réglages du ressenti ---------------------------------------------------
float gain     = 0.25f;  // cases/s de vitesse ajoutées par impulsion de l'encodeur
float friction = 0.97f;  // multiplicateur de vitesse à chaque image (plus petit = freine plus)
float vMax     = 30.0f;  // cases/s
// ---------------------------------------------------------------------------

float   pos = MATRIX_W / 2.0f, vel = 0;
int     score = 0;
bool    dirty = true;

// Encodeur en quadrature, lu par interruption (4 impulsions par cran sur un KY-040)
volatile int32_t encCount = 0;
volatile uint8_t encState = 0;
void IRAM_ATTR onEnc() {
  static const int8_t table[16] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};
  encState = ((encState << 2) | (digitalRead(PIN_ENC_A) << 1) | digitalRead(PIN_ENC_B)) & 0x0F;
  encCount += table[encState];
}

uint16_t XY(uint8_t x, uint8_t y) {
  if (MATRIX_SERPENTINE && (y & 1)) x = MATRIX_W - 1 - x;
  return y * MATRIX_W + x;
}

struct Button {
  uint8_t pin; bool last = true; uint32_t t = 0;
  bool pressed() {  // front descendant avec anti-rebond logiciel
    bool v = digitalRead(pin);
    bool p = (last && !v && millis() - t > 20);
    if (v != last) { last = v; t = millis(); }
    return p;
  }
};
Button btn1{PIN_BTN1}, btn2{PIN_BTN2}, btnEnc{PIN_ENC_SW};

void drawOled() {
  oled.clearBuffer();
  oled.setFont(u8g2_font_logisoso32_tn);
  oled.setCursor(0, 40);
  oled.print(score);
  oled.setFont(u8g2_font_6x10_tf);
  oled.setCursor(0, 58);
  oled.printf("g%.2f f%.3f m%.0f", gain, friction, vMax);
  oled.sendBuffer();
}

void handleSerial() {
  static String line;
  while (Serial.available()) {
    char c = Serial.read();
    if (c != '\n') { line += c; continue; }
    float v = line.substring(2).toFloat();
    switch (line[0]) {
      case 'g': gain = v; break;
      case 'f': friction = constrain(v, 0.80f, 0.999f); break;
      case 'm': vMax = v; break;
    }
    Serial.printf("gain=%.2f friction=%.3f vmax=%.0f\n", gain, friction, vMax);
    line = ""; dirty = true;
  }
}

void setup() {
  Serial.begin(115200);
  FastLED.addLeds<WS2812B, PIN_LED_DATA, GRB>(leds, NUM_LEDS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, MAX_MILLIAMPS);
  FastLED.setBrightness(40);

  pinMode(PIN_ENC_A, INPUT_PULLUP);
  pinMode(PIN_ENC_B, INPUT_PULLUP);
  pinMode(PIN_ENC_SW, INPUT_PULLUP);
  pinMode(PIN_BTN1, INPUT_PULLUP);
  pinMode(PIN_BTN2, INPUT);  // GPIO36 : pull-up externe uniquement
  attachInterrupt(PIN_ENC_A, onEnc, CHANGE);
  attachInterrupt(PIN_ENC_B, onEnc, CHANGE);

  oled.begin();
  Serial.println("Pret. Commandes : g <gain>  f <frottement>  m <vmax>");
}

void loop() {
  static uint32_t lastFrame = 0;
  static int32_t lastEnc = 0;
  uint32_t now = millis();
  handleSerial();

  if (btn1.pressed())   { score++; dirty = true; }
  if (btn2.pressed())   { score = 0; dirty = true; }
  if (btnEnc.pressed()) { pos = MATRIX_W / 2.0f; vel = 0; }

  if (now - lastFrame < 16) return;           // ~60 images/s
  float dt = (now - lastFrame) / 1000.0f;
  lastFrame = now;

  noInterrupts(); int32_t c = encCount; interrupts();
  vel += (c - lastEnc) * gain;
  lastEnc = c;
  vel = constrain(vel * friction, -vMax, vMax);
  pos += vel * dt;
  if (pos < 0)              { pos = 0;              vel = -vel * 0.5f; }
  if (pos > MATRIX_W - 1.0f){ pos = MATRIX_W - 1.0f; vel = -vel * 0.5f; }

  fadeToBlackBy(leds, NUM_LEDS, 90);          // traînée lumineuse
  leds[XY((uint8_t)roundf(pos), MATRIX_H - 2)] = CHSV(160, 255, 255);
  FastLED.show();

  if (dirty) { drawOled(); dirty = false; }
}
