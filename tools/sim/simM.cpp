// Banc d'essai de la Mood light : les commandes changent-elles bien l'ambiance et la luminosité ?
#include <Wire.h>
#include "hw.h"
#include "save.h"
#include "activities.h"
std::string oledText; TwoWire Wire; uint32_t fakeMillis=1000; bool fakePin[64]; int ledcVal[64]; SerialT Serial; FastLEDT FastLED;
const uint8_t u8g2_font_6x10_tf[1]={0}, u8g2_font_logisoso24_tn[1]={0}, u8g2_font_10x20_tf[1]={0};
void setup(); void onEnc();
static void step(int dir){ static const int seq[4][2]={{0,1},{0,0},{1,0},{1,1}}; static int ph=3; ph=(ph+(dir>0?1:3))%4; fakePin[PIN_ENC_A]=seq[ph][0]; fakePin[PIN_ENC_B]=seq[ph][1]; onEnc(); }
static void notch(int dir){ for(int k=0;k<4;k++) step(dir); }
static void run(int ms){ for(int i=0;i<ms/10;i++){ fakeMillis+=10; moodLoop(fakeMillis); } }
static void press(int pin){ fakePin[pin]=false; run(80); fakePin[pin]=true; run(80); }
static void joy(int x,int y){ fakeAnalogInit.v[PIN_JOY_X]=x; fakeAnalogInit.v[PIN_JOY_Y]=y; run(100); fakeAnalogInit.v[PIN_JOY_X]=2048; fakeAnalogInit.v[PIN_JOY_Y]=2048; run(100); }
int main(){ for(auto&p:fakePin)p=true; setup(); progressReset(); fakeAnalogInit.v[PIN_JOY_X]=2048; fakeAnalogInit.v[PIN_JOY_Y]=2048;
  moodBegin(); run(200); printf("départ : lumière %d, %d\n", FastLED.bright, progress.mood);
  press(PIN_BTN1); printf("bouton -> ligne Lumière\n");
  for(int i=0;i<3;i++) notch(-1); run(100); printf("3 crans à gauche : lumière %d\n", FastLED.bright);
  for(int i=0;i<3;i++) notch(1); run(100); printf("3 crans à droite : lumière %d\n", FastLED.bright);
  joy(0,2048); printf("joystick gauche : lumière %d\n", FastLED.bright);
  joy(2048,0); printf("joystick haut -> ligne Ambiance\n"); notch(1); run(100);
  run(9000); printf("après 9 s : écran éteint ; un bouton le rallume sans éteindre la borne\n"); press(PIN_BTN1);
  joy(2048,4095); joy(2048,4095); printf("joystick bas x2 -> Veille\n"); press(PIN_BTN1);
  printf("OLED : %s\n", oledText.substr(oledText.size()>200?oledText.size()-200:0).c_str());
}
