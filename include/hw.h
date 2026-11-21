#pragma once
// Matériel partagé par toutes les activités : matrice, OLED, boutons, encodeur, joystick, buzzer, LED RVB
#include <Arduino.h>
#include <FastLED.h>
#include <U8g2lib.h>
#include "config.h"

extern CRGB leds[NUM_LEDS];
extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled;
extern uint8_t oledAddress;  // adresse I2C trouvée au démarrage (0 = OLED absent)

void hwBegin();  // à appeler une fois dans setup()

// Matrice : coordonnées (0,0) en haut à gauche, texte en police 3x5
uint16_t XY(uint8_t x, uint8_t y);
void     setPixel(int x, int y, CRGB c);
int      textWidth(const char* s);
void     drawText(const char* s, int x, int y, CRGB col);
uint32_t scrollDuration(const char* s);
void     drawScroll(const char* s, uint32_t t, int y, CRGB col);

// Boutons : pressed() est vrai une seule fois par appui
struct Button {
  uint8_t pin; bool last = true; uint32_t t = 0;
  bool pressed();
  bool held() const { return !digitalRead(pin); }  // vrai tant que le bouton est enfoncé
};
extern Button btn1, btn2, btnEnc;  // bouton HW-483, bouton du joystick, bouton de l'encodeur

int32_t encoderCount();  // impulsions depuis le démarrage (4 par cran)
constexpr int8_t ENC_X_SIGN = -1;  // sens de l'encodeur pour aller à droite dans les jeux (à passer à 1 si c'est inversé)

// Joystick : position de -1 à 1 (0 au centre), toujours 0 s'il n'est pas branché
extern bool joyOk;
float joyX();
float joyY();          // négatif vers le haut
bool  joyUpPressed();  // vrai une seule fois quand on pousse vers le haut
bool  joyActive();     // vrai tant que le joystick est poussé dans n'importe quelle direction ou enfoncé

// Buzzer : une mélodie est une liste de notes terminée par {0, 0}
struct Note { uint16_t freq; uint16_t ms; };  // freq 0 = silence
extern bool soundOn;
void play(const Note* m, bool priority = false);  // une mélodie prioritaire n'est pas coupée par un bruitage
void updateSound(uint32_t now);                   // à appeler à chaque tour de loop()
uint16_t currentFreq();                           // fréquence de la note en cours (0 = silence)
void beep(uint16_t freq, uint16_t ms);            // un bip simple, sans mélodie

constexpr uint8_t BRIGHTNESS = 40;          // luminosité maximale de la matrice (règle de sécurité : 40 au maximum)
constexpr uint8_t BRIGHTNESS_MIN = 1;
// Échelle progressive : beaucoup de crans doux en bas, le maximum de sécurité (40) en haut
constexpr uint8_t BRIGHTNESS_STEPS[] = {1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 15, 19, 24, 30, 35, 40};
constexpr int     BRIGHTNESS_STEP_COUNT = sizeof(BRIGHTNESS_STEPS);
int               brightnessStep(uint8_t value);  // cran (0 à 15) le plus proche d'une luminosité
uint8_t           brightnessShift(uint8_t value, int steps);  // luminosité après quelques crans en plus ou en moins
extern uint8_t    matrixBrightness;          // luminosité choisie dans le menu (1 à 40)
void setMatrixBrightness(uint8_t level);     // applique et retient la luminosité (sans l'enregistrer)

void setRgb(CRGB c);  // LED RVB
