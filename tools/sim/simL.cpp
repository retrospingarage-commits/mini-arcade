// Contrôle de mise en page du Cercle de rythme : écrans d'aide, choix du morceau, décompte, morceau (avec des
// notes ratées et réussies), résultat — à chaque difficulté. Signale les textes qui se chevauchent ou sortent de l'écran.
#include <Wire.h>
#include "hw.h"
#include "save.h"
#include "activities.h"
std::string oledText; TwoWire Wire; uint32_t fakeMillis=1000; bool fakePin[64]; int ledcVal[64]; SerialT Serial; FastLEDT FastLED;
const uint8_t u8g2_font_6x10_tf[1]={0}, u8g2_font_logisoso24_tn[1]={0}, u8g2_font_10x20_tf[1]={0};
void setup();
namespace circle { enum Mode : uint8_t { SELECT, HELP, COUNT, PLAY, RESULT }; extern Mode mode; extern bool helpSeen; extern int sel; }
static void run(int ms){ for(int t=0;t<ms;t+=10){ fakeMillis+=10; circleLoop(fakeMillis); } }
static void press(int pin){ fakePin[pin]=false; run(80); fakePin[pin]=true; run(80); }
int main(){
  for(auto&p:fakePin)p=true; setup(); progressReset(); progress.charms=ALL_CHARMS;
  const char* NAMES[3]={"facile","moyen","difficile"};
  for(int d=0; d<3; d++){
    progress.difficulty=d;
    gLayoutContext = std::string("Cercle ") + NAMES[d];
    circle::helpSeen=false; circleBegin(); circle::sel=0; run(100);          // choix du morceau
    press(PIN_BTN1); run(200);                                                 // 1er écran d'aide
    press(PIN_BTN1); run(200); press(PIN_BTN1); run(200);                      // 2e et 3e écrans
    press(PIN_BTN1);                                                           // lancement : décompte
    run(1600);                                                                 // décompte
    for(int t=0;t<38000;t+=500){ press(PIN_BTN1); run(340); }                  // morceau, avec des appuis (réussis et ratés)
    run(100);                                                                  // résultat
    press(PIN_BTN1); run(200);                                                 // retour au choix
    press(PIN_BTN1); run(200);                                                 // (l'aide ne revient pas)
  }
  layoutReport();
}
