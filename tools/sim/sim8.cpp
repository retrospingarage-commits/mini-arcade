// Toutes les scènes tiennent-elles en 5 lignes avec la nouvelle règle de ponctuation ?
#include <Wire.h>
#include "hw.h"
#include "scenes.h"
#include "adventure.h"
std::string oledText;
TwoWire Wire; uint32_t fakeMillis=1000; bool fakePin[64]; int ledcVal[64]; SerialT Serial; FastLEDT FastLED;
const uint8_t u8g2_font_6x10_tf[1]={0}, u8g2_font_logisoso24_tn[1]={0}, u8g2_font_10x20_tf[1]={0};
#include <string>
int main(){ int bad=0; for (const Scene& sc : SCENES){ for (const char* nm : {"Lou","toi"}) { std::string t(sc.text); size_t p; while((p=t.find('~'))!=std::string::npos) t.replace(p,1,nm); Scene s = sc; s.text = t.c_str(); /*personal*/ char l[8][64]; const char* bar=strchr(s.text,'|'); int n;
  if(bar){ int tl=0; for(const char*q=s.text;q<bar;q++) if((*q&0xC0)!=0x80) tl++; if(tl>21){ printf("scene %d : titre trop long\n", s.id); bad++; } n=1+wrapText(bar+1,l+1,7); l[0][0]='T'; }
  else n=wrapText(s.text,l,8);
  for(const Choice& c: s.choices) if(c.label && strlen(c.label)>20 && !strchr(c.label,(char)0xC3)) { printf("scene %d : choix long %s\n", s.id, c.label); bad++; }
  for(int i=0;i<n;i++) if(strchr("!?:;",l[i][0])) { printf("scene %d : ponctuation seule\n", s.id); bad++; }
  if(n>5){ printf("scene %d : %d lignes\n", s.id, n); bad++; } } } printf("%d anomalies sur %d scenes\n", bad, NUM_SCENES); return bad; }
