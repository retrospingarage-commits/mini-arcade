// Banc d'essai de la difficulté : un robot « joueur correct » par jeu, taux de réussite à chaque difficulté
#include <Wire.h>
#include <random>
#include <queue>
#include "hw.h"
#include "save.h"
#include "activities.h"
#include "adventure.h"
#include "breakout.h"
std::string oledText; TwoWire Wire; uint32_t fakeMillis=1000; bool fakePin[64]; int ledcVal[64]; SerialT Serial; FastLEDT FastLED;
const uint8_t u8g2_font_6x10_tf[1]={0}, u8g2_font_logisoso24_tn[1]={0}, u8g2_font_10x20_tf[1]={0};
void setup(); void onEnc();
std::mt19937 rng(12345);
double gauss(double m, double sd){ return std::normal_distribution<double>(m,sd)(rng); }
int irand(int n){ return (int)(rng()%n); }
static void step(int dir){ static const int seq[4][2]={{0,1},{0,0},{1,0},{1,1}}; static int ph=3; ph=(ph+(dir>0?1:3))%4; fakePin[PIN_ENC_A]=seq[ph][0]; fakePin[PIN_ENC_B]=seq[ph][1]; onEnc(); }
static void encNotch(){ for(int k=0;k<4;k++) step(1); }
namespace football { enum Phase : uint8_t { AIM, SHOT, RESULT, END }; extern Phase phase; extern float px, bx, by, kx, dx; extern int goals, shots; extern bool charging; }
namespace pinball { extern float bx, by, vx, vy; extern int balls, score; extern bool tilted; }
namespace boss { struct Obj { float x, y, vx; bool on; }; extern Obj bubbles[10]; extern float px, kx; extern int hp, hearts; }
namespace race { struct Rival { float dist, x, targetX, speed; uint8_t hue; bool ahead; }; extern Rival rivals[8]; struct Oil { float dist, x; bool hit; }; extern Oil oils[16]; extern int nOils; extern float dist, carX, turbo; extern int place, crashes; }
namespace rhythm { enum Sym : uint8_t { S_BTN, S_ENC, S_JOY }; enum State : uint8_t { INTRO, LISTEN, ANSWER, WRONG, ROUND_OK, WIN, LOSE }; extern Sym seq[12]; extern int len, pos; extern State state; }
namespace ramen { enum State : uint8_t { PLAY, WIN, BURNT, COLD }; extern State state; struct Item { float x, y; uint8_t kind; bool on; }; extern Item items[6]; extern float bowlX; extern int have[3]; }
namespace sun { enum State : uint8_t { INTRO, BACK, WARN, FRONT, CAUGHT, WIN, LOSE }; extern State state; extern float py; }
namespace breakout { enum State { TITLE, SERVE, PLAY, PAUSE, CLEAR, GAME_OVER, VICTORY }; extern State state; extern float bx, by, padPos; extern uint8_t padW, lives, levelIdx; }

// ---- manette : le joystick « vers le haut » ou au centre
void stick(float up, float right=0){ // up, right dans [-1,1]
  fakeAnalogInit.v[PIN_JOY_Y]=2048; fakeAnalogInit.v[PIN_JOY_X]=2048;
  float want=0; (void)want;
  extern int JOY_UP_RAW, JOY_RIGHT_RAW;
  if(up>0.5f) fakeAnalogInit.v[PIN_JOY_Y]=JOY_UP_RAW;
  if(right>0.5f) fakeAnalogInit.v[PIN_JOY_X]=JOY_RIGHT_RAW;
}
int JOY_UP_RAW=0, JOY_RIGHT_RAW=3900;

