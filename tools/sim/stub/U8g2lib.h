#pragma once
#include <cstdio>
#include <set>
#include <string>
#include <vector>
#include "Arduino.h"
#define U8G2_R0 0
#define U8X8_PIN_NONE 255
extern const uint8_t u8g2_font_6x10_tf[], u8g2_font_logisoso24_tn[], u8g2_font_10x20_tf[];

// Contrôle de mise en page : chaque texte écrit est noté avec sa position (taille approchée des polices).
// À chaque envoi à l'écran, on vérifie qu'un texte ne sort pas de l'écran et ne chevauche pas un autre texte.
struct LayoutBox { int x, y0, y1, w, base; std::string t; };
inline std::vector<LayoutBox> gLayout;
inline std::set<std::string> gLayoutIssues;
inline std::string gLayoutContext;
inline void layoutCheck(){
  for (size_t i = 0; i < gLayout.size(); i++) {
    const LayoutBox& a = gLayout[i];
    if (a.x < 0 || a.x + a.w > 128 || a.y0 < 0 || a.base > 63)   // la ligne de base doit rester sur l'écran
      gLayoutIssues.insert(gLayoutContext + " : « " + a.t + " » sort de l'écran");
    for (size_t j = i + 1; j < gLayout.size(); j++) {
      const LayoutBox& b = gLayout[j];
      bool hit = a.x < b.x + b.w - 1 && b.x < a.x + a.w - 1 && a.y0 < b.y1 - 1 && b.y0 < a.y1 - 1;
      if (hit) gLayoutIssues.insert(gLayoutContext + " : « " + a.t + " » chevauche « " + b.t + " »");
    }
  }
  gLayout.clear();
}
inline void layoutReport(){
  printf("\n== Contrôle de mise en page ==\n");
  if (gLayoutIssues.empty()) printf("aucun chevauchement ni texte hors écran\n");
  for (const std::string& s : gLayoutIssues) printf("  %s\n", s.c_str());
  printf("%zu problème(s)\n", gLayoutIssues.size());
}

struct U8G2_SSD1306_128X64_NONAME_F_HW_I2C:Print{
  int cx = 0, cy = 0, fw = 6, fh = 10, asc = 8, desc = 2;   // taille approchée de la police courante
  U8G2_SSD1306_128X64_NONAME_F_HW_I2C(int,int){} void begin(){} void setBusClock(uint32_t){} void setI2CAddress(uint8_t){}
  void clearBuffer(){ oledText.clear(); gLayout.clear(); }
  void sendBuffer(){ layoutCheck(); }
  void setFont(const uint8_t* f){
    if (f == u8g2_font_logisoso24_tn) { fw = 20; asc = 24; desc = 1; }
    else if (f == u8g2_font_10x20_tf) { fw = 10; asc = 16; desc = 4; }
    else { fw = 6; asc = 8; desc = 2; }
    fh = asc + desc;
  }
  void setCursor(int x, int y){ cx = x; cy = y; }
  void rec(const char* t){
    int n = 0; for (const char* s = t; *s; s++) if ((*s & 0xC0) != 0x80) n++;
    LayoutBox b{cx, cy - asc, cy + desc, n * fw, cy, t};
    gLayout.push_back(b); cx += b.w;
  }
  using Print::print;
  void print(const char* t){ rec(t); Print::print(t); }
  void print(int v){ rec(std::to_string(v).c_str()); }
  void print(unsigned int v){ rec(std::to_string(v).c_str()); }
  void print(long v){ rec(std::to_string(v).c_str()); }
  void print(unsigned long v){ rec(std::to_string(v).c_str()); }
  void drawDisc(int,int,int){} void enableUTF8Print(){} void setPowerSave(int){} void setFontMode(int){} void setDrawColor(int){}
  void drawBox(int,int,int,int){} void drawFrame(int,int,int,int){} void drawHLine(int,int,int){} void drawPixel(int,int){}
  int getUTF8Width(const char* s){ int n=0; for(;*s;s++) if((*s&0xC0)!=0x80) n++; return n*6; }
};
using U8G2_SH1106_128X64_NONAME_F_HW_I2C = U8G2_SSD1306_128X64_NONAME_F_HW_I2C;
