// Banc d'essai du Cercle de rythme : un robot joue chaque morceau à chaque difficulté (précision humaine réglable)
#include <Wire.h>
#include <random>
#include "hw.h"
#include "save.h"
#include "songs.h"
#include "activities.h"
std::string oledText; TwoWire Wire; uint32_t fakeMillis=1000; bool fakePin[64]; int ledcVal[64]; SerialT Serial; FastLEDT FastLED;
const uint8_t u8g2_font_6x10_tf[1]={0}, u8g2_font_logisoso24_tn[1]={0}, u8g2_font_10x20_tf[1]={0};
void setup(); void onEnc();
namespace circle {
  enum NoteType : uint8_t { TAP, GOLD, SPIN };
  enum NoteState : uint8_t { PENDING, PERFECT, GOOD, MISS, ACTIVE };
  struct GNote { uint32_t t; uint16_t dur; NoteType type; uint8_t dir; NoteState state; int16_t got, need; };
  enum Mode : uint8_t { SELECT, HELP, COUNT, PLAY, RESULT };
  extern GNote ch[170]; extern int nCh; extern uint32_t clock0; extern Mode mode; extern bool helpSeen; extern int sel, score, maxScore, pct, nPerfect, nGood, nMiss, bestCombo;
}
std::mt19937 rng(7);
double gauss(double m,double sd){ return std::normal_distribution<double>(m,sd)(rng); }
int JOY_UP_RAW=0, JOY_RIGHT_RAW=3900;
static void step(){ static const int seq[4][2]={{0,1},{0,0},{1,0},{1,1}}; static int ph=3; ph=(ph+1)%4; fakePin[PIN_ENC_A]=seq[ph][0]; fakePin[PIN_ENC_B]=seq[ph][1]; onEnc(); }
void stickDir(int dir){ // 0 haut, 1 haut-droite, 2 droite … ; -1 = centre
  int x=2048,y=2048; if(dir>=0){ static const int DX[8]={0,1,1,1,0,-1,-1,-1}, DY[8]={-1,-1,0,1,1,1,0,-1};
    if(DX[dir]>0) x=JOY_RIGHT_RAW; if(DX[dir]<0) x=0; if(DY[dir]<0) y=JOY_UP_RAW; if(DY[dir]>0) y=(JOY_UP_RAW==0?3900:0); }
  fakeAnalogInit.v[PIN_JOY_X]=x; fakeAnalogInit.v[PIN_JOY_Y]=y; }
// sigma : imprécision (ms) ; miss : % d'actions oubliées ; spinRate : impulsions/s que le joueur sait tourner
int playSong(int song, double sigma, int missPct, double spinRate){
  using namespace circle;
  helpSeen=true; sel=song; circleBegin(); fakePin[PIN_BTN1]=false; fakeMillis+=60; circleLoop(fakeMillis); fakePin[PIN_BTN1]=true; fakeMillis+=60; circleLoop(fakeMillis); // choisir
  // le morceau démarre : on lance avec un appui
  fakePin[PIN_BTN1]=false; fakeMillis+=40; circleLoop(fakeMillis); fakePin[PIN_BTN1]=true; fakeMillis+=40; circleLoop(fakeMillis);
  static double jit[200]; static bool skip[200], done[200]; static uint32_t releaseAt; releaseAt=0;
  for(int i=0;i<170;i++){ jit[i]=gauss(18,sigma); skip[i]=(rng()%100)<(unsigned)missPct; done[i]=false; }
  static double spinAcc; spinAcc=0; uint32_t t0=fakeMillis; GameResult r=GAME_RUNNING; (void)r;
  while(mode!=RESULT && fakeMillis-t0<200000){ fakeMillis++;
    if(releaseAt && fakeMillis>=releaseAt){ fakePin[PIN_BTN1]=true; stickDir(-1); releaseAt=0; }
    if(mode==PLAY){ int32_t s=(int32_t)(fakeMillis-clock0);
      for(int i=0;i<nCh;i++){ GNote&g=ch[i];
        if(g.type==SPIN){ if(!skip[i] && s>=(int32_t)g.t && s<(int32_t)(g.t+g.dur)){ spinAcc+=spinRate/1000.0; while(spinAcc>=1){ step(); spinAcc-=1; } } continue; }
        if(done[i]||skip[i]) continue;
        if(s>=(int32_t)(g.t+jit[i]) && !releaseAt){ done[i]=true; if(g.type==TAP) stickDir(g.dir); else fakePin[PIN_BTN1]=false; releaseAt=fakeMillis+55; break; } } }
    circleLoop(fakeMillis); }
  stickDir(-1); fakePin[PIN_BTN1]=true;
  return circle::pct;
}
int main(){ for(auto&p:fakePin)p=true; setup(); progressReset();
  fakeAnalogInit.v[PIN_JOY_Y]=0; float a=joyY(); fakeAnalogInit.v[PIN_JOY_Y]=3900; float b=joyY(); fakeAnalogInit.v[PIN_JOY_Y]=2048; JOY_UP_RAW = a<b ? 0 : 3900;
  const char* dn[]={"facile","moyen","difficile"};
  struct { const char* n; double sigma; int miss; double spin; } bots[]={{"parfait",8,0,60},{"bon joueur",45,3,22},{"joueur moyen",65,8,16},{"débutant",95,18,10}};
  int nSongs=NUM_SONGS; const char* only = nullptr; (void)only;
  for(int d=0; d<3; d++){ progress.difficulty=d; printf("== %s\n", dn[d]);
    for(auto&bt:bots){ printf("  %-13s", bt.n); double tot=0; int cnt=0; for(int s=0;s<nSongs;s++){ double sum=0; int N=6; for(int k=0;k<N;k++) sum+=playSong(s,bt.sigma,bt.miss,bt.spin); printf(" %3.0f%%", sum/N); tot+=sum/N; cnt++; }
      printf("   (moyenne %.0f%%)\n", tot/cnt); } }
  printf("nombre de notes à jouer (par morceau) :\n");
  for(int d=0; d<3; d++){ progress.difficulty=d; printf("  %-9s", dn[d]); for(int s2=0;s2<nSongs;s2++){ playSong(s2,8,0,60); int sp=0,go=0; for(int i=0;i<circle::nCh;i++){ sp+=circle::ch[i].type==circle::SPIN; go+=circle::ch[i].type==circle::GOLD; } printf(" %3d(%d/%d)", circle::nCh, go, sp); } printf("\n"); }
  printf("morceaux :"); for(int s=0;s<nSongs;s++) printf(" %s", SONGS[s].name); printf("\n");
}