int sunCaught=0, sunCycles=0;
bool gSun(){
  sunBegin(); GameResult r=GAME_RUNNING; uint32_t t0=fakeMillis, stopAt=0, resumeAt=0; sun::State last=sun::INTRO;
  while(r==GAME_RUNNING && fakeMillis-t0<200000){ fakeMillis++;
    sun::State s=sun::state;
    if(s!=last){ if(s==sun::CAUGHT) sunCaught++; if(s==sun::WARN) sunCycles++;
                 if(s==sun::WARN){ stopAt=fakeMillis+ (irand(100)<6 ? 700 : (uint32_t)std::max(0.0,gauss(260,50))); }
                 if(s==sun::BACK && (last==sun::FRONT||last==sun::CAUGHT)) resumeAt=fakeMillis+(uint32_t)std::max(0.0,gauss(180,50));
                 last=s; }
    bool move = (s==sun::BACK && fakeMillis>=resumeAt) || ((s==sun::WARN||s==sun::FRONT) && fakeMillis<stopAt);
    stick(move?1:0);
    r=sunLoop(fakeMillis); }
  stick(0); return r==GAME_WON;
}
bool gRhythm(){
  rhythmBegin(); GameResult r=GAME_RUNNING; uint32_t t0=fakeMillis, lastAct=0, releaseAt=0;
  while(r==GAME_RUNNING && fakeMillis-t0<400000){ fakeMillis++;
    if(releaseAt && fakeMillis>=releaseAt){ fakePin[PIN_BTN1]=true; fakeAnalogInit.v[PIN_JOY_X]=2048; releaseAt=0; }
    if(rhythm::state==rhythm::ANSWER && !releaseAt && fakeMillis-lastAct>(uint32_t)gauss(450,90) && rhythm::pos<rhythm::len){
      int want=rhythm::seq[rhythm::pos]; if(irand(1000)<40+15*rhythm::len) want=(want+1+irand(2))%3;
      if(want==0){ fakePin[PIN_BTN1]=false; releaseAt=fakeMillis+60; } else if(want==1) encNotch(); else { fakeAnalogInit.v[PIN_JOY_X]=JOY_RIGHT_RAW; releaseAt=fakeMillis+80; }
      lastAct=fakeMillis; }
    r=rhythmLoop(fakeMillis); }
  fakePin[PIN_BTN1]=true; return r==GAME_WON;
}
int ramenEnd[4]; double ramenTime=0; int ramenN=0;
bool gRamen(){
  ramenBegin(); GameResult r=GAME_RUNNING; uint32_t t0=fakeMillis;
  while(r==GAME_RUNNING && fakeMillis-t0<300000){ fakeMillis++;
    if(fakeMillis%25==0 && irand(100)<90){ float target=-1,best=99;
      for(auto&it:ramen::items) if(it.on&&it.kind<3&&ramen::have[it.kind]<(progress.difficulty==2?(it.kind==0?4:3):(it.kind==0?3:2))&&14-it.y<best){best=14-it.y;target=it.x-1;}
      for(auto&it:ramen::items) if(it.on&&it.kind>=3&&it.y>9&&it.x+1>=ramen::bowlX-0.5f&&it.x<=ramen::bowlX+(progress.difficulty==2?2.5f:3.5f)) target=it.x>8?ramen::bowlX-4:ramen::bowlX+4;
      if(target>=0){ if(target>ramen::bowlX+0.3f) step(1); else if(target<ramen::bowlX-0.3f) step(-1); } }
    r=ramenLoop(fakeMillis); }
  ramenEnd[ramen::state]++; ramenTime+=(fakeMillis-t0)/1000.0; ramenN++;
  return r==GAME_WON;
}
bool gBoss(){
  bossBegin(ALL_CHARMS); fakePin[PIN_BTN1]=false; GameResult r=GAME_RUNNING; uint32_t t0=fakeMillis;
  while(r==GAME_RUNNING && fakeMillis-t0<300000){ fakeMillis++;
    if(fakeMillis%50==0){ float target=boss::kx+2.5f;
      for(auto&b:boss::bubbles) if(b.on&&b.y>8&&b.y<14&&fabsf(b.x-(boss::px+1))<2.2f&&irand(3)){ target=boss::px+(b.x>boss::px+1?-3:3); break; }
      if(target>boss::px+0.6f){ step(1); step(1);} else if(target<boss::px-0.6f){ step(-1); step(-1);} }
    r=bossLoop(fakeMillis); }
  fakePin[PIN_BTN1]=true; return r==GAME_WON;
}
bool gRace(){
  raceBegin(true); GameResult r=GAME_RUNNING; uint32_t t0=fakeMillis; bool tb=false; (void)tb;
  while(r==GAME_RUNNING && fakeMillis-t0<400000){ fakeMillis++;
    if(fakeMillis%60==0){ float best=race::carX,bc=1e9;
      for(float x=2;x<=12;x+=0.5f){ bool bl=false; for(auto&rv:race::rivals){ float rel=rv.dist-race::dist; if(rel>-1&&rel<7&&fabsf(rv.x-x)<2.4f) bl=true; }
        for(int i=0;i<race::nOils;i++){ auto&o=race::oils[i]; float rel=o.dist-race::dist; if(!o.hit&&rel>-1&&rel<7&&fabsf(o.x-x)<2.4f) bl=true; }
        float c=fabsf(x-race::carX)+(bl?100:0); if(c<bc){bc=c;best=x;} }
      if(irand(100)>=25){ if(best>race::carX+0.3f) step(1); else if(best<race::carX-0.3f) step(-1); }
      bool clear=bc<100&&fabsf(best-race::carX)<0.6f; fakePin[PIN_BTN1]=!(clear&&race::turbo>0); }
    r=raceLoop(fakeMillis); }
  fakePin[PIN_BTN1]=true; return r==GAME_WON;
}
bool gFootball(){
  footballBegin(true); GameResult r=GAME_RUNNING; uint32_t pressAt=0; float aim=0;
  for(int ms=0; ms<150000 && r==GAME_RUNNING; ms++){ fakeMillis++;
    if(football::phase!=football::AIM) aim=0;
    if(football::phase==football::AIM && !football::charging && !pressAt && !aim) { float L=5.4f,R=10.4f; float sl=fabsf(football::dx-L)+0.6f*fabsf(football::kx-L), sr=fabsf(football::dx-R)+0.6f*fabsf(football::kx-R); aim=(sr>=sl?R:L)+(irand(13)-6)/10.0f; }
    if(football::phase==football::AIM) football::px+=(aim-football::px)*0.01f;
    if(football::phase==football::AIM && fabsf(football::px-aim)<0.3f && !football::charging && !pressAt){ fakePin[PIN_BTN1]=false; pressAt=fakeMillis; }
    if(pressAt && fakeMillis-pressAt>700 && (fabsf(football::dx-aim)>3.2f || fakeMillis-pressAt>1150)){ fakePin[PIN_BTN1]=true; pressAt=0; }
    r=footballLoop(fakeMillis); }
  fakePin[PIN_BTN1]=true; return r==GAME_WON;
}
bool gPinball(){
  pinballBegin(true); GameResult r=GAME_RUNNING; uint32_t t0=fakeMillis; bool decided=false, willFlip=true; uint32_t lowSince=0;
  while(r==GAME_RUNNING && fakeMillis-t0<600000){ fakeMillis++;
    bool low=pinball::by>10.5f&&pinball::vy>0;
    if(low&&!decided){ decided=true; willFlip=irand(100)<85; lowSince=fakeMillis+40+irand(110); }
    if(!low&&pinball::by<9) decided=false;
    bool act=low&&willFlip&&fakeMillis>=lowSince;
    fakePin[PIN_BTN1]=!(act&&pinball::bx<8); fakePin[PIN_BTN2]=!(act&&pinball::bx>=8);
    r=pinballLoop(fakeMillis); }
  fakePin[PIN_BTN1]=fakePin[PIN_BTN2]=true; return r==GAME_WON;
}
bool gBreakout(){
  breakoutBeginStory(); using namespace breakout; static float hist[400]; int hp=0; float pv=0;
  for(uint32_t ms=0; ms<600000; ms++){ fakeMillis++;
    fakePin[PIN_BTN1]=!(fakeMillis%200<20);
    hist[hp%400]=bx; hp++; float seen=hist[(hp-1-(int)gauss(150,40)+400*4)%400]; // le joueur voit la balle avec un petit retard
    float target=std::min(MATRIX_W-padW/2.0f, std::max(padW/2.0f, seen+(float)gauss(0,0.6)));
    if(state==SERVE) target=bx;
    float d=target-padPos, mv=14.0f*0.001f; padPos+=std::max(-mv,std::min(mv,d)); pv=padPos;
    if(!breakoutLoop(fakeMillis)) return breakoutWon();
  }
  (void)pv; return false;
}
int redWait=0;
bool gReflex(){
  progress.scene=23; adventureBegin(); adventureGoto(23); for(int i=0;i<100;i++){ fakeMillis++; adventureLoop(fakeMillis); }
  fakePin[PIN_BTN1]=false; for(int i=0;i<40;i++){ fakeMillis++; adventureLoop(fakeMillis); } fakePin[PIN_BTN1]=true;
  uint32_t pressAt=0, releaseAt=0; bool wasGreen=false;
  for(int ms=0; ms<40000 && progress.scene==23; ms++){ fakeMillis++;
    bool green=ledcVal[PIN_RGB_R]==0&&ledcVal[PIN_RGB_G]>0&&ledcVal[PIN_RGB_B]==0;
    if(green&&!wasGreen) pressAt=fakeMillis+(uint32_t)std::max(120.0,gauss(260,55));
    wasGreen=green;
    if(pressAt&&fakeMillis>=pressAt){ fakePin[PIN_BTN1]=false; releaseAt=fakeMillis+40; pressAt=0; }
    if(releaseAt&&fakeMillis>=releaseAt){ fakePin[PIN_BTN1]=true; releaseAt=0; }
    adventureLoop(fakeMillis); }
  fakePin[PIN_BTN1]=true; for(int i=0;i<300;i++){ fakeMillis++; adventureLoop(fakeMillis); }
  return progress.scene==24;
}
struct G { const char* name; bool(*f)(); int n; };
int main(int argc,char**argv){ for(auto&p:fakePin)p=true; setup(); progressReset(); progress.charms=ALL_CHARMS;
  // repère du joystick : quelle valeur brute donne « haut » (joyY négatif) ?
  fakeAnalogInit.v[PIN_JOY_Y]=0; float a=joyY(); fakeAnalogInit.v[PIN_JOY_Y]=3900; float b=joyY(); fakeAnalogInit.v[PIN_JOY_Y]=2048;
  JOY_UP_RAW = a<b ? 0 : 3900;
  G games[]={{"1-2-3 soleil",gSun,40},{"Rythme grenouille",gRhythm,40},{"Ramen",gRamen,60},{"Boss (3 charmes)",gBoss,60},{"Course (1re place)",gRace,40},
             {"Football (3 buts/5)",gFootball,60},{"Flipper (record)",gPinball,40},{"Casse-briques (tanuki)",gBreakout,40},{"Etoiles (reflexe)",gReflex,60}};
  const char* only = argc>1?argv[1]:nullptr;
  printf("%-26s %8s %8s %8s\n","jeu (joueur correct)","facile","moyen","difficile");
  for(auto&g:games){ if(only && !strstr(g.name,only)) continue; printf("%-26s",g.name); fflush(stdout);
    for(int d=0; d<3; d++){ progress.difficulty=d; int w=0; for(int k=0;k<g.n;k++){ progress.scene=1; w+=g.f(); } printf(" %5d/%-2d", w, g.n); fflush(stdout); if(!strcmp(g.name,"Ramen")) { printf("(fin: gagné %d, feu %d, froid %d, durée moy %.0f s) ", ramenEnd[1], ramenEnd[2], ramenEnd[3], ramenTime/ramenN); ramenEnd[1]=ramenEnd[2]=ramenEnd[3]=0; ramenTime=0; ramenN=0; }
    if(!strcmp(g.name,"1-2-3 soleil")) { printf("(prises %d / cycles %d) ", sunCaught, sunCycles); sunCaught=sunCycles=0; } }
    printf("\n"); }
}
