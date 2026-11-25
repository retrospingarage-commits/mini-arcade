#pragma once
#include <cstdint>
#include <cstdlib>
typedef uint8_t fract8;
struct CHSV{uint8_t h,s,v; CHSV(uint8_t a,uint8_t b,uint8_t c):h(a),s(b),v(c){}};
struct CRGB{uint8_t r=0,g=0,b=0; enum HTMLColorCode{Black=0,White=0xFFFFFF,Red=0xFF0000,Green=0x008000};
 CRGB(){} CRGB(uint8_t a,uint8_t b_,uint8_t c):r(a),g(b_),b(c){} CRGB(uint32_t c):r(c>>16),g(c>>8),b(c){} CRGB(HTMLColorCode c):r(c>>16),g(c>>8),b(c){} CRGB(const CHSV&h):r(h.v),g(h.v),b(h.v){}
 CRGB& nscale8(uint8_t s){r=r*s/255;g=g*s/255;b=b*s/255;return *this;}};
inline void nblend(CRGB&a,const CRGB&b,fract8 f){a.r+=(b.r-a.r)*f/255;}
inline uint8_t qsub8(uint8_t a,uint8_t b){return a>b?a-b:0;}
inline uint8_t random8(){return rand();} inline uint16_t random16(uint16_t l){return rand()%l;}
inline void fill_solid(CRGB*l,int n,const CRGB&c){for(int i=0;i<n;i++)l[i]=c;}
inline void fadeToBlackBy(CRGB*,int,uint8_t){}
struct WS2812B{}; enum EOrder{GRB};
struct FastLEDT{ template<class C,int P,EOrder O> void addLeds(CRGB*,int){} void setMaxPowerInVoltsAndMilliamps(int,int){} int bright=0; void setBrightness(int b){ bright=b; } void clear(){} void show(){} };
extern FastLEDT FastLED;
inline uint8_t scale8(uint8_t a,uint8_t b){return a*b/256;}
inline uint8_t sin8(uint8_t a){return a;}
inline uint8_t inoise8(uint16_t x,uint16_t y,uint16_t z){return (x*7+y*13+z*3)&255;}
inline uint8_t quadwave8(uint8_t a){return a;}
inline uint8_t cos8(uint8_t a){return a+64;}
inline CRGB blend(const CRGB&a,const CRGB&b,uint8_t f){return CRGB(a.r+(b.r-a.r)*f/255,a.g+(b.g-a.g)*f/255,a.b+(b.b-a.b)*f/255);}
inline uint8_t random8(uint8_t l){return rand()%l;}
inline CRGB HeatColor(uint8_t t){return CRGB(t,t/2,0);}
inline uint8_t qadd8(uint8_t a,uint8_t b){int s=a+b;return s>255?255:s;}
inline uint8_t random8(uint8_t a,uint8_t b){return a+rand()%(b-a);}
